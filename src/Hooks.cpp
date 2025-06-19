#include "Hooks.h"
#include "Settings.h"

namespace
{
    constexpr RE::FormID shadowOpportunistSpellID = 0x1;
    constexpr RE::FormID shadowOpportunistFeedbackSpellID = 0x2;
    constexpr RE::FormID shadowOpportunistEffectID = 0x3;
    std::atomic combatThreshold{70};
    std::atomic applyRequested{false};
    RE::SpellItem* shadowOpportunistSpell = nullptr;
    RE::SpellItem* shadowOpportunistFeedbackSpell = nullptr;
    RE::EffectSetting* shadowOpportunistEffect = nullptr;
    std::vector<RE::BGSPerk*> requiredPerks;

    RE::PlayerCharacter* GetValidatedPlayer()
    {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            logger::error("Invalid player reference");
        }
        return player;
    }

    RE::BSSimpleList<RE::ActiveEffect*>* GetPlayerActiveEffects()
    {
        auto* player = GetValidatedPlayer();
        if (!player)
            return nullptr;

        auto* magicTarget = player->AsMagicTarget();
        if (!magicTarget) {
            logger::debug("No MagicTarget yet (likely during load/menu)");
            return nullptr;
        }

        return magicTarget->GetActiveEffectList();
    }

    bool CheckEffectPrerequisites()
    {
        const auto* player = GetValidatedPlayer();
        if (!player)
            return false;

        if (!stl::has_all_required_perks(player, requiredPerks)) {
            logger::debug("Missing required perks");
            return false;
        }

        return true;
    }

    bool ValidateGameResources()
    {
        if (!shadowOpportunistSpell || !shadowOpportunistFeedbackSpell || !shadowOpportunistEffect) {
            logger::error("Critical game resources not loaded properly");
            return false;
        }
        return true;
    }

    bool ShouldProcessDetection(const RE::Actor* self, const RE::Actor* target)
    {
        if (!target || !target->IsPlayerRef() || !target->IsInCombat() || !target->IsSneaking())
            return false;
        if (!self || self->IsDeleted() || self->IsDead() || self->IsMarkedForDeletion())
            return false;

        return true;
    }

    bool ShouldApply(RE::Actor* self, RE::Actor* target)
    {
        if (Hooks::HasShadowOpportunistEffectActive()) {
            return false;
        }

        return self->RequestDetectionLevel(target) >= combatThreshold.load(std::memory_order_acquire);
    }
}

namespace Hooks
{
    static void ResolveRequiredPerks()
    {
        if (auto* settings = Settings::GetSingleton(); !settings->requiredPerks.empty()) {
            requiredPerks.reserve(settings->requiredPerks.size());

            for (auto&& [plugin, id] : settings->requiredPerks) {
                requiredPerks.push_back(stl::require_form<RE::BGSPerk>(plugin, id, "sRequiredPerks"));
                logger::info("Resolved perk {}|0x{:06X}", plugin, id);
            }
        }
    }

    void ModifyEffectsDuration(const uint32_t a_duration)
    {
        auto cloneEffectsWithDuration = [&](const RE::BSTArray<RE::Effect*>& source, const uint32_t duration) {
            RE::BSTArray<RE::Effect*> result;
            for (const auto* effect : source) {
                if (effect) {
                    auto* cloned = new RE::Effect(*effect);
                    cloned->effectItem.duration = duration;
                    result.emplace_back(cloned);
                }
            }
            return result;
        };

        shadowOpportunistSpell->effects = cloneEffectsWithDuration(shadowOpportunistSpell->effects, a_duration);
        shadowOpportunistFeedbackSpell->effects = cloneEffectsWithDuration(shadowOpportunistFeedbackSpell->effects, a_duration);
    }

    void Install() noexcept
    {
        shadowOpportunistSpell = stl::require_form<RE::SpellItem>(Settings::pluginName, shadowOpportunistSpellID);
        shadowOpportunistFeedbackSpell = stl::require_form<RE::SpellItem>(Settings::pluginName, shadowOpportunistFeedbackSpellID);
        shadowOpportunistEffect = stl::require_form<RE::EffectSetting>(Settings::pluginName, shadowOpportunistEffectID);

        if (const auto duration = Settings::GetSingleton()->duration; duration != Settings::defaultDuration) {
            ModifyEffectsDuration(duration);
        }

        ResolveRequiredPerks();

        if (auto* gsCollection = RE::GameSettingCollection::GetSingleton()) {
            if (const auto* gs = gsCollection->GetSetting("iCombatStealthPointSneakDetectionThreshold")) {
                combatThreshold.store(gs->GetSInt(), std::memory_order_release);
            }
        }

        const REL::Relocation SetMagicTimeSlowdown_Loc{RELOCATION_ID(34175, 34968), REL::Relocate(0x5D, 0x5D)};
        stl::write_thunk_call<SetMagicTimeSlowdown>(SetMagicTimeSlowdown_Loc);

        // SSE: Down    p   DoDetectionJob_140719990+526    call    Actor__CalculateDetection_1405FD870
        // AE:  Down    p   DoDetectionJob_1407B0710+67B    call    Actor__CalculateDetection_140690870
        const REL::Relocation CalculateDetection_Loc{RELOCATION_ID(41659, 42742), REL::Relocate(0x526, 0x67B)};
        stl::write_thunk_call<CalculateDetection>(CalculateDetection_Loc);

        logger::info("Hooks installed successfully");
    }

    [[nodiscard]] bool HasShadowOpportunistEffectActive() noexcept
    {
        auto* activeEffects = GetPlayerActiveEffects();
        if (!activeEffects)
            return false;

        for (const auto& effect : *activeEffects) {
            if (effect && effect->GetBaseObject() == shadowOpportunistEffect) {
                return true;
            }
        }
        return false;
    }

    void SetMagicTimeSlowdown::Thunk(RE::VATS* vats, float worldMag, [[maybe_unused]] float playerMag)
    {
        if (Settings::GetSingleton()->enableSlowTimeImmunity && HasShadowOpportunistEffectActive()) {
            playerMag = 1.0f;
        }
        func(vats, worldMag, playerMag);
    }

    void Apply() noexcept
    {
        if (!ValidateGameResources() || !CheckEffectPrerequisites()) {
            return;
        }

        auto* player = GetValidatedPlayer();
        if (!player)
            return;

        if (HasShadowOpportunistEffectActive()) {
            logger::debug("Effect already active");
            return;
        }

        auto* magicCaster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
        if (!magicCaster) {
            logger::error("Invalid MagicTarget");
            return;
        }

        if (Settings::GetSingleton()->enableEffectFeedback) {
            magicCaster->CastSpellImmediate(
                    shadowOpportunistFeedbackSpell,
                    false,
                    player,
                    1.0f,
                    false,
                    0.0f,
                    player);
        }

        magicCaster->CastSpellImmediate(
            shadowOpportunistSpell,
            !Settings::GetSingleton()->enableEffectFeedback,
            player,
            1.0f,
            false,
            0.0f,
            player);
    }

    void CalculateDetection::Thunk(RE::Actor* self,
                                   RE::Actor* target,
                                   std::int32_t* score,
                                   bool* spotted,
                                   bool* hasLOS,
                                   std::int32_t* reason,
                                   RE::NiPoint3* lastPos,
                                   std::int32_t* soundLvl,
                                   std::int32_t* unk8,
                                   std::int32_t* unk9) noexcept
    {
        func(self, target, score, spotted, hasLOS, reason, lastPos, soundLvl, unk8, unk9);

        if (target && target->IsPlayerRef() && !target->IsInCombat()) {
            if (applyRequested.exchange(false, std::memory_order_acq_rel)) {
                logger::debug("Combat session ended");
            }
            return;
        }

        if (!ShouldProcessDetection(self, target)) {
            return;
        }

        if (ShouldApply(self, target)) {
            if (!applyRequested.exchange(true, std::memory_order_acq_rel)) {
                stl::add_thread_task([] { Apply(); }, 0ms);
            }
        }
    }
}
