#pragma once

#include "core.h"
#include "core/sdk.h"
#include "sdk/datatypes/vector.h" // Для Vector_t и QAngle_t
#include "sdk/entity.h" // Для C_CSPlayerPawn и HITBOX_*
#include "sdk/datatypes/usercmd.h"

namespace F::RAGEBOT
{
    void OnMove(CUserCmd* pCmd);
    void Initialize();
    void DumpSchema();
}