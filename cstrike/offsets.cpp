#pragma once

#include <cstddef>

namespace offsets {
    // Module: client.dll
    constexpr std::ptrdiff_t dwLocalPlayerController = 0x1A890C0; // �� ������ �����
    constexpr std::ptrdiff_t dwLocalPlayerPawn = 0x188BF30;      // �� ������ �����
    constexpr std::ptrdiff_t dwEntityList = 0x1A37A30;           // �� ������ �����
    constexpr std::ptrdiff_t dwViewAngles = 0x1AADAA0;           // �� ������ �����
    constexpr std::ptrdiff_t dwGameRules = 0x1A9E850;            // �� ������ �����
    constexpr std::ptrdiff_t dwGlobalVars = 0x187FC90;           // �� ������ �����
    constexpr std::ptrdiff_t dwGameEntitySystem = 0x1B5E798;      // �� ������ �����
    constexpr std::ptrdiff_t dwGameEntitySystem_highestEntityIndex = 0x20F0; // �� ������ �����

    // ��� bunnyhop � ������ �������
    constexpr std::ptrdiff_t m_iHealth = 0x32C;                  // ��� ���������
    constexpr std::ptrdiff_t m_vecOrigin = 0x134;                // ��� ���������
    constexpr std::ptrdiff_t m_iTeamNum = 0xF4;                  // ��� ���������
    constexpr std::ptrdiff_t m_aimPunchAngle = 0x304;            // ��� ���������
    constexpr std::ptrdiff_t m_fFlags = 0x104;                   // ��� ���������

    // Module: engine2.dll
    constexpr std::ptrdiff_t dwNetworkGameClient = 0x53FCE0;     // �� ������ �����
}