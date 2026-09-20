# In-game tests

The pytest suite covers sneak activation, combat and perk requirements, repeated
encounters, effect duration, feedback, immunity and save/load through
[DevBench](https://github.com/alandtse/devbench).

## Setup

Install the development dependencies and prepare the baseline save:

```powershell
uv sync
New-Item -ItemType Directory -Force test-results | Out-Null
uv run devbench-scenario --url http://127.0.0.1:8920 --output test-results/prepare.json tests/game/prepare.json
```

Run the scenario from a disposable profile with ShadowOpportunist and DevBench
enabled. It enters QASmoke, gives the player 10,000 health and replaces the
`ShadowOpportunistTest_QASmoke` save. The player must be outside combat and
without perks that change slow-time magnitude or duration.

## Run

```powershell
uv run pytest tests/game --game-tests
```

Use `--baseline` for another save name and pytest's `-k` for a subset. The
default run expects a three-second effect, feedback and immunity enabled, and no
required perk.

The suite supports the enabled New Perk Addon and automatically uses its perk.
To test a custom requirement without the addon, add:

```powershell
--required-perk '0x800~SomePlugin.esp'
```

Each test reloads the baseline before and afterward. Reload it manually if a run
is interrupted.
