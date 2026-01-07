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
    # Optional board-variant identifier (extra qualifier segment).
    # Used by some generators to represent things like SoC part-number variants.
    variant: str | None = None

    @property
    def qualifier(self) -> str:
        parts: list[str] = [self.soc]
        if self.cpucluster:
            parts.append(self.cpucluster)
        if self.variant:
            parts.append(self.variant)
        return "/".join(parts)


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


def collect_kconfig_soc_part_numbers(
    kconfig_soc_files: list[Path],
) -> dict[str, list[str]]:
    """Collect SOC part-number Kconfig symbols, grouped by SoC name.

    Returns:
      { "mimxrt1189": ["SOC_PART_NUMBER_MIMXRT1189CVM8C", ...], ... }

    Heuristics:
    - If a part-number config block contains a `select SOC_<...>` line (excluding
      SOC_SERIES/SOC_FAMILY), map it to that SoC.
    - Otherwise, if the file has exactly one `config SOC` default value, map the
      part-number symbol(s) to that SoC.

    This is intentionally lightweight text scanning; it does not require Kconfiglib.
    """

    cfg_re = re.compile(r"^\s*config\s+(SOC_PART_NUMBER_[A-Za-z0-9_]+)\b")
    select_re = re.compile(r"^\s*select\s+(SOC_[A-Za-z0-9_]+)\b")
    config_start_re = re.compile(r"^\s*config\s+([A-Za-z0-9_]+)\b")
    soc_default_re = re.compile(r"^\s*default\s+\"([^\"]+)\"\s+if\s+SOC_[A-Za-z0-9_]+\b")

    def soc_name_from_soc_symbol(sym: str) -> str | None:
        s = (sym or "").strip()
        if not s.startswith("SOC_"):
            return None
        if s.startswith(("SOC_SERIES_", "SOC_FAMILY_")):
            return None
        tail = s[len("SOC_") :]
        if not tail:
            return None
        soc_token = tail.split("_", 1)[0]
        return soc_token.lower() if soc_token else None

    out: dict[str, list[str]] = {}

    for p in kconfig_soc_files:
        if not p.exists():
            continue

        try:
            lines = p.read_text(encoding="utf-8", errors="ignore").splitlines()
        except OSError:
            continue

        soc_defaults: set[str] = set()

        current_cfg: str | None = None
        current_part: str | None = None
        part_select_soc: str | None = None
        part_records: list[tuple[str, str | None]] = []

        def flush_part(
            _part_records: list[tuple[str, str | None]] = part_records,
        ) -> None:
            nonlocal current_part, part_select_soc
            if current_part:
                _part_records.append((current_part, part_select_soc))
            current_part = None
            part_select_soc = None

        for line in lines:
            mstart = config_start_re.match(line)
            if mstart:
                flush_part()
                current_cfg = mstart.group(1)
                current_part = None
                part_select_soc = None

            if current_cfg == "SOC":
                md = soc_default_re.match(line)
                if md:
                    soc_defaults.add(md.group(1).strip().lower())

            mpart = cfg_re.match(line)
            if mpart:
                flush_part()
                current_part = mpart.group(1)
                part_select_soc = None
                continue

            if current_part:
                ms = select_re.match(line)
                if ms:
                    sel = ms.group(1)
                    if not sel.startswith(("SOC_SERIES_", "SOC_FAMILY_")):
                        part_select_soc = sel

        flush_part()

        fallback_soc: str | None = None
        if len(soc_defaults) == 1:
            fallback_soc = next(iter(soc_defaults))

        for part_sym, sel_soc_sym in part_records:
            soc_name: str | None = None
            if sel_soc_sym:
                soc_name = soc_name_from_soc_symbol(sel_soc_sym)
            if not soc_name:
                soc_name = fallback_soc
            if not soc_name:
                continue
            out.setdefault(soc_name, []).append(part_sym)

    # De-dupe each list but preserve order.
    for soc, parts in list(out.items()):
        seen: set[str] = set()
        dedup: list[str] = []
        for x in parts:
            if x in seen:
                continue
            seen.add(x)
            dedup.append(x)
        out[soc] = dedup

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
    """Render a HWMv2 board.yml.

    Note: SoC *cpuclusters* are defined in soc.yml (SoC metadata), not in
    board.yml. The `variants:` property in board.yml is for *board variants*
    that exist *under* a given SoC/cpucluster qualifier.

    The NXP virtual-board generator emits per-SoC and per-SoC-cpucluster targets
    only; it does not create any additional board variants. Therefore we must
    not emit `variants:` entries that mirror cpuclusters, otherwise west can end
    up with bogus duplicated qualifiers like `.../cpu0/cpu0`.
    """

    # Collect per-SoC variant definitions (board variants, *not* cpuclusters).
    #
    # We only emit `variants:` when a Target carries an explicit `variant` field.
    # This keeps the historical behavior (no variants emitted) for generators that
    # only produce soc[/cpucluster] qualifiers.
    socs = sorted({t.soc for t in targets})

    variants_by_soc: dict[str, list[tuple[str, str | None]]] = {}
    for t in targets:
        if not t.variant:
            continue
        variants_by_soc.setdefault(t.soc, []).append((str(t.variant), t.cpucluster))

    # De-dupe variants per SoC (preserve order).
    for soc, items in list(variants_by_soc.items()):
        seen: set[tuple[str, str | None]] = set()
        dedup: list[tuple[str, str | None]] = []
        for vname, c in items:
            key = (vname, c)
            if key in seen:
                continue
            seen.add(key)
            dedup.append((vname, c))
        variants_by_soc[soc] = dedup

    lines: list[str] = []
    lines.append("board:")
    lines.append(f"  name: {board_name}")
    lines.append(f"  full_name: {full_name}")
    lines.append(f"  vendor: {vendor}")
    lines.append("  socs:")

    for soc in socs:
        lines.append(f"  - name: {soc}")
        vlist = variants_by_soc.get(soc) or []
        if vlist:
            lines.append("    variants:")
            for vname, c in vlist:
                # Always quote variant names: they are often not valid bare YAML scalars
                # (e.g. contain dots), and quoting keeps output stable.
                lines.append(f"    - name: '{vname}'")
                if c:
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
    ident = (
        f"{board_name}/{t.soc}"
        + (f"/{t.cpucluster}" if t.cpucluster else "")
        + (f"/{t.variant}" if t.variant else "")
    )

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

    dts, _ = re.subn(r"^\s*compatible\s*=\s*.*;\s*$", repl_compat, dts, count=1, flags=re.MULTILINE)
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
    """Copy DTS quoted-include files ("foo.dtsi") next to generated DTS files.

    This is recursive: if a copied quoted-include itself contains quoted-includes,
    those are copied too. This is needed for some board DTS templates where the
    top-level DTS includes a local .dtsi that further includes other local .dtsi
    files.
    """

    visited: set[Path] = set()
    queue: list[Path] = [src_dts_path]

    while queue:
        cur = queue.pop(0)
        cur = cur.resolve()
        if cur in visited:
            continue
        visited.add(cur)

        try:
            src = cur.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue

        for rel in extract_quoted_includes(src):
            rel_path = Path(rel)
            if rel_path.is_absolute() or ".." in rel_path.parts:
                print(f"WARNING: skipping unsafe quoted include path in DTS: {rel}")
                continue

            src_path = (cur.parent / rel_path).resolve()
            if not src_path.exists():
                print(f"WARNING: DTS quoted include not found: {src_path}")
                continue

            dst_path = dst_dir / rel_path
            dst_path.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src_path, dst_path)

            # Recurse into the include we just copied.
            queue.append(src_path)


def apply_prefix_filters(targets: list[Target], prefixes: list[str] | None) -> list[Target]:
    if not prefixes:
        return targets
    prefixes = [str(p) for p in prefixes]
    return [t for t in targets if any(t.soc.startswith(p) for p in prefixes)]
