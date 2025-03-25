#include "movement.h"

#include <algorithm> // Для std::clamp
#include <cmath>     // Для std::atan2, std::cos, std::sin
#include <random>    // Для std::random_device, std::mt19937
#include <optional>  // Для std::optional

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// Определяем макрос для логирования (отключается в релизной сборке)
#ifdef _DEBUG
#define LOG_INFO L_PRINT(LOG_INFO)
#else
#define LOG_INFO if (false) L_PRINT(LOG_INFO)
#endif

namespace F::MISC::MOVEMENT
{
    // Статическая переменная для хранения углов коррекции
    static QAngle_t angCorrectionView = {};

    // Константы для кнопок (аналог IN_XXX)
    enum class Button : int
    {
        Forward = IN_FORWARD,
        Backward = IN_BACK,
        Left = IN_LEFT,
        Right = IN_RIGHT,
        Jump = IN_JUMP
    };

    // Структура для хранения состояния WASD
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

    // Класс для управления рандомизацией
    class Randomizer
    {
    public:
        Randomizer() noexcept : gen(rd()) {}

        // Получить случайное отклонение угла (±0.05 радиан) каждые 5 тиков
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

    // Константы стрейфа
    inline constexpr float MaxSpeed = 450.0f; // Максимальная скорость в CS2
    inline constexpr float PerfectAngle = 45.0f * (M_PI / 180.0f); // Идеальный угол (45 градусов)
    inline constexpr float SpeedThreshold = 300.0f; // Порог скорости для чередования направления
    inline constexpr float MaxValidSpeed = 2000.0f; // Максимальная допустимая скорость
    inline constexpr float MinMoveThreshold = 0.01f; // Минимальный порог для движения
    inline constexpr float SpeedCapMultiplier = 1.1f; // Множитель для ограничения скорости (495.0f)

    // Получить скорость игрока
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

    // Добавить субтик для движения
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

    // Основная функция обработки движения
    void OnMove(CUserCmd* pCmd, CBaseUserCmdPB* pBaseCmd, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn) noexcept
    {
        // Проверка входных параметров
        if (!pCmd || !pBaseCmd || !pLocalController || !pLocalPawn)
        {
            L_PRINT(LOG_ERROR) << "OnMove: Invalid input parameters";
            return;
        }

        // Проверка, жив ли игрок
        if (!pLocalController->IsPawnAlive())
        {
            LOG_INFO << "OnMove: Player is not alive";
            return;
        }

        // Проверка режима движения (ноклип, лестница, вода)
        const int32_t moveType = pLocalPawn->GetMoveType();
        if (moveType == MOVETYPE_NOCLIP || moveType == MOVETYPE_LADDER || pLocalPawn->GetWaterLevel() >= WL_WAIST)
        {
            LOG_INFO << "OnMove: Invalid movement type (noclip, ladder, or in water)";
            return;
        }

        // Выполняем банихоп и автострейф
        BunnyHop(pCmd, pBaseCmd, pLocalPawn);
        AutoStrafe(*pBaseCmd, *pLocalPawn);

        // Коррекция движения для всех субтиков
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

                // Сохраняем текущие углы обзора
                angCorrectionView = inputEntry->pViewAngles->angValue;
                ValidateUserCommand(pCmd, pBaseCmd, inputEntry);
            }
        }
        else
        {
            L_PRINT(LOG_WARNING) << "OnMove: Input history is null";
        }
    }

    // Функция для банихопа
    void BunnyHop(CUserCmd* pCmd, CBaseUserCmdPB* pUserCmd, C_CSPlayerPawn* pLocalPawn) noexcept
    {
        // Проверка входных параметров
        if (!pCmd || !pUserCmd || !pLocalPawn)
        {
            L_PRINT(LOG_ERROR) << "BunnyHop: Invalid input parameters";
            return;
        }

        // Проверка активности банихопа и серверной настройки
        if (!C_GET(bool, Vars.bAutoBHop) || CONVAR::sv_autobunnyhopping->value.i1)
        {
            LOG_INFO << "BunnyHop: Disabled or server has autobunnyhopping enabled";
            return;
        }

        // Проверка состояния на земле
        const bool onGround = (pLocalPawn->GetFlags() & FL_ONGROUND) != 0;
        LOG_INFO << "BunnyHop: OnGround = " << (onGround ? "true" : "false");

        static bool lastJumped = false;
        static bool shouldFakeJump = false;

        if (onGround)
        {
            // Если игрок на земле и не прыгал в прошлом тике
            if (!lastJumped && !shouldFakeJump)
            {
                lastJumped = true;
                shouldFakeJump = true;

                // Устанавливаем состояние прыжка
                pCmd->nButtons.nValue |= IN_JUMP;
                if (pUserCmd->pInButtonState)
                {
                    pUserCmd->pInButtonState->nValue |= IN_JUMP;
                }

                // Добавляем субтик для прыжка
                AddSubtick(*pUserCmd, Button::Jump, true);
                LOG_INFO << "BunnyHop: Jump initiated";
            }
            else
            {
                // Отпускаем кнопку прыжка, чтобы игра зарегистрировала новый прыжок
                pCmd->nButtons.nValue &= ~IN_JUMP;
                if (pUserCmd->pInButtonState)
                {
                    pUserCmd->pInButtonState->nValue &= ~IN_JUMP;
                }
            }
        }
        else
        {
            // Игрок в воздухе, сбрасываем состояние
            lastJumped = false;
            if (shouldFakeJump)
            {
                shouldFakeJump = false;
                pCmd->nButtons.nValue &= ~IN_JUMP;
                if (pUserCmd->pInButtonState)
                {
                    pUserCmd->pInButtonState->nValue &= ~IN_JUMP;
                }

                // Добавляем субтик для отпускания прыжка
                AddSubtick(*pUserCmd, Button::Jump, false);
                LOG_INFO << "BunnyHop: Jump released";
            }
        }
    }

    // Функция для автострейфа
    void AutoStrafe(CBaseUserCmdPB& userCmd, C_CSPlayerPawn& localPawn) noexcept
    {
        // Проверка активности автострейфа и состояния на земле
        if (!C_GET(bool, Vars.bAutoStrafe) || (localPawn.GetFlags() & FL_ONGROUND))
        {
            LOG_INFO << "AutoStrafe: Disabled or on ground";
            return;
        }

        // Получаем скорость игрока
        auto speedOpt = GetPlayerSpeed(localPawn);
        if (!speedOpt)
        {
            return;
        }
        const float speed = *speedOpt;
        LOG_INFO << "AutoStrafe: Speed = " << speed;

        // Определяем состояние WASD
        WASDState wasd(userCmd.flForwardMove, userCmd.flSideMove);

        // Определяем направление стрейфа
        float wishDirection = 0.0f; // Направление бокового стрейфа (A/D)
        float forwardMove = 0.0f;   // Направление вперед/назад (W/S)

        if (wasd.left) // A
        {
            wishDirection = 1.0f; // Стрейф влево
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
            else // Только A
            {
                forwardMove = 0.0f;
                LOG_INFO << "AutoStrafe: Moving left (A)";
            }
        }
        else if (wasd.right) // D
        {
            wishDirection = -1.0f; // Стрейф вправо
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
            else // Только D
            {
                forwardMove = 0.0f;
                LOG_INFO << "AutoStrafe: Moving right (D)";
            }
        }
        else if (wasd.forward) // W
        {
            forwardMove = 1.0f;
            wishDirection = (speed > SpeedThreshold) ? -1.0f : 1.0f; // Чередуем направление
            LOG_INFO << "AutoStrafe: Moving forward (W)";
        }
        else if (wasd.backward) // S
        {
            forwardMove = -1.0f;
            wishDirection = (speed > SpeedThreshold) ? -1.0f : 1.0f; // Чередуем направление
            LOG_INFO << "AutoStrafe: Moving backward (S)";
        }
        else
        {
            // Если клавиши не нажаты, чередуем направление
            static bool lastDirectionLeft = false;
            wishDirection = lastDirectionLeft ? 1.0f : -1.0f;
            lastDirectionLeft = !lastDirectionLeft;
            forwardMove = 0.0f;
            LOG_INFO << "AutoStrafe: No keys pressed, default direction";
        }

        // Динамический расчет оптимального угла стрейфа
        float optimalAngle = std::atan2(15.0f, speed);
        optimalAngle = std::clamp(optimalAngle, 0.0f, PerfectAngle);

        // Добавляем рандомизацию для защиты от античита
        static Randomizer randomizer;
        static int tickCounter = 0;
        optimalAngle += randomizer.GetAngleOffset(tickCounter++);

        // Применяем расчеты для стрейфа
        if (wishDirection != 0.0f)
        {
            const float finalAngle = optimalAngle * wishDirection;
            const float newSideMove = std::cos(finalAngle) * MaxSpeed * wishDirection;
            const float newForwardMove = (forwardMove != 0.0f) ? std::sin(finalAngle) * MaxSpeed * forwardMove : userCmd.flForwardMove;

            // Применяем новые значения движения
            userCmd.flSideMove = newSideMove;
            userCmd.flForwardMove = newForwardMove;

            // Добавляем субтики для точного управления
            if (std::abs(newSideMove) > MinMoveThreshold)
            {
                AddSubtick(userCmd, (newSideMove < 0.0f) ? Button::Left : Button::Right, true);
            }

            if (forwardMove != 0.0f && std::abs(newForwardMove) > MinMoveThreshold)
            {
                AddSubtick(userCmd, (newForwardMove > 0.0f) ? Button::Forward : Button::Backward, true);
            }

            // Синхронизируем состояние кнопок
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

        // Оптимизация скорости для предотвращения превышения
        if (speed > MaxSpeed * SpeedCapMultiplier) // 495.0f
        {
            userCmd.flForwardMove = std::clamp(userCmd.flForwardMove, -MaxSpeed, MaxSpeed);
            userCmd.flSideMove = std::clamp(userCmd.flSideMove, -MaxSpeed, MaxSpeed);
            LOG_INFO << "AutoStrafe: Speed capped to prevent overspeed";
        }
    }

    // Валидация пользовательской команды
    void ValidateUserCommand(CUserCmd* pCmd, CBaseUserCmdPB* pUserCmd, CCSGOInputHistoryEntryPB* pInputEntry) noexcept
    {
        if (!pCmd || !pUserCmd || !pInputEntry)
        {
            L_PRINT(LOG_ERROR) << "ValidateUserCommand: Invalid input parameters";
            return;
        }

        // Защита от антраста (проверка углов)
        if (C_GET(bool, Vars.bAntiUntrusted))
        {
            pInputEntry->SetBits(EInputHistoryBits::INPUT_HISTORY_BITS_VIEWANGLES);
            if (pInputEntry->pViewAngles && pInputEntry->pViewAngles->angValue.IsValid())
            {
                pInputEntry->pViewAngles->angValue.Clamp();
                pInputEntry->pViewAngles->angValue.z = 0.0f; // Убираем roll
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

        // Коррекция движения
        MovementCorrection(pUserCmd, pInputEntry, angCorrectionView);

        // Обновление состояний кнопок движения
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

    // Коррекция движения
    void MovementCorrection(CBaseUserCmdPB* pUserCmd, CCSGOInputHistoryEntryPB* pInputEntry, const QAngle_t& angDesiredViewPoint) noexcept
    {
        if (!pUserCmd || !pInputEntry || !pInputEntry->pViewAngles)
        {
            L_PRINT(LOG_ERROR) << "MovementCorrection: Invalid input parameters";
            return;
        }

        // Расчет векторов движения
        Vector_t vecForward{}, vecRight{}, vecUp{};
        angDesiredViewPoint.ToDirections(&vecForward, &vecRight, &vecUp);

        // Нормализация векторов
        vecForward.z = vecRight.z = vecUp.x = vecUp.y = 0.0f;
        vecForward.NormalizeInPlace();
        vecRight.NormalizeInPlace();
        vecUp.NormalizeInPlace();

        // Получение старых векторов движения
        Vector_t vecOldForward{}, vecOldRight{}, vecOldUp{};
        pInputEntry->pViewAngles->angValue.ToDirections(&vecOldForward, &vecOldRight, &vecOldUp);

        // Нормализация старых векторов
        vecOldForward.z = vecOldRight.z = vecOldUp.x = vecOldUp.y = 0.0f;
        vecOldForward.NormalizeInPlace();
        vecOldRight.NormalizeInPlace();
        vecOldUp.NormalizeInPlace();

        // Расчет новых значений движения
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