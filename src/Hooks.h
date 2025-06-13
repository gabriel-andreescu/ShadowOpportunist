#pragma once

namespace Hooks
{
    void Install() noexcept;
    [[nodiscard]] bool HasSlowTimeEffectActive() noexcept;
    [[nodiscard]] bool HasAttackBonusActive() noexcept;
    void RestoreAttackDamageBonus() noexcept;

    class SetMagicTimeSlowdown : REX::Singleton<SetMagicTimeSlowdown>
    {
    public:
        static void Thunk(RE::VATS* vats, float worldMag, float playerMag);

        inline static REL::Relocation<decltype(Thunk)> func;
    };

    class CalculateDetection : REX::Singleton<CalculateDetection>
    {
    public:
        static void Thunk(RE::Actor* self,
                          RE::Actor* target,
                          std::int32_t* score,
                          bool* spotted,
                          bool* hasLOS,
                          std::int32_t* reason,
                          RE::NiPoint3* lastPos,
                          std::int32_t* soundLvl,
                          std::int32_t* unk8,
                          std::int32_t* unk9) noexcept;

        inline static REL::Relocation<decltype(Thunk)> func;
    };
}
