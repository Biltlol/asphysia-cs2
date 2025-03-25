#include "RageBot.h"
#include "core/interfaces.h"
#include "utilities/log.h"

namespace F::RAGEBOT
{
    void OnMove(CUserCmd* pCmd)
    {
        L_PRINT(LOG_INFO) << "RageBot::OnMove called";
    }

    void Initialize()
    {
        L_PRINT(LOG_INFO) << "F::RAGEBOT::Initialize started.";
        DumpSchema();
        L_PRINT(LOG_INFO) << "F::RAGEBOT::Initialize finished.";
    }

    void DumpSchema()
    {
        L_PRINT(LOG_INFO) << "DumpSchema started.";

        // Проверяем, что I::SchemaSystem не nullptr
        if (!I::SchemaSystem)
        {
            L_PRINT(LOG_ERROR) << "I::SchemaSystem is nullptr.";
            return;
        }
        L_PRINT(LOG_INFO) << "I::SchemaSystem is valid.";

        // Пробуем разные модули
        const char* moduleNames[] = { "client.dll", "server.dll", "engine.dll" };
        CSchemaSystemTypeScope* pTypeScope = nullptr;

        for (const char* moduleName : moduleNames)
        {
            L_PRINT(LOG_INFO) << "Finding type scope for " << moduleName << "...";
            pTypeScope = I::SchemaSystem->FindTypeScopeForModule(moduleName);
            if (pTypeScope)
            {
                L_PRINT(LOG_INFO) << "Type scope for " << moduleName << " found.";
                break;
            }
            L_PRINT(LOG_ERROR) << "Failed to find type scope for " << moduleName;
        }

        if (!pTypeScope)
        {
            L_PRINT(LOG_ERROR) << "Failed to find any type scope.";
            return;
        }

        // Пробуем разные имена классов
        const char* playerPawnNames[] = { "C_CSPlayerPawn", "C_CSPlayerPawnBase", "C_BasePlayerPawn" };
        SchemaClassInfoData_t* pPlayerPawnInfo = nullptr;

        for (const char* playerPawnName : playerPawnNames)
        {
            L_PRINT(LOG_INFO) << "Finding class info for " << playerPawnName << "...";
            pTypeScope->FindDeclaredClass(&pPlayerPawnInfo, playerPawnName);
            if (pPlayerPawnInfo)
            {
                L_PRINT(LOG_INFO) << "Class info for " << playerPawnName << " found.";
                break;
            }
            L_PRINT(LOG_ERROR) << "Failed to find class info for " << playerPawnName;
        }

        if (!pPlayerPawnInfo)
        {
            L_PRINT(LOG_ERROR) << "Failed to find any player pawn class.";
            return;
        }

        // Выводим информацию о классе
        L_PRINT(LOG_INFO) << "Class: " << pPlayerPawnInfo->szName << ", Field Count: " << pPlayerPawnInfo->nFieldSize;

        // Ищем поле m_hitboxComponent
        L_PRINT(LOG_INFO) << "Searching for m_hitboxComponent in " << pPlayerPawnInfo->szName << "...";
        SchemaClassFieldData_t* pHitboxField = nullptr;
        for (int i = 0; i < pPlayerPawnInfo->nFieldSize; i++)
        {
            SchemaClassFieldData_t& field = pPlayerPawnInfo->pFields[i];
            L_PRINT(LOG_INFO) << "Field[" << i << "]: " << field.szName << ", Type: " << (field.pSchemaType ? field.pSchemaType->szName : "null") << ", Offset: " << field.nSingleInheritanceOffset;
            if (FNV1A::Hash(field.szName) == FNV1A::HashConst("m_hitboxComponent"))
            {
                pHitboxField = &field;
            }
        }

        if (!pHitboxField || !pHitboxField->pSchemaType)
        {
            L_PRINT(LOG_ERROR) << "Failed to find m_hitboxComponent in " << pPlayerPawnInfo->szName;
            return;
        }
        L_PRINT(LOG_INFO) << "m_hitboxComponent found.";

        // Находим информацию о классе CHitboxComponent
        L_PRINT(LOG_INFO) << "Finding class info for CHitboxComponent...";
        SchemaClassInfoData_t* pHitboxComponentInfo = nullptr;
        pTypeScope->FindDeclaredClass(&pHitboxComponentInfo, "CHitboxComponent");
        if (!pHitboxComponentInfo)
        {
            L_PRINT(LOG_ERROR) << "Failed to find class info for CHitboxComponent";
            return;
        }
        L_PRINT(LOG_INFO) << "Class info for CHitboxComponent found.";

        // Выводим информацию о классе CHitboxComponent
        L_PRINT(LOG_INFO) << "Class: " << pHitboxComponentInfo->szName << ", Field Count: " << pHitboxComponentInfo->nFieldSize;

        // Перебираем поля класса CHitboxComponent
        L_PRINT(LOG_INFO) << "Enumerating fields of CHitboxComponent...";
        for (int i = 0; i < pHitboxComponentInfo->nFieldSize; i++)
        {
            SchemaClassFieldData_t& field = pHitboxComponentInfo->pFields[i];
            L_PRINT(LOG_INFO) << "Field[" << i << "]: " << field.szName << ", Type: " << (field.pSchemaType ? field.pSchemaType->szName : "null") << ", Offset: " << field.nSingleInheritanceOffset;
        }

        // Ищем поле, связанное с хитбоксами (например, m_hitboxes)
        L_PRINT(LOG_INFO) << "Searching for hitbox array field in CHitboxComponent...";
        SchemaClassFieldData_t* pHitboxesField = nullptr;
        for (int i = 0; i < pHitboxComponentInfo->nFieldSize; i++)
        {
            SchemaClassFieldData_t& field = pHitboxComponentInfo->pFields[i];
            if (field.pSchemaType && strstr(field.pSchemaType->szName, "Hitbox"))
            {
                pHitboxesField = &field;
                break;
            }
        }

        if (!pHitboxesField || !pHitboxField->pSchemaType)
        {
            L_PRINT(LOG_ERROR) << "Failed to find hitbox array field in CHitboxComponent";
            return;
        }
        L_PRINT(LOG_INFO) << "Hitbox array field found.";

        // Находим информацию о классе Hitbox_t
        L_PRINT(LOG_INFO) << "Finding class info for " << pHitboxesField->pSchemaType->szName << "...";
        SchemaClassInfoData_t* pHitboxInfo = nullptr;
        pTypeScope->FindDeclaredClass(&pHitboxInfo, pHitboxesField->pSchemaType->szName);
        if (!pHitboxInfo)
        {
            L_PRINT(LOG_ERROR) << "Failed to find class info for " << pHitboxesField->pSchemaType->szName;
            return;
        }
        L_PRINT(LOG_INFO) << "Class info for " << pHitboxesField->pSchemaType->szName << " found.";

        // Выводим информацию о классе Hitbox_t
        L_PRINT(LOG_INFO) << "Class: " << pHitboxInfo->szName << ", Field Count: " << pHitboxInfo->nFieldSize;

        // Перебираем поля класса Hitbox_t
        L_PRINT(LOG_INFO) << "Enumerating fields of " << pHitboxInfo->szName << "...";
        for (int i = 0; i < pHitboxInfo->nFieldSize; i++)
        {
            SchemaClassFieldData_t& field = pHitboxInfo->pFields[i];
            L_PRINT(LOG_INFO) << "Field[" << i << "]: " << field.szName << ", Type: " << (field.pSchemaType ? field.pSchemaType->szName : "null") << ", Offset: " << field.nSingleInheritanceOffset;
        }

        L_PRINT(LOG_INFO) << "DumpSchema finished successfully.";
    }
}