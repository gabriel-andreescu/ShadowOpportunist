#include "Hooks.h"
#include "Settings.h"

#include "DevBenchIntegration.h"

#include <RE/Skyrim.h> // IWYU pragma: keep

#include <RE/A/Actor.h>
#include <RE/B/BGSPerk.h>
#include <RE/B/BSPointerHandle.h>
#include <RE/E/EffectSetting.h>
#include <RE/G/GameSettingCollection.h>
#include <RE/M/MagicSystem.h>
#include <RE/M/MagicTarget.h>
#include <RE/N/NiPoint3.h>
#include <RE/Offsets_VTABLE.h>
#include <RE/P/PlayerCharacter.h>
#include <RE/S/SpellItem.h>
#include <RE/T/TESDataHandler.h>
#include <RE/V/VATS.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <vector>

namespace {
constexpr auto kPluginName = "ShadowOpportunist.esp";
constexpr auto kPerkAddonName = "ShadowOpportunist_Perk.esp";
constexpr auto kPerkAddonFormId = 0x800;

struct HookState {
    RE::SpellItem* spell {};
    RE::SpellItem* feedbackSpell {};
    RE::EffectSetting* effect {};
    std::vector<RE::BGSPerk*> perks;
    std::int32_t detectionThreshold {70};
    bool applied {};
    std::atomic<std::uint64_t> loadGeneration {0};
    std::atomic<bool> loading {false};
    Hooks::Diagnostics diagnostics;
};

HookState& GetState() {
    static HookState state;
    return state;
}

void Apply(RE::PlayerCharacter* a_player) {
    const auto& state = GetState();
    const auto& settings = Settings::GetSingleton()->GetValues();
    if (a_player->AsMagicTarget()->HasMagicEffect(state.effect)
        || !std::ranges::all_of(state.perks, [a_player](auto* a_perk) { return a_player->HasPerk(a_perk); })) {
        return;
    }
    auto* caster = a_player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
    if (caster == nullptr) {
        SKSE::log::error("Cannot apply Shadow Opportunist: the player's instant magic caster is unavailable");
        return;
    }
    if (settings.effectFeedback) {
        caster->CastSpellImmediate(state.feedbackSpell, false, a_player, 1.0F, false, 0.0F, a_player);
    }
    caster->CastSpellImmediate(state.spell, !settings.effectFeedback, a_player, 1.0F, false, 0.0F, a_player);
    DevBenchIntegration::NotifyActivation(++GetState().diagnostics.activations);
    SKSE::log::debug("Applied Shadow Opportunist");
}

void ProcessDetection(const RE::ActorHandle& a_detectorHandle, std::uint64_t a_generation) {
    auto& state = GetState();
    if (state.loading.load() || a_generation != state.loadGeneration.load()) {
        return;
    }
    ++state.diagnostics.detections;
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (player == nullptr) {
        return;
    }
    if (!player->IsInCombat()) {
        state.applied = false;
        return;
    }
    if (state.applied || !player->IsSneaking()) {
        return;
    }
    const auto detector = a_detectorHandle.get();
    if (!detector || detector->IsDeleted() || detector->IsDead() || detector->IsMarkedForDeletion()) {
        return;
    }
    const auto score = detector->RequestDetectionLevel(player);
    state.diagnostics.lastDetectionScore = score;
    state.diagnostics.lastDetector = detector->GetFormID();
    if (score < state.detectionThreshold) {
        return;
    }
    state.applied = true;
    Apply(player);
}

struct CalculateDetection {
    // The engine ABI requires all ten parameters.
    // NOLINTNEXTLINE(readability-function-size)
    static void Thunk(
        RE::Actor* a_detector,
        RE::Actor* a_target,
        std::int32_t* a_score,
        bool* a_spotted,
        bool* a_hasLos,
        std::int32_t* a_reason,
        RE::NiPoint3* a_lastPosition,
        float* a_soundLevel,
        float* a_visualLevel,
        float* a_lightLevel
    ) {
        func(
            a_detector,
            a_target,
            a_score,
            a_spotted,
            a_hasLos,
            a_reason,
            a_lastPosition,
            a_soundLevel,
            a_visualLevel,
            a_lightLevel
        );
        if ((a_detector == nullptr) || (a_target == nullptr) || !a_target->IsPlayerRef()) {
            return;
        }
        const auto generation = GetState().loadGeneration.load();
        if (GetState().loading.load()) {
            return;
        }
        // Detection jobs pass handles to the game thread before inspecting mutable actor state.
        SKSE::GetTaskInterface()->AddTask([handle = a_detector->GetHandle(), generation] {
            ProcessDetection(handle, generation);
        });
    }
    inline static REL::Relocation<decltype(Thunk)> func;
};

struct SetMagicTimeSlowdown {
    static void Thunk(RE::VATS* a_vats, float a_worldMagnitude, float a_playerMagnitude) {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (Settings::GetSingleton()->GetValues().slowTimeImmunity
            && (player != nullptr)
            && player->AsMagicTarget()->HasMagicEffect(GetState().effect)) {
            a_playerMagnitude = 1.0F;
        }
        func(a_vats, a_worldMagnitude, a_playerMagnitude);
    }
    inline static REL::Relocation<decltype(Thunk)> func;
};

bool ResolveForms() {
    auto& state = GetState();
    auto* data = RE::TESDataHandler::GetSingleton();
    state.spell = data->LookupForm<RE::SpellItem>(0x800, kPluginName);
    state.feedbackSpell = data->LookupForm<RE::SpellItem>(0x801, kPluginName);
    state.effect = data->LookupForm<RE::EffectSetting>(0x802, kPluginName);
    if ((state.spell == nullptr) || (state.feedbackSpell == nullptr) || (state.effect == nullptr)) {
        SKSE::log::error(
            "ShadowOpportunist.esp must contain spells 0x800/0x801 and magic effect 0x802. Activation is disabled."
        );
        return false;
    }

    state.perks.clear();
    state.diagnostics.perkAddonEnabled = data->LookupLoadedModByName(kPerkAddonName)
                                         != nullptr
                                         || data->LookupLoadedLightModByName(kPerkAddonName)
                                         != nullptr;
    state.diagnostics.requirementsValid = false;
    state.diagnostics.requiredPerks = 0;
    if (state.diagnostics.perkAddonEnabled) {
        auto* perk = data->LookupForm<RE::BGSPerk>(kPerkAddonFormId, kPerkAddonName);
        if (perk == nullptr) {
            SKSE::log::error(
                "{} is enabled but does not contain perk 0x{:X}. Activation is disabled.",
                kPerkAddonName,
                kPerkAddonFormId
            );
            return false;
        }
        state.perks.push_back(perk);
        state.diagnostics.requirementsValid = true;
        state.diagnostics.requiredPerks = 1;
        return true;
    }

    const auto& settings = Settings::GetSingleton()->GetValues();
    if (!settings.validPerks) {
        SKSE::log::error("Custom perk requirements are invalid. Activation is disabled.");
        return false;
    }
    for (const auto& requirement : settings.requiredPerks) {
        auto* perk = data->LookupForm<RE::BGSPerk>(requirement.formId, requirement.plugin);
        if (perk == nullptr) {
            SKSE::log::error(
                "Required perk 0x{:X}~{} was not found. Activation is disabled.",
                requirement.formId,
                requirement.plugin
            );
            return false;
        }
        state.perks.push_back(perk);
    }
    state.diagnostics.requirementsValid = true;
    state.diagnostics.requiredPerks = state.perks.size();
    return true;
}

void SetDuration() {
    const auto duration = Settings::GetSingleton()->GetValues().duration;
    if (duration == ModSettings {}.duration) {
        return;
    }
    for (const auto* spell : {GetState().spell, GetState().feedbackSpell}) {
        for (auto* effect : spell->effects) {
            if (effect != nullptr) {
                effect->effectItem.duration = duration;
            }
        }
    }
}

bool IsCall(std::uintptr_t a_address) {
    const REL::Relocation<const std::uint8_t*> instruction {a_address};
    // Address Library resolves this instruction in the running executable.
    // NOLINTNEXTLINE(clang-analyzer-core.FixedAddressDereference)
    return *instruction == 0xE8;
}
}

void Hooks::Install() {
    if (!ResolveForms()) {
        return;
    }
    SetDuration();
    if (const auto* threshold = RE::GameSettingCollection::GetSingleton()->GetSetting(
            "iCombatStealthPointSneakDetectionThreshold"
        )) {
        GetState().detectionThreshold = threshold->GetInteger();
    }
    const REL::Relocation<std::uintptr_t*> vtable {RE::VTABLE_SlowTimeEffect[0]};
    // Start is slot 0x14 on SE, AE and VR. Its slowdown call is at +0x5D.
    const REL::Relocation<const std::uintptr_t*> startSlot {vtable.address() + (0x14 * sizeof(std::uintptr_t))};
    // This slot belongs to the executable's resolved SlowTimeEffect vtable.
    // NOLINTNEXTLINE(clang-analyzer-core.FixedAddressDereference)
    const auto slowdown = *startSlot + 0x5D;
    const REL::Relocation<> detection {RELOCATION_ID(41659, 42742), REL::Relocate(0x526, 0x67B)};
    if (!IsCall(slowdown) || !IsCall(detection.address())) {
        SKSE::log::error(
            "Unexpected hook instructions at slowdown {:X} or detection {:X}. Activation is disabled.",
            slowdown,
            detection.address()
        );
        return;
    }
    auto& trampoline = SKSE::GetTrampoline();
    SetMagicTimeSlowdown::func = trampoline.write_call<5>(slowdown, SetMagicTimeSlowdown::Thunk);
    CalculateDetection::func = trampoline.write_call<5>(detection.address(), CalculateDetection::Thunk);
    GetState().diagnostics.installed = true;
    SKSE::log::info("Installed slow-time and detection hooks");
}

void Hooks::Suspend() {
    auto& state = GetState();
    state.loading.store(true);
    state.loadGeneration.fetch_add(1);
    state.applied = false;
}

void Hooks::Resume() {
    auto& state = GetState();
    state.loadGeneration.fetch_add(1);
    state.applied = false;
    state.loading.store(false);
}

Hooks::Diagnostics Hooks::GetDiagnostics() {
    const auto& state = GetState();
    auto result = state.diagnostics;
    result.appliedThisCombat = state.applied;
    result.loading = state.loading.load();
    result.loadGeneration = state.loadGeneration.load();
    return result;
}
