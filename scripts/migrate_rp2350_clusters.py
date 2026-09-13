#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: Apache-2.0

"""Migrate rp2350a/rp2350b boards from the legacy bare "hazard3"/"m33"
cpucluster qualifiers to the dual-core-aware "hazard3_0"/"m33_0" qualifiers.

For each board that declares an rp2350a or rp2350b SoC (discovered by
scanning board.yml files under boards/), this script:

  1. Creates the corresponding "_0"-suffixed board file triple
     (<board>_<soc>_<cluster>_0.dts / .yaml / _defconfig), for every
     existing bare-cluster triple (including "_w"/"_mcuboot" siblings).
     For rpi_pico2 (which already has a partially-migrated "m33" cluster
     with both bare and "_0" files kept side by side) the bare files are
     copied, not moved, so nothing that still depends on the bare
     qualifier is broken mid-transition. Every other board gets a
     straight rename (git mv) since no "_0" files exist for them yet and
     nothing external depends on their bare qualifier.
  2. Adds the missing "select SOC_..._0 if BOARD_..._0" line to each
     board's Kconfig.<boardname>. For boards that keep their bare board
     files around (rpi_pico2), the new line is inserted alongside the
     existing bare-cluster select line, since both remain valid targets.
     For every other board, the old bare-cluster select line is retired
     (replaced by the "_0" line, or dropped outright if the "_0" line was
     already added by an earlier run) since its "BOARD_..." condition no
     longer corresponds to any board file once the bare files are renamed
     away.
  3. Renames SoC-scoped overlays under tests/**/socs/rp2350[ab]_(hazard3|m33).overlay
     to their "_0" counterpart (git mv), when no "_0" file exists yet.
  4. Renames per-board overlay/conf files (including "_w"/"_mcuboot"
     siblings) under tests/**/boards/, samples/**/boards/, and
     boards/shields/**/boards/ that are named
     "<board>_<soc>_(hazard3|m33)[_w|_mcuboot|_w_mcuboot].{overlay,conf}"
     to their "_0" counterpart - copied instead of renamed for boards
     that keep their bare board files around, renamed (git mv)
     otherwise.
  5. Fixes platform_allow / platform_exclude / integration_platforms
     entries in tests.yaml / sample.yaml files that reference a bare
     "<board>/rp2350[ab]/(hazard3|m33)" qualifier, rewriting them to the
     "_0" form. Matching is scoped to exact board/soc/cluster triples
     already migrated (bare or "_0" board files present), never a blind
     "/m33" or "/hazard3" replace, since unrelated SoCs (imx943_evk,
     kit_pse84_eval, etc.) share the "/m33" substring.
  6. Fixes the same "<board>/rp2350[ab]/(hazard3|m33)" qualifier
     references in each migrated board's doc/index.rst, scoped the same
     way as step 5.
  7. Fixes the same qualifier references in tests/**/README.rst and
     samples/**/README.rst files, scoped the same way as step 5.
  8. Fixes "cpucluster: <old_cluster>" lines in a migrated board's own
     board.yml (e.g. rpi_pico2's "w"/"mcuboot" variants, which pin a
     specific cpucluster) to the "_0" form, so board target qualifiers
     derived from board.yml (list_boards.py) match the actual "_0"
     board files on disk.
  9. Fixes "#include "<old_file>"" directives anywhere under the repo that
     reference a .dts/.overlay file renamed by this run (steps 1, 3, 4),
     rewriting them to the "_0" filename. Devicetree files include each
     other by bare filename across .dts/.dtsi/.overlay, so a board that
     reuses another board's overlay/dts (e.g. pico_plus2 including
     rpi_pico2's overlay) breaks once the included file is renamed unless
     the include is fixed too. Scoped to the exact old/new filename pairs
     produced by this run's own renames - never a blind "hazard3"/"m33"
     substring replace - and skipped for files that were copied rather
     than renamed, since the old filename still exists on disk for those.
     ".conf" files are out of scope; they aren't pulled in via "#include".

Dry-run by default: prints the full plan and writes nothing. Pass
--apply to perform the renames/edits. Pass --boards to restrict the run
to a comma-separated list of board names (as declared in board.yml's
"name:" field).
"""

import argparse
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

import yaml

ZEPHYR_BASE = Path(__file__).resolve().parents[1]

CLUSTER_MAP = {"hazard3": "hazard3_0", "m33": "m33_0"}
SOCS = ("rp2350a", "rp2350b")

# Boards whose bare-cluster board files must be kept (copied, not
# renamed) because something may still depend on the bare qualifier
# during a multi-step migration. Every other board gets a straight
# rename since its "_0" migration hasn't started at all yet.
COPY_INSTEAD_OF_RENAME = set()

SUFFIX_VARIANTS = ("", "_w", "_mcuboot", "_w_mcuboot")

# Directories (relative to ZEPHYR_BASE) that hold per-board overlay/conf
# files named "<board>_<soc>_<cluster>.{overlay,conf}", as opposed to the
# soc-scoped overlays handled by find_overlay_renames.
PER_BOARD_OVERLAY_CONF_GLOBS = (
    "tests/**/boards",
    "samples/**/boards",
    "boards/shields/**/boards",
)


@dataclass
class BoardFileTriple:
    board_dir: Path
    board_name: str
    soc: str
    old_cluster: str
    new_cluster: str
    suffix: str  # "", "_w", "_mcuboot", "_w_mcuboot"

    def old_stem(self) -> str:
        return f"{self.board_name}_{self.soc}_{self.old_cluster}{self.suffix}"

    def new_stem(self) -> str:
        return f"{self.board_name}_{self.soc}_{self.new_cluster}{self.suffix}"

    def old_paths(self) -> dict:
        stem = self.old_stem()
        return {
            "dts": self.board_dir / f"{stem}.dts",
            "yaml": self.board_dir / f"{stem}.yaml",
            "defconfig": self.board_dir / f"{stem}_defconfig",
        }

    def new_paths(self) -> dict:
        stem = self.new_stem()
        return {
            "dts": self.board_dir / f"{stem}.dts",
            "yaml": self.board_dir / f"{stem}.yaml",
            "defconfig": self.board_dir / f"{stem}_defconfig",
        }


@dataclass
class Plan:
    board_file_ops: list = field(default_factory=list)  # (mode, triple)
    board_overlay_conf_ops: list = field(default_factory=list)  # (mode, old_path, new_path)
    kconfig_edits: list = field(default_factory=list)  # (path, op, anchor_line, new_line)
    overlay_renames: list = field(default_factory=list)  # (old, new)
    yaml_edits: list = field(default_factory=list)  # (path, lineno, old, new)
    doc_edits: list = field(default_factory=list)  # (path, lineno, old, new)
    readme_edits: list = field(default_factory=list)  # (path, lineno, old, new)
    board_yml_edits: list = field(default_factory=list)  # (path, lineno, old, new)
    include_edits: list = field(default_factory=list)  # (path, lineno, old, new)


def discover_boards(filter_names):
    """Yield (board_dir, board_name, socs_present) for every board.yml
    that declares an rp2350a or rp2350b SoC."""
    for board_yml in sorted((ZEPHYR_BASE / "boards").rglob("board.yml")):
        try:
            data = yaml.safe_load(board_yml.read_text())
        except yaml.YAMLError:
            continue
        board = data.get("board") if data else None
        if not board or "name" not in board:
            continue
        socs_present = {s["name"] for s in board.get("socs", []) if s.get("name") in SOCS}
        if not socs_present:
            continue
        board_name = board["name"]
        if filter_names and board_name not in filter_names:
            continue
        yield board_yml.parent, board_name, socs_present


def find_board_file_ops(board_dir, board_name, soc, old_cluster, new_cluster):
    ops = []
    for suffix in SUFFIX_VARIANTS:
        triple = BoardFileTriple(board_dir, board_name, soc, old_cluster, new_cluster, suffix)
        old_paths = triple.old_paths()
        if not old_paths["dts"].exists():
            continue
        new_paths = triple.new_paths()
        if any(p.exists() for p in new_paths.values()):
            # Target already exists (already migrated) - nothing to do.
            continue
        mode = "copy" if board_name in COPY_INSTEAD_OF_RENAME else "rename"
        ops.append((mode, triple))
    return ops


def find_per_board_overlay_conf_ops(board_name, soc, old_cluster, new_cluster):
    """Find per-board overlay/conf files (as opposed to the soc-scoped
    ones under a "socs/" dir) named "<board>_<soc>_<cluster>[_w|_mcuboot|_w_mcuboot].{overlay,conf}"
    under tests/**/boards, samples/**/boards, or boards/shields/**/boards,
    and plan their rename (or copy, for boards that keep bare files) to
    the "_0" form."""
    ops = []
    mode = "copy" if board_name in COPY_INSTEAD_OF_RENAME else "rename"

    boards_dirs = set()
    for pattern in PER_BOARD_OVERLAY_CONF_GLOBS:
        boards_dirs.update(ZEPHYR_BASE.glob(pattern))

    for boards_dir in sorted(boards_dirs):
        for suffix in SUFFIX_VARIANTS:
            old_stem = f"{board_name}_{soc}_{old_cluster}{suffix}"
            new_stem = f"{board_name}_{soc}_{new_cluster}{suffix}"
            for ext in ("overlay", "conf"):
                old_path = boards_dir / f"{old_stem}.{ext}"
                if not old_path.exists():
                    continue
                new_path = boards_dir / f"{new_stem}.{ext}"
                if new_path.exists():
                    continue
                ops.append((mode, old_path, new_path))
    return ops


def find_kconfig_edit(board_dir, board_name, soc, old_cluster, new_cluster):
    kconfig_path = board_dir / f"Kconfig.{board_name}"
    if not kconfig_path.exists():
        return None

    text = kconfig_path.read_text()
    board_prefix = f"BOARD_{board_name.upper()}"
    soc_upper = soc.upper()
    old_cluster_upper = old_cluster.upper()
    new_cluster_upper = new_cluster.upper()

    old_soc_symbol = f"SOC_{soc_upper}_{old_cluster_upper}"
    new_soc_symbol = f"SOC_{soc_upper}_{new_cluster_upper}"

    # Find the existing "select SOC_..._<cluster> if ..." line for this
    # soc/cluster combination, to know which BOARD_... conditions to
    # mirror onto the new "_0" select line.
    old_select_re = re.compile(rf"^(\tselect {re.escape(old_soc_symbol)} if )(.+)$", re.MULTILINE)
    match = old_select_re.search(text)
    if not match:
        return None

    old_conditions = match.group(2)
    # Each condition is a BOARD_<NAME>_<SOC>_<CLUSTER>[_SUFFIX] symbol;
    # rewrite <CLUSTER> to <CLUSTER>_0 in each one, preserving suffixes
    # like _MCUBOOT / _W / _W_MCUBOOT.
    cluster_token_re = re.compile(
        rf"\b{re.escape(board_prefix)}_{re.escape(soc_upper)}_{re.escape(old_cluster_upper)}\b"
    )
    new_conditions = cluster_token_re.sub(
        f"{board_prefix}_{soc_upper}_{new_cluster_upper}", old_conditions
    )

    new_line = f"\tselect {new_soc_symbol} if {new_conditions}"
    anchor_line = match.group(0)

    if board_name in COPY_INSTEAD_OF_RENAME:
        # Bare board files stick around here, so the bare select line
        # must keep working - just add the "_0" line alongside it.
        if new_line in text:
            return None
        return kconfig_path, "insert_after", anchor_line, new_line

    # Every other board did a straight rename: the bare select line's
    # BOARD_... condition no longer corresponds to any board file, so
    # retire it instead of leaving it dangling next to the "_0" line.
    if new_line in text:
        # The "_0" line was already added by an earlier run; just drop
        # the now-dead bare line.
        return kconfig_path, "delete_only", anchor_line, new_line
    return kconfig_path, "replace", anchor_line, new_line


def find_board_doc_edits(board_dir, board_name, soc, old_cluster, new_cluster):
    """Rewrite "<board>/<soc>/<cluster>" qualifier references (":board:"
    directives, inline code spans, etc.) in a migrated board's
    doc/index.rst to the "_0" form."""
    doc_path = board_dir / "doc" / "index.rst"
    if not doc_path.exists():
        return []

    old_qual = f"{board_name}/{soc}/{old_cluster}"
    new_qual = f"{board_name}/{soc}/{new_cluster}"
    pattern = re.compile(rf"\b{re.escape(old_qual)}\b")

    edits = []
    lines = doc_path.read_text().splitlines()
    for i, line in enumerate(lines):
        if pattern.search(line):
            edits.append((doc_path, i, line, pattern.sub(new_qual, line)))
    return edits


def find_board_yml_cpucluster_edits(board_dir, old_cluster, new_cluster):
    """Rewrite "cpucluster: <old_cluster>" lines in a board's board.yml
    (used by variants like rpi_pico2's "w"/"mcuboot" that pin a specific
    cpucluster) to the "_0" form. Scoped to this board's own board.yml,
    so no cross-board substring risk."""
    yml_path = board_dir / "board.yml"
    if not yml_path.exists():
        return []

    pattern = re.compile(rf"^(\s*cpucluster:\s*){re.escape(old_cluster)}\s*$")

    edits = []
    lines = yml_path.read_text().splitlines()
    for i, line in enumerate(lines):
        m = pattern.match(line)
        if m:
            edits.append((yml_path, i, line, f"{m.group(1)}{new_cluster}"))
    return edits


def soc_clusters_fully_migrated(migrated_now):
    """Return the set of (soc, old_cluster) pairs for which every board
    in the *entire* repo using that soc/cluster combination already has
    (or will have, after this run) its "_0" board files. A soc-scoped
    overlay under tests/**/socs/<soc>_<cluster>.overlay is shared by
    every board with that soc+cluster - renaming it is only safe once
    none of them are still relying on the bare cluster (boards that
    keep their bare files around, like rpi_pico2, don't count as
    "still relying on it" once their own "_0" files exist)."""
    fully_migrated = set()
    for soc in SOCS:
        for old_cluster in CLUSTER_MAP:
            new_cluster = CLUSTER_MAP[old_cluster]
            all_boards_have_zero_variant = True
            for board_dir, board_name, socs_present in discover_boards(None):
                if soc not in socs_present:
                    continue
                triple = BoardFileTriple(board_dir, board_name, soc, old_cluster, new_cluster, "")
                if not triple.old_paths()["dts"].exists():
                    # No bare file for this board/soc/cluster at all
                    # (e.g. board only has the other cluster) - doesn't
                    # depend on the shared overlay either way.
                    continue
                has_zero_already = triple.new_paths()["dts"].exists()
                being_migrated_now = (board_name, soc, old_cluster) in migrated_now
                if not has_zero_already and not being_migrated_now:
                    all_boards_have_zero_variant = False
                    break
            if all_boards_have_zero_variant:
                fully_migrated.add((soc, old_cluster))
    return fully_migrated


def find_overlay_renames(fully_migrated_soc_clusters):
    renames = []
    socs_pattern = "|".join(SOCS)
    clusters_pattern = "|".join(CLUSTER_MAP.keys())
    name_re = re.compile(rf"^({socs_pattern})_({clusters_pattern})\.overlay$")
    for socs_dir in sorted(ZEPHYR_BASE.glob("tests/**/socs")) + sorted(
        ZEPHYR_BASE.glob("samples/**/socs")
    ):
        for f in sorted(socs_dir.iterdir()):
            m = name_re.match(f.name)
            if not m:
                continue
            soc, old_cluster = m.group(1), m.group(2)
            if (soc, old_cluster) not in fully_migrated_soc_clusters:
                # Other boards not in scope for this run still have bare
                # board files depending on this soc-scoped overlay -
                # renaming it now would break them.
                continue
            new_cluster = CLUSTER_MAP[old_cluster]
            new_path = socs_dir / f"{soc}_{new_cluster}.overlay"
            if new_path.exists():
                continue
            renames.append((f, new_path))
    return renames


def build_qualifier_patterns(board_qualifiers):
    """board_qualifiers: iterable of (board_name, soc, old_cluster, new_cluster)
    for boards already migrated (bare or "_0" board files present), used
    to scope substitutions so we never touch an unrelated board/soc
    sharing the "/m33" or "/hazard3" substring."""
    patterns = []
    for board_name, soc, old_cluster, new_cluster in board_qualifiers:
        old_qual = f"{board_name}/{soc}/{old_cluster}"
        new_qual = f"{board_name}/{soc}/{new_cluster}"
        patterns.append((re.compile(rf"\b{re.escape(old_qual)}\b"), old_qual, new_qual))
    return patterns


def find_qualifier_edits_in_files(patterns, files):
    edits = []
    if not patterns:
        return edits
    for file in files:
        lines = file.read_text().splitlines()
        for i, line in enumerate(lines):
            for pattern, _old_qual, new_qual in patterns:
                if pattern.search(line):
                    new_line = pattern.sub(new_qual, line)
                    edits.append((file, i, line, new_line))
    return edits


def find_yaml_qualifier_edits(patterns):
    return find_qualifier_edits_in_files(
        patterns,
        sorted(ZEPHYR_BASE.glob("tests/**/tests.yaml"))
        + sorted(ZEPHYR_BASE.glob("tests/**/sample.yaml"))
        + sorted(ZEPHYR_BASE.glob("samples/**/tests.yaml"))
        + sorted(ZEPHYR_BASE.glob("samples/**/sample.yaml")),
    )


def find_readme_edits(patterns):
    return find_qualifier_edits_in_files(
        patterns,
        sorted(ZEPHYR_BASE.glob("tests/**/README.rst"))
        + sorted(ZEPHYR_BASE.glob("samples/**/README.rst")),
    )


def find_include_edits(renamed_names):
    """renamed_names: iterable of (old_filename, new_filename) basename
    pairs for .dts/.overlay files actually renamed (git mv) by this run.
    Finds "#include "<old_filename>"" (optionally with a directory
    prefix) anywhere under the repo's .dts/.dtsi/.overlay files and
    rewrites it to the new filename, so files that reuse another board's
    devicetree source by bare #include don't end up pointing at a
    filename that no longer exists."""
    edits = []
    renamed_names = list(renamed_names)
    if not renamed_names:
        return edits

    patterns = [
        (
            re.compile(rf'(#include\s+"(?:[^"]*/)?){re.escape(old_name)}"'),
            new_name,
        )
        for old_name, new_name in renamed_names
    ]

    files = []
    for top_dir in ("boards", "tests", "samples", "dts", "soc"):
        for ext in ("dts", "dtsi", "overlay"):
            files.extend(sorted((ZEPHYR_BASE / top_dir).glob(f"**/*.{ext}")))
    for file in files:
        lines = file.read_text().splitlines()
        for i, line in enumerate(lines):
            for pattern, new_name in patterns:
                if pattern.search(line):
                    new_line = pattern.sub(rf'\g<1>{new_name}"', line)
                    edits.append((file, i, line, new_line))
    return edits


def build_plan(filter_names):
    plan = Plan()
    migrated_now = set()  # (board_name, soc, old_cluster) renamed by *this* run
    board_migrations = []  # (board_dir, board_name, soc, old_cluster, new_cluster)
    board_yml_seen = set()  # (board_dir, old_cluster) already scanned for cpucluster: edits

    for board_dir, board_name, socs_present in discover_boards(filter_names):
        for soc in sorted(socs_present):
            for old_cluster, new_cluster in CLUSTER_MAP.items():
                ops = find_board_file_ops(board_dir, board_name, soc, old_cluster, new_cluster)
                if ops:
                    plan.board_file_ops.extend(ops)
                    migrated_now.add((board_name, soc, old_cluster))

                triple = BoardFileTriple(board_dir, board_name, soc, old_cluster, new_cluster, "")
                already_migrated = triple.new_paths()["dts"].exists()
                if not (already_migrated or ops):
                    # This board doesn't use this soc/cluster combination
                    # at all, or already fully retired it - nothing to
                    # clean up.
                    continue
                board_migrations.append((board_dir, board_name, soc, old_cluster, new_cluster))

                kconfig_edit = find_kconfig_edit(
                    board_dir, board_name, soc, old_cluster, new_cluster
                )
                if kconfig_edit:
                    plan.kconfig_edits.append(kconfig_edit)

                plan.board_overlay_conf_ops.extend(
                    find_per_board_overlay_conf_ops(board_name, soc, old_cluster, new_cluster)
                )
                plan.doc_edits.extend(
                    find_board_doc_edits(board_dir, board_name, soc, old_cluster, new_cluster)
                )

                # cpucluster: lines in board.yml aren't soc-scoped, so
                # only scan each (board, old_cluster) pair once even if
                # the board declares multiple socs.
                if (board_dir, old_cluster) not in board_yml_seen:
                    board_yml_seen.add((board_dir, old_cluster))
                    plan.board_yml_edits.extend(
                        find_board_yml_cpucluster_edits(board_dir, old_cluster, new_cluster)
                    )

    fully_migrated_soc_clusters = soc_clusters_fully_migrated(migrated_now)
    plan.overlay_renames = find_overlay_renames(fully_migrated_soc_clusters)
    qualifier_patterns = build_qualifier_patterns(
        (board_name, soc, old_cluster, new_cluster)
        for _board_dir, board_name, soc, old_cluster, new_cluster in board_migrations
    )
    plan.yaml_edits = find_yaml_qualifier_edits(qualifier_patterns)
    plan.readme_edits = find_readme_edits(qualifier_patterns)

    renamed_names = (
        [
            (t.old_paths()["dts"].name, t.new_paths()["dts"].name)
            for mode, t in plan.board_file_ops
            if mode == "rename"
        ]
        + [
            (old.name, new.name)
            for mode, old, new in plan.board_overlay_conf_ops
            if mode == "rename"
        ]
        + [(old.name, new.name) for old, new in plan.overlay_renames]
    )
    plan.include_edits = find_include_edits(renamed_names)
    return plan


def rewrite_identifier(text: str, old_qualifier: str, new_qualifier: str) -> str:
    return re.sub(
        rf"^identifier: {re.escape(old_qualifier)}$",
        f"identifier: {new_qualifier}",
        text,
        flags=re.MULTILINE,
    )


def git_mv(src: Path, dst: Path):
    subprocess.run(["git", "mv", str(src), str(dst)], cwd=ZEPHYR_BASE, check=True)


def git_add(*paths: Path):
    subprocess.run(["git", "add", *(str(p) for p in paths)], cwd=ZEPHYR_BASE, check=True)


def apply_board_file_op(mode, triple: BoardFileTriple):
    old_paths = triple.old_paths()
    new_paths = triple.new_paths()

    old_identifier_qual = f"{triple.board_name}/{triple.soc}/{triple.old_cluster}" + (
        "/" + "/".join(part for part in triple.suffix.strip("_").split("_") if part)
        if triple.suffix
        else ""
    )
    new_identifier_qual = f"{triple.board_name}/{triple.soc}/{triple.new_cluster}" + (
        "/" + "/".join(part for part in triple.suffix.strip("_").split("_") if part)
        if triple.suffix
        else ""
    )

    if mode == "rename":
        git_mv(old_paths["dts"], new_paths["dts"])
        git_mv(old_paths["defconfig"], new_paths["defconfig"])
        git_mv(old_paths["yaml"], new_paths["yaml"])
        yaml_text = new_paths["yaml"].read_text()
        new_paths["yaml"].write_text(
            rewrite_identifier(yaml_text, old_identifier_qual, new_identifier_qual)
        )
    elif mode == "copy":
        new_paths["dts"].write_bytes(old_paths["dts"].read_bytes())
        new_paths["defconfig"].write_bytes(old_paths["defconfig"].read_bytes())
        yaml_text = old_paths["yaml"].read_text()
        new_paths["yaml"].write_text(
            rewrite_identifier(yaml_text, old_identifier_qual, new_identifier_qual)
        )
        git_add(new_paths["dts"], new_paths["defconfig"], new_paths["yaml"])
    else:
        raise ValueError(f"unknown mode {mode!r}")


def apply_line_edits(edits):
    """edits: iterable of (path, lineno, old_line, new_line). Groups by
    file so each file is rewritten once even with multiple edits."""
    edits_by_file = {}
    for path, lineno, _old_line, new_line in edits:
        edits_by_file.setdefault(path, {})[lineno] = new_line
    for path, line_edits in edits_by_file.items():
        lines = path.read_text().splitlines(keepends=False)
        for lineno, new_line in line_edits.items():
            lines[lineno] = new_line
        path.write_text("\n".join(lines) + "\n")


def apply_plan(plan: Plan):
    for mode, triple in plan.board_file_ops:
        apply_board_file_op(mode, triple)

    for mode, old_path, new_path in plan.board_overlay_conf_ops:
        if mode == "rename":
            git_mv(old_path, new_path)
        elif mode == "copy":
            new_path.write_bytes(old_path.read_bytes())
            git_add(new_path)
        else:
            raise ValueError(f"unknown mode {mode!r}")

    for kconfig_path, op, anchor_line, new_line in plan.kconfig_edits:
        text = kconfig_path.read_text()
        if op == "insert_after":
            pos = text.index(anchor_line)
            insert_pos = pos + len(anchor_line)
            text = text[:insert_pos] + "\n" + new_line + text[insert_pos:]
        elif op == "replace":
            text = text.replace(anchor_line, new_line, 1)
        elif op == "delete_only":
            text = text.replace(anchor_line + "\n", "", 1)
        else:
            raise ValueError(f"unknown kconfig edit op {op!r}")
        kconfig_path.write_text(text)

    for old_path, new_path in plan.overlay_renames:
        git_mv(old_path, new_path)

    apply_line_edits(plan.yaml_edits)
    apply_line_edits(plan.doc_edits)
    apply_line_edits(plan.readme_edits)
    apply_line_edits(plan.board_yml_edits)
    apply_line_edits(plan.include_edits)


def print_plan(plan: Plan, board_names):
    scope = ", ".join(sorted(board_names)) if board_names else "all discovered boards"
    print(f"Scope: {scope}\n")

    print(f"== Board file operations ({len(plan.board_file_ops)}) ==")
    for mode, triple in plan.board_file_ops:
        old_stem = triple.old_stem()
        new_stem = triple.new_stem()
        verb = "COPY  " if mode == "copy" else "RENAME"
        board_dir = triple.board_dir.relative_to(ZEPHYR_BASE)
        print(
            f"  {verb} {board_dir}/{old_stem}.{{dts,yaml,_defconfig}}"
            f" -> {new_stem}.{{dts,yaml,_defconfig}}"
        )

    print(f"\n== Board overlay/conf operations ({len(plan.board_overlay_conf_ops)}) ==")
    for mode, old_path, new_path in plan.board_overlay_conf_ops:
        verb = "COPY  " if mode == "copy" else "RENAME"
        print(f"  {verb} {old_path.relative_to(ZEPHYR_BASE)} -> {new_path.name}")

    print(f"\n== Kconfig edits ({len(plan.kconfig_edits)}) ==")
    kconfig_verbs = {"insert_after": "ADD", "replace": "REPLACE", "delete_only": "DELETE"}
    for kconfig_path, op, anchor_line, new_line in plan.kconfig_edits:
        label = anchor_line.strip() if op == "delete_only" else new_line.strip()
        print(f"  {kconfig_path.relative_to(ZEPHYR_BASE)}: {kconfig_verbs[op]} line {label!r}")

    print(f"\n== SoC-scoped overlay renames ({len(plan.overlay_renames)}) ==")
    for old_path, new_path in plan.overlay_renames:
        print(f"  RENAME {old_path.relative_to(ZEPHYR_BASE)} -> {new_path.name}")

    print(f"\n== tests.yaml/sample.yaml qualifier edits ({len(plan.yaml_edits)}) ==")
    for path, lineno, old_line, new_line in plan.yaml_edits:
        rel = path.relative_to(ZEPHYR_BASE)
        print(f"  {rel}:{lineno + 1}: {old_line.strip()!r} -> {new_line.strip()!r}")

    print(f"\n== Board doc edits ({len(plan.doc_edits)}) ==")
    for path, lineno, old_line, new_line in plan.doc_edits:
        rel = path.relative_to(ZEPHYR_BASE)
        print(f"  {rel}:{lineno + 1}: {old_line.strip()!r} -> {new_line.strip()!r}")

    print(f"\n== tests/samples README.rst qualifier edits ({len(plan.readme_edits)}) ==")
    for path, lineno, old_line, new_line in plan.readme_edits:
        rel = path.relative_to(ZEPHYR_BASE)
        print(f"  {rel}:{lineno + 1}: {old_line.strip()!r} -> {new_line.strip()!r}")

    print(f"\n== board.yml cpucluster edits ({len(plan.board_yml_edits)}) ==")
    for path, lineno, old_line, new_line in plan.board_yml_edits:
        rel = path.relative_to(ZEPHYR_BASE)
        print(f"  {rel}:{lineno + 1}: {old_line.strip()!r} -> {new_line.strip()!r}")

    print(f"\n== #include fixes ({len(plan.include_edits)}) ==")
    for path, lineno, old_line, new_line in plan.include_edits:
        rel = path.relative_to(ZEPHYR_BASE)
        print(f"  {rel}:{lineno + 1}: {old_line.strip()!r} -> {new_line.strip()!r}")

    total = (
        len(plan.board_file_ops)
        + len(plan.board_overlay_conf_ops)
        + len(plan.kconfig_edits)
        + len(plan.overlay_renames)
        + len(plan.yaml_edits)
        + len(plan.doc_edits)
        + len(plan.readme_edits)
        + len(plan.board_yml_edits)
        + len(plan.include_edits)
    )
    print(f"\nTotal planned changes: {total}")
    if total == 0:
        print("Nothing to do.")


def main():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument(
        "--apply",
        action="store_true",
        help="Perform the migration. Without this flag, only prints the plan.",
    )
    parser.add_argument(
        "--boards",
        type=str,
        default=None,
        help="Comma-separated list of board names (board.yml 'name:' field) to restrict"
        " the run to. Default: all discovered rp2350a/rp2350b boards.",
    )
    args = parser.parse_args()

    filter_names = set(n.strip() for n in args.boards.split(",")) if args.boards else None

    plan = build_plan(filter_names)
    print_plan(plan, filter_names)

    if args.apply:
        print("\nApplying...")
        apply_plan(plan)
        print("Done.")
    else:
        print("\nDry run only - no changes written. Pass --apply to perform these changes.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
