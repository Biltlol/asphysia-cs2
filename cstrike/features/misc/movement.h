#pragma once

#include "../../sdk/entity.h"
#include "../../sdk/datatypes/usercmd.h"
#include "../../core/convars.h"
#include "../../sdk/interfaces/ienginecvar.h"
#include "../../core/variables.h"

namespace F::MISC::MOVEMENT
{
    // �������� ������� ��������� ��������
    void OnMove(CUserCmd* pCmd, CBaseUserCmdPB* pBaseCmd, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn) noexcept;

    // ������� ��� ��������
    void BunnyHop(CUserCmd* pCmd, CBaseUserCmdPB* pUserCmd, C_CSPlayerPawn* pLocalPawn) noexcept;

    // ������� ��� �����������
    void AutoStrafe(CBaseUserCmdPB& userCmd, C_CSPlayerPawn& localPawn) noexcept;

    // ��������� ���������������� �������
    void ValidateUserCommand(CUserCmd* pCmd, CBaseUserCmdPB* pUserCmd, CCSGOInputHistoryEntryPB* pInputEntry) noexcept;

    // ��������� ��������
    void MovementCorrection(CBaseUserCmdPB* pUserCmd, CCSGOInputHistoryEntryPB* pInputEntry, const QAngle_t& angDesiredViewPoint) noexcept;
}