#pragma once

#include <memory> // IWYU pragma: keep. REX/REX/Singleton.h uses std::addressof without including <memory>.

#include <REX/REX/Singleton.h>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

struct PerkRequirement {
    std::string plugin;
    std::uint32_t formId;
};

struct ModSettings {
    bool debugLogging {false};
    bool effectFeedback {true};
    bool slowTimeImmunity {true};
    std::uint32_t duration {3};
    std::vector<PerkRequirement> requiredPerks;
    bool validPerks {true};
};

[[nodiscard]] std::optional<ModSettings> ReadSettings(
    const std::filesystem::path& a_defaultPath,
    const std::filesystem::path& a_userPath
);

class Settings : public REX::Singleton<Settings> {
public:
    void Load();
    [[nodiscard]] const ModSettings& GetValues() const {
        return _values;
    }

private:
    ModSettings _values;
};
