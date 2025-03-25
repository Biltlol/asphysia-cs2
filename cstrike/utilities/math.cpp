#include "math.h"
// used: getexportaddr
#include "memory.h"
// used: QAngle_t
#include "../sdk/datatypes/qangle.h"

namespace MATH
{
    bool Setup()
    {
        bool bSuccess = true;

        const void* hTier0Lib = MEM::GetModuleBaseHandle(TIER0_DLL);
        if (hTier0Lib == nullptr)
            return false;

        fnRandomSeed = reinterpret_cast<decltype(fnRandomSeed)>(MEM::GetExportAddress(hTier0Lib, CS_XOR("RandomSeed")));
        bSuccess &= (fnRandomSeed != nullptr);

        fnRandomFloat = reinterpret_cast<decltype(fnRandomFloat)>(MEM::GetExportAddress(hTier0Lib, CS_XOR("RandomFloat")));
        bSuccess &= (fnRandomFloat != nullptr);

        fnRandomFloatExp = reinterpret_cast<decltype(fnRandomFloatExp)>(MEM::GetExportAddress(hTier0Lib, CS_XOR("RandomFloatExp")));
        bSuccess &= (fnRandomFloatExp != nullptr);

        fnRandomInt = reinterpret_cast<decltype(fnRandomInt)>(MEM::GetExportAddress(hTier0Lib, CS_XOR("RandomInt")));
        bSuccess &= (fnRandomInt != nullptr);

        fnRandomGaussianFloat = reinterpret_cast<decltype(fnRandomGaussianFloat)>(MEM::GetExportAddress(hTier0Lib, CS_XOR("RandomGaussianFloat")));
        bSuccess &= (fnRandomGaussianFloat != nullptr);

        return bSuccess;
    }

    void VectorAngles(const Vector_t& forward, QAngle_t& angles)
    {
        float tmp, yaw, pitch;

        if (forward.y == 0.f && forward.x == 0.f)
        {
            yaw = 0.f;
            if (forward.z > 0.f)
                pitch = 270.f;
            else
                pitch = 90.f;
        }
        else
        {
            yaw = (atan2(forward.y, forward.x) * 180.f / _PI);
            if (yaw < 0.f)
                yaw += 360.f;

            tmp = sqrt(forward.x * forward.x + forward.y * forward.y);
            pitch = (atan2(-forward.z, tmp) * 180.f / _PI);
            if (pitch < 0.f)
                pitch += 360.f;
        }

        angles.x = pitch;
        angles.y = yaw;
        angles.z = 0.f;
    }

    float GetFOV(const QAngle_t& viewAngles, const QAngle_t& aimAngles)
    {
        QAngle_t delta = aimAngles - viewAngles;

        // Нормализация углов
        while (delta.x > 180.f) delta.x -= 360.f;
        while (delta.x < -180.f) delta.x += 360.f;
        while (delta.y > 180.f) delta.y -= 360.f;
        while (delta.y < -180.f) delta.y += 360.f;

        return sqrt(delta.x * delta.x + delta.y * delta.y);
    }
}