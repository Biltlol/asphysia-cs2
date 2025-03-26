#pragma once

#include <cstddef>

namespace offsets {
    // Module: client.dll
    constexpr std::ptrdiff_t dwLocalPlayerController = 0x1A890C0; // Из твоего дампа
    constexpr std::ptrdiff_t dwLocalPlayerPawn = 0x188BF30;      // Из твоего дампа
    constexpr std::ptrdiff_t dwEntityList = 0x1A37A30;           // Из твоего дампа
    constexpr std::ptrdiff_t dwViewAngles = 0x1AADAA0;           // Из твоего дампа
    constexpr std::ptrdiff_t dwGameRules = 0x1A9E850;            // Из твоего дампа
    constexpr std::ptrdiff_t dwGlobalVars = 0x187FC90;           // Из твоего дампа
    constexpr std::ptrdiff_t dwGameEntitySystem = 0x1B5E798;      // Из твоего дампа
    constexpr std::ptrdiff_t dwGameEntitySystem_highestEntityIndex = 0x20F0; // Из твоего дампа

    // Для bunnyhop и других функций
    constexpr std::ptrdiff_t m_iHealth = 0x32C;                  // Без изменений
    constexpr std::ptrdiff_t m_vecOrigin = 0x134;                // Без изменений
    constexpr std::ptrdiff_t m_iTeamNum = 0xF4;                  // Без изменений
    constexpr std::ptrdiff_t m_aimPunchAngle = 0x304;            // Без изменений
    constexpr std::ptrdiff_t m_fFlags = 0x104;                   // Без изменений

    // Module: engine2.dll
    constexpr std::ptrdiff_t dwNetworkGameClient = 0x53FCE0;     // Из твоего дампа
}