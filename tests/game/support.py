"""Combat and effect observations for Shadow Opportunist."""

import time

from bmk.testing import wait_for


class ShadowSession:
    def __init__(self, client, config):
        self.client = client
        self.call = client.call
        self.p = client.papyrus
        self.config = config
        self.duration = config.getoption("expected_duration")
        self.feedback = config.getoption("expected_feedback") == "true"
        self.immunity = config.getoption("expected_immunity") == "true"
        self.perk = None

    def state(self):
        return self.call("inspect", {"kind": "shadowopportunist"})

    def wait(self, predicate, message):
        return wait_for(self.state, predicate, message, timeout=15, interval=0.1)

    def restore(self):
        self.client.load(
            self.config.getoption("baseline"), cell="QASmoke", settle_ms=3000
        )

    def form(self, local_id, plugin="ShadowOpportunist.esp"):
        return f"0x{self.client.form_id(local_id, plugin):08X}"

    def initialize(self):
        self.initial = self.state()
        assert self.initial["installed"] and not self.initial["loading"]
        assert not self.initial["sneaking"] and not self.initial["inCombat"]
        settings = self.initial["settings"]
        assert settings["duration"] == self.duration
        assert settings["feedback"] == self.feedback
        assert settings["playerImmunity"] == self.immunity
        required = self.config.getoption("required_perk")
        if settings["perkAddon"]:
            required = "0x800~ShadowOpportunist_Perk.esp"
        assert settings["requiredPerks"] == bool(required) and settings["validPerks"]
        self.spell, self.effect, self.feedback_effect = (
            self.form(i) for i in (0x800, 0x802, 0x803)
        )
        assert self.p("Spell", "GetNthEffectDuration", [0], self.spell) == self.duration
        self.player_speed = 1.0
        if not self.immunity:
            self.player_speed = self.p(
                "Game", "GetGameSettingFloat", ["fVATSPlayerMagicTimeSlowdownMult"]
            )
            assert 0 < self.player_speed < 1
        if required:
            local, plugin = required.split("~")
            self.perk = self.form(int(local, 16), plugin)
            self.p("Actor", "RemovePerk", [{"form": self.perk}], "0x14")

    def spawn_enemy(self):
        actor = self.client.spawn(0x9F358, "ACHR")
        self.p("Actor", "SetActorValue", ["Aggression", 0.0], actor)
        self.p("Actor", "SetActorValue", ["Confidence", 4.0], actor)
        self.p(
            "ObjectReference",
            "MoveTo",
            [{"form": "0x14"}, -150.0, 0.0, 0.0, False],
            actor,
        )
        return actor

    def combat(self, enemy):
        self.p(
            "ObjectReference",
            "MoveTo",
            [{"form": "0x14"}, -150.0, 0.0, 0.0, False],
            enemy,
        )
        self.p("ObjectReference", "SetAngle", [0.0, 0.0, 90.0], enemy)
        self.p("Actor", "StartCombat", [{"form": "0x14"}], enemy)
        self.wait(lambda s: s["inCombat"], "The enemy did not enter combat")

    def reset_combat(self, enemy):
        self.p("Actor", "SetActorValue", ["Aggression", 0.0], enemy)
        for actor in (enemy, "0x14"):
            self.p("Actor", "StopCombat", self_form=actor)
        self.wait(
            lambda s: not s["inCombat"] and not s["appliedThisCombat"],
            "Combat did not reset",
        )

    def activate(self, enemy):
        if self.perk:
            self.p("Actor", "AddPerk", [{"form": self.perk}], "0x14")
        before = self.state()["activations"]
        self.combat(enemy)
        if not self.state()["sneaking"]:
            self.p("Actor", "StartSneaking", self_form="0x14")
        return self.wait(
            lambda s: s["activations"] == before + 1,
            "Sneaking did not activate slow time",
        )

    def assert_effect(self, active):
        active = self.wait(
            lambda s: (
                s["activations"] == active["activations"]
                and abs(s["worldSlowdown"] - 0.2) < 0.001
            ),
            "The activated spell did not slow time",
        )
        assert abs(active["playerSlowdown"] - self.player_speed) < 0.001
        effects = self.call("inspect", {"kind": "effects"})["activeEffects"]
        slow = [e for e in effects if e["effect"]["formId"] == self.effect]
        assert len(slow) == 1
        # SlowTimeEffect scales its active duration by the player's time multiplier.
        assert abs(slow[0]["duration"] - self.duration * self.player_speed) < 0.001
        feedback = [e for e in effects if e["effect"]["formId"] == self.feedback_effect]
        assert len(feedback) == int(self.feedback)

    def expire(self, count):
        time.sleep(self.duration + 1)
        state = self.state()
        assert state["activations"] == count
        assert abs(state["worldSlowdown"]) < 0.001
