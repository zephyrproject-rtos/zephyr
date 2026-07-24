#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Tests for the Sidecar classes of twister."""

import os
from dataclasses import dataclass
from unittest import mock

import pytest
from twisterlib.sidecars import Sidecar, SidecarImporter, sidecar_config_schema
from twisterlib.sidecars.net_tools import (
    NET_TOOLS_SHARED_SUBNET,
    NetToolsSidecar,
    get_net_addresses,
    get_net_iface_name,
)
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


# --- net-tools --------------------------------------------------------------


def test_net_tools_is_registered():
    assert isinstance(SidecarImporter.get_sidecar('net-tools'), NetToolsSidecar)


def test_net_iface_and_addresses_are_unique_and_valid():
    iface = get_net_iface_name("/some/long/build/dir")
    assert iface.startswith("zeth") and len(iface) <= 15
    assert iface == get_net_iface_name("/some/long/build/dir")
    assert get_net_iface_name("/other") != iface

    host, guest, prefix = get_net_addresses("/some/long/build/dir")
    assert host.endswith(".2") and guest.endswith(".1") and prefix == 24
    assert host.rsplit(".", 1)[0] == guest.rsplit(".", 1)[0]
    assert get_net_addresses("/other")[0] != host


def test_net_shared_subnet_uses_fixed_addresses(tmp_path):
    # Without the flag, each instance gets its own private subnet.
    (tmp_path / "a").mkdir()
    private = NetToolsSidecar()
    private.configure(_make_instance(tmp_path / "a"))
    assert private.host_ip.startswith("10.")

    # With it, the fixed 192.0.2.x addresses the companion expects are used.
    (tmp_path / "b").mkdir()
    shared = NetToolsSidecar()
    shared.configure(_make_instance(tmp_path / "b", {"net-tools": {"shared_subnet": True}}))
    assert (shared.host_ip, shared.guest_ip, shared.prefix) == NET_TOOLS_SHARED_SUBNET


def test_net_tools_cmake_args_bakes_iface_and_addresses(tmp_path):
    instance = _make_instance(tmp_path, {"net-tools": {"iface": "zeth7"}})
    sidecar = NetToolsSidecar()
    sidecar.configure(instance)

    args = sidecar.cmake_args(instance.build_dir)
    assert '-DCONFIG_ETH_QEMU_IFACE_NAME="zeth7"' in args
    assert f'-DCONFIG_NET_CONFIG_MY_IPV4_ADDR="{sidecar.guest_ip}"' in args
    assert f'-DCONFIG_NET_CONFIG_PEER_IPV4_ADDR="{sidecar.host_ip}"' in args


def test_net_tools_cmake_args_omits_addresses_on_shared_subnet(tmp_path):
    # A shared subnet means the test keeps the fixed addresses; only the
    # interface name is baked so the guest still attaches to this instance's tap.
    instance = _make_instance(tmp_path, {"net-tools": {"shared_subnet": True}})
    sidecar = NetToolsSidecar()
    sidecar.configure(instance)

    args = sidecar.cmake_args(instance.build_dir)
    assert any(a.startswith('-DCONFIG_ETH_QEMU_IFACE_NAME=') for a in args)
    assert not any('IPV4_ADDR' in a for a in args)


def test_net_tools_uses_sudo_when_not_root(tmp_path):
    instance = _make_instance(tmp_path)
    sidecar = NetToolsSidecar()
    sidecar.configure(instance)
    sidecar.net_setup = "/opt/net-tools/net-setup.sh"

    with mock.patch("os.geteuid", return_value=1000):
        cmd = sidecar._net_setup_cmd("start")

    assert cmd[:2] == ["sudo", "-n"]
    assert cmd[-1] == "start"


def test_net_tools_setup_skips_when_script_missing(tmp_path):
    instance = _make_instance(tmp_path)
    sidecar = NetToolsSidecar()
    sidecar.configure(instance)
    sidecar.net_setup = None

    with mock.patch("subprocess.run") as run_mock:
        proceed = sidecar.setup()

    assert proceed is False
    run_mock.assert_not_called()
    assert instance.status == TwisterStatus.SKIP


def test_net_tools_start_stop_commands(tmp_path):
    instance = _make_instance(tmp_path, {"net-tools": {"iface": "zeth7"}})
    os.makedirs(instance.build_dir, exist_ok=True)
    sidecar = NetToolsSidecar()
    sidecar.configure(instance)
    sidecar.net_setup = "/opt/net-tools/net-setup.sh"

    calls = []

    def _fake_run(cmd, **_kwargs):
        calls.append(cmd)
        return mock.Mock(returncode=0, stderr="", stdout="")

    with mock.patch("os.geteuid", return_value=0), \
         mock.patch("subprocess.run", side_effect=_fake_run):
        assert sidecar.setup() is True
        sidecar.teardown()

    # A stale-cleanup stop, then start, then the teardown stop, all on zeth7 with
    # the generated per-instance config.
    conf = os.path.join(instance.build_dir, "net-tools.conf")
    assert [c[-1] for c in calls] == ["stop", "start", "stop"]
    for c in calls:
        assert c == ["/opt/net-tools/net-setup.sh", "--iface", "zeth7",
                     "--config", conf, c[-1]]
    # The generated config puts the host on this instance's own subnet.
    with open(conf) as f:
        assert sidecar.host_ip in f.read()


def test_net_tools_launches_and_stops_companion(tmp_path):
    instance = _make_instance(
        tmp_path, {"net-tools": {"iface": "zeth7", "apps": ["echo-server"]}}
    )
    os.makedirs(instance.build_dir, exist_ok=True)
    sidecar = NetToolsSidecar()
    sidecar.configure(instance)
    sidecar.net_setup = "/opt/net-tools/net-setup.sh"

    app_proc = mock.Mock()

    with mock.patch("os.geteuid", return_value=0), \
         mock.patch("subprocess.run", return_value=mock.Mock(returncode=0, stderr="", stdout="")), \
         mock.patch("os.path.exists", return_value=True), \
         mock.patch("subprocess.Popen", return_value=app_proc) as popen_mock:
        assert sidecar.setup() is True
        # The known "echo-server" shortcut expands and binds to this iface.
        assert popen_mock.call_args.args[0] == ["/opt/net-tools/echo-server", "-i", "zeth7"]

    with mock.patch("twisterlib.sidecars.net_tools.terminate_process") as term_mock:
        sidecar.teardown()

    term_mock.assert_called_once_with(app_proc)


def test_net_tools_skips_when_companion_missing(tmp_path):
    instance = _make_instance(tmp_path, {"net-tools": {"apps": ["echo-server"]}})
    os.makedirs(instance.build_dir, exist_ok=True)
    sidecar = NetToolsSidecar()
    sidecar.configure(instance)
    sidecar.net_setup = "/opt/net-tools/net-setup.sh"

    with mock.patch("os.geteuid", return_value=0), \
         mock.patch("subprocess.run", return_value=mock.Mock(returncode=0, stderr="", stdout="")), \
         mock.patch("os.path.exists", return_value=False), \
         mock.patch("subprocess.Popen") as popen_mock:
        proceed = sidecar.setup()

    assert proceed is False
    popen_mock.assert_not_called()
    assert instance.status == TwisterStatus.SKIP
