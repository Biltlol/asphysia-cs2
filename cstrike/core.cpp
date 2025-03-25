// used: [win] shgetknownfolderpath
#include <shlobj_core.h>

#include "core.h"

// used: features setup
#include "features.h"
// used: string copy
#include "utilities/crt.h"
// used: mem
#include "utilities/memory.h"
// used: l_print
#include "utilities/log.h"
// used: inputsystem setup/restore
#include "utilities/inputsystem.h"
// used: draw destroy
#include "utilities/draw.h"

// used: interfaces setup/destroy
#include "core/interfaces.h"
// used: sdk setup
#include "core/sdk.h"
// used: config setup & variables
#include "core/variables.h"
// used: hooks setup/destroy
#include "core/hooks.h"
// used: schema setup/dump
#include "core/schema.h"
// used: convar setup
#include "core/convars.h"
// used: menu
#include "core/menu.h"

// used: product version
#include "sdk/interfaces/iengineclient.h"

// Добавляем include для RageBot
#include "RageBot.h"

bool CORE::GetWorkingPath(wchar_t* wszDestination)
{
    bool bSuccess = false;
    PWSTR wszPathToDocuments = nullptr;

    if (SUCCEEDED(::SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_CREATE, nullptr, &wszPathToDocuments)))
    {
        CRT::StringCat(CRT::StringCopy(wszDestination, wszPathToDocuments), CS_XOR(L"\\.asphyxia\\"));
        bSuccess = true;

        if (!::CreateDirectoryW(wszDestination, nullptr))
        {
            if (::GetLastError() != ERROR_ALREADY_EXISTS)
            {
                L_PRINT(LOG_ERROR) << CS_XOR("failed to create default working directory, because one or more intermediate directories don't exist");
                bSuccess = false;
            }
        }
    }
    ::CoTaskMemFree(wszPathToDocuments);

    return bSuccess;
}

static bool Setup(HMODULE hModule)
{
#ifdef CS_LOG_CONSOLE
    if (!L::AttachConsole(CS_XOR(L"asphyxia developer-mode")))
    {
        CS_ASSERT(false); // failed to attach console
        return false;
    }
#endif
#ifdef CS_LOG_FILE
    if (!L::OpenFile(CS_XOR(L"asphyxia.log")))
    {
        CS_ASSERT(false); // failed to open file
        return false;
    }
#endif
    L_PRINT(LOG_NONE) << L::SetColor(LOG_COLOR_FORE_GREEN | LOG_COLOR_FORE_INTENSITY) << CS_XOR("logging system initialization completed");

    if (!MEM::Setup())
    {
        CS_ASSERT(false); // failed to setup memory system
        return false;
    }
    L_PRINT(LOG_NONE) << L::SetColor(LOG_COLOR_FORE_GREEN | LOG_COLOR_FORE_INTENSITY) << CS_XOR("memory system initialization completed");

    if (!MATH::Setup())
    {
        CS_ASSERT(false); // failed to setup math system
        return false;
    }
    L_PRINT(LOG_NONE) << L::SetColor(LOG_COLOR_FORE_GREEN | LOG_COLOR_FORE_INTENSITY) << CS_XOR("math system initialization completed");

    if (!I::Setup())
    {
        CS_ASSERT(false); // failed to setup interfaces
        return false;
    }
    L_PRINT(LOG_NONE) << L::SetColor(LOG_COLOR_FORE_GREEN | LOG_COLOR_FORE_INTENSITY) << CS_XOR("interfaces initialization completed");

    if (!SDK::Setup())
    {
        CS_ASSERT(false); // failed to setup sdk
        return false;
    }
    L_PRINT(LOG_NONE) << L::SetColor(LOG_COLOR_FORE_GREEN | LOG_COLOR_FORE_INTENSITY) << CS_XOR("sdk initialization completed");

    if (!IPT::Setup())
    {
        CS_ASSERT(false); // failed to setup input system
        return false;
    }
    L_PRINT(LOG_NONE) << L::SetColor(LOG_COLOR_FORE_GREEN | LOG_COLOR_FORE_INTENSITY) << CS_XOR("input system initialization completed");

    D::Setup(IPT::hWindow, I::Device, I::DeviceContext);
    MENU::UpdateStyle(nullptr);
    while (D::bInitialized == false)
        ::Sleep(200U);
    L_PRINT(LOG_NONE) << L::SetColor(LOG_COLOR_FORE_GREEN | LOG_COLOR_FORE_INTENSITY) << CS_XOR("renderer backend initialization completed");

    if (!F::Setup())
    {
        CS_ASSERT(false); // failed to setup features
        return false;
    }
    L_PRINT(LOG_NONE) << L::SetColor(LOG_COLOR_FORE_GREEN | LOG_COLOR_FORE_INTENSITY) << CS_XOR("features initialization completed");

    std::vector<std::string> vecNeededModules = { CS_XOR("client.dll"), CS_XOR("engine2.dll"), CS_XOR("schemasystem.dll") };
    for (auto& szModule : vecNeededModules)
    {
        if (!SCHEMA::Setup(CS_XOR(L"schema.txt"), szModule.c_str()))
        {
            CS_ASSERT(false); // failed to setup schema system
            return false;
        }
    }
    L_PRINT(LOG_NONE) << L::SetColor(LOG_COLOR_FORE_GREEN | LOG_COLOR_FORE_INTENSITY) << CS_XOR("schema system initialization completed");

    if (!CONVAR::Dump(CS_XOR(L"convars.txt")))
    {
        CS_ASSERT(false); // failed to setup convars system
        return false;
    }
    L_PRINT(LOG_NONE) << L::SetColor(LOG_COLOR_FORE_GREEN | LOG_COLOR_FORE_INTENSITY) << CS_XOR("convars dumped completed, output: \"convars.txt\"");

    if (!CONVAR::Dump(CS_XOR(L"convars.txt")))
    {
        CS_ASSERT(false); // failed to setup convars system
        return false;
    }
    L_PRINT(LOG_NONE) << L::SetColor(LOG_COLOR_FORE_GREEN | LOG_COLOR_FORE_INTENSITY) << CS_XOR("convars dumped completed, output: \"convars.txt\"");

    if (!CONVAR::Setup())
    {
        CS_ASSERT(false); // failed to setup convars system
        return false;
    }
    L_PRINT(LOG_NONE) << L::SetColor(LOG_COLOR_FORE_GREEN | LOG_COLOR_FORE_INTENSITY) << CS_XOR("convars system initialization completed");

    if (!H::Setup())
    {
        CS_ASSERT(false); // failed to setup hooks
        return false;
    }
    L_PRINT(LOG_NONE) << CS_XOR("hooks initialization completed");

    if (!C::Setup(CS_XOR(CS_CONFIGURATION_DEFAULT_FILE_NAME)))
        L_PRINT(LOG_WARNING) << CS_XOR("failed to setup and/or load default configuration");
    else
        L_PRINT(LOG_NONE) << L::SetColor(LOG_COLOR_FORE_GREEN | LOG_COLOR_FORE_INTENSITY) << CS_XOR("configuration system initialization completed");

    if (CRT::StringCompare(I::Engine->GetProductVersionString(), CS_PRODUCTSTRINGVERSION) != 0)
        L_PRINT(LOG_WARNING) << L::SetColor(LOG_COLOR_FORE_YELLOW | LOG_COLOR_FORE_INTENSITY) << CS_XOR("version mismatch! local CS2 version: ") << CS_PRODUCTSTRINGVERSION << CS_XOR(", current CS2 version: ") << I::Engine->GetProductVersionString() << CS_XOR(". asphyxia might not function as normal.");

    L_PRINT(LOG_NONE) << L::SetColor(LOG_COLOR_FORE_CYAN | LOG_COLOR_FORE_INTENSITY) << CS_XOR("asphyxia initialization completed, version: ") << CS_STRINGIFY(CS_VERSION);

    // Добавляем вызов RageBot
    L_PRINT(LOG_INFO) << "Calling F::RAGEBOT::Initialize...";
    F::RAGEBOT::Initialize();
    L_PRINT(LOG_INFO) << "F::RAGEBOT::Initialize finished.";

    return true;
}

static void Destroy()
{
    IPT::Destroy();
    H::Destroy();
    D::Destroy();
    F::Destroy();

#ifdef CS_LOG_CONSOLE
    L::DetachConsole();
#endif
#ifdef CS_LOG_FILE
    L::CloseFile();
#endif
}

DWORD WINAPI PanicThread(LPVOID lpParameter)
{
    while (!IPT::IsKeyReleased(C_GET(unsigned int, Vars.nPanicKey)))
        ::Sleep(500UL);

    ::FreeLibraryAndExitThread(static_cast<HMODULE>(lpParameter), EXIT_SUCCESS);
}

extern "C" BOOL WINAPI _CRT_INIT(HMODULE hModule, DWORD dwReason, LPVOID lpReserved);

BOOL APIENTRY CoreEntryPoint(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    DisableThreadLibraryCalls(hModule);

    if (dwReason == DLL_PROCESS_DETACH)
        Destroy();

    if (!_CRT_INIT(hModule, dwReason, lpReserved))
        return FALSE;

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        CORE::hProcess = MEM::GetModuleBaseHandle(nullptr);

        if (CORE::hProcess == nullptr)
            return FALSE;

        if (MEM::GetModuleBaseHandle(NAVSYSTEM_DLL) == nullptr)
            return FALSE;

        CORE::hDll = hModule;

        if (!Setup(hModule))
        {
            Destroy();
            return FALSE;
        }

        if (const HANDLE hThread = ::CreateThread(nullptr, 0U, &PanicThread, hModule, 0UL, nullptr); hThread != nullptr)
            ::CloseHandle(hThread);
    }

    return TRUE;
}