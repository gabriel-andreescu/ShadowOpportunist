#include "Settings.h"

#include <SKSE/SKSE.h>

#include <BMK/Settings.h>
#include <CLIBUtil/distribution.hpp>
#include <CLIBUtil/simpleINI.hpp>
#include <spdlog/spdlog.h>

#include <cstdint>
#include <exception>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace {
constexpr auto kGeneralSection = "General";
constexpr auto kDebugLoggingKey = "bDebugLogging";
constexpr auto kEffectFeedbackKey = "bEnableEffectFeedback";
constexpr auto kSlowTimeImmunityKey = "bSlowTimePlayerImmunity";
constexpr auto kDurationKey = "iDuration";
constexpr auto kRequiredPerksKey = "sRequiredPerks";

const std::filesystem::path& DefaultSettingsPath() {
    static const auto path = std::filesystem::path {L"Data/MCM/Config/ShadowOpportunist/settings.ini"};
    return path;
}

const std::filesystem::path& UserSettingsPath() {
    static const auto path = std::filesystem::path {L"Data/MCM/Settings/ShadowOpportunist.ini"};
    return path;
}

void ReadDuration(CSimpleIniA& a_ini, ModSettings& a_values, const std::filesystem::path& a_path) {
    clib_util::ini::get_value(a_ini, a_values.duration, kGeneralSection, kDurationKey, "; Duration in seconds");
    constexpr auto kMaxDuration = static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max());
    if (a_values.duration == 0 || a_values.duration > kMaxDuration) {
        SKSE::log::warn("{}: invalid iDuration={} replaced with 3", a_path.string(), a_values.duration);
        a_values.duration = ModSettings {}.duration;
    }
    a_ini.SetLongValue(kGeneralSection, kDurationKey, static_cast<long>(a_values.duration), "; Duration in seconds");
}

void ReadPerks(
    const CSimpleIniA& a_defaults,
    CSimpleIniA& a_user,
    ModSettings& a_values,
    const std::filesystem::path& a_path
) {
    const std::string packaged = a_defaults.GetValue(kGeneralSection, kRequiredPerksKey, "");
    const std::string raw = a_user.GetValue(kGeneralSection, kRequiredPerksKey, packaged.c_str());
    a_values.requiredPerks.clear();
    a_values.validPerks = true;

    for (auto entry : clib_util::distribution::split_entry(raw)) {
        clib_util::string::trim(entry);
        try {
            auto record = clib_util::distribution::get_record(entry);
            auto* form = std::get_if<clib_util::distribution::formid_pair>(&record);
            if (form == nullptr || !form->first || !form->second) {
                a_values.validPerks = false;
                continue;
            }
            a_values.requiredPerks.push_back(
                PerkRequirement {.plugin = std::move(form->second.value()), .formId = form->first.value()}
            );
        } catch (const std::exception&) {
            a_values.validPerks = false;
        }
    }
    if (!a_values.validPerks) {
        SKSE::log::warn("{}: invalid sRequiredPerks='{}'.", a_path.string(), raw);
    }
    a_user.SetValue(
        kGeneralSection,
        kRequiredPerksKey,
        raw.c_str(),
        "; Required perks when the New Perk Addon is disabled: 0xFORMID~Plugin.esp,...\n"
        "; Leave empty for no requirements"
    );
}

void ReadValues(CSimpleIniA& a_ini, ModSettings& a_values, const std::filesystem::path& a_path) {
    clib_util::ini::get_value(
        a_ini,
        a_values.debugLogging,
        kGeneralSection,
        kDebugLoggingKey,
        "; Toggle debug logging",
        clib_util::ini::bool_format::kNumeric
    );
    clib_util::ini::get_value(
        a_ini,
        a_values.effectFeedback,
        kGeneralSection,
        kEffectFeedbackKey,
        "; Toggle visuals and sound",
        clib_util::ini::bool_format::kNumeric
    );
    clib_util::ini::get_value(
        a_ini,
        a_values.slowTimeImmunity,
        kGeneralSection,
        kSlowTimeImmunityKey,
        "; Keep the player at normal speed",
        clib_util::ini::bool_format::kNumeric
    );
    ReadDuration(a_ini, a_values, a_path);
}

}

std::optional<ModSettings> ReadSettings(
    const std::filesystem::path& a_defaultPath,
    const std::filesystem::path& a_userPath
) {
    auto initialValues = ModSettings {};
    auto loaded = BMK::Settings::Load(
        {
            .defaults = a_defaultPath,
            .user = a_userPath,
        },
        std::move(initialValues),
        [&a_defaultPath, &a_userPath](CSimpleIniA& a_defaults, CSimpleIniA& a_user, ModSettings& a_candidate) {
            ReadValues(a_defaults, a_candidate, a_defaultPath);
            ReadValues(a_user, a_candidate, a_userPath);
            ReadPerks(a_defaults, a_user, a_candidate, a_userPath);
        }
    );
    if (!loaded) {
        SKSE::log::warn("Cannot load settings: {}", loaded.error().message);
        return std::nullopt;
    }
    if (loaded->saveFailure) {
        SKSE::log::warn("Cannot save settings: {}", loaded->saveFailure->message);
    }
    return std::move(loaded->values);
}

void Settings::Load() {
    const auto values = ReadSettings(DefaultSettingsPath(), UserSettingsPath());
    if (!values) {
        return;
    }

    _values = *values;
    BMK::Settings::ApplyLogLevel(_values.debugLogging, SKSE::InitInfo {}.logLevel);
    SKSE::log::info(
        "Settings: duration={}, feedback={}, playerImmunity={}, requiredPerks={}",
        _values.duration,
        _values.effectFeedback,
        _values.slowTimeImmunity,
        _values.requiredPerks.size()
    );
}
