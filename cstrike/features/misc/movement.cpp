#include "movement.h"

#include <algorithm> // ��� std::clamp
#include <cmath>     // ��� std::atan2, std::cos, std::sin
#include <random>    // ��� std::random_device, std::mt19937
#include <optional>  // ��� std::optional

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// ���������� ������ ��� ����������� (����������� � �������� ������)
#ifdef _DEBUG
#define LOG_INFO L_PRINT(LOG_INFO)
#else
#define LOG_INFO if (false) L_PRINT(LOG_INFO)
#endif

namespace F::MISC::MOVEMENT
{
    // ����������� ���������� ��� �������� ����� ���������
    static QAngle_t angCorrectionView = {};

    // ��������� ��� ������ (������ IN_XXX)
    enum class Button : int
    {
        Forward = IN_FORWARD,
        Backward = IN_BACK,
        Left = IN_LEFT,
        Right = IN_RIGHT,
        Jump = IN_JUMP
    };

    // ��������� ��� �������� ��������� WASD
    struct WASDState
    {
        bool forward = false;  // W
        bool backward = false; // S
        bool left = false;     // A
        bool right = false;    // D

        WASDState(float forwardMove, float sideMove) noexcept
        {
            forward = forwardMove > 0.0f;
            backward = forwardMove < 0.0f;
            left = sideMove < 0.0f;
            right = sideMove > 0.0f;
        }
    };

    // ����� ��� ���������� �������������
    class Randomizer
    {
    public:
        Randomizer() noexcept : gen(rd()) {}

        // �������� ��������� ���������� ���� (�0.05 ������) ������ 5 �����
        float GetAngleOffset(int tick) noexcept
        {
            if (tick % 5 == 0)
            {
                lastOffset = dis(gen);
            }
            return lastOffset;
        }

    private:
        std::random_device rd;
        std::mt19937 gen;
        std::uniform_real_distribution<float> dis{ -0.05f, 0.05f };
        float lastOffset = 0.0f;
    };

    // ��������� �������
    inline constexpr float MaxSpeed = 450.0f; // ������������ �������� � CS2
    inline constexpr float PerfectAngle = 45.0f * (M_PI / 180.0f); // ��������� ���� (45 ��������)
    inline constexpr float SpeedThreshold = 300.0f; // ����� �������� ��� ����������� �����������
    inline constexpr float MaxValidSpeed = 2000.0f; // ������������ ���������� ��������
    inline constexpr float MinMoveThreshold = 0.01f; // ����������� ����� ��� ��������
    inline constexpr float SpeedCapMultiplier = 1.1f; // ��������� ��� ����������� �������� (495.0f)

    // �������� �������� ������
    std::optional<float> GetPlayerSpeed(C_CSPlayerPawn& localPawn) noexcept
    {
        Vector_t velocity = localPawn.GetAbsVelocity();
        velocity.z = 0.0f;
        const float speed = velocity.Length2D();

        if (speed < 0.0f || speed > MaxValidSpeed)
        {
            L_PRINT(LOG_WARNING) << "AutoStrafe: Invalid speed detected: " << speed;
            return std::nullopt;
        }
        return speed;
    }

    // �������� ������ ��� ��������
    void AddSubtick(CBaseUserCmdPB& userCmd, Button button, bool pressed, float when = 0.0f) noexcept
    {
        CSubtickMoveStep* subtick = userCmd.add_subtick_move();
        if (!subtick)
        {
            L_PRINT(LOG_ERROR) << "AutoStrafe: Failed to add subtick";
            return;
        }

        subtick->nButton = static_cast<int>(button);
        subtick->bPressed = pressed;
        subtick->flWhen = when;
        subtick->SetBits(ESubtickMoveStepBits::MOVESTEP_BITS_BUTTON |
            ESubtickMoveStepBits::MOVESTEP_BITS_PRESSED |
            ESubtickMoveStepBits::MOVESTEP_BITS_WHEN);
    }

    // �������� ������� ��������� ��������
    void OnMove(CUserCmd* pCmd, CBaseUserCmdPB* pBaseCmd, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn) noexcept
    {
        // �������� ������� ����������
        if (!pCmd || !pBaseCmd || !pLocalController || !pLocalPawn)
        {
            L_PRINT(LOG_ERROR) << "OnMove: Invalid input parameters";
            return;
        }

        // ��������, ��� �� �����
        if (!pLocalController->IsPawnAlive())
        {
            LOG_INFO << "OnMove: Player is not alive";
            return;
        }

        // �������� ������ �������� (������, ��������, ����)
        const int32_t moveType = pLocalPawn->GetMoveType();
        if (moveType == MOVETYPE_NOCLIP || moveType == MOVETYPE_LADDER || pLocalPawn->GetWaterLevel() >= WL_WAIST)
        {
            LOG_INFO << "OnMove: Invalid movement type (noclip, ladder, or in water)";
            return;
        }

        // ��������� ������� � ����������
        BunnyHop(pCmd, pBaseCmd, pLocalPawn);
        AutoStrafe(*pBaseCmd, *pLocalPawn);

        // ��������� �������� ��� ���� ��������
        if (pCmd->csgoUserCmd.inputHistoryField.pRep)
        {
            for (int nSubTick = 0; nSubTick < pCmd->csgoUserCmd.inputHistoryField.pRep->nAllocatedSize; nSubTick++)
            {
                CCSGOInputHistoryEntryPB* inputEntry = pCmd->GetInputHistoryEntry(nSubTick);
                if (!inputEntry)
                {
                    LOG_INFO << "OnMove: Invalid input entry at subtick " << nSubTick;
                    continue;
                }

                // ��������� ������� ���� ������
                angCorrectionView = inputEntry->pViewAngles->angValue;
                ValidateUserCommand(pCmd, pBaseCmd, inputEntry);
            }
        }
        else
        {
            L_PRINT(LOG_WARNING) << "OnMove: Input history is null";
        }
    }

    // ������� ��� ��������
    void BunnyHop(CUserCmd* pCmd, CBaseUserCmdPB* pUserCmd, C_CSPlayerPawn* pLocalPawn) noexcept
    {
        // �������� ������� ����������
        if (!pCmd || !pUserCmd || !pLocalPawn)
        {
            L_PRINT(LOG_ERROR) << "BunnyHop: Invalid input parameters";
            return;
        }

        // �������� ���������� �������� � ��������� ���������
        if (!C_GET(bool, Vars.bAutoBHop) || CONVAR::sv_autobunnyhopping->value.i1)
        {
            LOG_INFO << "BunnyHop: Disabled or server has autobunnyhopping enabled";
            return;
        }

        // �������� ��������� �� �����
        const bool onGround = (pLocalPawn->GetFlags() & FL_ONGROUND) != 0;
        LOG_INFO << "BunnyHop: OnGround = " << (onGround ? "true" : "false");

        static bool lastJumped = false;
        static bool shouldFakeJump = false;

        if (onGround)
        {
            // ���� ����� �� ����� � �� ������ � ������� ����
            if (!lastJumped && !shouldFakeJump)
            {
                lastJumped = true;
                shouldFakeJump = true;

                // ������������� ��������� ������
                pCmd->nButtons.nValue |= IN_JUMP;
                if (pUserCmd->pInButtonState)
                {
                    pUserCmd->pInButtonState->nValue |= IN_JUMP;
                }

                // ��������� ������ ��� ������
                AddSubtick(*pUserCmd, Button::Jump, true);
                LOG_INFO << "BunnyHop: Jump initiated";
            }
            else
            {
                // ��������� ������ ������, ����� ���� ���������������� ����� ������
                pCmd->nButtons.nValue &= ~IN_JUMP;
                if (pUserCmd->pInButtonState)
                {
                    pUserCmd->pInButtonState->nValue &= ~IN_JUMP;
                }
            }
        }
        else
        {
            // ����� � �������, ���������� ���������
            lastJumped = false;
            if (shouldFakeJump)
            {
                shouldFakeJump = false;
                pCmd->nButtons.nValue &= ~IN_JUMP;
                if (pUserCmd->pInButtonState)
                {
                    pUserCmd->pInButtonState->nValue &= ~IN_JUMP;
                }

                // ��������� ������ ��� ���������� ������
                AddSubtick(*pUserCmd, Button::Jump, false);
                LOG_INFO << "BunnyHop: Jump released";
            }
        }
    }

    // ������� ��� �����������
    void AutoStrafe(CBaseUserCmdPB& userCmd, C_CSPlayerPawn& localPawn) noexcept
    {
        // �������� ���������� ����������� � ��������� �� �����
        if (!C_GET(bool, Vars.bAutoStrafe) || (localPawn.GetFlags() & FL_ONGROUND))
        {
            LOG_INFO << "AutoStrafe: Disabled or on ground";
            return;
        }

        // �������� �������� ������
        auto speedOpt = GetPlayerSpeed(localPawn);
        if (!speedOpt)
        {
            return;
        }
        const float speed = *speedOpt;
        LOG_INFO << "AutoStrafe: Speed = " << speed;

        // ���������� ��������� WASD
        WASDState wasd(userCmd.flForwardMove, userCmd.flSideMove);

        // ���������� ����������� �������
        float wishDirection = 0.0f; // ����������� �������� ������� (A/D)
        float forwardMove = 0.0f;   // ����������� ������/����� (W/S)

        if (wasd.left) // A
        {
            wishDirection = 1.0f; // ������ �����
            if (wasd.forward) // W+A
            {
                forwardMove = 0.5f;
                LOG_INFO << "AutoStrafe: Moving forward-left (W+A)";
            }
            else if (wasd.backward) // S+A
            {
                forwardMove = -0.5f;
                LOG_INFO << "AutoStrafe: Moving backward-left (S+A)";
            }
            else // ������ A
            {
                forwardMove = 0.0f;
                LOG_INFO << "AutoStrafe: Moving left (A)";
            }
        }
        else if (wasd.right) // D
        {
            wishDirection = -1.0f; // ������ ������
            if (wasd.forward) // W+D
            {
                forwardMove = 0.5f;
                LOG_INFO << "AutoStrafe: Moving forward-right (W+D)";
            }
            else if (wasd.backward) // S+D
            {
                forwardMove = -0.5f;
                LOG_INFO << "AutoStrafe: Moving backward-right (S+D)";
            }
            else // ������ D
            {
                forwardMove = 0.0f;
                LOG_INFO << "AutoStrafe: Moving right (D)";
            }
        }
        else if (wasd.forward) // W
        {
            forwardMove = 1.0f;
            wishDirection = (speed > SpeedThreshold) ? -1.0f : 1.0f; // �������� �����������
            LOG_INFO << "AutoStrafe: Moving forward (W)";
        }
        else if (wasd.backward) // S
        {
            forwardMove = -1.0f;
            wishDirection = (speed > SpeedThreshold) ? -1.0f : 1.0f; // �������� �����������
            LOG_INFO << "AutoStrafe: Moving backward (S)";
        }
        else
        {
            // ���� ������� �� ������, �������� �����������
            static bool lastDirectionLeft = false;
            wishDirection = lastDirectionLeft ? 1.0f : -1.0f;
            lastDirectionLeft = !lastDirectionLeft;
            forwardMove = 0.0f;
            LOG_INFO << "AutoStrafe: No keys pressed, default direction";
        }

        // ������������ ������ ������������ ���� �������
        float optimalAngle = std::atan2(15.0f, speed);
        optimalAngle = std::clamp(optimalAngle, 0.0f, PerfectAngle);

        // ��������� ������������ ��� ������ �� ��������
        static Randomizer randomizer;
        static int tickCounter = 0;
        optimalAngle += randomizer.GetAngleOffset(tickCounter++);

        // ��������� ������� ��� �������
        if (wishDirection != 0.0f)
        {
            const float finalAngle = optimalAngle * wishDirection;
            const float newSideMove = std::cos(finalAngle) * MaxSpeed * wishDirection;
            const float newForwardMove = (forwardMove != 0.0f) ? std::sin(finalAngle) * MaxSpeed * forwardMove : userCmd.flForwardMove;

            // ��������� ����� �������� ��������
            userCmd.flSideMove = newSideMove;
            userCmd.flForwardMove = newForwardMove;

            // ��������� ������� ��� ������� ����������
            if (std::abs(newSideMove) > MinMoveThreshold)
            {
                AddSubtick(userCmd, (newSideMove < 0.0f) ? Button::Left : Button::Right, true);
            }

            if (forwardMove != 0.0f && std::abs(newForwardMove) > MinMoveThreshold)
            {
                AddSubtick(userCmd, (newForwardMove > 0.0f) ? Button::Forward : Button::Backward, true);
            }

            // �������������� ��������� ������
            if (userCmd.pInButtonState)
            {
                userCmd.pInButtonState->nValue &= ~(IN_LEFT | IN_RIGHT | IN_FORWARD | IN_BACK);
                if (userCmd.flSideMove < 0.0f)
                    userCmd.pInButtonState->nValue |= IN_LEFT;
                else if (userCmd.flSideMove > 0.0f)
                    userCmd.pInButtonState->nValue |= IN_RIGHT;

                if (userCmd.flForwardMove > 0.0f)
                    userCmd.pInButtonState->nValue |= IN_FORWARD;
                else if (userCmd.flForwardMove < 0.0f)
                    userCmd.pInButtonState->nValue |= IN_BACK;
            }
            else
            {
                L_PRINT(LOG_WARNING) << "AutoStrafe: pInButtonState is nullptr";
            }

            LOG_INFO << "AutoStrafe: flSideMove = " << userCmd.flSideMove
                << ", flForwardMove = " << userCmd.flForwardMove;
        }

        // ����������� �������� ��� �������������� ����������
        if (speed > MaxSpeed * SpeedCapMultiplier) // 495.0f
        {
            userCmd.flForwardMove = std::clamp(userCmd.flForwardMove, -MaxSpeed, MaxSpeed);
            userCmd.flSideMove = std::clamp(userCmd.flSideMove, -MaxSpeed, MaxSpeed);
            LOG_INFO << "AutoStrafe: Speed capped to prevent overspeed";
        }
    }

    // ��������� ���������������� �������
    void ValidateUserCommand(CUserCmd* pCmd, CBaseUserCmdPB* pUserCmd, CCSGOInputHistoryEntryPB* pInputEntry) noexcept
    {
        if (!pCmd || !pUserCmd || !pInputEntry)
        {
            L_PRINT(LOG_ERROR) << "ValidateUserCommand: Invalid input parameters";
            return;
        }

        // ������ �� �������� (�������� �����)
        if (C_GET(bool, Vars.bAntiUntrusted))
        {
            pInputEntry->SetBits(EInputHistoryBits::INPUT_HISTORY_BITS_VIEWANGLES);
            if (pInputEntry->pViewAngles && pInputEntry->pViewAngles->angValue.IsValid())
            {
                pInputEntry->pViewAngles->angValue.Clamp();
                pInputEntry->pViewAngles->angValue.z = 0.0f; // ������� roll
                LOG_INFO << "ValidateUserCommand: View angles clamped";
            }
            else
            {
                if (pInputEntry->pViewAngles)
                {
                    pInputEntry->pViewAngles->angValue = QAngle_t{ 0.0f, 0.0f, 0.0f };
                }
                L_PRINT(LOG_WARNING) << "ValidateUserCommand: Invalid view angles, reset to zero";
            }
        }

        // ��������� ��������
        MovementCorrection(pUserCmd, pInputEntry, angCorrectionView);

        // ���������� ��������� ������ ��������
        if (pUserCmd->pInButtonState)
        {
            pUserCmd->pInButtonState->SetBits(EButtonStatePBBits::BUTTON_STATE_PB_BITS_BUTTONSTATE1);
            pUserCmd->pInButtonState->nValue &= ~(IN_FORWARD | IN_BACK | IN_LEFT | IN_RIGHT);

            if (pUserCmd->flForwardMove > 0.0f)
                pUserCmd->pInButtonState->nValue |= IN_FORWARD;
            else if (pUserCmd->flForwardMove < 0.0f)
                pUserCmd->pInButtonState->nValue |= IN_BACK;

            if (pUserCmd->flSideMove > 0.0f)
                pUserCmd->pInButtonState->nValue |= IN_RIGHT;
            else if (pUserCmd->flSideMove < 0.0f)
                pUserCmd->pInButtonState->nValue |= IN_LEFT;

            LOG_INFO << "ValidateUserCommand: Button states updated";
        }
        else
        {
            L_PRINT(LOG_WARNING) << "ValidateUserCommand: pInButtonState is nullptr";
        }
    }

    // ��������� ��������
    void MovementCorrection(CBaseUserCmdPB* pUserCmd, CCSGOInputHistoryEntryPB* pInputEntry, const QAngle_t& angDesiredViewPoint) noexcept
    {
        if (!pUserCmd || !pInputEntry || !pInputEntry->pViewAngles)
        {
            L_PRINT(LOG_ERROR) << "MovementCorrection: Invalid input parameters";
            return;
        }

        // ������ �������� ��������
        Vector_t vecForward{}, vecRight{}, vecUp{};
        angDesiredViewPoint.ToDirections(&vecForward, &vecRight, &vecUp);

        // ������������ ��������
        vecForward.z = vecRight.z = vecUp.x = vecUp.y = 0.0f;
        vecForward.NormalizeInPlace();
        vecRight.NormalizeInPlace();
        vecUp.NormalizeInPlace();

        // ��������� ������ �������� ��������
        Vector_t vecOldForward{}, vecOldRight{}, vecOldUp{};
        pInputEntry->pViewAngles->angValue.ToDirections(&vecOldForward, &vecOldRight, &vecOldUp);

        // ������������ ������ ��������
        vecOldForward.z = vecOldRight.z = vecOldUp.x = vecOldUp.y = 0.0f;
        vecOldForward.NormalizeInPlace();
        vecOldRight.NormalizeInPlace();
        vecOldUp.NormalizeInPlace();

        // ������ ����� �������� ��������
        const float newForwardMove = vecOldForward.x * (vecRight.x * pUserCmd->flSideMove) +
            vecOldForward.y * (vecRight.y * pUserCmd->flSideMove) +
            vecOldForward.x * (vecForward.x * pUserCmd->flForwardMove) +
            vecOldForward.y * (vecForward.y * pUserCmd->flForwardMove);

        const float newSideMove = vecOldRight.x * (vecRight.x * pUserCmd->flSideMove) +
            vecOldRight.y * (vecRight.y * pUserCmd->flSideMove) +
            vecOldRight.x * (vecForward.x * pUserCmd->flForwardMove) +
            vecOldRight.y * (vecForward.y * pUserCmd->flForwardMove);

        pUserCmd->flForwardMove = newForwardMove;
        pUserCmd->flSideMove = newSideMove;

        LOG_INFO << "MovementCorrection: New flForwardMove = " << newForwardMove << ", flSideMove = " << newSideMove;
    }
}