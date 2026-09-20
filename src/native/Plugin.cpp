#include "Hooks.h"
#include "Settings.h"

#include "DevBenchIntegration.h"

#include <SKSE/SKSE.h>

namespace {
void MessageHandler(SKSE::MessagingInterface::Message* a_message) { // NOLINT(misc-const-correctness)
    switch (a_message->type) {
        case SKSE::MessagingInterface::kPostPostLoad: DevBenchIntegration::Register(); break;
        case SKSE::MessagingInterface::kDataLoaded:   Hooks::Install(); break;
        case SKSE::MessagingInterface::kPreLoadGame:  Hooks::Suspend(); break;
        case SKSE::MessagingInterface::kNewGame:
        case SKSE::MessagingInterface::kPostLoadGame: Hooks::Resume(); break;
        default:                                      break;
    }
}
}

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* a_extender) {
    SKSE::Init(
        a_extender,
        {
            .logPattern = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v",
            .trampoline = true,
            .trampolineSize = 64,
        }
    );
    Settings::GetSingleton()->Load();
    SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);
    return true;
}
