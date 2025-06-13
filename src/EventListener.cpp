#include "EventListener.h"
#include "Hooks.h"

void EventListener::Register()
{
    const auto scriptEventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
    if (!scriptEventSourceHolder) {
        logger::critical("Failed to get ScriptEventSourceHolder");
        return;
    }

    const auto listener = GetSingleton();
    if (!listener) {
        logger::error("Failed to get EventListener singleton");
        return;
    }

    scriptEventSourceHolder->GetEventSource<RE::TESActiveEffectApplyRemoveEvent>()->AddEventSink(listener);
    logger::info("EventListener registered successfully");
}

RE::BSEventNotifyControl
EventListener::ProcessEvent(const RE::TESActiveEffectApplyRemoveEvent* event,
                            [[maybe_unused]] RE::BSTEventSource<RE::TESActiveEffectApplyRemoveEvent>* source)
{
    if (!event || event->isApplied || !event->target || !event->target->IsPlayerRef()) {
        return RE::BSEventNotifyControl::kContinue;
    }

    if (Hooks::HasAttackBonusActive()) {
        logger::debug("Processing active effect removal for player");
        Hooks::RestoreAttackDamageBonus();
    }

    return RE::BSEventNotifyControl::kContinue;
}
