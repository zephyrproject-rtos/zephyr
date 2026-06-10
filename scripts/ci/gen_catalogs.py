#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Generate per-board JSON databases for Zephyr: DTS, compile coverage, and Kconfig.

This script runs twister in cmake-only mode for every board and harvests
three independent queryable databases from the resulting build artefacts:

  DTS catalog (``--dts-db``)
      EDT pickle files → per-board devicetree hardware information.

  Compile database (``--compile-db``)
      ``compile_commands.json`` → which source files each platform compiles
      and, conversely, which platforms compile a given source file.

  Kconfig database (``--kconfig-db``)
      ``zephyr/.config`` → which Kconfig options each platform sets and,
      conversely, which platforms set a given option.

All three outputs are optional; pass one or more flags to select what to
generate.

Typical usage::

    # Generate all three databases (may take a long time for --all):
    python scripts/gen_dts_catalog.py \\
        --dts-db dts_catalog.json \\
        --compile-db compile_db.json \\
        --kconfig-db kconfig_db.json

    # Limit to a subset of vendors to iterate quickly:
    python scripts/gen_dts_catalog.py \\
        --dts-db dts_catalog.json \\
        --compile-db compile_db.json \\
        --kconfig-db kconfig_db.json \\
        --vendor nordic --vendor st

    # Re-use a twister output directory from a previous run:
    python scripts/gen_dts_catalog.py \\
        --dts-db dts_catalog.json \\
        --compile-db compile_db.json \\
        --kconfig-db kconfig_db.json \\
        --twister-outdir /tmp/my-twister-out --skip-twister

    # Generate only one database:
    python scripts/gen_dts_catalog.py --dts-db dts_catalog.json
    python scripts/gen_dts_catalog.py --compile-db compile_db.json
    python scripts/gen_dts_catalog.py --kconfig-db kconfig_db.json

Database schemas
-----------------

All databases share two common top-level metadata fields:

generated_at : str
    ISO-8601 timestamp of generation.
zephyr_base : str
    Absolute path of the Zephyr tree used for generation.

DTS catalog (``--dts-db``)
~~~~~~~~~~~~~~~~~~~~~~~~~~~
boards : dict[board_name, BoardEntry]
    Per-board DTS data.  Each ``BoardEntry`` is::

        {
          "targets": {
            "<board_target>": {
              "compatibles": ["compat-a", "compat-b", ...],
              "hardware": {
                "<binding-type>": {
                  "<compatible>": {
                    "description": str,
                    "title": str,
                    "binding_path": str,
                    "custom_binding": bool,
                    "locations": ["board" | "soc"],
                    "okay": bool,
                    "dts_sources": [{"file": str, "line": int, "okay": bool}]
                  }
                }
              }
            }
          },
          "runners": {
            "runners": ["runner-a", ...],
            "flash_runner": str,
            "debug_runner": str
          }
        }

compatibles : dict[compatible_str, CompatEntry]
    DTS reverse index.  Each ``CompatEntry`` is::

        {
          "description": str,
          "title": str,
          "binding_type": str,
          "boards": ["board_name", ...]
        }

Compile database (``--compile-db``)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
files : dict[relative_path, FileEntry]
    Forward index keyed by workspace-relative source file path
    (relative to ZEPHYR_BASE for in-tree files, or to the west
    workspace root for module files).  Each ``FileEntry`` is::

        {
          "platforms": ["board_target_a", "board_target_b", ...]
        }

    Query example — which platforms compile ``kernel/sched.c``?::

        db["files"]["kernel/sched.c"]["platforms"]

platforms : dict[board_target, PlatformEntry]
    Reverse index keyed by board target string
    (``board[@revision][/qualifiers]``).  Each ``PlatformEntry`` is::

        {
          "files": ["path/to/file1.c", ...]
        }

Kconfig database (``--kconfig-db``)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
options : dict[option_name, OptionEntry]
    Forward index keyed by Kconfig option name (e.g. ``CONFIG_UART_CONSOLE``).
    Each ``OptionEntry`` is::

        {
          "platforms": {
            "<board_target>": "<value>"
          }
        }

    The value is the string from ``.config``: ``"y"`` for boolean options,
    a quoted string for string options, or a decimal/hex literal for
    integer options.

    Query example — which platforms enable ``CONFIG_UART_CONSOLE``?::

        db["options"]["CONFIG_UART_CONSOLE"]["platforms"]

platforms : dict[board_target, KconfigPlatformEntry]
    Reverse index keyed by board target string.  Each entry is::

        {
          "options": {
            "CONFIG_UART_CONSOLE": "y",
            "CONFIG_MAIN_STACK_SIZE": "1024",
            ...
          }
        }
"""

import argparse
import datetime
import json
import logging
import os
import pickle
import re
import subprocess
import sys
import tempfile
from pathlib import Path

import yaml

ZEPHYR_BASE = os.environ.get('ZEPHYR_BASE')
if ZEPHYR_BASE:
    ZEPHYR_BASE = Path(ZEPHYR_BASE)
else:
    ZEPHYR_BASE = Path(__file__).resolve().parents[2]
    # Propagate this decision to child processes.
    os.environ['ZEPHYR_BASE'] = str(ZEPHYR_BASE)

# West workspace virtual environment (created by pip install -r requirements.txt
# or `west setup`).  Activating it ensures all Zephyr Python dependencies
# (twister, devicetree, runners, …) are available both in this process and in
# the twister sub-process.
_WEST_VENV = ZEPHYR_BASE.parent / ".venv"
_VENV_BIN = _WEST_VENV / "bin"
_VENV_PYTHON = _VENV_BIN / "python3"

# The devicetree package is not installed globally or inside the venv; it lives
# as in-tree source.  Add it unconditionally so that pickle.load() can
# deserialise EDT objects regardless of how the script is invoked.
_DEVICETREE_SRC = ZEPHYR_BASE / "scripts" / "dts" / "python-devicetree" / "src"
if str(_DEVICETREE_SRC) not in sys.path:
    sys.path.insert(0, str(_DEVICETREE_SRC))

EDT_PICKLE_PATHS = (
    "zephyr/edt.pickle",
    "hello_world/zephyr/edt.pickle",  # for board targets using sysbuild
)
RUNNERS_YAML_PATHS = (
    "zephyr/runners.yaml",
    "hello_world/zephyr/runners.yaml",  # for board targets using sysbuild
)
COMPILE_COMMANDS_PATHS = (
    "compile_commands.json",
    "hello_world/compile_commands.json",  # for board targets using sysbuild
)
KCONFIG_PATHS = (
    "zephyr/.config",
    "hello_world/zephyr/.config",  # for board targets using sysbuild
)

# Absolute path of the west workspace root (parent of ZEPHYR_BASE).
_WEST_TOPDIR = ZEPHYR_BASE.parent

ZEPHYR_BINDINGS = ZEPHYR_BASE / "dts" / "bindings"

logger = logging.getLogger(__name__)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def _get_binding_type(binding_path: Path) -> str:
    """Return the binding-type category for a binding file path.

    Mirrors the logic in ``doc/_scripts/dts_binding_types.py``
    without requiring a cross-directory import.
    """
    if binding_path.is_relative_to(ZEPHYR_BINDINGS):
        return binding_path.relative_to(ZEPHYR_BINDINGS).parts[0]
    return "misc"


# ---------------------------------------------------------------------------
# Twister execution
# ---------------------------------------------------------------------------


def run_twister_cmake_only(outdir: Path, vendor_filter: list) -> None:
    """Run twister in cmake-only mode to generate build-info and EDT files.

    Args:
        outdir: Directory where twister should write its output.
        vendor_filter: If non-empty, restrict to boards from these vendors.
    """
    python = str(_VENV_PYTHON) if _VENV_PYTHON.exists() else sys.executable

    twister_cmd = [
        python,
        str(ZEPHYR_BASE / "scripts" / "twister"),
        "-T",
        "samples/hello_world/",
        "-M",
        *[arg for path in EDT_PICKLE_PATHS for arg in ("--keep-artifacts", path)],
        *[arg for path in RUNNERS_YAML_PATHS for arg in ("--keep-artifacts", path)],
        *[arg for path in COMPILE_COMMANDS_PATHS for arg in ("--keep-artifacts", path)],
        *[arg for path in KCONFIG_PATHS for arg in ("--keep-artifacts", path)],
        "--cmake-only",
        "-v",
        "--outdir",
        str(outdir),
    ]

    if vendor_filter:
        for vendor in vendor_filter:
            twister_cmd += ["--vendor", vendor]
    else:
        twister_cmd += ["--all"]

    path = os.environ.get("PATH", "")
    if _VENV_BIN.exists():
        path = str(_VENV_BIN) + os.pathsep + path

    minimal_env = {
        "PATH": path,
        "ZEPHYR_BASE": str(ZEPHYR_BASE),
        "HOME": os.environ.get("HOME", ""),
        "PYTHONPATH": os.environ.get("PYTHONPATH", ""),
    }
    if _WEST_VENV.exists():
        minimal_env["VIRTUAL_ENV"] = str(_WEST_VENV)

    logger.info("Running twister (cmake-only) …")
    logger.debug("Command: %s", " ".join(twister_cmd))

    try:
        subprocess.run(twister_cmd, check=True, cwd=ZEPHYR_BASE, env=minimal_env)
    except subprocess.CalledProcessError as exc:
        logger.warning("Twister exited with an error; the catalog may be incomplete.\n%s", exc)


# ---------------------------------------------------------------------------
# Build-info harvesting
# ---------------------------------------------------------------------------


def gather_board_edts(twister_out_dir: Path) -> dict:
    """Load EDT objects for every board target from a twister output directory.

    Args:
        twister_out_dir: Root of the twister output tree.

    Returns:
        Nested dict ``{board_name: {board_target: edt_object}}``.
    """
    board_edts = {}

    if not twister_out_dir.exists():
        logger.warning("Twister output directory does not exist: %s", twister_out_dir)
        return board_edts

    build_info_files = list(twister_out_dir.glob("*/**/build_info.yml"))
    logger.info("Found %d build_info.yml files", len(build_info_files))

    for build_info_file in build_info_files:
        edt_pickle_file = None
        for pickle_path in EDT_PICKLE_PATHS:
            candidate = build_info_file.parent / pickle_path
            if candidate.exists():
                edt_pickle_file = candidate
                break

        if not edt_pickle_file:
            continue

        try:
            with open(build_info_file) as fh:
                build_info = yaml.safe_load(fh)
                board_info = build_info.get("cmake", {}).get("board", {})
                board_name = board_info.get("name")
                qualifier = board_info.get("qualifiers", "")
                revision = board_info.get("revision", "")

            if not board_name:
                continue

            board_target = board_name
            if revision:
                board_target = f"{board_target}@{revision}"
            if qualifier:
                board_target = f"{board_target}/{qualifier}"

            with open(edt_pickle_file, "rb") as fh:
                edt = pickle.load(fh)

            board_edts.setdefault(board_name, {})[board_target] = edt

        except Exception as exc:  # noqa: BLE001
            logger.error("Error processing %s: %s", build_info_file, exc)

    return board_edts


def gather_board_runners(twister_out_dir: Path) -> dict:
    """Load runners.yaml for every board target from a twister output directory.

    Args:
        twister_out_dir: Root of the twister output tree.

    Returns:
        Nested dict ``{board_name: {board_target: runners_yaml_dict}}``.
    """
    board_runners: dict = {}

    if not twister_out_dir.exists():
        return board_runners

    build_info_files = list(twister_out_dir.glob("*/**/build_info.yml"))

    for build_info_file in build_info_files:
        runners_file = None
        for runners_path in RUNNERS_YAML_PATHS:
            candidate = build_info_file.parent / runners_path
            if candidate.exists():
                runners_file = candidate
                break

        if runners_file is None:
            continue

        try:
            with open(build_info_file) as fh:
                build_info = yaml.safe_load(fh)
                board_info = build_info.get("cmake", {}).get("board", {})
                board_name = board_info.get("name")
                qualifier = board_info.get("qualifiers", "")
                revision = board_info.get("revision", "")

            if not board_name:
                continue

            board_target = board_name
            if revision:
                board_target = f"{board_target}@{revision}"
            if qualifier:
                board_target = f"{board_target}/{qualifier}"

            with open(runners_file) as fh:
                runners_data = yaml.safe_load(fh)

            board_runners.setdefault(board_name, {})[board_target] = runners_data

        except Exception as exc:  # noqa: BLE001
            logger.error("Error processing %s: %s", runners_file, exc)

    return board_runners


# ---------------------------------------------------------------------------
# Compile-commands harvesting
# ---------------------------------------------------------------------------


def _rel_source_path(src: str) -> str | None:
    """Return a workspace-relative path for *src*, or None to skip it.

    Paths inside the Zephyr tree are made relative to ZEPHYR_BASE.
    Paths inside other west modules are made relative to the west
    workspace root so they remain meaningful across machines.
    Generated files inside a twister output directory are skipped.
    """
    try:
        p = Path(src).resolve()
    except Exception:  # noqa: BLE001
        return None

    # Skip build-directory artifacts (inside a twister-out tree).
    if "twister-out" in p.parts:
        return None

    if p.is_relative_to(ZEPHYR_BASE):
        return str(p.relative_to(ZEPHYR_BASE))

    if p.is_relative_to(_WEST_TOPDIR):
        return str(p.relative_to(_WEST_TOPDIR))

    return None


def gather_compile_commands(twister_out_dir: Path) -> dict:
    """Load compile_commands.json for every board target.

    For each board target the function reads the ``compile_commands.json``
    file produced by CMake and extracts the set of source files that are
    compiled.  Paths are normalised to be relative to either ZEPHYR_BASE
    (for in-tree files) or the west workspace root (for module files).

    Args:
        twister_out_dir: Root of the twister output tree.

    Returns:
        Nested dict ``{board_name: {board_target: sorted_file_list}}``.
    """
    board_files: dict = {}

    if not twister_out_dir.exists():
        logger.warning("Twister output directory does not exist: %s", twister_out_dir)
        return board_files

    build_info_files = list(twister_out_dir.glob("*/**/build_info.yml"))
    logger.info(
        "Found %d build_info.yml files for compile-commands harvesting",
        len(build_info_files),
    )

    for build_info_file in build_info_files:
        cc_file = None
        for cc_path in COMPILE_COMMANDS_PATHS:
            candidate = build_info_file.parent / cc_path
            if candidate.exists():
                cc_file = candidate
                break

        if cc_file is None:
            continue

        try:
            with open(build_info_file) as fh:
                build_info = yaml.safe_load(fh)
                board_info = build_info.get("cmake", {}).get("board", {})
                board_name = board_info.get("name")
                qualifier = board_info.get("qualifiers", "")
                revision = board_info.get("revision", "")

            if not board_name:
                continue

            board_target = board_name
            if revision:
                board_target = f"{board_target}@{revision}"
            if qualifier:
                board_target = f"{board_target}/{qualifier}"

            with open(cc_file, encoding="utf-8") as fh:
                commands = json.load(fh)

            files: set = set()
            for entry in commands:
                rel = _rel_source_path(entry.get("file", ""))
                if rel is not None:
                    files.add(rel)

            board_files.setdefault(board_name, {})[board_target] = sorted(files)

        except Exception as exc:  # noqa: BLE001
            logger.error("Error processing %s: %s", cc_file, exc)

    return board_files


# ---------------------------------------------------------------------------
# File-index construction
# ---------------------------------------------------------------------------


def build_file_index(board_files: dict) -> dict:
    """Build forward and reverse indexes from compile_commands data.

    Args:
        board_files: Nested dict ``{board_name: {board_target: [files]}}``.

    Returns:
        Dict with two top-level keys:

        ``files``
            Maps each source file (workspace-relative path) to the list of
            board targets that compile it::

                {
                  "path/to/file.c": {
                    "platforms": ["board_target_a", "board_target_b", ...]
                  }
                }

        ``platforms``
            Maps each board target to the list of source files it compiles::

                {
                  "board_target": {
                    "files": ["path/to/file1.c", ...]
                  }
                }
    """
    files_db: dict = {}
    platforms_db: dict = {}

    for _board_name, targets in board_files.items():
        for board_target, files in targets.items():
            platforms_db[board_target] = {"files": files}
            for f in files:
                files_db.setdefault(f, {"platforms": []})["platforms"].append(board_target)

    for entry in files_db.values():
        entry["platforms"].sort()

    return {"files": files_db, "platforms": platforms_db}


# ---------------------------------------------------------------------------
# Kconfig harvesting
# ---------------------------------------------------------------------------

# Matches lines like:  CONFIG_FOO=y  CONFIG_BAR="hello"  CONFIG_BAZ=1024
_KCONFIG_RE = re.compile(r'^(CONFIG_\w+)=(.+)$')


def _parse_kconfig(path: Path) -> dict:
    """Parse a ``.config`` file and return a dict of ``{option: value}``.

    Only ``CONFIG_*=<value>`` lines are collected.  Comment lines of the
    form ``# CONFIG_FOO is not set`` (disabled boolean options) are skipped
    because they do not represent an active configuration choice.
    """
    options: dict = {}
    with open(path, encoding="utf-8") as fh:
        for line in fh:
            m = _KCONFIG_RE.match(line.rstrip())
            if m:
                options[m.group(1)] = m.group(2)
    return options


def gather_kconfigs(twister_out_dir: Path) -> dict:
    """Load ``.config`` for every board target from a twister output directory.

    Args:
        twister_out_dir: Root of the twister output tree.

    Returns:
        Nested dict ``{board_name: {board_target: {option: value}}}``.
    """
    board_kconfigs: dict = {}

    if not twister_out_dir.exists():
        logger.warning("Twister output directory does not exist: %s", twister_out_dir)
        return board_kconfigs

    build_info_files = list(twister_out_dir.glob("*/**/build_info.yml"))
    logger.info(
        "Found %d build_info.yml files for Kconfig harvesting",
        len(build_info_files),
    )

    for build_info_file in build_info_files:
        kconfig_file = None
        for kc_path in KCONFIG_PATHS:
            candidate = build_info_file.parent / kc_path
            if candidate.exists():
                kconfig_file = candidate
                break

        if kconfig_file is None:
            continue

        try:
            with open(build_info_file) as fh:
                build_info = yaml.safe_load(fh)
                board_info = build_info.get("cmake", {}).get("board", {})
                board_name = board_info.get("name")
                qualifier = board_info.get("qualifiers", "")
                revision = board_info.get("revision", "")

            if not board_name:
                continue

            board_target = board_name
            if revision:
                board_target = f"{board_target}@{revision}"
            if qualifier:
                board_target = f"{board_target}/{qualifier}"

            options = _parse_kconfig(kconfig_file)
            board_kconfigs.setdefault(board_name, {})[board_target] = options

        except Exception as exc:  # noqa: BLE001
            logger.error("Error processing %s: %s", kconfig_file, exc)

    return board_kconfigs


def build_kconfig_db(board_kconfigs: dict) -> dict:
    """Build forward and reverse indexes from per-board Kconfig data.

    Args:
        board_kconfigs: Nested dict
            ``{board_name: {board_target: {option: value}}}``.

    Returns:
        Dict with two top-level keys:

        ``options``
            Maps each Kconfig option name to the platforms that set it and
            the value each platform uses::

                {
                  "CONFIG_UART_CONSOLE": {
                    "platforms": {
                      "board_target_a": "y",
                      "board_target_b": "y"
                    }
                  }
                }

        ``platforms``
            Maps each board target to its full Kconfig dictionary::

                {
                  "board_target": {
                    "options": {
                      "CONFIG_UART_CONSOLE": "y",
                      "CONFIG_MAIN_STACK_SIZE": "1024"
                    }
                  }
                }
    """
    options_db: dict = {}
    platforms_db: dict = {}

    for _board_name, targets in board_kconfigs.items():
        for board_target, options in targets.items():
            platforms_db[board_target] = {"options": options}
            for opt, val in options.items():
                options_db.setdefault(opt, {"platforms": {}})["platforms"][board_target] = val

    return {"options": options_db, "platforms": platforms_db}


def _node_location(node, board_name: str) -> str:
    """Return 'board' or 'soc' depending on where the DTS node is defined."""
    filename = node.filename
    if not filename:
        return "soc"
    path = Path(filename)
    if path.is_relative_to(ZEPHYR_BASE):
        rel = path.relative_to(ZEPHYR_BASE)
        if rel.parts[0] == "boards":
            return "board"
    return "soc"


def build_catalog(board_edts: dict, board_runners: dict | None = None) -> dict:
    """Build the DTS catalog from harvested EDT objects.

    Args:
        board_edts: Nested dict ``{board_name: {board_target: edt_object}}``.
        board_runners: Optional nested dict
            ``{board_name: {board_target: runners_yaml_dict}}``.  When
            provided, per-board runner information is embedded in the catalog.

    Returns:
        Dict with ``boards`` and ``compatibles`` top-level keys (see module
        docstring for the full schema).
    """
    if board_runners is None:
        board_runners = {}
    boards_db = {}
    compatibles_db = {}

    for board_name, targets in board_edts.items():
        board_entry = {"targets": {}}

        for board_target, edt in targets.items():
            target_compatibles = set()
            hardware = {}

            for node in edt.nodes:
                if node.binding_path is None or node.matching_compat is None:
                    continue

                # Skip synthetic "zephyr," nodes (except native_sim where they
                # represent real peripherals).
                if node.matching_compat.startswith("zephyr,") and board_name != "native_sim":
                    continue

                binding_path = Path(node.binding_path)
                binding_type = _get_binding_type(binding_path)
                compat = node.matching_compat

                # Compute a workspace-relative binding path for consumers.
                rel_binding_path = str(binding_path)
                if binding_path.is_relative_to(ZEPHYR_BASE):
                    rel_binding_path = str(binding_path.relative_to(ZEPHYR_BASE))
                elif binding_path.is_relative_to(_WEST_TOPDIR):
                    rel_binding_path = str(binding_path.relative_to(_WEST_TOPDIR))

                # A binding is "custom" when it lives outside the Zephyr
                # bindings tree (i.e. it comes from an out-of-tree module).
                custom_binding = binding_type == "misc" and not binding_path.is_relative_to(
                    ZEPHYR_BINDINGS
                )

                target_compatibles.add(compat)

                # Build human-readable node source location.
                filename = node.filename or ""
                rel_filename = filename
                if filename and Path(filename).is_relative_to(ZEPHYR_BASE):
                    rel_filename = str(Path(filename).relative_to(ZEPHYR_BASE))

                is_okay = node.status == "okay"
                node_info = {"file": rel_filename, "line": node.lineno, "okay": is_okay}

                # Accumulate per-compatible data inside the hardware map.
                compat_entry = hardware.setdefault(binding_type, {}).get(compat)
                if compat_entry is None:
                    compat_entry = {
                        "description": _first_sentence(node.description),
                        "title": node.title or "",
                        "binding_path": rel_binding_path,
                        "custom_binding": custom_binding,
                        "locations": set(),
                        "okay": False,
                        "dts_sources": [],
                    }
                    hardware.setdefault(binding_type, {})[compat] = compat_entry

                compat_entry["locations"].add(_node_location(node, board_name))
                if is_okay:
                    compat_entry["okay"] = True
                compat_entry["dts_sources"].append(node_info)

                # Update global reverse index.
                if compat not in compatibles_db:
                    compatibles_db[compat] = {
                        "description": _first_sentence(node.description),
                        "title": node.title or "",
                        "binding_type": binding_type,
                        "boards": [],
                    }
                if board_name not in compatibles_db[compat]["boards"]:
                    compatibles_db[compat]["boards"].append(board_name)

            # Convert sets → sorted lists for JSON serialisation.
            for btype in hardware:
                for compat_data in hardware[btype].values():
                    compat_data["locations"] = sorted(compat_data["locations"])

            board_entry["targets"][board_target] = {
                "compatibles": sorted(target_compatibles),
                "hardware": hardware,
            }

        # Embed runner info at the board level; all targets are assumed to
        # share the same flash/debug runners so the first available target is
        # used (matching the behaviour in the doc build).
        runners_entry: dict = {}
        if board_name in board_runners:
            r = next(iter(board_runners[board_name].values()))
            runners_entry = {
                "runners": r.get("runners") or [],
                "flash_runner": r.get("flash-runner") or "",
                "debug_runner": r.get("debug-runner") or "",
            }
        board_entry["runners"] = runners_entry

        boards_db[board_name] = board_entry

    # Sort the reverse-index board lists for deterministic output.
    for entry in compatibles_db.values():
        entry["boards"].sort()

    return {
        "generated_at": datetime.datetime.now(datetime.UTC).isoformat(),
        "zephyr_base": str(ZEPHYR_BASE),
        "boards": boards_db,
        "compatibles": compatibles_db,
    }


def _first_sentence(text: str | None) -> str:
    """Return the first sentence of *text*, or the whole text if none found."""
    if not text:
        return ""
    text = text.replace("\n", " ")
    first_para = text.split("  ")[0].strip()
    match = re.search(r"(.*?)\.(?:\s|$)", first_para)
    if match:
        return match.group(1).strip()
    return first_para


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def parse_args(argv=None):
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    parser.add_argument(
        "--dts-db",
        metavar="FILE",
        help="Path for the DTS catalog JSON output.  Skipped when omitted.",
    )
    parser.add_argument(
        "--compile-db",
        metavar="FILE",
        help=(
            "Path for the compile-commands database JSON output "
            "(file→platforms and platform→files indexes).  Skipped when omitted."
        ),
    )
    parser.add_argument(
        "--kconfig-db",
        metavar="FILE",
        help=(
            "Path for the Kconfig database JSON output "
            "(option→platforms and platform→options indexes).  Skipped when omitted."
        ),
    )
    parser.add_argument(
        "--twister-outdir",
        metavar="DIR",
        help=(
            "Directory to use for twister output.  "
            "Defaults to a temporary directory that is deleted after the run."
        ),
    )
    parser.add_argument(
        "--skip-twister",
        action="store_true",
        help=(
            "Skip the twister cmake-only step and use whatever build artefacts "
            "already exist in --twister-outdir.  Implies --twister-outdir."
        ),
    )
    parser.add_argument(
        "--vendor",
        action="append",
        dest="vendor_filter",
        default=[],
        metavar="VENDOR",
        help="Restrict to boards from this vendor (may be repeated).",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Enable verbose/debug logging.",
    )
    return parser.parse_args(argv)


def main(argv=None):
    args = parse_args(argv)

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(levelname)s: %(message)s",
    )

    if not args.dts_db and not args.compile_db and not args.kconfig_db:
        logger.error("At least one of --dts-db, --compile-db, or --kconfig-db must be specified")
        sys.exit(1)

    if args.skip_twister and not args.twister_outdir:
        logger.error("--skip-twister requires --twister-outdir")
        sys.exit(1)

    tmp_dir = None
    try:
        if args.twister_outdir:
            twister_outdir = Path(args.twister_outdir)
            twister_outdir.mkdir(parents=True, exist_ok=True)
        else:
            tmp_dir = tempfile.mkdtemp(prefix="zephyr-dts-catalog-")
            twister_outdir = Path(tmp_dir)

        if not args.skip_twister:
            run_twister_cmake_only(twister_outdir, args.vendor_filter)

        timestamp = datetime.datetime.now(datetime.UTC).isoformat()
        meta = {
            "generated_at": timestamp,
            "zephyr_base": str(ZEPHYR_BASE),
        }

        if args.dts_db:
            logger.info("Harvesting EDT data from %s …", twister_outdir)
            board_edts = gather_board_edts(twister_outdir)
            logger.info("Loaded EDT data for %d boards", len(board_edts))

            logger.info("Harvesting runners data from %s …", twister_outdir)
            board_runners = gather_board_runners(twister_outdir)
            logger.info("Loaded runners data for %d boards", len(board_runners))

            logger.info("Building DTS catalog …")
            catalog = build_catalog(board_edts, board_runners)
            catalog["generated_at"] = timestamp

            output_path = Path(args.dts_db)
            output_path.parent.mkdir(parents=True, exist_ok=True)
            with open(output_path, "w", encoding="utf-8") as fh:
                json.dump(catalog, fh, indent=2, default=str)

            logger.info(
                "Wrote %s  (%d boards, %d compatibles)",
                output_path,
                len(catalog["boards"]),
                len(catalog["compatibles"]),
            )

        if args.compile_db:
            logger.info("Harvesting compile_commands data from %s …", twister_outdir)
            board_files = gather_compile_commands(twister_outdir)
            logger.info(
                "Loaded compile_commands data for %d boards",
                len(board_files),
            )

            logger.info("Building file index …")
            file_index = build_file_index(board_files)
            file_index.update(meta)

            compile_db_path = Path(args.compile_db)
            compile_db_path.parent.mkdir(parents=True, exist_ok=True)
            with open(compile_db_path, "w", encoding="utf-8") as fh:
                json.dump(file_index, fh, indent=2, default=str)

            logger.info(
                "Wrote %s  (%d source files, %d platforms)",
                compile_db_path,
                len(file_index.get("files", {})),
                len(file_index.get("platforms", {})),
            )

        if args.kconfig_db:
            logger.info("Harvesting Kconfig data from %s …", twister_outdir)
            board_kconfigs = gather_kconfigs(twister_outdir)
            logger.info(
                "Loaded Kconfig data for %d boards",
                len(board_kconfigs),
            )

            logger.info("Building Kconfig index …")
            kconfig_index = build_kconfig_db(board_kconfigs)
            kconfig_index.update(meta)

            kconfig_db_path = Path(args.kconfig_db)
            kconfig_db_path.parent.mkdir(parents=True, exist_ok=True)
            with open(kconfig_db_path, "w", encoding="utf-8") as fh:
                json.dump(kconfig_index, fh, indent=2, default=str)

            logger.info(
                "Wrote %s  (%d options, %d platforms)",
                kconfig_db_path,
                len(kconfig_index.get("options", {})),
                len(kconfig_index.get("platforms", {})),
            )

    finally:
        if tmp_dir:
            import shutil

            shutil.rmtree(tmp_dir, ignore_errors=True)


if __name__ == "__main__":
    main()
