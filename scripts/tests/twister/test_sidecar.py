#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Tests for the Sidecar classes of twister."""

import os
from dataclasses import dataclass
from unittest import mock

import pytest
from twisterlib.sidecars import Sidecar, SidecarImporter, sidecar_config_schema
from twisterlib.sidecars.virtiofs import (
    VirtiofsSidecar,
    get_virtiofs_socket_path,
)
from twisterlib.statuses import TwisterStatus
from twisterlib.testinstance import TestInstance


def _make_instance(tmp_path, sidecar_config=None):
    mock_platform = mock.Mock()
    mock_platform.name = "qemu_x86_64"
    mock_platform.normalized_name = "qemu_x86_64"

    mock_testsuite = mock.Mock(id="id", testcases=[])
    mock_testsuite.name = "mock_testsuite"
    mock_testsuite.harness_config = {}
    mock_testsuite.sidecar_config = sidecar_config or {}
    mock_testsuite.source_dir = str(tmp_path / "src")
    os.makedirs(mock_testsuite.source_dir, exist_ok=True)

    outdir = tmp_path / "out"
    outdir.mkdir()

    return TestInstance(
        testsuite=mock_testsuite, platform=mock_platform, toolchain='zephyr', outdir=outdir
    )


def test_sidecar_importer_resolves_names():
    assert isinstance(SidecarImporter.get_sidecar('virtiofs'), VirtiofsSidecar)
    assert SidecarImporter.get_sidecar(None) is None
    assert SidecarImporter.get_sidecar('') is None
    # An unknown name (typically a typo) is an error, not a silent no-op.
    with pytest.raises(ValueError, match="unknown sidecar 'nope'"):
        SidecarImporter.get_sidecar('nope')


def test_sidecar_subclass_auto_registers():
    # Defining a Sidecar subclass with a NAME is all it takes to register it:
    # no central list is edited. Clean up so the global registry is not left
    # polluted for other tests.
    class _ProbeSidecar(Sidecar):
        NAME = 'probe-xyz'

    try:
        assert isinstance(SidecarImporter.get_sidecar('probe-xyz'), _ProbeSidecar)
    finally:
        Sidecar._registry.pop('probe-xyz', None)


def test_sidecar_config_schema_is_drop_in():
    # A sidecar's config schema is derived from its Config dataclass and folded
    # into the assembled `sidecar_config` schema, so a new sidecar extends
    # validation without editing the schema file.
    @dataclass
    class _Cfg:
        level: int | None = None
        names: list[str] = None

    class _ProbeSidecar(Sidecar):
        NAME = 'probe-schema'
        Config = _Cfg

    try:
        block = sidecar_config_schema()['properties']['probe-schema']
        assert block == {
            'type': 'object',
            'properties': {
                'level': {'type': 'integer'},
                'names': {'type': 'array', 'items': {'type': 'string'}},
            },
            'additionalProperties': False,
        }
    finally:
        Sidecar._registry.pop('probe-schema', None)


# --- virtiofs ---------------------------------------------------------------


def test_virtiofs_configure_reads_namespaced_config(tmp_path):
    instance = _make_instance(
        tmp_path,
        {"virtiofs": {"bin": "/opt/virtiofsd", "extra_args": ["--sandbox=none"]}},
    )
    sidecar = VirtiofsSidecar()
    sidecar.configure(instance)

    assert sidecar.virtiofsd_bin == "/opt/virtiofsd"
    assert sidecar.virtiofs_extra_args == ["--sandbox=none"]
    assert sidecar.virtiofs_shared is None


def test_virtiofs_configure_defaults_without_config(tmp_path):
    # No sidecar_config at all: the block is absent, so typed defaults apply and
    # the binary falls back to autodetection (patched out here).
    instance = _make_instance(tmp_path)
    sidecar = VirtiofsSidecar()
    with mock.patch.object(VirtiofsSidecar, "find_virtiofsd", return_value="/found"):
        sidecar.configure(instance)

    assert sidecar.virtiofsd_bin == "/found"
    assert sidecar.virtiofs_shared is None
    assert sidecar.virtiofs_extra_args == []


def test_virtiofs_host_ready_requires_virtiofsd(tmp_path):
    instance = _make_instance(tmp_path)
    sidecar = VirtiofsSidecar()
    with mock.patch.object(VirtiofsSidecar, "find_virtiofsd", return_value=None):
        sidecar.configure(instance)
    assert sidecar.host_ready() is False

    with mock.patch.object(VirtiofsSidecar, "find_virtiofsd", return_value="/found"):
        sidecar.configure(instance)
    assert sidecar.host_ready() is True


def test_virtiofs_config_schema_matches_dataclass():
    assert VirtiofsSidecar.config_schema() == {
        'type': 'object',
        'properties': {
            'shared': {'type': 'string'},
            'bin': {'type': 'string'},
            'extra_args': {'type': 'array', 'items': {'type': 'string'}},
        },
        'additionalProperties': False,
    }


def test_assembled_sidecar_config_schema_validates_strictly():
    import jsonschema

    schema = sidecar_config_schema()
    # A correct block validates.
    jsonschema.validate({'virtiofs': {'shared': 'x'}}, schema)
    # A typo'd key inside a sidecar block is rejected.
    with pytest.raises(jsonschema.ValidationError):
        jsonschema.validate({'virtiofs': {'shard': 'x'}}, schema)
    # An unknown sidecar name is rejected.
    with pytest.raises(jsonschema.ValidationError):
        jsonschema.validate({'nope': {}}, schema)


def test_virtiofs_cmake_env_injects_chardev(tmp_path):
    instance = _make_instance(tmp_path)
    sidecar = VirtiofsSidecar()
    sidecar.configure(instance)

    flags = sidecar.cmake_env(instance.build_dir)["QEMU_EXTRA_FLAGS"]
    assert "-chardev socket,id=char0,path=" in flags
    assert get_virtiofs_socket_path(instance.build_dir) in flags


def test_get_virtiofs_socket_path_is_short_and_stable():
    # Only the length matters here: nothing is created at this path, it just has
    # to be long enough that the socket path could not simply live inside it.
    long_build_dir = "/build/" + "a" * 300 + "/out"
    path = get_virtiofs_socket_path(long_build_dir)

    assert path == get_virtiofs_socket_path(long_build_dir)
    assert len(path) < 108
    assert get_virtiofs_socket_path("/other/build") != path


def test_virtiofs_setup_skips_when_virtiofsd_missing(tmp_path):
    instance = _make_instance(tmp_path)
    sidecar = VirtiofsSidecar()
    sidecar.configure(instance)
    sidecar.virtiofsd_bin = None

    with mock.patch("subprocess.Popen") as popen_mock:
        proceed = sidecar.setup()

    assert proceed is False
    popen_mock.assert_not_called()
    assert instance.status == TwisterStatus.SKIP


def test_virtiofs_setup_seeds_shared_dir_and_starts_daemon(tmp_path):
    template = tmp_path / "src" / "shared"
    (template / "sub").mkdir(parents=True)
    (template / "file").write_text("host content")

    instance = _make_instance(tmp_path, {"virtiofs": {"shared": "shared"}})
    sidecar = VirtiofsSidecar()
    sidecar.configure(instance)
    sidecar.virtiofsd_bin = "/usr/bin/virtiofsd"

    with mock.patch("subprocess.Popen") as popen_mock:
        popen_mock.return_value.pid = 4321
        proceed = sidecar.setup()

    assert proceed is True
    assert os.path.isfile(os.path.join(sidecar.shared_dir, "file"))
    assert os.path.isdir(os.path.join(sidecar.shared_dir, "sub"))
    args = popen_mock.call_args.args[0]
    assert args[0] == "/usr/bin/virtiofsd"
    assert f"--socket-path={sidecar.socket_path}" in args
    assert sidecar.shared_dir in args


def test_virtiofs_teardown_terminates_daemon(tmp_path):
    instance = _make_instance(tmp_path)
    sidecar = VirtiofsSidecar()
    sidecar.configure(instance)
    proc = mock.Mock()
    sidecar._virtiofsd_proc = proc
    sidecar._virtiofsd_log = mock.Mock()
    open(sidecar.socket_path, "w").close()

    with mock.patch("twisterlib.sidecars.virtiofs.terminate_process") as term_mock:
        sidecar.teardown()

    term_mock.assert_called_once_with(proc)
    proc.wait.assert_called_once()
    assert sidecar._virtiofsd_proc is None
    assert not os.path.exists(sidecar.socket_path)


# --- bumble -----------------------------------------------------------------


def _bumble_instance(tmp_path, sidecar_config=None):
    instance = _make_instance(tmp_path, sidecar_config)
    instance.handler = mock.Mock(extra_test_args=None)
    instance.handler.get_test_timeout.return_value = 60
    instance.testsuite.compose_case_name = lambda name: f"id.{name}"
    os.makedirs(instance.build_dir, exist_ok=True)
    return instance


def _fake_controllers(stdout: bytes, returncode=None):
    """A Popen stand-in whose stdout is a real pipe holding ``stdout``, then EOF."""
    rfd, wfd = os.pipe()
    os.write(wfd, stdout)
    os.close(wfd)
    proc = mock.Mock(pid=4321)
    proc.stdout = os.fdopen(rfd, 'rb', buffering=0)
    proc.poll.return_value = returncode
    return proc


def test_bumble_configure_reads_namespaced_config(tmp_path):
    from twisterlib.sidecars.bumble import BumbleSidecar

    instance = _make_instance(
        tmp_path,
        {
            "bumble": {
                "addresses": ["00:00:01:00:00:0A", "00:00:01:00:00:0B"],
                "devices": ["-test=a::b", "-test=c::d"],
                "controllers_script": "custom/controllers.py",
            }
        },
    )
    sidecar = BumbleSidecar()
    sidecar.configure(instance)

    assert sidecar.addresses == ["00:00:01:00:00:0A", "00:00:01:00:00:0B"]
    assert sidecar.devices == ["-test=a::b", "-test=c::d"]
    assert sidecar.script.endswith(os.path.join("custom", "controllers.py"))


def test_bumble_configure_defaults_without_config(tmp_path):
    from twisterlib.sidecars.bumble import BumbleSidecar

    instance = _make_instance(tmp_path)
    sidecar = BumbleSidecar()
    sidecar.configure(instance)

    assert sidecar.addresses == list(BumbleSidecar.DEFAULT_ADDRESSES)
    assert sidecar.devices == ['']
    assert sidecar.script.endswith(
        os.path.join('tests', 'bluetooth', 'classic', 'bumble', 'common', 'controllers.py')
    )


def test_bumble_host_ready_requires_bumble_and_script(tmp_path):
    from twisterlib.sidecars.bumble import BumbleSidecar

    # The default controllers script is in the tree; Bumble decides.
    sidecar = BumbleSidecar()
    sidecar.configure(_make_instance(tmp_path))
    with mock.patch("importlib.util.find_spec", return_value=None):
        assert sidecar.host_ready() is False
    with mock.patch("importlib.util.find_spec", return_value=mock.Mock()):
        assert sidecar.host_ready() is True

    # Bumble alone is not enough without the simulation framework.
    sidecar = BumbleSidecar()
    (tmp_path / "second").mkdir()
    sidecar.configure(
        _make_instance(tmp_path / "second", {"bumble": {"controllers_script": "does/not/exist.py"}})
    )
    with mock.patch("importlib.util.find_spec", return_value=mock.Mock()):
        assert sidecar.host_ready() is False


def test_bumble_resolves_address_and_controller_placeholders(tmp_path):
    from twisterlib.sidecars.bumble import BumbleSidecar

    instance = _make_instance(tmp_path)
    sidecar = BumbleSidecar()
    sidecar.configure(instance)
    sidecar.ports = [1234, 5678]

    resolved = sidecar._resolve('--peer_bd_address={addr1} --ctrl={ctrl0}')
    assert resolved == f'--peer_bd_address={sidecar.addresses[1]} --ctrl=127.0.0.1:1234'


def test_bumble_setup_skips_without_controllers_script(tmp_path):
    from twisterlib.sidecars.bumble import BumbleSidecar

    instance = _bumble_instance(tmp_path, {"bumble": {"controllers_script": "does/not/exist.py"}})
    sidecar = BumbleSidecar()
    sidecar.configure(instance)

    with mock.patch("subprocess.Popen") as popen_mock:
        assert sidecar.setup() is False

    popen_mock.assert_not_called()
    assert instance.status == TwisterStatus.SKIP


def test_bumble_setup_wires_guest_and_peers_to_reported_ports(tmp_path):
    import sys

    from twisterlib.sidecars.bumble import BumbleSidecar

    instance = _bumble_instance(
        tmp_path,
        {
            "bumble": {
                "addresses": ["00:00:01:00:00:0A", "00:00:01:00:00:0B"],
                "devices": ["--peer={addr1} -test=c::x", "--peer={addr0} -test=p::x"],
            }
        },
    )
    sidecar = BumbleSidecar()
    sidecar.configure(instance)

    # The controllers bind kernel-chosen ports and report them; nothing in
    # the sidecar picks a port up front, so parallel workers cannot collide.
    controllers = _fake_controllers(b'HCI0 00:00:01:00:00:0A 40001\nHCI1 00:00:01:00:00:0B 40002\n')
    peer = mock.Mock(pid=4322)
    with (
        mock.patch("importlib.util.find_spec", return_value=mock.Mock()),
        mock.patch("subprocess.Popen", side_effect=[controllers, peer]) as popen_mock,
    ):
        assert sidecar.setup() is True

    assert sidecar.ports == [40001, 40002]
    controllers_cmd = popen_mock.call_args_list[0].args[0]
    assert controllers_cmd[:2] == [sys.executable, sidecar.script]
    assert controllers_cmd[2:] == [
        'tcp-server:127.0.0.1:0@00:00:01:00:00:0A',
        'tcp-server:127.0.0.1:0@00:00:01:00:00:0B',
    ]
    assert instance.handler.extra_test_args == [
        '--bt-dev=127.0.0.1:40001',
        '--peer=00:00:01:00:00:0B',
        '-test=c::x',
    ]
    peer_cmd = popen_mock.call_args_list[1].args[0]
    assert peer_cmd == [
        os.path.join(instance.build_dir, 'zephyr', 'zephyr.exe'),
        '--bt-dev=127.0.0.1:40002',
        '--peer=00:00:01:00:00:0A',
        '-test=p::x',
    ]

    # A peer that fails turns a passing guest into a failed test.
    instance.status = TwisterStatus.PASS
    peer.wait.return_value = 3
    with mock.patch("twisterlib.sidecars.bumble.terminate_process") as term_mock:
        sidecar.teardown()

    term_mock.assert_called_once_with(controllers)
    assert instance.status == TwisterStatus.FAIL
    assert instance.reason == "Bumble peer 1 exited with 3"
    assert sidecar._proc is None
    assert controllers.stdout.closed


def test_bumble_setup_errors_when_controllers_do_not_start(tmp_path):
    from twisterlib.sidecars.bumble import BumbleSidecar

    instance = _bumble_instance(tmp_path, {"bumble": {"devices": ["", ""]}})
    sidecar = BumbleSidecar()
    sidecar.configure(instance)

    controllers = _fake_controllers(b'', returncode=1)
    with (
        mock.patch("importlib.util.find_spec", return_value=mock.Mock()),
        mock.patch("subprocess.Popen", return_value=controllers) as popen_mock,
    ):
        assert sidecar.setup() is False

    # Only the controllers were started; no peer runs against nothing. With
    # Bumble present this is a broken backend, which must not pass as a skip.
    assert popen_mock.call_count == 1
    assert instance.status == TwisterStatus.ERROR
    assert instance.handler.extra_test_args is None

    with mock.patch("twisterlib.sidecars.bumble.terminate_process"):
        sidecar.teardown()


def test_bumble_setup_rejects_more_devices_than_controllers(tmp_path):
    from twisterlib.sidecars.bumble import BumbleSidecar

    # Two default addresses, three devices.
    instance = _bumble_instance(tmp_path, {"bumble": {"devices": ["", "", ""]}})
    sidecar = BumbleSidecar()
    sidecar.configure(instance)

    with (
        mock.patch("importlib.util.find_spec", return_value=mock.Mock()),
        mock.patch("subprocess.Popen") as popen_mock,
    ):
        assert sidecar.setup() is False

    popen_mock.assert_not_called()
    assert instance.status == TwisterStatus.ERROR


def test_bumble_setup_rejects_unknown_placeholder(tmp_path):
    from twisterlib.sidecars.bumble import BumbleSidecar

    instance = _bumble_instance(tmp_path, {"bumble": {"devices": ["--peer={addr2}"]}})
    sidecar = BumbleSidecar()
    sidecar.configure(instance)

    controllers = _fake_controllers(b'HCI0 00:00:01:00:00:01 40001\nHCI1 00:00:01:00:00:02 40002\n')
    with (
        mock.patch("importlib.util.find_spec", return_value=mock.Mock()),
        mock.patch("subprocess.Popen", return_value=controllers),
    ):
        assert sidecar.setup() is False

    assert instance.status == TwisterStatus.ERROR
    assert instance.handler.extra_test_args is None

    with mock.patch("twisterlib.sidecars.bumble.terminate_process"):
        sidecar.teardown()


def _finished_peer(tmp_path, returncode=0, output=''):
    """A peer that has exited, with ``output`` as its log."""
    from twisterlib.sidecars.bumble import Peer

    path = tmp_path / f'peer{len(list(tmp_path.glob("peer*.log")))}.log'
    path.write_text(output)
    proc = mock.Mock()
    proc.wait.return_value = returncode
    return Peer(proc, mock.Mock(), str(path))


def test_bumble_peers_get_the_rest_of_the_test_timeout(tmp_path):
    import time

    from twisterlib.sidecars.bumble import BumbleSidecar

    instance = _bumble_instance(tmp_path)
    sidecar = BumbleSidecar()
    sidecar.configure(instance)

    # A passing guest leaves the peers what is left of the scenario's timeout.
    sidecar._deadline = time.monotonic() + 45
    instance.status = TwisterStatus.PASS
    sidecar._peers = [_finished_peer(tmp_path), _finished_peer(tmp_path)]
    peers = [peer.proc for peer in sidecar._peers]
    sidecar.teardown()
    assert 44 < peers[0].wait.call_args.kwargs['timeout'] <= 45
    assert 44 < peers[1].wait.call_args.kwargs['timeout'] <= 45
    assert instance.status == TwisterStatus.PASS

    # With the timeout used up there is one grace period, not one per peer.
    sidecar._deadline = time.monotonic() - 1
    sidecar._peers = [_finished_peer(tmp_path), _finished_peer(tmp_path)]
    peers = [peer.proc for peer in sidecar._peers]
    sidecar.teardown()
    assert peers[0].wait.call_args.kwargs['timeout'] <= BumbleSidecar.PEER_GRACE
    assert peers[1].wait.call_args.kwargs['timeout'] <= BumbleSidecar.PEER_GRACE

    # A failed guest settles the result, so the grace period is all there is.
    sidecar._deadline = time.monotonic() + 45
    instance.status = TwisterStatus.FAIL
    instance.reason = "Timeout"
    sidecar._peers = [_finished_peer(tmp_path)]
    peers = [peer.proc for peer in sidecar._peers]
    sidecar.teardown()
    assert peers[0].wait.call_args.kwargs['timeout'] <= BumbleSidecar.PEER_GRACE
    assert instance.reason == "Timeout"


def test_bumble_names_a_peer_that_did_not_finish(tmp_path):
    import subprocess

    from twisterlib.sidecars.bumble import BumbleSidecar

    instance = _bumble_instance(tmp_path)
    sidecar = BumbleSidecar()
    sidecar.configure(instance)
    instance.status = TwisterStatus.PASS

    peer = _finished_peer(tmp_path)
    peer.proc.wait.side_effect = [subprocess.TimeoutExpired('zephyr.exe', 45), -9]
    sidecar._peers = [peer]
    sidecar.teardown()

    peer.proc.kill.assert_called_once()
    peer.log.close.assert_called_once()
    assert instance.status == TwisterStatus.FAIL
    assert instance.reason == "Bumble peer 1 did not finish"


PEER_OUTPUT = """\
*** Booting Zephyr OS build v4.4.0 ***
Running TESTSUITE peripheral
===================================================================
START - test_connect
{end}
------ TESTSUITE SUMMARY START ------

SUITE SKIP - 0.00% [central]: pass = 0, fail = 0, skip = 1, total = 1 duration = 0.000 seconds
 - SKIP - [central.test_connect] duration = 0.000 seconds

SUITE {status} - 100.00% [peripheral]: pass = 1, fail = 0, total = 1 duration = 1.230 seconds
 - {status} - [peripheral.test_connect] duration = 1.230 seconds

------ TESTSUITE SUMMARY END ------

===================================================================
PROJECT EXECUTION {verdict}
"""

PEER_PASSED = PEER_OUTPUT.format(
    end=" PASS - test_connect in 1.230 seconds\nTESTSUITE peripheral succeeded",
    status="PASS",
    verdict="SUCCESSFUL",
)
PEER_FAILED = PEER_OUTPUT.format(
    end=" FAIL - test_connect in 1.230 seconds\nTESTSUITE peripheral failed.",
    status="FAIL",
    verdict="FAILED",
)
# The case started, and nothing came after it.
PEER_HUNG = PEER_OUTPUT.split("{end}")[0]
# The case passed, and the run never said so.
PEER_NO_VERDICT = PEER_PASSED.replace("PROJECT EXECUTION SUCCESSFUL\n", "")


def _bumble_after_guest(tmp_path):
    """A sidecar whose instance is as the harness leaves it once the guest has passed."""
    from twisterlib.sidecars.bumble import BumbleSidecar

    tmp_path.mkdir(exist_ok=True)
    instance = _bumble_instance(tmp_path)
    instance.status = TwisterStatus.PASS
    instance.set_case_status_by_name("id.central.connect", TwisterStatus.PASS)
    # The guest left this one out, which its summary reports as skipped.
    instance.set_case_status_by_name("id.peripheral.connect", TwisterStatus.SKIP, "ztest skip")
    sidecar = BumbleSidecar()
    sidecar.configure(instance)
    return instance, sidecar


def test_bumble_reports_the_cases_a_peer_ran(tmp_path):
    instance, sidecar = _bumble_after_guest(tmp_path)
    sidecar._peers = [_finished_peer(tmp_path, output=PEER_PASSED)]

    sidecar.teardown()

    ran_on_peer = instance.get_case_by_name("id.peripheral.connect")
    assert ran_on_peer.status == TwisterStatus.PASS
    assert ran_on_peer.reason is None
    assert ran_on_peer.duration == 1.23
    # What the peer skipped is the guest's to report.
    assert instance.get_case_by_name("id.central.connect").status == TwisterStatus.PASS
    assert instance.status == TwisterStatus.PASS
    assert len(instance.testcases) == 2


def test_bumble_names_the_case_a_peer_failed(tmp_path):
    instance, sidecar = _bumble_after_guest(tmp_path)
    sidecar._peers = [_finished_peer(tmp_path, returncode=1, output=PEER_FAILED)]

    sidecar.teardown()

    failed = instance.get_case_by_name("id.peripheral.connect")
    assert failed.status == TwisterStatus.FAIL
    assert failed.reason == "Failed on Bumble peer 1"
    assert instance.get_case_by_name("id.central.connect").status == TwisterStatus.PASS
    assert instance.status == TwisterStatus.FAIL
    assert instance.reason == "Bumble peer 1: id.peripheral.connect failed"


def test_bumble_names_the_case_a_peer_did_not_finish(tmp_path):
    import subprocess

    instance, sidecar = _bumble_after_guest(tmp_path)
    peer = _finished_peer(tmp_path, output=PEER_HUNG)
    peer.proc.wait.side_effect = [subprocess.TimeoutExpired('zephyr.exe', 60), -9]
    sidecar._peers = [peer]

    sidecar.teardown()

    assert instance.get_case_by_name("id.peripheral.connect").status == TwisterStatus.FAIL
    assert instance.status == TwisterStatus.FAIL
    assert instance.reason == "Bumble peer 1: id.peripheral.connect failed"


def test_bumble_takes_no_exit_status_for_a_verdict(tmp_path):
    instance, sidecar = _bumble_after_guest(tmp_path)
    sidecar._peers = [_finished_peer(tmp_path, output=PEER_NO_VERDICT)]

    sidecar.teardown()

    assert instance.status == TwisterStatus.FAIL
    assert instance.reason == "Bumble peer 1 ended without a verdict"


def test_bumble_requires_a_verdict_of_any_ztest_run(tmp_path):
    # Nothing the peer ran is left to report, and it is a Ztest run all the same.
    skipped = """\
Running TESTSUITE peripheral
START - test_connect
 SKIP - test_connect in 0.000 seconds
"""
    summary_only = (
        "SUITE SKIP - 0.00% [peripheral]: pass = 0, fail = 0, skip = 1, total = 1"
        " duration = 0.000 seconds\n"
    )
    suite_end_only = "TESTSUITE peripheral succeeded\n"
    started_only = "Running TESTSUITE peripheral\n"
    for output in (skipped, started_only, summary_only, suite_end_only):
        instance, sidecar = _bumble_after_guest(tmp_path / str(len(output)))
        sidecar._peers = [_finished_peer(tmp_path, output=output)]

        sidecar.teardown()

        assert instance.status == TwisterStatus.FAIL
        assert instance.reason == "Bumble peer 1 ended without a verdict"


def test_bumble_leaves_the_logging_of_a_peer_alone(tmp_path):
    # Not a Ztest application, and one of its lines looks like a case.
    output = "*** Booting Zephyr OS ***\nSTART - advertising\nPASS - scan in 1 seconds\n"
    instance, sidecar = _bumble_after_guest(tmp_path)
    sidecar._peers = [_finished_peer(tmp_path, output=output)]

    sidecar.teardown()

    assert instance.status == TwisterStatus.PASS
    assert len(instance.testcases) == 2


def test_bumble_peer_without_results_is_judged_by_its_exit(tmp_path):
    instance, sidecar = _bumble_after_guest(tmp_path)
    booted = "*** Booting Zephyr OS ***\n"
    sidecar._peers = [_finished_peer(tmp_path, output=booted)]
    sidecar.teardown()
    assert instance.status == TwisterStatus.PASS

    sidecar._peers = [_finished_peer(tmp_path, returncode=-11, output=booted)]
    sidecar.teardown()
    assert instance.get_case_by_name("id.peripheral.connect").status == TwisterStatus.SKIP
    assert instance.status == TwisterStatus.FAIL
    assert instance.reason == "Bumble peer 1 exited with -11"


def test_bumble_names_peer_cases_after_the_peer(tmp_path):
    # A peer built from other sources: twister found none of these in the
    # guest's sources, and two suites share a case name.
    output = """\
Running TESTSUITE peer_a
START - test_connect
 PASS - test_connect in 0.500 seconds
TESTSUITE peer_a succeeded
Running TESTSUITE peer_b
START - test_connect
 PASS - test_connect in 0.250 seconds
TESTSUITE peer_b succeeded
PROJECT EXECUTION SUCCESSFUL
"""
    instance, sidecar = _bumble_after_guest(tmp_path)
    sidecar._peers = [_finished_peer(tmp_path, output=output)]

    sidecar.teardown()

    assert instance.get_case_by_name("id.peer_a.connect").status == TwisterStatus.PASS
    assert instance.get_case_by_name("id.peer_b.connect").status == TwisterStatus.PASS
    assert instance.get_case_by_name("id.peer_b.connect").duration == 0.25
    assert instance.status == TwisterStatus.PASS


def test_bumble_stops_everything_before_it_reads_results(tmp_path):
    instance, sidecar = _bumble_after_guest(tmp_path)
    peer = _finished_peer(tmp_path, output=PEER_PASSED)
    sidecar._peers = [peer]
    controllers = mock.Mock()
    sidecar._proc = controllers
    order = []
    peer.proc.wait.side_effect = lambda **_: order.append('peer') or 0

    with (
        mock.patch(
            "twisterlib.sidecars.bumble.terminate_process",
            side_effect=lambda _: order.append('controllers'),
        ),
        mock.patch.object(
            sidecar, "_peer_results", side_effect=lambda _: order.append('results') or ({}, True)
        ),
    ):
        sidecar.teardown()

    assert order == ['peer', 'controllers', 'results']
    assert sidecar._proc is None
    assert instance.status == TwisterStatus.PASS


def _bumble_device_commands(tmp_path, config):
    """What setup() makes the guest and the one peer run with ``config``."""
    from twisterlib.sidecars.bumble import BumbleSidecar

    instance = _bumble_instance(tmp_path, {"bumble": config})
    sidecar = BumbleSidecar()
    sidecar.configure(instance)

    controllers = _fake_controllers(b'HCI0 00:00:01:00:00:01 40001\nHCI1 00:00:01:00:00:02 40002\n')
    with (
        mock.patch("importlib.util.find_spec", return_value=mock.Mock()),
        mock.patch("subprocess.Popen", side_effect=[controllers, mock.Mock(pid=1)]) as popen_mock,
    ):
        assert sidecar.setup() is True
    peer_command = popen_mock.call_args_list[1].args[0]

    sidecar._peers = []
    with mock.patch("twisterlib.sidecars.bumble.terminate_process"):
        sidecar.teardown()
    return instance.handler.extra_test_args, peer_command


def test_bumble_adds_the_controller_option_a_test_names(tmp_path):
    guest, peer = _bumble_device_commands(
        tmp_path, {"devices": ["-test=c::x", "-test=p::x"], "controller_option": "--hci"}
    )

    assert guest == ['--hci=127.0.0.1:40001', '-test=c::x']
    assert peer[1:] == ['--hci=127.0.0.1:40002', '-test=p::x']


def test_bumble_adds_nothing_for_an_empty_controller_option(tmp_path):
    guest, peer = _bumble_device_commands(
        tmp_path,
        {"devices": ["--bt-dev={ctrl1} -test=c::x", "--bt-dev={ctrl0}"], "controller_option": ""},
    )

    # Written by the test, which also chose the controllers the other way round.
    assert guest == ['--bt-dev=127.0.0.1:40002', '-test=c::x']
    assert peer[1:] == ['--bt-dev=127.0.0.1:40001']


def test_bumble_placeholders_do_not_change_what_is_added(tmp_path):
    # Naming a controller, or braces that only look like it, is no request to
    # leave the option out.
    guest, peer = _bumble_device_commands(
        tmp_path, {"devices": ["--other={ctrl1}", "--label={{ctrl0}}"]}
    )

    assert guest == ['--bt-dev=127.0.0.1:40001', '--other=127.0.0.1:40002']
    assert peer[1:] == ['--bt-dev=127.0.0.1:40002', '--label={ctrl0}']


def test_bumble_config_schema_takes_strings_and_mappings():
    import jsonschema
    from twisterlib.sidecars.bumble import BumbleSidecar

    schema = BumbleSidecar.config_schema()
    jsonschema.validate(
        {
            'devices': ['-test=a::b', {'args': '-test=c::d', 'image': 'x.y'}],
            'controller_option': '',
        },
        schema,
    )
    with pytest.raises(jsonschema.ValidationError):
        jsonschema.validate({'devices': [{'args': '-test=c::d', 'imag': 'x.y'}]}, schema)
    with pytest.raises(jsonschema.ValidationError):
        jsonschema.validate({'devices': [7]}, schema)


def _bumble_with_required_peer(tmp_path, image):
    from twisterlib.sidecars.bumble import BumbleSidecar
    from twisterlib.testsuitedata import RequiredApplication

    instance = _bumble_instance(
        tmp_path,
        {"bumble": {"devices": ["-test=c::x", {"args": "-test=p::x", "image": image}]}},
    )
    instance.testsuite.required_applications = [
        RequiredApplication(application='other.app'),
        RequiredApplication(application='peer.app'),
    ]
    instance.required_build_dirs = ['/builds/other', '/builds/peer']
    sidecar = BumbleSidecar()
    sidecar.configure(instance)
    return instance, sidecar


def test_bumble_runs_a_peer_from_a_required_application(tmp_path):
    _, sidecar = _bumble_with_required_peer(tmp_path, 'peer.app')
    assert sidecar.devices == ["-test=c::x", "-test=p::x"]

    controllers = _fake_controllers(b'HCI0 00:00:01:00:00:01 40001\nHCI1 00:00:01:00:00:02 40002\n')
    with (
        mock.patch("importlib.util.find_spec", return_value=mock.Mock()),
        mock.patch("subprocess.Popen", side_effect=[controllers, mock.Mock(pid=1)]) as popen_mock,
    ):
        assert sidecar.setup() is True

    assert popen_mock.call_args_list[1].args[0] == [
        os.path.join('/builds/peer', 'zephyr', 'zephyr.exe'),
        '--bt-dev=127.0.0.1:40002',
        '-test=p::x',
    ]

    sidecar._peers = []
    with mock.patch("twisterlib.sidecars.bumble.terminate_process"):
        sidecar.teardown()


def test_bumble_setup_rejects_an_image_that_is_not_required(tmp_path):
    instance, sidecar = _bumble_with_required_peer(tmp_path, 'missing.app')

    with (
        mock.patch("importlib.util.find_spec", return_value=mock.Mock()),
        mock.patch("subprocess.Popen") as popen_mock,
    ):
        assert sidecar.setup() is False

    popen_mock.assert_not_called()
    assert instance.status == TwisterStatus.ERROR
    assert instance.reason == "image 'missing.app' is not among the required applications"


def test_bumble_setup_rejects_an_image_for_the_guest(tmp_path):
    from twisterlib.sidecars.bumble import BumbleSidecar

    instance = _bumble_instance(
        tmp_path, {"bumble": {"devices": [{"args": "-test=c::x", "image": "peer.app"}]}}
    )
    sidecar = BumbleSidecar()
    sidecar.configure(instance)

    with (
        mock.patch("importlib.util.find_spec", return_value=mock.Mock()),
        mock.patch("subprocess.Popen") as popen_mock,
    ):
        assert sidecar.setup() is False

    popen_mock.assert_not_called()
    assert instance.status == TwisterStatus.ERROR


def test_bumble_finds_the_executable_of_a_sysbuild_build(tmp_path):
    _, sidecar = _bumble_with_required_peer(tmp_path, 'peer.app')
    build_dir = tmp_path / 'peer-build'
    (build_dir / 'app').mkdir(parents=True)
    (build_dir / 'domains.yaml').write_text(
        f"""\
default: app
build_dir: {build_dir}
domains:
  - name: app
    build_dir: {build_dir / 'app'}
flash_order:
  - app
"""
    )
    sidecar.instance.required_build_dirs = ['/builds/other', str(build_dir)]

    assert sidecar._executable(1) == str(build_dir / 'app' / 'zephyr' / 'zephyr.exe')
    # Without sysbuild the executable is where it has always been.
    assert sidecar._executable(0) == os.path.join(
        sidecar.instance.build_dir, 'zephyr', 'zephyr.exe'
    )
