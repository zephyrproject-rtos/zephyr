# Copyright (c) 2026 Zephyr Project contributors
# SPDX-License-Identifier: Apache-2.0

"""Tests for west completion IR generation from live argparse."""

import json
import os
import shlex
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

COMPLETION_DIR = Path(__file__).resolve().parents[1] / "completion"
sys.path.insert(0, str(COMPLETION_DIR))

import generate  # noqa: E402

BASH_SCRIPT = COMPLETION_DIR / "west-completion.bash"
ZSH_SCRIPT = COMPLETION_DIR / "west-completion.zsh"


def test_ir_generation():
    ir, errors = generate.collect_ir()
    assert not errors

    commands = ir["commands"]
    for name in ("build", "twister", "blobs", "init", "update"):
        assert name in commands

    build_flags = commands["build"]["flags"]
    assert any("--board" in f["opts"] for f in build_flags)
    assert any("--pristine" in f["opts"] for f in build_flags)
    assert any("-h" in f["opts"] or "--help" in f["opts"] for f in ir["global_flags"])


def test_dump_json_is_object():
    ir, errors = generate.collect_ir()
    assert not errors
    data = json.loads(generate.dump_json(ir))
    assert "global_flags" in data
    assert "commands" in data
    assert "build" in data["commands"]


def test_check_scripts():
    import check_completion

    ir, errors = generate.collect_ir()
    assert not errors
    rendered = generate.render(ir)
    if "scripts/west_commands/completion/generate.py" in BASH_SCRIPT.read_text(encoding="utf-8"):
        assert check_completion.check_scripts(rendered) == []
    else:
        stale = check_completion.check_scripts(rendered)
        assert len(stale) > 0


def test_zsh_repeatable_flag_spec_star_after_exclusion():
    spec = generate.zsh_flag_spec(
        {
            "opts": ["-o", "--build-opt"],
            "takes_value": True,
            "hint": "file",
            "repeatable": True,
            "help": "options to pass to the build tool",
        }
    )
    assert spec.startswith("'(-o --build-opt)*'")
    assert not spec.startswith("'*(-o --build-opt)'")


def test_generated_zsh_repeatable_specs_are_valid():
    ir, errors = generate.collect_ir()
    assert not errors
    text = generate.render(ir)["zsh"]
    # '*(-o --opt)' is parsed as a rest argument and fails at Tab time.
    assert "'*(" not in text


def _bash_complete(script_path, words):
    quoted = " ".join(shlex.quote(w) for w in words)
    script = f"""
source {shlex.quote(script_path.as_posix())}
COMP_WORDS=({quoted})
COMP_CWORD={len(words) - 1}
COMP_LINE={shlex.quote(" ".join(words))}
COMP_POINT=${{#COMP_LINE}}
COMPREPLY=()
__comp_west
printf '%s\\n' "${{COMPREPLY[@]}}"
"""
    proc = subprocess.run(
        ["bash", "-c", script],
        capture_output=True,
        text=True,
        check=True,
    )
    return [line for line in proc.stdout.splitlines() if line]


def test_bash_completes_commands_and_flags(tmp_path):
    if sys.platform == "win32" or not shutil.which("bash"):
        pytest.skip("bash completion test requires POSIX environment")

    ir, errors = generate.collect_ir()
    assert not errors
    rendered = generate.render(ir)
    script_path = tmp_path / "west-completion.bash"
    script_path.write_text(rendered["bash"], encoding="utf-8")

    cmds = _bash_complete(script_path, ["west", ""])
    for name in ("build", "reset", "snippets", "packages", "gtags", "robot"):
        assert name in cmds

    flags = _bash_complete(script_path, ["west", "build", "-"])
    for opt in (
        "--extra-conf",
        "--extra-dtc-overlay",
        "--cmake-opt",
        "--board",
        "--shield",
        "--snippet",
        "--pristine",
        "-p",
    ):
        assert opt in flags

    tools = _bash_complete(script_path, ["west", "sign", "-t", ""])
    assert set(tools) >= {"picotool", "rimage", "silabs_commander"}
    assert "imgtool" not in tools

    blobs = _bash_complete(script_path, ["west", "blobs", ""])
    assert set(blobs) >= {"list", "fetch", "clean"}

    shells = _bash_complete(script_path, ["west", "completion", ""])
    assert set(shells) >= {"bash", "fish", "powershell", "zsh"}


def test_bash_blobs_fetch_completes_modules():
    ir, errors = generate.collect_ir()
    assert not errors
    text = generate.render(ir)["bash"]
    start = text.index("__comp_west_blobs()")
    end = text.index("\n__comp_west_", start + 1)
    body = text[start:end]
    assert "__set_comp list fetch clean" in body
    assert "__set_comp_west_projs" in body


def test_load_extension_class_caches_by_path():
    generate._EXTENSION_MODULES.clear()
    debug = generate.load_extension_class("scripts/west_commands/debug.py", "Debug")
    attach = generate.load_extension_class("scripts/west_commands/debug.py", "Attach")
    assert debug is not attach
    assert debug.__module__ == attach.__module__
    key = os.fspath((generate.ZEPHYR_BASE / "scripts/west_commands/debug.py").resolve())
    assert key in generate._EXTENSION_MODULES


def test_zsh_west_build_function_has_flags(tmp_path):
    if not shutil.which("zsh"):
        pytest.skip("zsh not installed")
    ir, errors = generate.collect_ir()
    assert not errors
    script_path = tmp_path / "west-completion.zsh"
    script_path.write_text(generate.render(ir)["zsh"], encoding="utf-8")
    script = f"""
autoload -Uz compinit
compinit -u -D
source {shlex.quote(str(script_path))}
typeset -f _west_build
"""
    proc = subprocess.run(
        ["zsh", "-f", "-c", script],
        capture_output=True,
        text=True,
        check=True,
    )
    assert "--extra-conf" in proc.stdout
    assert "--board" in proc.stdout
    assert "--pristine" in proc.stdout
    assert "invalid rest argument definition" not in proc.stderr
