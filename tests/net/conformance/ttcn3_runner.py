"""
Shared driver for the TTCN-3 conformance suites.

Builds a suite from the net-tools repository, runs it against the device
Twister has started, and turns Titan's verdict into a pytest result.

Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: Apache-2.0

"""

import fcntl
import logging
import os
import re
import shutil
import signal
import subprocess
import tempfile
from pathlib import Path

import pytest

logger = logging.getLogger(__name__)

TESTER_INTERFACE = 'zeth'

# Every conformance test drives a device that answers to the same address on
# the same interface, so only one of them can be running at a time. Twister
# runs each of them in its own pytest process, so the exclusion has to hold
# between processes rather than within one.
#
# The name carries the user id because a suite that has to bind a privileged
# port is run as root, and on a system that protects regular files in a
# sticky directory root cannot open a lock file another user left there. A
# run is either wholly privileged or wholly not, so a lock per user still
# excludes everything that could collide.
NETWORK_LOCK = Path(tempfile.gettempdir()) / f'zephyr-net-conformance-{os.geteuid()}.lock'

# The last two lines a Titan run prints. The first counts each verdict, the
# second gives the verdict for the run as a whole.
STATISTICS = re.compile(
    r'Verdict statistics:.*?(\d+) pass .*?(\d+) inconc .*?(\d+) fail .*?(\d+) error'
)
OVERALL = re.compile(r'Overall verdict: (\w+)')


@pytest.fixture(scope='session')
def network_lock():
    """Hold the interface for the whole session.

    Ask for this before the device fixture, so that the device is not even
    started while another conformance test is using the interface.
    """
    # The name is this user's alone, so the file needs no access beyond
    # the owner, and O_NOFOLLOW refuses a symlink someone else may have
    # left under that name - a suite that binds a privileged port opens
    # this as root.
    fd = os.open(NETWORK_LOCK, os.O_CREAT | os.O_RDWR | os.O_NOFOLLOW, 0o600)
    try:
        logger.info('Waiting for the %s interface', TESTER_INTERFACE)
        fcntl.flock(fd, fcntl.LOCK_EX)
        logger.info('Holding the %s interface', TESTER_INTERFACE)
        yield
    finally:
        fcntl.flock(fd, fcntl.LOCK_UN)
        os.close(fd)


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


def requirements(suite: str) -> str | None:
    """Explain what is missing, or None when the suite can be run."""
    if not os.environ.get('TTCN3_DIR'):
        return 'TTCN3_DIR is unset, no Titan installation to build the suite with'

    if not shutil.which('make'):
        return 'make is not installed'

    suites = ttcn3_dir()
    if not (suites / 'build.sh').is_file():
        return f'no TTCN-3 suites under {suites}, set NET_TOOLS_BASE'

    if not (suites / 'suites' / suite).is_dir():
        return f'{suites} has no {suite} suite'

    if not (suites / 'modules').is_dir():
        return f'third party modules are missing, run {suites}/fetch-modules.sh'

    if not Path('/sys/class/net').joinpath(TESTER_INTERFACE).exists():
        return f'the {TESTER_INTERFACE} interface does not exist, run net-setup.sh to create it'

    if is_parallel(suite) and not (Path(os.environ['TTCN3_DIR']) / 'bin' / 'ttcn3_start').is_file():
        return 'this suite needs a main controller, and ttcn3_start is not in TTCN3_DIR/bin'

    if needs_privilege(suite) and os.geteuid() != 0:
        return (
            f'the {suite} suite has to bind a privileged port, which the protocol fixes '
            'and which cannot be moved, so it has to be run as root'
        )

    return None


def build_conf(suite: str) -> str:
    conf = ttcn3_dir() / 'suites' / suite / 'build.conf'

    return conf.read_text() if conf.is_file() else ''


def is_parallel(suite: str) -> bool:
    """A suite whose test cases create parallel components says so here."""
    return 'MODE=parallel' in build_conf(suite)


def needs_privilege(suite: str) -> bool:
    """A suite that has to bind a port below 1024 says so here."""
    return 'PRIVILEGED=yes' in build_conf(suite)


def build_suite(suite: str) -> Path:
    """Build the suite, skipping the test when it cannot be built at all."""
    missing = requirements(suite)
    if missing:
        pytest.skip(missing)

    suites = ttcn3_dir()
    binary = suites / 'suites' / suite / 'build' / suite

    logger.info('Building the %s suite in %s', suite, suites)
    result = subprocess.run(
        [str(suites / 'build.sh'), suite],
        cwd=suites,
        capture_output=True,
        text=True,
        timeout=1800,
        check=False,
    )
    if result.returncode != 0:
        logger.error('build.sh output:\n%s\n%s', result.stdout, result.stderr)
        pytest.fail(f'cannot build the {suite} suite')

    assert binary.is_file(), f'{binary} was not produced'
    return binary


def run_suite(binary: Path, suite: str, timeout: float = 600.0) -> None:
    """Run the suite and assert that every test case passed."""
    titan = Path(os.environ['TTCN3_DIR'])
    lib = titan / 'lib' / 'titan' if (titan / 'lib' / 'titan').is_dir() else titan / 'lib'

    env = dict(os.environ)
    env['LD_LIBRARY_PATH'] = os.pathsep.join([str(lib), env.get('LD_LIBRARY_PATH', '')]).strip(
        os.pathsep
    )
    env['PATH'] = os.pathsep.join([str(titan / 'bin'), env.get('PATH', '')]).strip(os.pathsep)

    if is_parallel(suite):
        # A parallel suite is a main controller and a host controller rather
        # than one process. ttcn3_start starts both and drives the controller.
        command = [str(titan / 'bin' / 'ttcn3_start'), f'./{suite}', f'../{suite}.cfg']
    else:
        command = [str(binary), f'../{suite}.cfg']

    # Its own session, so that a suite which hangs can be killed along with
    # whatever it started rather than leaving a controller behind.
    process = subprocess.Popen(
        command,
        cwd=binary.parent,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        env=env,
        start_new_session=True,
    )
    try:
        output = process.communicate(timeout=timeout)[0]
    except subprocess.TimeoutExpired:
        os.killpg(os.getpgid(process.pid), signal.SIGKILL)
        output = process.communicate()[0] or ''
        logger.error('Suite output before the timeout:\n%s', output)
        raise AssertionError(f'the {suite} suite did not finish within {timeout}s') from None

    logger.info('Suite output:\n%s', output)

    statistics = STATISTICS.search(output)
    assert statistics, 'the suite printed no verdict statistics'

    passed, inconc, failed, errored = (int(n) for n in statistics.groups())
    logger.info(
        '%d passed, %d inconclusive, %d failed, %d errored', passed, inconc, failed, errored
    )

    overall = OVERALL.search(output)
    assert overall, 'the suite printed no overall verdict'

    assert passed > 0, 'the suite ran no test case'
    assert overall.group(1) == 'pass', (
        f'{failed} failed and {errored} errored; see the suite output above'
    )
