import pytest

from .support import ShadowSession


def pytest_addoption(parser):
    group = parser.getgroup("shadow-opportunist")
    group.addoption("--baseline", default="ShadowOpportunistTest_QASmoke")
    group.addoption("--expected-duration", type=int, default=3)
    group.addoption("--expected-feedback", choices=["true", "false"], default="true")
    group.addoption("--expected-immunity", choices=["true", "false"], default="true")
    group.addoption(
        "--required-perk", default="", help="Expected 0xLocalFormID~Plugin.esp"
    )


def pytest_collection_modifyitems(items):
    for item in items:
        item.add_marker(pytest.mark.game)


@pytest.fixture
def shadow(request, devbench):
    bench = ShadowSession(devbench, request.config)
    try:
        bench.restore()
        bench.initialize()
        yield bench
    finally:
        bench.restore()


@pytest.fixture
def enemy(shadow):
    return shadow.spawn_enemy()
