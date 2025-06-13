#include "Hooks.h"
#include "Settings.h"

namespace
{
    constexpr RE::FormID kSlowTimeSpellID = 0x802;
    constexpr RE::FormID kSlowTimeSpellFeedbackID = 0x2;
    constexpr RE::FormID kSlowTimeEffectID = 0x800;
    std::atomic combatThreshold{70};
    std::atomic bonusRequested{false};
    std::atomic attackBonusActive{false};
    RE::SpellItem* slowTimeSpell = nullptr;
    RE::SpellItem* slowTimeSpellFeedback = nullptr;
    RE::EffectSetting* slowTimeEffect = nullptr;
    std::vector<RE::BGSPerk*> g_requiredPerksAttackBonus;
    std::vector<RE::BGSPerk*> g_requiredPerksSlowTime;

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

    bool CheckEffectPrerequisites(bool& canApplySlowTime, bool& canApplyAttackBonus)
    {
        canApplySlowTime = false;
        canApplyAttackBonus = false;

        const auto* player = GetValidatedPlayer();
        if (!player)
            return false;

        const auto* settings = Settings::GetSingleton();
        if (!settings->enableSlowTimeEffect && !settings->enableAttackDamageBonus) {
            logger::debug("Effects disabled in settings");
            return false;
        }

        canApplySlowTime = settings->enableSlowTimeEffect &&
                           stl::has_all_required_perks(player, g_requiredPerksSlowTime);
        canApplyAttackBonus = settings->enableAttackDamageBonus &&
                              stl::has_all_required_perks(player, g_requiredPerksAttackBonus);

        if (!canApplySlowTime && !canApplyAttackBonus) {
            logger::debug("Missing required perks for all effects");
            return false;
        }

        return true;
    }

    bool ValidateGameResources()
    {
        if (!slowTimeSpell || !slowTimeSpellFeedback || !slowTimeEffect) {
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

    bool ShouldApplyBonus(RE::Actor* self, RE::Actor* target)
    {
        if (attackBonusActive.load(std::memory_order_relaxed) || Hooks::HasSlowTimeEffectActive()) {
            return false;
        }

        return self->RequestDetectionLevel(target) >= combatThreshold.load(std::memory_order_acquire);
    }
}

namespace Hooks
{
    static void ResolveRequiredPerks()
    {
        if (auto* settings = Settings::GetSingleton(); settings->perkRequirement) {
            g_requiredPerksAttackBonus.reserve(settings->requiredPerksAttackBonus.size());
            g_requiredPerksSlowTime.reserve(settings->requiredPerksSlowTime.size());

            for (auto&& [plugin, id] : settings->requiredPerksAttackBonus) {
                g_requiredPerksAttackBonus.push_back(stl::require_form<RE::BGSPerk>(plugin, id, "sAttackBonusRequiredPerks"));
                logger::info("Resolved perk {}|0x{:06X}", plugin, id);
            }

            for (auto&& [plugin, id] : settings->requiredPerksSlowTime) {
                g_requiredPerksSlowTime.push_back(stl::require_form<RE::BGSPerk>(plugin, id, "sSlowTimeRequiredPerks"));
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

        slowTimeSpell->effects = cloneEffectsWithDuration(slowTimeSpell->effects, a_duration);
        slowTimeSpellFeedback->effects = cloneEffectsWithDuration(slowTimeSpellFeedback->effects, a_duration);
    }

    void Install() noexcept
    {
        slowTimeSpell = stl::require_form<RE::SpellItem>(Settings::pluginName, kSlowTimeSpellID);
        slowTimeSpellFeedback = stl::require_form<RE::SpellItem>(Settings::pluginName, kSlowTimeSpellFeedbackID);
        slowTimeEffect = stl::require_form<RE::EffectSetting>(Settings::pluginName, kSlowTimeEffectID);

        if (const auto duration = Settings::GetSingleton()->duration; duration != Settings::GetSingleton()->defaultDuration) {
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

    [[nodiscard]] bool HasSlowTimeEffectActive() noexcept
    {
        auto* activeEffects = GetPlayerActiveEffects();
        if (!activeEffects)
            return false;

        for (const auto& effect : *activeEffects) {
            if (effect && effect->GetBaseObject() == slowTimeEffect) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] bool HasAttackBonusActive() noexcept
    {
        return attackBonusActive.load(std::memory_order_relaxed);
    }

    void SetMagicTimeSlowdown::Thunk(RE::VATS* vats, float worldMag, [[maybe_unused]] float playerMag)
    {
        if (Settings::GetSingleton()->enableSlowTimeImmunity && HasSlowTimeEffectActive()) {
            playerMag = 1.0f;
        }
        func(vats, worldMag, playerMag);
    }

    void ApplyAttackDamageBonus() noexcept
    {
        const auto* settings = Settings::GetSingleton();

        if (settings->attackDamageBonus <= 0.0f) {
            logger::debug("Skipping attack damage bonus (value is 0 or negative)");
            return;
        }

        if (attackBonusActive.load(std::memory_order_relaxed)) {
            logger::debug("Attack damage bonus already active");
            return;
        }

        auto* player = GetValidatedPlayer();
        if (!player)
            return;

        auto* avOwner = player->AsActorValueOwner();
        if (!avOwner) {
            logger::error("Invalid ActorValueOwner");
            return;
        }

        const float current = avOwner->GetActorValue(RE::ActorValue::kAttackDamageMult);
        const float boosted = current + settings->attackDamageBonus;

        avOwner->SetActorValue(RE::ActorValue::kAttackDamageMult, boosted);
        attackBonusActive.store(true, std::memory_order_release);

        logger::debug("Applied AttackDamageMult Bonus ({}  ->  {})", current, boosted);
    }

    void RestoreAttackDamageBonus() noexcept
    {
        if (!attackBonusActive.load(std::memory_order_relaxed)) {
            return;
        }

        auto* player = GetValidatedPlayer();
        if (!player)
            return;

        auto* avOwner = player->AsActorValueOwner();
        if (!avOwner) {
            logger::error("Invalid ActorValueOwner");
            return;
        }

        const float current = avOwner->GetActorValue(RE::ActorValue::kAttackDamageMult);
        const float restored = current - Settings::GetSingleton()->attackDamageBonus;

        avOwner->SetActorValue(RE::ActorValue::kAttackDamageMult, restored);
        attackBonusActive.store(false, std::memory_order_release);

        logger::debug("Restored AttackDamageMult ({}  ->  {})", current, restored);
    }

    void ApplySlowTimeEffect(RE::MagicCaster* a_magicCaster) noexcept
    {
        if (HasSlowTimeEffectActive()) {
            logger::debug("Slow time effect already active");
            return;
        }

        a_magicCaster->CastSpellImmediate(
            slowTimeSpell,
            !Settings::GetSingleton()->enableEffectFeedback,
            a_magicCaster->GetCasterAsActor(),
            1.0f,
            false,
            0.0f,
            a_magicCaster->GetCasterAsActor());
    }

    void ApplyAllEffects() noexcept
    {
        if (!ValidateGameResources()) {
            return;
        }

        bool canApplySlowTime, canApplyAttackBonus;
        if (!CheckEffectPrerequisites(canApplySlowTime, canApplyAttackBonus)) {
            return;
        }

        auto* player = GetValidatedPlayer();
        if (!player)
            return;

        if (Settings::GetSingleton()->enableEffectFeedback) {
            if (auto* magicCaster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)) {
                magicCaster->CastSpellImmediate(
                    slowTimeSpellFeedback,
                    false,
                    player,
                    1.0f,
                    false,
                    0.0f,
                    player);
            }
        }

        if (canApplySlowTime) {
            if (auto* magicCaster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)) {
                ApplySlowTimeEffect(magicCaster);
            } else {
                logger::error("Invalid magicCaster for slow time effect");
            }
        }

        if (canApplyAttackBonus) {
            ApplyAttackDamageBonus();
        }
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
            if (bonusRequested.exchange(false, std::memory_order_acq_rel)) {
                logger::debug("Combat session ended");
            }
            return;
        }

        if (!ShouldProcessDetection(self, target)) {
            return;
        }

        if (ShouldApplyBonus(self, target)) {
            if (!bonusRequested.exchange(true, std::memory_order_acq_rel)) {
                logger::debug("{} first to spot player -> applying bonus", self->GetDisplayFullName());
                stl::add_thread_task([] { ApplyAllEffects(); }, 0ms);
            }
        }
    }
}
