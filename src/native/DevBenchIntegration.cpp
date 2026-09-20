#include "DevBenchIntegration.h"
#include "Hooks.h"
#include "Settings.h"

#include <BMK/Skyrim/DevBench.h>
#include <DevBenchAPI.h>
#include <RE/P/PlayerCharacter.h>
#include <RE/V/VATS.h>
#include <SKSE/SKSE.h>

#include <cstdint>
#include <format>
#include <string>

namespace {
std::string Inspect() {
    const auto state = Hooks::GetDiagnostics();
    const auto& settings = Settings::GetSingleton()->GetValues();
    const auto* player = RE::PlayerCharacter::GetSingleton();
    const auto* vats = RE::VATS::GetSingleton();
    return std::format(
        R"({{"ok":true,"installed":{},"loading":{},"loadGeneration":{},"activations":{},)"
        R"("detections":{},"lastDetector":{},"lastDetectionScore":{},"appliedThisCombat":{},)"
        R"("inCombat":{},"sneaking":{},"worldSlowdown":{},"playerSlowdown":{},)"
        R"("settings":{{"duration":{},"feedback":{},"playerImmunity":{},"perkAddon":{},)"
        R"("requiredPerks":{},"validPerks":{}}}}})",
        state.installed,
        state.loading,
        state.loadGeneration,
        state.activations,
        state.detections,
        state.lastDetector,
        state.lastDetectionScore,
        state.appliedThisCombat,
        (player != nullptr) && player->IsInCombat(),
        (player != nullptr) && player->IsSneaking(),
        (vats != nullptr) ? vats->magicTimeSlowdown : 1.0F,
        (vats != nullptr) ? vats->playerMagicTimeSlowdown : 1.0F,
        settings.duration,
        settings.effectFeedback,
        settings.slowTimeImmunity,
        state.perkAddonEnabled,
        state.requiredPerks,
        state.requirementsValid
    );
}

constexpr BMK::Skyrim::DevBench::Inspection kInspection {
    .name = "shadowopportunist",
    .descriptor
    = R"({"description":"ShadowOpportunist hook status, activation counts, combat state, slowdown multipliers and settings.","readOnly":true})",
    .snapshot = Inspect,
    .timeoutResponse = R"({"ok":false,"error":"ShadowOpportunist inspection timed out waiting for the game thread"})",
    .failureResponse = R"({"ok":false,"error":"ShadowOpportunist inspection failed. See the plugin log."})",
};
}

void DevBenchIntegration::Register() {
    BMK::Skyrim::DevBench::RegisterInspection<kInspection>();
}

void DevBenchIntegration::NotifyActivation(std::uint64_t a_count) {
    if (g_devBenchInterface != nullptr) {
        const auto payload = std::format(R"({{"count":{}}})", a_count);
        g_devBenchInterface->EmitEvent("shadowopportunist.activated", payload.c_str());
    }
}
