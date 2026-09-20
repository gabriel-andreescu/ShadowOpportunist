#pragma once

#include <cstdint>

namespace Hooks {
void Install();
void Suspend();
void Resume();
struct Diagnostics {
    bool installed {};
    bool perkAddonEnabled {};
    bool requirementsValid {};
    bool appliedThisCombat {};
    bool loading {};
    std::uint64_t requiredPerks {};
    std::uint64_t activations {};
    std::uint64_t loadGeneration {};
    std::uint64_t detections {};
    std::int32_t lastDetectionScore {};
    std::uint32_t lastDetector {};
};
Diagnostics GetDiagnostics();
}
