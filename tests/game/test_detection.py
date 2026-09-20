import time

import pytest


def test_combat_activation_and_rearming(shadow, enemy):
    shadow.combat(enemy)
    time.sleep(2)
    assert shadow.state()["activations"] == shadow.initial["activations"]
    active = shadow.activate(enemy)
    shadow.assert_effect(active)
    shadow.expire(active["activations"])
    shadow.reset_combat(enemy)
    shadow.combat(enemy)
    shadow.wait(
        lambda s: s["activations"] == active["activations"] + 1,
        "A new combat did not reactivate slow time",
    )


def test_loading_during_slow_time(shadow, enemy):
    active = shadow.activate(enemy)
    shadow.assert_effect(active)
    shadow.restore()
    state = shadow.state()
    assert state["loadGeneration"] > shadow.initial["loadGeneration"]
    assert not state["appliedThisCombat"] and not state["loading"]
    assert abs(state["worldSlowdown"]) < 0.001


def test_required_perk(shadow, enemy):
    if not shadow.perk:
        pytest.skip("No required perk is configured")
    shadow.combat(enemy)
    shadow.p("Actor", "StartSneaking", self_form="0x14")
    time.sleep(3)
    assert shadow.state()["activations"] == shadow.initial["activations"]
    shadow.p("Actor", "AddPerk", [{"form": shadow.perk}], "0x14")
    shadow.reset_combat(enemy)
    active = shadow.activate(enemy)
    shadow.assert_effect(active)


def test_sneaking_without_combat(shadow, enemy):
    shadow.p("Actor", "StartSneaking", self_form="0x14")
    time.sleep(3)
    state = shadow.state()
    assert not state["inCombat"]
    assert state["activations"] == shadow.initial["activations"]


def test_sneaking_before_repeated_combats(shadow, enemy):
    if shadow.perk:
        shadow.p("Actor", "AddPerk", [{"form": shadow.perk}], "0x14")
    shadow.p("Actor", "StartSneaking", self_form="0x14")
    shadow.wait(lambda s: s["sneaking"], "The player did not start sneaking")
    for encounter in range(5):
        shadow.combat(enemy)
        active = shadow.wait(
            lambda s, encounter=encounter: (
                s["activations"] == shadow.initial["activations"] + encounter + 1
            ),
            f"Encounter {encounter + 1} did not activate slow time",
        )
        shadow.assert_effect(active)
        shadow.expire(active["activations"])
        shadow.reset_combat(enemy)


def test_multiple_detectors_activate_once(shadow, enemy):
    for other in (shadow.spawn_enemy(), shadow.spawn_enemy()):
        shadow.combat(other)
    active = shadow.activate(enemy)
    shadow.assert_effect(active)
    shadow.expire(active["activations"])


def test_existing_effect_does_not_stack(shadow, enemy):
    if shadow.perk:
        shadow.p("Actor", "AddPerk", [{"form": shadow.perk}], "0x14")
    shadow.combat(enemy)
    # Detection can take longer than the normal three-second effect.
    shadow.p("Spell", "SetNthEffectDuration", [0, 30], shadow.spell)
    try:
        shadow.p("Spell", "Cast", [{"form": "0x14"}, {"form": "0x14"}], shadow.spell)
        shadow.wait(
            lambda s: abs(s["worldSlowdown"] - 0.2) < 0.001,
            "The spell did not slow time",
        )
        shadow.p("Actor", "StartSneaking", self_form="0x14")
        state = shadow.wait(
            lambda s: s["appliedThisCombat"], "The detection hook did not run"
        )
        assert state["activations"] == shadow.initial["activations"]
        effects = shadow.call("inspect", {"kind": "effects"})["activeEffects"]
        assert sum(e["effect"]["formId"] == shadow.effect for e in effects) == 1
    finally:
        shadow.p("Spell", "SetNthEffectDuration", [0, shadow.duration], shadow.spell)
        shadow.p("Actor", "DispelSpell", [{"form": shadow.spell}], "0x14")
    shadow.wait(
        lambda s: abs(s["worldSlowdown"]) < 0.001, "Dispelling did not restore time"
    )
    shadow.reset_combat(enemy)
    shadow.assert_effect(shadow.activate(enemy))
