#!/usr/bin/env python3
#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

"""Common helpers for virtual-board generators.

This module is intentionally vendor-agnostic and can be shared by multiple
virtual board generators.
"""

from __future__ import annotations

import json
import re
import shutil
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import yaml


@dataclass(frozen=True)
class Target:
    soc: str
    cpucluster: str | None = None
    series: str | None = None

    @property
    def qualifier(self) -> str:
        return f"{self.soc}/{self.cpucluster}" if self.cpucluster else self.soc


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def load_yaml(path: Path) -> Any:
    return yaml.safe_load(path.read_text(encoding="utf-8"))


def ensure_empty_dir(path: Path) -> None:
    if path.exists():
        shutil.rmtree(path)
    path.mkdir(parents=True, exist_ok=True)


def normalize_soc_yml(data: Any) -> list[dict[str, Any]]:
    # soc.yml can be either:
    # - {'family': [ ... ]}
    # - [ ... ]
    if isinstance(data, dict) and "family" in data:
        return data["family"]
    if isinstance(data, list):
        return data
    raise ValueError("Unsupported soc.yml format")


def extract_targets_from_soc_yml(soc_yml_path: Path, include_all_cpuclusters: bool) -> list[Target]:
    data = normalize_soc_yml(load_yaml(soc_yml_path))

    out: list[Target] = []

    def add_soc(
        name: str,
        cpuclusters: list[dict[str, Any]] | None = None,
        *,
        series: str | None = None,
    ) -> None:
        if include_all_cpuclusters and cpuclusters:
            for c in cpuclusters:
                cname = c.get("name")
                if cname:
                    out.append(Target(soc=name, cpucluster=str(cname), series=series))
        else:
            out.append(Target(soc=name, series=series))

    for fam in data:
        if not isinstance(fam, dict):
            continue

        fam_series = fam.get("name")
        if fam_series is not None:
            fam_series = str(fam_series)

        for soc in fam.get("socs", []) or []:
            if isinstance(soc, dict) and soc.get("name"):
                add_soc(str(soc["name"]), soc.get("cpuclusters"), series=fam_series)

        for series in fam.get("series", []) or []:
            if not isinstance(series, dict):
                continue
            series_name = series.get("name")
            if series_name is not None:
                series_name = str(series_name)
            for soc in series.get("socs", []) or []:
                if isinstance(soc, dict) and soc.get("name"):
                    add_soc(
                        str(soc["name"]),
                        soc.get("cpuclusters"),
                        series=series_name or fam_series,
                    )

    # De-dupe (preserve order)
    seen: set[str] = set()
    dedup: list[Target] = []
    for t in out:
        if t.qualifier in seen:
            continue
        seen.add(t.qualifier)
        dedup.append(t)

    return dedup


def collect_kconfig_soc_symbols(kconfig_soc_roots: list[Path]) -> set[str]:
    """Collect all `config SOC_*` symbols defined in given Kconfig.soc trees."""

    sym_re = re.compile(r"^\s*config\s+(SOC_[A-Za-z0-9_]+)\b")
    out: set[str] = set()

    for root in kconfig_soc_roots:
        if not root.exists():
            continue
        for p in root.rglob("Kconfig.soc"):
            try:
                for line in p.read_text(encoding="utf-8", errors="ignore").splitlines():
                    m = sym_re.match(line)
                    if m:
                        out.add(m.group(1))
            except OSError:
                continue

    return out


def extract_cpuclusters_from_kconfig_symbols(soc: str, symbols: set[str]) -> set[str]:
    socu = soc.upper()
    prefix = f"SOC_{socu}_"
    clusters: set[str] = set()

    for s in symbols:
        if s.startswith(prefix):
            suffix = s[len(prefix) :].strip("_")
            if suffix:
                clusters.add(suffix.lower())

    return clusters


def render_board_yml(board_name: str, full_name: str, vendor: str, targets: list[Target]) -> str:
    by_soc: dict[str, list[str]] = {}
    for t in targets:
        by_soc.setdefault(t.soc, [])
        if t.cpucluster and t.cpucluster not in by_soc[t.soc]:
            by_soc[t.soc].append(t.cpucluster)

    lines: list[str] = []
    lines.append("board:")
    lines.append(f"  name: {board_name}")
    lines.append(f"  full_name: {full_name}")
    lines.append(f"  vendor: {vendor}")
    lines.append("  socs:")

    for soc in sorted(by_soc.keys()):
        clusters = by_soc[soc]
        lines.append(f"  - name: {soc}")
        if clusters:
            lines.append("    variants:")
            for c in clusters:
                lines.append(f"    - name: {c}")
                lines.append(f"      cpucluster: '{c}'")

    lines.append("")
    return "\n".join(lines)


def render_target_yaml(
    *,
    board_name: str,
    t: Target,
    arch: str,
    vendor: str,
    supported: list[str] | None = None,
) -> str:
    ident = f"{board_name}/{t.soc}" + (f"/{t.cpucluster}" if t.cpucluster else "")

    lines: list[str] = []
    lines.append(f"identifier: {ident}")
    lines.append(f"name: {vendor.upper()} Virtual ({ident})")
    lines.append("type: mcu")
    lines.append(f"arch: {arch}")
    lines.append("toolchain:")
    lines.append("  - zephyr")
    if supported:
        lines.append("supported:")
        for feat in supported:
            lines.append(f"  - {feat}")
    lines.append(f"vendor: {vendor}")
    lines.append("")
    return "\n".join(lines)


def patch_model_and_compatible(dts: str, *, model: str, compat_soc: str) -> str:
    dts, _ = re.subn(
        r'(^\s*model\s*=\s*")([^"]+)("\s*;\s*$)',
        rf'\1{model}\3',
        dts,
        count=1,
        flags=re.MULTILINE,
    )

    def repl_compat(m: re.Match[str]) -> str:
        line = m.group(0)
        return re.sub(r'"[^"]+"', f'"{compat_soc}"', line, count=1)

    dts, _ = re.subn(
        r"^\s*compatible\s*=\s*.*;\s*$", repl_compat, dts, count=1, flags=re.MULTILINE
    )
    return dts


def extract_quoted_includes(dts_source: str) -> list[str]:
    inc_q_re = re.compile(r'^\s*#include\s+"([^"]+)"\s*$')
    incs: list[str] = []
    for line in dts_source.splitlines():
        m = inc_q_re.match(line)
        if m:
            incs.append(m.group(1))

    # De-dupe (preserve order)
    seen: set[str] = set()
    out: list[str] = []
    for x in incs:
        if x in seen:
            continue
        seen.add(x)
        out.append(x)

    return out


def copy_quoted_includes_from_file(*, src_dts_path: Path, dst_dir: Path) -> None:
    """Copy DTS quoted-include files ("foo.dtsi") next to generated DTS files."""

    src = src_dts_path.read_text(encoding="utf-8", errors="ignore")
    for rel in extract_quoted_includes(src):
        rel_path = Path(rel)
        if rel_path.is_absolute() or ".." in rel_path.parts:
            print(f"WARNING: skipping unsafe quoted include path in DTS: {rel}")
            continue

        src_path = (src_dts_path.parent / rel_path).resolve()
        if not src_path.exists():
            print(f"WARNING: DTS quoted include not found: {src_path}")
            continue

        dst_path = dst_dir / rel_path
        dst_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src_path, dst_path)


def apply_prefix_filters(targets: list[Target], prefixes: list[str] | None) -> list[Target]:
    if not prefixes:
        return targets
    prefixes = [str(p) for p in prefixes]
    return [t for t in targets if any(t.soc.startswith(p) for p in prefixes)]
