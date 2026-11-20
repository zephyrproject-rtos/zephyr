# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Exercise board resolution without a toolchain or a west workspace."""

import json
import shutil
import subprocess
import sys
from pathlib import Path

import pytest
import yaml

ZEPHYR_BASE = Path(__file__).resolve().parents[2]
MODULES = ZEPHYR_BASE / "cmake" / "modules"
CMAKE = shutil.which("cmake")
pytestmark = pytest.mark.skipif(CMAKE is None, reason="cmake is not installed")


@pytest.fixture
def board_project(tmp_path):
    root = tmp_path / "root"
    board_dir = root / "boards" / "test"
    soc_dir = root / "soc" / "test"
    board_dir.mkdir(parents=True)
    soc_dir.mkdir(parents=True)
    (root / "scripts").symlink_to(ZEPHYR_BASE / "scripts", target_is_directory=True)

    socs = [
        {"name": "chip", "cpuclusters": [{"name": "cpu"}]},
        {"name": "other", "cpuclusters": [{"name": "cpu"}]},
        {"name": "plain"},
    ]
    (soc_dir / "soc.yml").write_text(yaml.safe_dump({"socs": socs}), encoding="utf-8")
    chip = {
        "name": "chip",
        "variants": [{"name": "ns", "cpucluster": "cpu", "variants": [{"name": "mcuboot"}]}],
    }
    boards = [
        {"name": "foo", "socs": [chip]},
        {"name": "simple", "socs": [{"name": "plain"}]},
        {"name": "multi", "socs": [chip, {"name": "other"}]},
        {
            "name": "revboard",
            "socs": [chip],
            "revision": {
                "format": "number",
                "default": "1",
                "revisions": [{"name": "1"}, {"name": "2"}],
            },
        },
    ]
    for board in boards:
        board["full_name"] = board["name"]
    (board_dir / "board.yml").write_text(yaml.safe_dump({"boards": boards}), encoding="utf-8")
    (tmp_path / "CMakeLists.txt").write_text(
        """\
cmake_minimum_required(VERSION 3.28.0)
project(board_resolution NONE)
list(APPEND CMAKE_MODULE_PATH "${MODULES}")
include(extensions)
# Build metadata is unrelated to board resolution.
function(build_info)
endfunction()
include(boards)
file(WRITE "${CMAKE_BINARY_DIR}/resolved.txt" "${BOARD}@${BOARD_REVISION}/${BOARD_QUALIFIERS}")
""",
        encoding="utf-8",
    )

    def configure(board, mappings="", aliases="", rerun=False):
        (root / "boards" / "deprecated.cmake").write_text(mappings, encoding="utf-8")
        alias_file = tmp_path / "aliases.cmake"
        alias_file.write_text(aliases, encoding="utf-8")
        command = [
            CMAKE,
            "-S",
            str(tmp_path),
            "-B",
            str(tmp_path / "build"),
            f"-DBOARD={board}",
            f"-DZEPHYR_BASE={root}",
            f"-DMODULES={MODULES}",
            f"-DPython3_EXECUTABLE={sys.executable}",
            f"-DZEPHYR_BOARD_ALIASES={alias_file}",
            "--trace-expand",
            "--trace-format=json-v1",
            f"--trace-source={MODULES / 'boards.cmake'}",
        ]
        result = subprocess.run(command, capture_output=True, text=True, check=False)
        if rerun:
            assert result.returncode == 0, result.stdout + result.stderr
            result = subprocess.run(command, capture_output=True, text=True, check=False)
        trace = [json.loads(line) for line in result.stderr.splitlines() if line.startswith("{")]
        calls = [
            entry
            for entry in trace
            if entry.get("cmd") == "execute_process"
            and any("/list_boards.py" in arg for arg in entry["args"])
        ]
        # The trace expands error-message arguments even if CMake does not emit them.
        output = result.stdout + "\n".join(
            line for line in result.stderr.splitlines() if not line.startswith("{")
        )
        resolved = tmp_path / "build" / "resolved.txt"
        return result.returncode, output, resolved, len(calls)

    return configure


@pytest.mark.parametrize(
    "board, expected",
    [
        ("foo//old", "chip/cpu"),
        ("foo/chip/old", "chip/cpu"),
        ("foo//old/ns", "chip/cpu/ns"),
        ("foo//old/ns/mcuboot", "chip/cpu/ns/mcuboot"),
    ],
)
def test_qualifier_renames(board_project, board, expected):
    code, output, resolved, calls = board_project(
        board, "set(foo/chip/old_DEPRECATED foo/chip/cpu)"
    )
    assert code == 0, output
    assert resolved.read_text() == f"foo@/{expected}"
    assert "Deprecated BOARD=" in output
    assert calls == 1


@pytest.mark.parametrize(
    "board, mapping, expected",
    [
        ("plank//", "plank simple", "simple@/plain"),
        ("plank", "plank simple", "simple@/plain"),
        ("plank//cpu", "plank foo", "foo@/chip/cpu"),
        ("plank/chip/cpu/ns", "plank foo", "foo@/chip/cpu/ns"),
        ("plank//cpu/ns", "plank/chip/cpu foo/chip/cpu", "foo@/chip/cpu/ns"),
        ("plank//cpu", "plank foo/chip", "foo@/chip/cpu"),
        ("plank//", "plank simple/plain", "simple@/plain"),
        ("simple//", "simple/old simple/plain", "simple@/plain"),
        ("foo/old/cpu", "foo/old foo/chip", "foo@/chip/cpu"),
        ("foo//cpu", "foo/old/cpu foo/chip/cpu", "foo@/chip/cpu"),
        ("foo//cpu/ns", "foo/old foo/chip", "foo@/chip/cpu/ns"),
        ("plank//", "plank/old simple/plain", "simple@/plain"),
        ("plank//cpu", "plank/old foo/chip", "foo@/chip/cpu"),
        ("plank//cpu", "plank/old foo", "foo@/chip/cpu"),
        ("plank/old/cpu/ns", "plank/old foo", "foo@/chip/cpu/ns"),
        ("plank@2//cpu", "plank revboard", "revboard@2/chip/cpu"),
        ("plank//cpu", "plank revboard@2/chip", "revboard@2/chip/cpu"),
    ],
)
def test_board_and_soc_renames(board_project, board, mapping, expected):
    old, new = mapping.split()
    code, output, resolved, calls = board_project(board, f"set({old}_DEPRECATED {new})")
    assert code == 0, output
    assert resolved.read_text() == expected
    assert "Deprecated BOARD=" in output
    assert calls == 1


@pytest.mark.parametrize(
    "board, expected",
    [
        ("foo/chip/cpu", "foo@/chip/cpu"),
        ("foo//cpu/ns", "foo@/chip/cpu/ns"),
        ("simple", "simple@/plain"),
        ("simple//", "simple@/plain"),
        ("multi/other/cpu", "multi@/other/cpu"),
    ],
)
def test_current_targets(board_project, board, expected):
    code, output, resolved, calls = board_project(board)
    assert code == 0, output
    assert resolved.read_text() == expected
    assert "Deprecated BOARD=" not in output
    assert calls == 1


def test_specific_mapping_takes_precedence(board_project):
    code, output, resolved, calls = board_project(
        "plank//cpu/ns",
        """\
set(plank_DEPRECATED simple)
set(plank/old_DEPRECATED simple/plain)
set(plank/old/cpu_DEPRECATED foo/chip/cpu)
""",
    )
    assert code == 0, output
    assert resolved.read_text() == "foo@/chip/cpu/ns"
    assert calls == 1


@pytest.mark.parametrize("enabled", [False, True])
def test_disabled_mappings(board_project, enabled):
    mappings = "set(foo/old/cpu_DEPRECATED OFF)"
    if enabled:
        mappings += "\nset(foo/other/cpu_DEPRECATED foo/chip/cpu)"
    code, output, resolved, calls = board_project("foo//cpu", mappings)
    assert code == 0, output
    assert resolved.read_text() == "foo@/chip/cpu"
    assert ("Deprecated BOARD=" in output) == enabled
    assert calls == 1


def test_explicit_shorthand_takes_precedence(board_project):
    code, output, resolved, calls = board_project(
        "plank//cpu",
        """\
set(plank//cpu_DEPRECATED foo/chip/cpu/ns)
set(plank/old/cpu_DEPRECATED foo/chip/cpu)
set(plank/other/cpu_DEPRECATED multi/other/cpu)
""",
    )
    assert code == 0, output
    assert resolved.read_text() == "foo@/chip/cpu/ns"
    assert calls == 1


@pytest.mark.parametrize(
    "board, mappings, error, calls_expected",
    [
        (
            "plank//cpu",
            "set(plank/one/cpu_DEPRECATED foo/chip/cpu)\n"
            "set(plank/two/cpu_DEPRECATED multi/other/cpu)",
            "Ambiguous deprecated BOARD=plank//cpu",
            0,
        ),
        (
            "multi//old",
            "set(multi/chip/old_DEPRECATED multi/chip/cpu)",
            "Cannot omit the SoC",
            1,
        ),
        ("multi//cpu", "", "Please specify a valid board target", 1),
        ("foo//missing", "", "Please specify a valid board target", 1),
        (
            "foo//old/",
            "set(foo/chip/old_DEPRECATED foo/chip/cpu)",
            "Please specify a valid board target",
            1,
        ),
        ("foo/unknown/cpu", "", "Please specify a valid board target", 1),
        ("unknown//cpu", "", "No board named 'unknown' found", 2),
        (
            "plank@1//cpu",
            "set(plank_DEPRECATED revboard@2/chip)",
            "Invalid board revision: 1",
            0,
        ),
    ],
)
def test_invalid_targets(board_project, board, mappings, error, calls_expected):
    code, output, _, calls = board_project(board, mappings)
    assert code != 0
    assert error in " ".join(output.split())
    assert "ARCH not defined" not in output
    assert calls == calls_expected


def test_alias_and_reconfigure(board_project):
    code, output, resolved, calls = board_project(
        "alias//old",
        "set(foo/chip/old_DEPRECATED foo/chip/cpu)",
        "set(alias_BOARD_ALIAS foo)",
        rerun=True,
    )
    assert code == 0, output
    assert resolved.read_text() == "foo@/chip/cpu"
    assert calls == 1


@pytest.mark.parametrize(
    "stub, arch, expected",
    [
        (True, "", "The build uses the empty devicetree"),
        (False, "", "ARCH not defined"),
        (True, "arm", None),
        (False, "arm", None),
    ],
)
def test_missing_architecture(tmp_path, stub, arch, expected):
    script = tmp_path / "arch.cmake"
    dts = ZEPHYR_BASE / "boards/common/stub.dts" if stub else tmp_path / "board.dts"
    script.write_text(
        f"""\
cmake_minimum_required(VERSION 3.28.0)
set(ZEPHYR_BASE "{ZEPHYR_BASE}")
set(BOARD foo)
set(BOARD_QUALIFIERS chip/cpu)
set(DTS_SOURCE "{dts};{tmp_path}/overlay.dts")
set(CONFIG_ARCH "{arch}")
set(ARCH_V2_ARM_DIR "{ZEPHYR_BASE}/arch/arm")
include("{MODULES}/arch.cmake")
""",
        encoding="utf-8",
    )
    result = subprocess.run([CMAKE, "-P", str(script)], capture_output=True, text=True, check=False)
    if expected is None:
        assert result.returncode == 0, result.stderr
    else:
        assert result.returncode != 0
        assert expected in result.stderr
        assert "ARCH not defined" in result.stderr
        if stub:
            assert "BOARD=foo/chip/cpu" in result.stderr
