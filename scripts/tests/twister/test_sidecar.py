#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Tests for the Sidecar classes of twister."""

import os
from dataclasses import dataclass
from unittest import mock

import pytest
from twisterlib.sidecars import Sidecar, SidecarImporter, sidecar_config_schema
from twisterlib.sidecars.can import (
    CanSidecar,
    get_can_iface_name,
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


# --- can --------------------------------------------------------------------

def test_get_can_iface_name_is_short_and_unique():
    iface = get_can_iface_name("/some/long/build/dir")
    assert iface.startswith("zcan") and len(iface) <= 15
    assert iface == get_can_iface_name("/some/long/build/dir")
    assert get_can_iface_name("/other") != iface


def test_can_configure_reads_namespaced_config(tmp_path):
    instance = _make_instance(
        tmp_path, {"can": {"iface": "vcan9", "tools_apps": ["echo {iface}"]}}
    )
    sidecar = CanSidecar()
    sidecar.configure(instance)

    assert sidecar.iface == "vcan9"
    assert sidecar.tools_apps == ["echo {iface}"]


def test_can_configure_defaults_without_config(tmp_path):
    instance = _make_instance(tmp_path)
    sidecar = CanSidecar()
    sidecar.configure(instance)

    assert sidecar.iface == get_can_iface_name(instance.build_dir)
    assert sidecar.tools_apps == []


def test_can_cmake_args_bake_iface_name(tmp_path):
    instance = _make_instance(tmp_path)
    sidecar = CanSidecar()
    sidecar.configure(instance)

    args = sidecar.cmake_args(instance.build_dir)
    iface = get_can_iface_name(instance.build_dir)
    assert args == [f'-DCONFIG_CAN_QEMU_IFACE_NAME="{iface}"']


def test_can_setup_skips_without_root(tmp_path):
    instance = _make_instance(tmp_path)
    sidecar = CanSidecar()
    sidecar.configure(instance)

    with mock.patch("os.geteuid", return_value=1000), mock.patch(
        "subprocess.run", return_value=mock.Mock(returncode=1, stderr="")
    ):
        assert sidecar.setup() is False

    assert instance.status == TwisterStatus.SKIP
    assert "root or passwordless sudo" in instance.reason


def test_can_setup_skips_when_vcan_unsupported(tmp_path):
    instance = _make_instance(tmp_path)
    sidecar = CanSidecar()
    sidecar.configure(instance)

    def _run(cmd, *args, **kwargs):
        # The sudo probe succeeds; creating the vcan interface does not.
        if "link" in cmd and "add" in cmd:
            return mock.Mock(returncode=1, stderr="not supported")
        return mock.Mock(returncode=0, stderr="")

    with mock.patch("os.geteuid", return_value=0), mock.patch("subprocess.run", side_effect=_run):
        assert sidecar.setup() is False

    assert instance.status == TwisterStatus.SKIP
    assert "vcan" in instance.reason


def test_can_setup_brings_up_iface_and_passes_native_arg(tmp_path):
    instance = _make_instance(tmp_path)
    instance.platform.arch = "posix"
    instance.handler = mock.Mock(extra_test_args=[])
    sidecar = CanSidecar()
    sidecar.configure(instance)

    with mock.patch("os.geteuid", return_value=0), mock.patch(
        "subprocess.run", return_value=mock.Mock(returncode=0, stderr="")
    ) as run_mock:
        assert sidecar.setup() is True

    commands = [call.args[0] for call in run_mock.call_args_list]
    assert ["ip", "link", "add", "dev", sidecar.iface, "type", "vcan"] in commands
    assert ["ip", "link", "set", "up", sidecar.iface] in commands
    assert instance.handler.extra_test_args == [f"--can-if={sidecar.iface}"]


def test_can_setup_does_not_duplicate_native_arg(tmp_path):
    instance = _make_instance(tmp_path)
    instance.platform.arch = "posix"
    instance.handler = mock.Mock(extra_test_args=[])
    sidecar = CanSidecar()
    sidecar.configure(instance)

    with mock.patch("os.geteuid", return_value=0), mock.patch(
        "subprocess.run", return_value=mock.Mock(returncode=0, stderr="")
    ):
        assert sidecar.setup() is True
        assert sidecar.setup() is True

    assert instance.handler.extra_test_args == [f"--can-if={sidecar.iface}"]


def test_can_teardown_removes_iface(tmp_path):
    instance = _make_instance(tmp_path)
    sidecar = CanSidecar()
    sidecar.configure(instance)
    sidecar._started = True

    with mock.patch("os.geteuid", return_value=0), mock.patch(
        "subprocess.run", return_value=mock.Mock(returncode=0)
    ) as run_mock:
        sidecar.teardown()

    commands = [call.args[0] for call in run_mock.call_args_list]
    assert ["ip", "link", "del", sidecar.iface] in commands
    assert sidecar._started is False
