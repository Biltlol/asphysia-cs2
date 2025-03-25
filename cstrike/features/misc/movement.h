#pragma once

#include "../../sdk/entity.h"
#include "../../sdk/datatypes/usercmd.h"
#include "../../core/convars.h"
#include "../../sdk/interfaces/ienginecvar.h"
#include "../../core/variables.h"

namespace F::MISC::MOVEMENT
{
    // Основная функция обработки движения
    void OnMove(CUserCmd* pCmd, CBaseUserCmdPB* pBaseCmd, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn) noexcept;

    // Функция для банихопа
    void BunnyHop(CUserCmd* pCmd, CBaseUserCmdPB* pUserCmd, C_CSPlayerPawn* pLocalPawn) noexcept;

    // Функция для автострейфа
    void AutoStrafe(CBaseUserCmdPB& userCmd, C_CSPlayerPawn& localPawn) noexcept;

    // Валидация пользовательской команды
    void ValidateUserCommand(CUserCmd* pCmd, CBaseUserCmdPB* pUserCmd, CCSGOInputHistoryEntryPB* pInputEntry) noexcept;

    // Коррекция движения
    void MovementCorrection(CBaseUserCmdPB* pUserCmd, CCSGOInputHistoryEntryPB* pInputEntry, const QAngle_t& angDesiredViewPoint) noexcept;
}