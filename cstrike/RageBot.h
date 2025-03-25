#pragma once

#include "core.h"
#include "core/sdk.h"
#include "sdk/datatypes/vector.h" // ��� Vector_t � QAngle_t
#include "sdk/entity.h" // ��� C_CSPlayerPawn � HITBOX_*
#include "sdk/datatypes/usercmd.h"

namespace F::RAGEBOT
{
    void OnMove(CUserCmd* pCmd);
    void Initialize();
    void DumpSchema();
}