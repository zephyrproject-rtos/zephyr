"""
Run the TTCN-3 mDNS conformance suite against a Zephyr instance.

Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: Apache-2.0
"""

import logging
import os
import re
import shutil
import subprocess
from pathlib import Path

import pytest
from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)

SUITE = 'mdns'
TESTER_INTERFACE = 'zeth'

# The last two lines a Titan run prints. The first counts each verdict, the
# second gives the verdict for the run as a whole.
STATISTICS = re.compile(
    r'Verdict statistics:.*?(\d+) pass .*?(\d+) inconc .*?(\d+) fail .*?(\d+) error'
)
OVERALL = re.compile(r'Overall verdict: (\w+)')


def ttcn3_dir() -> Path:
    """Where the suites live, inside a net-tools checkout."""
    base = os.environ.get('NET_TOOLS_BASE')
    if base:
        return Path(base) / 'ttcn3'

    zephyr_base = Path(os.environ.get('ZEPHYR_BASE', '.')).resolve()
    for candidate in (zephyr_base.parent, zephyr_base.parent.parent):
        path = candidate / 'tools' / 'net-tools' / 'ttcn3'
        if path.is_dir():
            return path

    return zephyr_base.parent / 'tools' / 'net-tools' / 'ttcn3'


def requirements() -> str | None:
    """Explain what is missing, or None when the suite can be run."""
    if not os.environ.get('TTCN3_DIR'):
        return 'TTCN3_DIR is unset, no Titan installation to build the suite with'

    if not shutil.which('make'):
        return 'make is not installed'

    suites = ttcn3_dir()
    if not (suites / 'build.sh').is_file():
        return f'no TTCN-3 suites under {suites}, set NET_TOOLS_BASE'

    if not (suites / 'modules').is_dir():
        return f'third party modules are missing, run {suites}/fetch-modules.sh'

    if not Path('/sys/class/net').joinpath(TESTER_INTERFACE).exists():
        return f'the {TESTER_INTERFACE} interface does not exist, run net-setup.sh to create it'

    return None


@pytest.fixture(scope='module')
def suite_binary() -> Path:
    """Build the suite once, and hand back the executable."""
    missing = requirements()
    if missing:
        pytest.skip(missing)

    suites = ttcn3_dir()
    binary = suites / 'suites' / SUITE / 'build' / SUITE

    logger.info('Building the %s suite in %s', SUITE, suites)
    result = subprocess.run(
        [str(suites / 'build.sh'), SUITE],
        cwd=suites,
        capture_output=True,
        text=True,
        timeout=1800,
        check=False,
    )
    if result.returncode != 0:
        logger.error('build.sh output:\n%s\n%s', result.stdout, result.stderr)
        pytest.fail(f'cannot build the {SUITE} suite')

    assert binary.is_file(), f'{binary} was not produced'
    return binary


def test_mdns_conformance(dut: DeviceAdapter, suite_binary: Path):
    """The responder answers the suite the way the tests expect."""
    dut.readlines_until(regex='mDNS responder ready', timeout=30.0)

    build_dir = suite_binary.parent
    env = dict(os.environ)
    titan = Path(os.environ['TTCN3_DIR'])
    lib = titan / 'lib' / 'titan' if (titan / 'lib' / 'titan').is_dir() else titan / 'lib'
    env['LD_LIBRARY_PATH'] = os.pathsep.join([str(lib), env.get('LD_LIBRARY_PATH', '')]).strip(
        os.pathsep
    )

    result = subprocess.run(
        [str(suite_binary), f'../{SUITE}.cfg'],
        cwd=build_dir,
        capture_output=True,
        text=True,
        timeout=600,
        env=env,
        check=False,
    )
    output = result.stdout + result.stderr
    logger.info('Suite output:\n%s', output)

    statistics = STATISTICS.search(output)
    assert statistics, 'the suite printed no verdict statistics'

    passed, inconc, failed, errored = (int(n) for n in statistics.groups())
    logger.info(
        '%d passed, %d inconclusive, %d failed, %d errored',
        passed,
        inconc,
        failed,
        errored,
    )

    overall = OVERALL.search(output)
    assert overall, 'the suite printed no overall verdict'

    assert passed > 0, 'the suite ran no test case'
    assert overall.group(1) == 'pass', (
        f'{failed} failed and {errored} errored; see the suite output above'
    )
