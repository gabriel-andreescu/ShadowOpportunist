// ReSharper disable CppDFAConstantParameter
#pragma once

class Settings : public REX::Singleton<Settings>
{
public:
    void Load()
    {
        const auto path = std::format("Data/SKSE/Plugins/{}.ini", Plugin::NAME);

        CSimpleIniA ini;
        ini.SetUnicode();

        ini.LoadFile(path.c_str());

        get_value(ini, debugLogging, "General", "bDebugLogging", "; Toggle debug logging");
        get_value(ini, enableEffectFeedback, "General", "bEnableEffectFeedback", "; Toggle visuals and sound");
        get_value(ini, perkRequirement, "General", "bRequiredPerk", "; Toggle perk requirement");
        get_value(ini, duration, "General", "iDuration", "; Duration (seconds) for attack damage bonus and slow time effect");

        get_value(ini, enableAttackDamageBonus, "AttackDamageBonus", "bEnabled", "; Toggle the attack damage bonus");
        get_value(ini, attackDamageBonus, "AttackDamageBonus", "fAttackDamageBonus", "; Amount added to AttackDamageMult (0.5 = +50%)");
        get_value(ini, requiredPerksAttackBonus, "AttackDamageBonus", "sAttackBonusRequiredPerks", "; Perk requirements for the attack damage bonus (format: plugin|formID)");

        get_value(ini, enableSlowTimeEffect, "SlowTimeEffect", "bEnabled", "; Toggle the slow time effect");
        get_value(ini, enableSlowTimeImmunity, "SlowTimeEffect", "sSlowTimePlayerImmunity", "; Toggle player immunity to the slow time effect");
        get_value(ini, requiredPerksSlowTime, "SlowTimeEffect", "sSlowTimeRequiredPerks", "; Perk requirements for the slow time effect (format: plugin|formID)");

        (void)ini.SaveFile(path.c_str());
    }

    // members
    static constexpr uint32_t defaultDuration = 3;
    static constexpr auto pluginName = "ShadowOpportunist.esp";
    bool debugLogging{false};
    bool enableEffectFeedback{true};
    bool perkRequirement{true};
    uint32_t duration{defaultDuration};

    bool enableAttackDamageBonus{true};
    float attackDamageBonus{0.5f};
    std::vector<std::pair<std::string, uint32_t>> requiredPerksAttackBonus{{pluginName, 0x5}};

    bool enableSlowTimeEffect{true};
    bool enableSlowTimeImmunity{true};
    std::vector<std::pair<std::string, uint32_t>> requiredPerksSlowTime{{pluginName, 0x6}};

private:
    template <class T>
    static void get_value(CSimpleIniA& a_ini, T& a_value, const char* a_section, const char* a_key, const char* a_comment)
    {
        clib_util::ini::get_value(a_ini, a_value, a_section, a_key, a_comment);
    }

    static void get_value(CSimpleIniA& a_ini, std::vector<std::pair<std::string, uint32_t>>& a_value, const char* a_section, const char* a_key, const char* a_comment)
    {
        std::vector<std::string> raw;
        raw.reserve(a_value.size());
        for (auto&& [plugin, id] : a_value)
            raw.emplace_back(std::format("{}|0x{:X}", plugin, id));

        clib_util::ini::get_value(a_ini, raw, a_section, a_key, a_comment, ",");

        a_value.clear();
        for (auto&& entry : raw)
            if (auto parsed = stl::detail::parse_plugin_form(entry))
                a_value.emplace_back(std::move(*parsed));
    }
};
