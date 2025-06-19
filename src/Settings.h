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
        get_value(ini, duration, "General", "iDuration", "; Duration (seconds) for the effect");
        get_value(ini, enableSlowTimeImmunity, "General", "bSlowTimePlayerImmunity", "; Toggle player immunity to the slow time effect");
        get_value(ini, requiredPerks, "General", "sRequiredPerks", "; Perk requirements for the effect (format: plugin|formID,plugin|formID)\n; Leave empty for no requirements");

        (void)ini.SaveFile(path.c_str());
    }

    // members
    static constexpr uint32_t defaultDuration = 3;
    static constexpr auto pluginName = "ShadowOpportunist.esp";
    static constexpr auto pluginNamePerk = "ShadowOpportunist_Perk.esp";
    bool debugLogging{false};
    bool enableEffectFeedback{true};
    uint32_t duration{defaultDuration};
    bool enableSlowTimeImmunity{true};
    std::vector<std::pair<std::string, uint32_t>> requiredPerks{};

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
