# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

"""Check the EDS tools against the object dictionary of the running node."""

import os
import re
import subprocess
import sys
from pathlib import Path

import canopen
import pytest

ZEPHYR_BASE = Path(os.environ.get("ZEPHYR_BASE", Path(__file__).resolve().parents[6]))
SCRIPTS = ZEPHYR_BASE / "scripts" / "canopen"
TEST_EDS = Path(__file__).resolve().parents[1] / "test.eds"
NODE_ID = 0x22

sys.path.insert(0, str(SCRIPTS))
import canopen_eds as eds  # noqa: E402


@pytest.fixture(scope="module")
def build_dir(twister_harness_config):
    return Path(twister_harness_config.devices[0].app_build_dir)


@pytest.fixture(scope="module")
def config(build_dir):
    return eds.read_kconfig(build_dir / "zephyr" / ".config")


def scan(build_dir):
    result = subprocess.run(
        [str(build_dir / "zephyr" / "zephyr.exe")],
        capture_output=True,
        text=True,
        timeout=120,
        check=True,
    )
    assert "OD end" in result.stdout
    entries = {}
    for line in result.stdout.splitlines():
        match = re.match(
            r"OD ([0-9a-f]{4}) ([0-9a-f]{2}) (nodata|([0-9a-f]{2}) ([0-9a-f]{2})"
            r"(?: ([0-9a-f]*))?)$",
            line.strip(),
        )
        if match is None:
            continue
        key = (int(match.group(1), 16), int(match.group(2), 16))
        if match.group(3) == "nodata":
            entries[key] = None
        else:
            entries[key] = (
                int(match.group(4), 16),
                int(match.group(5), 16),
                bytes.fromhex(match.group(6) or ""),
            )
    return entries


def access(attr):
    attr &= eds.ATTR_READ | eds.ATTR_WRITE | eds.ATTR_CONST
    return {eds.CONST: "const", eds.RO: "ro", eds.ATTR_WRITE: "wo", eds.RW: "rw"}[attr]


def test_eds_matches_node(build_dir):
    entries = scan(build_dir)
    od = canopen.import_od(str(build_dir / "zephyr" / "canopen.eds"), node_id=NODE_ID)
    variables = {}
    for obj in od.values():
        if isinstance(obj, canopen.objectdictionary.ODVariable):
            if obj.index > 0x1B:
                variables[(obj.index, 0)] = obj
        else:
            for var in obj.values():
                variables[(var.index, var.subindex)] = var

    assert set(variables) == set(entries)
    for key, entry in entries.items():
        var = variables[key]
        if entry is None:
            continue
        data_type, attr, raw = entry
        assert var.data_type == data_type, key
        assert {"rwr": "rw", "rww": "rw"}.get(var.access_type, var.access_type) == access(attr), key
        assert var.pdo_mappable == bool(attr & (eds.ATTR_TPDO | eds.ATTR_RPDO)), key
        if attr & eds.ATTR_READ and data_type != eds.DOMAIN:
            if data_type == eds.VISIBLE_STRING:
                assert var.default == raw.decode("ascii"), key
            else:
                assert var.encode_raw(var.default) == raw, key


def same_sub(a, b):
    readonly = {"ro", "const"}
    x, y = eds.access_name(a.attr), eds.access_name(b.attr)
    return (
        a.data_type == b.data_type
        and (x == y or {x, y} <= readonly)
        and (a.attr ^ b.attr) & ~eds.ATTR_CONST == 0
        and a.default == b.default
    )


def test_round_trip(build_dir, config):
    if not eds.kconfig_bool(config, "CONFIG_TEST_OD_FROM_EDS"):
        pytest.skip("object dictionary from C macros")
    source = eds.read_eds(TEST_EDS)
    exported = eds.read_eds(build_dir / "zephyr" / "canopen.eds")
    assert set(source) == set(exported)
    for index, obj in source.items():
        out = exported[index]
        assert obj.code == out.code, f"{index:04X}h"
        assert set(obj.subs) == set(out.subs), f"{index:04X}h"
        if 0x1000 <= index <= 0x1FFF:
            assert obj.name.lower() == out.name.lower(), f"{index:04X}h"
        for sub, s in obj.subs.items():
            assert same_sub(s, out.subs[sub]), f"{index:04X}h sub {sub}: {s} {out.subs[sub]}"


def run_eds2od(tmp_path, build_dir, text):
    path = tmp_path / "bad.eds"
    path.write_text(text, encoding="ascii")
    return subprocess.run(
        [
            sys.executable,
            str(SCRIPTS / "eds2od.py"),
            "--config",
            str(build_dir / "zephyr" / ".config"),
            "--name",
            "bad_od",
            str(path),
            str(tmp_path / "bad_od.h"),
        ],
        capture_output=True,
        text=True,
        check=False,
    )


@pytest.mark.parametrize(
    "old,new,message",
    [
        (
            "DefaultValue=$NODEID + 0x40000000 + 0x180",
            "DefaultValue=$NODEID+0x180",
            "1800h sub-index 1: bit 30 must be set",
        ),
        (
            "Vendor-ID\nObjectType=0x7\nDataType=0x0007\nAccessType=ro\nDefaultValue=0x0",
            "Vendor-ID\nObjectType=0x7\nDataType=0x0007\nAccessType=ro\nDefaultValue=0x1234",
            "1018h sub-index 1: value 0x1234, stack has 0x0",
        ),
        (
            "[1005]",
            "[100C]\nParameterName=Guard time\nDataType=0x0006\nAccessType=rw\n\n[1005]",
            "100Ch: not provided by the stack",
        ),
        (
            "DefaultValue=zephyr",
            "DefaultValue=",
            "2004h sub-index 0: a string needs a DefaultValue",
        ),
        (
            "[1017]\nParameterName=Producer heartbeat time\nObjectType=0x7\nDataType=0x0006\n"
            "AccessType=rw\nDefaultValue=0x1F4",
            "[1017]\nParameterName=Producer heartbeat time\nObjectType=0x7\nDataType=0x0006\n"
            "AccessType=ro\nDefaultValue=0x1F4",
            "1017h sub-index 0: AccessType ro, stack has rw",
        ),
        ("0x22000420", "0x22000520", "TPDO1: 0x22000520 does not name a mappable object"),
    ],
)
def test_eds2od_errors(tmp_path, build_dir, config, old, new, message):
    if not eds.kconfig_bool(config, "CONFIG_TEST_OD_FROM_EDS"):
        pytest.skip("checks use test.eds")
    text = TEST_EDS.read_text(encoding="ascii").replace("\r\n", "\n")
    assert old in text
    result = run_eds2od(tmp_path, build_dir, text.replace(old, new, 1))
    assert result.returncode != 0
    assert message in result.stderr


def test_numbers():
    assert eds.parse_int("012").value == 10
    assert eds.parse_int("0x0A").value == 10
    assert eds.parse_int("10").value == 10
    assert eds.parse_int("-0x10").value == -16
    assert eds.parse_int("$NODEID+0x180") == eds.Value(0x180, True)
    assert eds.parse_int("$NODEID + 0x40000000 + 0x180") == eds.Value(0x40000180, True)
    with pytest.raises(eds.EdsError):
        eds.parse_int("0x180+$NODEID")
