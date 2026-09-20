#include "Settings.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

namespace {
class SettingsFiles {
public:
    SettingsFiles(const std::string_view defaults, const std::string_view user)
        : _directory(
              std::filesystem::temp_directory_path()
              / ("shadow-settings-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))
          )
        , _defaultPath(_directory / "default.ini")
        , _userPath(_directory / "user.ini") {
        std::filesystem::create_directories(_directory);
        std::ofstream(_defaultPath) << defaults;
        std::ofstream(_userPath) << user;
    }

    ~SettingsFiles() {
        std::error_code error;
        std::filesystem::remove_all(_directory, error);
    }

    SettingsFiles(const SettingsFiles&) = delete;
    SettingsFiles& operator=(const SettingsFiles&) = delete;
    SettingsFiles(SettingsFiles&&) = delete;
    SettingsFiles& operator=(SettingsFiles&&) = delete;

    [[nodiscard]] std::optional<ModSettings> Read() const {
        return ReadSettings(_defaultPath, _userPath);
    }

private:
    std::filesystem::path _directory;
    std::filesystem::path _defaultPath;
    std::filesystem::path _userPath;
};
}

TEST_CASE("Invalid custom perk lists remain unavailable") {
    const SettingsFiles invalidUser("[General]\n", "[General]\nsRequiredPerks=ShadowOpportunist_Perk.esp\n");
    const auto invalidUserSettings = invalidUser.Read();
    REQUIRE(invalidUserSettings);
    CHECK_FALSE(invalidUserSettings->validPerks);

    const SettingsFiles invalidPackaged("[General]\nsRequiredPerks=ShadowOpportunist_Perk.esp\n", "[General]\n");
    const auto invalidPackagedSettings = invalidPackaged.Read();
    REQUIRE(invalidPackagedSettings);
    CHECK_FALSE(invalidPackagedSettings->validPerks);
}
