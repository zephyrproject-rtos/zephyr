#!/usr/bin/env python3
#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

"""NXP virtual board generator (implementation).

The entry point wrapper is:
  boards/virtual_board/tools/gen_nxp_virtual_board.py

Most vendor-agnostic helpers live in:
  boards/virtual_board/tools/virtual_board_gen_common.py
"""

from __future__ import annotations

import argparse
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any

# This module is normally imported via boards/virtual_board/tools/gen_nxp_virtual_board.py,
# which sets up sys.path so this import resolves.
import virtual_board_gen_common as common

Target = common.Target


def _strip_dts_block(*, text: str, start_re: str, end_re: str) -> str:
    """Remove a DTS block defined by start/end regex markers.

    This is used to remove master-board overlays that refer to nodes which are
    deleted by a per-SoC DTSI (e.g. /delete-node/ &usb;).
    """

    sr = re.compile(start_re, flags=re.MULTILINE)
    er = re.compile(end_re, flags=re.MULTILINE)

    out = text
    while True:
        ms = sr.search(out)
        if not ms:
            break
        me = er.search(out, ms.end())
        if not me:
            # If end marker not found, stop to avoid deleting everything.
            break
        out = out[: ms.start()] + out[me.end() :]
    return out


def _is_likely_dts_node_ref(s: str) -> bool:
    """Heuristic: does this string look like a DTS node reference?

    We treat strings like '&lpuart_5' (phandle/label references) and node names
    containing '@' (e.g. 'cpu@1') as DTS node references. Everything else is
    treated as a generic feature name (for filtering YAML `supported:`).
    """

    s = str(s).strip()
    if not s:
        return False
    return s.startswith("&") or ("@" in s)


def patch_generated_dts(
    *,
    t: Target,
    dts_text: str,
    remove: list[str] | None = None,
) -> str:
    """Post-process generated DTS text for build-only robustness.

    Node-removal is driven by the generator JSON config (see the
    `overrides.patch_generated_dts` mapping). Entries that do not look like DTS
    node references are ignored here (but can be used to filter YAML
    `supported:` features).
    """

    out = dts_text

    # Config-driven removal of board overlay blocks.
    for item in remove or []:
        if not _is_likely_dts_node_ref(str(item)):
            continue
        esc = re.escape(str(item).strip())

        # Remove any overlay block whose opening line references this node.
        # Examples we need to handle:
        #   &lpi2c1 { ... };
        #   zephyr_udc0: &usb { ... };
        out = _strip_dts_block(
            text=out,
            start_re=rf"^\s*.*{esc}.*\{{\s*$",
            end_re=r"^\s*\};\s*$",
        )

    # MCXE31x:
    # - Some SoCs delete the LPUART5 node, but the master DTS uses it as console.
    # - The master DTS enables EIM/EDAC; for hello_world build-only coverage we
    #   can drop those chosen entries to avoid binding/property requirements.
    if t.soc in {"mcxe315", "mcxe316", "mcxe317"}:
        # Remove chosen entries that reference LPUART5 and other optional hw.
        out = re.sub(
            r"^\s*\t\tzephyr,(console|shell-uart|edac)\s*=\s*&[^;]+;\s*$\n",
            "",
            out,
            flags=re.MULTILINE,
        )

        # If the master DTS chooses an SRAM label that doesn't exist on the
        # derived SoC DTSI, use DTCM which is present on these parts.
        out = out.replace("\t\tzephyr,sram = &sram;\n", "\t\tzephyr,sram = &dtcm;\n")

    return out


def build_dts_include_index(zephyr_root: Path) -> dict[str, list[Path]]:
    """Build an index of DTS include files under <zephyr_root>/dts.

    Generated DTS files use C-preprocessor includes like:

      #include <nxp/nxp_mcxc141.dtsi>

    while the actual file is often located under an arch subdir, e.g.:

      dts/arm/nxp/nxp_mcxc141.dtsi

    This function creates a mapping from the include string
    ("nxp/nxp_mcxc141.dtsi") to one or more candidate paths.
    """

    dts_root = zephyr_root / "dts"
    index: dict[str, list[Path]] = {}
    if not dts_root.exists():
        return index

    # Common architecture folder names under dts/.
    known_arch_roots = {
        "arm",
        "arm64",
        "riscv",
        "xtensa",
        "x86",
        "sparc",
        "mips",
        "posix",
    }

    def _add(key: str, path: Path) -> None:
        if not key:
            return
        index.setdefault(key, []).append(path)

    for p in dts_root.rglob("*.dts*"):
        # Keep the index focused: only DTS/DTSI files.
        if p.suffix not in {".dts", ".dtsi"}:
            continue

        try:
            rel = p.relative_to(dts_root)
        except ValueError:
            continue

        rel_posix = rel.as_posix()
        _add(rel_posix, p)

        # Also allow includes that omit the leading arch folder.
        if rel.parts and rel.parts[0] in known_arch_roots and len(rel.parts) >= 2:
            _add(Path(*rel.parts[1:]).as_posix(), p)

    return index


def dts_include_exists(index: dict[str, list[Path]], include: str) -> bool:
    return bool(index.get(str(include)))


def expected_per_soc_dtsi(t: Target) -> str | None:
    """Return the expected per-SoC DTSI include for families that have them."""

    if t.soc.startswith(("mcxa", "mcxc", "mcxe")):
        return f"nxp/nxp_{t.soc}.dtsi"
    return None


def validate_and_fix_dtsi_include(
    *,
    t: Target,
    dtsi_include: str,
    dts_index: dict[str, list[Path]],
) -> str:
    """Validate the generated SoC DTSI include.

    Requirements:
    - When a family has per-SoC DTSI files (e.g. MCXC), ensure the include
      matches the target SoC name (e.g. mcxc141 -> <nxp/nxp_mcxc141.dtsi>).
    - Ensure the included DTSI file exists in the Zephyr DTS tree.
    """

    if dtsi_include.startswith("template:"):
        return dtsi_include

    expected = expected_per_soc_dtsi(t)
    if expected and dtsi_include != expected:
        # Prefer the per-SoC DTSI if it exists; this prevents cases where we
        # reuse a master board DTS (e.g. frdm_mcxc242) and accidentally keep
        # the master's SoC DTSI include.
        if dts_include_exists(dts_index, expected):
            print(
                f"INFO: overriding DTSI include for {t.qualifier}: <{dtsi_include}> -> <{expected}>"
            )
            dtsi_include = expected
        else:
            print(
                f"WARN: expected per-SoC DTSI include for {t.qualifier} not found: "
                f"<{expected}> (keeping <{dtsi_include}>)"
            )

    if not dts_include_exists(dts_index, dtsi_include):
        raise SystemExit(
            f"ERROR: DTSI include for {t.qualifier} not found in dts/ tree: <{dtsi_include}>"
        )

    return dtsi_include


# SPDX headers for generated artifacts.
#
# Keep these as the very first lines in generated files so that Zephyr's
# LicenseAndCopyrightCheck can detect them reliably.
_SPDX_C_COPYRIGHT = "/* SPDX-FileCopyrightText: 2025 NXP */"
_SPDX_C_LICENSE = "/* SPDX-License-Identifier: Apache-2.0 */"


def _default_zephyr_root() -> Path:
    # This file lives at:
    #   <ZEPHYR_BASE>/boards/nxp/virtual_boards/nxp/gen_nxp_virtual_board_impl.py
    return Path(__file__).resolve().parents[4]


def guess_arch(t: Target) -> str:
    # DSP clusters
    if t.cpucluster in {"adsp", "hifi4", "hifi1", "f1"}:
        return "xtensa"

    # A-class clusters
    if t.cpucluster in {"a55", "a53", "a9", "a7"}:
        return "arm64"

    return "arm"


def default_dtsi(t: Target) -> str:
    # MCXC / MCXE / MCXA have per-soc dtsi
    if t.soc.startswith("mcxc"):
        return f"nxp/nxp_{t.soc}.dtsi"
    if t.soc.startswith("mcxe"):
        return f"nxp/nxp_{t.soc}.dtsi"
    if t.soc.startswith("mcxa"):
        return f"nxp/nxp_{t.soc}.dtsi"

    # MCXN are family-based
    if t.soc == "mcxn947":
        return "nxp/nxp_mcxn94x.dtsi"
    if t.soc == "mcxn547":
        return "nxp/nxp_mcxn54x.dtsi"
    if t.soc == "mcxn236":
        return "nxp/nxp_mcxn23x.dtsi"

    # RW
    if t.soc in {"rw610", "rw612"}:
        return "nxp/nxp_rw6xx.dtsi"

    # MCXW
    # The upstream DTS tree does not provide a dedicated nxp_mcxw235.dtsi.
    # For build-only coverage, reuse the MCXW23x common DTSI.
    if t.soc in {"mcxw235", "mcxw236"}:
        return "nxp/nxp_mcxw236_ns.dtsi"
    if t.soc in {"mcxw716c", "mcxw727c"}:
        return "nxp/nxp_mcxw72.dtsi"

    # i.MXRT is too varied; default is a fallback only
    if t.soc.startswith("mimxrt"):
        return "nxp/nxp_rt10xx.dtsi"

    # i.MX is too varied; rely on overrides
    return f"nxp/nxp_{t.soc}.dtsi"


def soc_kconfig_symbol(t: Target) -> str:
    # Many NXP SoCs encode cluster in symbol suffix.
    socu = t.soc.upper()
    if t.cpucluster:
        return f"SOC_{socu}_{t.cpucluster.upper()}"
    return f"SOC_{socu}"


def soc_kconfig_symbol_without_variant(t: Target) -> str:
    """Return SOC_* symbol without including any board-variant suffix."""
    socu = t.soc.upper()
    if t.cpucluster:
        return f"SOC_{socu}_{t.cpucluster.upper()}"
    return f"SOC_{socu}"


# Some NXP families require selecting a concrete part number to get the
# correct HAL CPU_ macro definitions.
_SOC_PART_NUMBER_BY_SOC: dict[str, str] = {
    # MCXA
    # (build-only: pick a representative part number per SoC)
    "mcxa153": "SOC_PART_NUMBER_MCXA153VFM",
    "mcxa156": "SOC_PART_NUMBER_MCXA156VPJ",
    "mcxa266": "SOC_PART_NUMBER_MCXA266VLQ",
    "mcxa344": "SOC_PART_NUMBER_MCXA344VFM",
    "mcxa346": "SOC_PART_NUMBER_MCXA346VLQ",
    "mcxa366": "SOC_PART_NUMBER_MCXA366VLQ",
    "mcxa577": "SOC_PART_NUMBER_MCXA577VLQ",
    # MCXC
    "mcxc141": "SOC_PART_NUMBER_MCXC141VLH",
    "mcxc142": "SOC_PART_NUMBER_MCXC142VFM",
    "mcxc242": "SOC_PART_NUMBER_MCXC242VLH",
    "mcxc444": "SOC_PART_NUMBER_MCXC444VLH",
    # MCXE
    # (build-only: pick a representative part number per SoC)
    "mcxe245": "SOC_PART_NUMBER_MCXE245VLF",
    "mcxe246": "SOC_PART_NUMBER_MCXE246VLH",
    "mcxe247": "SOC_PART_NUMBER_MCXE247VLL",
    "mcxe315": "SOC_PART_NUMBER_MCXE315MLF",
    "mcxe316": "SOC_PART_NUMBER_MCXE316MLF",
    "mcxe317": "SOC_PART_NUMBER_MCXE317MPA",
    "mcxe31b": "SOC_PART_NUMBER_MCXE31BMPB",
    # MCXN
    "mcxn947": "SOC_PART_NUMBER_MCXN947VDF",
    "mcxn547": "SOC_PART_NUMBER_MCXN547VDF",
    "mcxn236": "SOC_PART_NUMBER_MCXN236VDF",
    # RW
    "rw610": "SOC_PART_NUMBER_RW610ETA2I",
    "rw612": "SOC_PART_NUMBER_RW612ETA2I",
    # i.MX RT
    "mimxrt1024": "SOC_PART_NUMBER_MIMXRT1024DAG5A",
    "mimxrt1021": "SOC_PART_NUMBER_MIMXRT1021DAG5A",
    "mimxrt1011": "SOC_PART_NUMBER_MIMXRT1011DAE5A",
    "mimxrt1015": "SOC_PART_NUMBER_MIMXRT1015DAF5A",
    "mimxrt1042": "SOC_PART_NUMBER_MIMXRT1042XJM5B",
    "mimxrt1052": "SOC_PART_NUMBER_MIMXRT1052DVL6B",
    "mimxrt1062": "SOC_PART_NUMBER_MIMXRT1062DVL6A",
    "mimxrt1064": "SOC_PART_NUMBER_MIMXRT1064DVL6A",
    "mimxrt595s": "SOC_PART_NUMBER_MIMXRT595SFFOC",
    "mimxrt685s": "SOC_PART_NUMBER_MIMXRT685SFVKB",
    "mimxrt1189": "SOC_PART_NUMBER_MIMXRT1189CVM8C",
    "mimxrt1176": "SOC_PART_NUMBER_MIMXRT1176DVMAA",
    "mimxrt1166": "SOC_PART_NUMBER_MIMXRT1166DVM6A",
    "mimxrt798s": "SOC_PART_NUMBER_MIMXRT798SGFOB",
    # MCXW7xx
    "mcxw727c": "SOC_PART_NUMBER_MCXW727CMFTA",
    "mcxw716c": "SOC_PART_NUMBER_MCXW716CMFTA",
    # MCXW2xx
    "mcxw235": "SOC_PART_NUMBER_MCXW235BIHNAR",
    "mcxw236": "SOC_PART_NUMBER_MCXW236BIHNAR",
    # i.MX8 (ADSP)
    "mimx8qm6": "SOC_PART_NUMBER_MIMX8QM6AVUFF",
    "mimx8qx6": "SOC_PART_NUMBER_MIMX8QX6AVLFZ",
    "mimx8ud7": "SOC_PART_NUMBER_MIMX8UD7DVK08",
    # i.MX8M
    "mimx8ml8": "SOC_PART_NUMBER_MIMX8ML8DVNLZ",
    "mimx8mn6": "SOC_PART_NUMBER_MIMX8MN6DVTJZ",
    "mimx8mm6": "SOC_PART_NUMBER_MIMX8MM6DVTLZ",
    "mimx8mq6": "SOC_PART_NUMBER_MIMX8MQ6DVAJZ",
    # i.MX9
    "mimx9596": "SOC_PART_NUMBER_MIMX9596AVZXN",
    "mimx94398": "SOC_PART_NUMBER_MIMX94398AVKM",
    "mimx9352": "SOC_PART_NUMBER_MIMX9352DVVXM",
    "mimx9111": "SOC_PART_NUMBER_MIMX9111CVXXJ",
    "mimx9131": "SOC_PART_NUMBER_MIMX9131CVVXJ",
}


# RT595 (MIMXRT595S F1 / Xtensa) SoC needs board-level memory address/size
# Kconfig symbols for its linker script.
_RT595_F1_KCONFIG_DEFAULTS: list[tuple[str, str, str]] = [
    ("RT595_ADSP_STACK_SIZE", 'hex "Boot time stack size"', "0x1000"),
    ("RT595_ADSP_RESET_MEM_ADDR", "hex", "0x0"),
    ("RT595_ADSP_RESET_MEM_SIZE", "hex", "0x400  # 1 KiB"),
    ("RT595_ADSP_TEXT_MEM_ADDR", "hex", "0x400"),
    ("RT595_ADSP_TEXT_MEM_SIZE", "hex", "0x3FC00  # 255 KiB"),
    ("RT595_ADSP_DATA_MEM_ADDR", "hex", "0x840000"),
    ("RT595_ADSP_DATA_MEM_SIZE", "hex", "0x40000  # 256 KiB"),
]


def board_target_kconfig_symbol(board_name: str, t: Target) -> str:
    b = board_name.upper()
    sym = f"BOARD_{b}_{t.soc.upper()}" + (f"_{t.cpucluster.upper()}" if t.cpucluster else "")
    if t.variant:
        sym += f"_{t.variant.upper()}"
    return sym


def soc_part_number_kconfig_symbol(t: Target) -> str | None:
    # If a specific part-number variant is requested, select it directly.
    if t.variant:
        return f"SOC_PART_NUMBER_{t.variant.upper()}"
    return _SOC_PART_NUMBER_BY_SOC.get(t.soc)


def render_rt595_f1_kconfig_defaults() -> list[str]:
    lines: list[str] = []
    for sym, ktype, default in _RT595_F1_KCONFIG_DEFAULTS:
        lines.append(f"config {sym}")
        lines.append(f"\t{ktype}")
        lines.append(f"\tdefault {default}")
        lines.append("")
    return lines


def render_kconfig_board(board_name: str, targets: list[Target]) -> str:
    # Modeled after boards/qemu/xtensa/Kconfig.qemu_xtensa: a single file with conditional selects.
    b = board_name.upper()

    lines: list[str] = []
    lines.append("# Auto-generated by gen_nxp_virtual_board.py")
    lines.append("")
    lines.append("# SPDX-FileCopyrightText: 2025 NXP")
    lines.append("# SPDX-License-Identifier: Apache-2.0")
    lines.append("")
    lines.append(f"config BOARD_{b}")
    lines.append("\tbool")

    has_rt595_f1 = False

    # Define per-target BOARD_<board>_<soc>[_<cpucluster>] symbols.
    # Put SoC/part selection inside the symbol so it is applied when the
    # qualifier is enabled via the per-target defconfig.
    for t in targets:
        target_sym = board_target_kconfig_symbol(board_name, t)
        part_sym = soc_part_number_kconfig_symbol(t)

        lines.append("")

        lines.append(f"config {target_sym}")
        lines.append("\tbool")
        lines.append(f"\tdepends on BOARD_{b}")
        # SoC select should ignore any board-variant suffix.
        lines.append(f"\tselect {soc_kconfig_symbol_without_variant(t)}")
        # NOTE: Do not select PINCTRL from boards. The driver(s) are enabled via DT,
        # and selecting PINCTRL in board Kconfig is disallowed by CI in some cases.
        if part_sym:
            lines.append(f"\tselect {part_sym}")

        if t.soc == "mimxrt595s" and t.cpucluster == "f1":
            has_rt595_f1 = True

    if has_rt595_f1:
        lines.append("")
        # Any RT595 F1 (Xtensa) target selects SOC_MIMXRT595S_F1. The generated
        # memory address/size symbols are consumed by the SoC linker scripts, so
        # gate them on the SoC selection (not a non-existent aggregate board
        # symbol).
        lines.append("if SOC_MIMXRT595S_F1")
        lines.append("")
        lines.extend(render_rt595_f1_kconfig_defaults())
        lines.append("endif")

    lines.append("")
    return "\n".join(lines)


def extract_kconfig_value_from_text(txt: str, symbol: str) -> str | None:
    """Return the value assigned to a symbol in a defconfig-like text, if present."""

    # NOTE: Be conservative; ignore '# CONFIG_FOO is not set' forms.
    re_sym = re.compile(rf"^\s*{re.escape(symbol)}\s*=\s*(.+?)\s*$")
    for line in (txt or "").splitlines():
        m = re_sym.match(line)
        if m:
            return m.group(1).strip()
    return None


def extract_kconfig_value_from_defconfig(defconfig_path: Path, symbol: str) -> str | None:
    """Return the value assigned to a symbol in a defconfig, if present.

    Matches lines like:
      CONFIG_FOO=bar
      CONFIG_FOO="bar"

    Returns the raw value string (right-hand side) without surrounding whitespace.
    """

    try:
        txt = defconfig_path.read_text(encoding="utf-8", errors="ignore")
    except OSError:
        return None

    return extract_kconfig_value_from_text(txt, symbol)


def master_defconfig_path(master: PlatformInfo) -> Path:
    # Convention: <yaml-stem>_defconfig in the same directory.
    return master.yaml_path.parent / f"{master.yaml_path.stem}_defconfig"


def master_board_name_from_identifier(master_ident: str) -> str | None:
    """Extract the board name from a platform identifier.

    NXP platform identifiers are often of the form:
      <board>/<soc>[/<cpucluster>[/...]]

    Some boards use revision-suffixed identifiers (e.g. "mimxrt1170_evk@A/").
    When locating board-level artifacts (like "<board>_defconfig"), strip any
    "@<rev>" suffix.
    """

    parts = [p for p in str(master_ident).split("/") if p]
    if not parts:
        return None

    board = parts[0]
    # Normalize board revisions like "mimxrt1170_evk@A" -> "mimxrt1170_evk".
    if "@" in board:
        board = board.split("@", 1)[0]

    return board or None


def find_board_defconfig(zephyr_root: Path, board_name: str) -> Path:
    """Find <board>_defconfig anywhere under zephyr_root/boards.

    Search under:
      <ZEPHYR_BASE>/boards/**/<board>_defconfig

    If multiple candidates exist, prefer the conventional layout:
      boards/<vendor>/<board>/<board>_defconfig

    Raises FileNotFoundError if no defconfig is found.
    Raises ValueError if multiple candidates are found and cannot be disambiguated.
    """

    boards_root = zephyr_root / "boards"
    if not boards_root.exists():
        raise FileNotFoundError(f"Missing boards/ directory under: {zephyr_root}")

    hits = sorted(boards_root.rglob(f"{board_name}_defconfig"), key=str)
    if not hits:
        raise FileNotFoundError(f"No defconfig found for board '{board_name}' under: {boards_root}")

    # Prefer boards/<vendor>/<board>/<board>_defconfig.
    preferred = [
        p for p in hits if (p.parent.name == board_name and p.name == f"{board_name}_defconfig")
    ]
    if preferred:
        if len(preferred) != 1:
            raise ValueError(
                f"Ambiguous preferred defconfig for board '{board_name}':\n  "
                + "\n  ".join(map(str, preferred))
            )
        return preferred[0]

    if len(hits) != 1:
        raise ValueError(
            f"Ambiguous defconfig for board '{board_name}':\n  " + "\n  ".join(map(str, hits))
        )

    return hits[0]


def find_master_board_defconfig(zephyr_root: Path, master_ident: str) -> Path:
    """Locate the master board defconfig.

    Per requirements, locate the board defconfig via a workspace-wide search:
      <ZEPHYR_BASE>/boards/**/<board>_defconfig

    The board name is derived from the master platform identifier by stripping
    everything after the first '/'.

    Raises FileNotFoundError if the defconfig cannot be found.
    """

    board_name = master_board_name_from_identifier(master_ident)
    if not board_name:
        raise FileNotFoundError(f"Cannot derive master board name from identifier: {master_ident}")

    return find_board_defconfig(zephyr_root, board_name)


def _kconfig_value_is_zero(v: str) -> bool:
    sv = str(v).strip().strip('"')
    try:
        return int(sv, 0) == 0
    except ValueError:
        return sv == "0" or sv.lower() == "0x0"


def upsert_kconfig_line(defconfig_text: str, symbol: str, value: str) -> str:
    """Insert or replace a Kconfig assignment line in a defconfig-like text.

    Ensures there is exactly one "<symbol>=<value>" assignment in the output.
    """

    re_sym = re.compile(rf"^\s*{re.escape(symbol)}\s*=\s*.+?\s*$")
    out: list[str] = []
    replaced = False

    for line in (defconfig_text or "").splitlines():
        if re_sym.match(line):
            if not replaced:
                out.append(f"{symbol}={value}")
                replaced = True
            # Drop any duplicate occurrences.
            continue
        out.append(line)

    if replaced:
        return "\n".join(out) + ("\n" if defconfig_text.endswith("\n") else "")

    # Insert before trailing blank lines if any, else append at end.
    insert_at = len(out)
    while insert_at > 0 and out[insert_at - 1].strip() == "":
        insert_at -= 1

    out.insert(insert_at, f"{symbol}={value}")
    return "\n".join(out) + "\n"


def render_defconfig(t: Target) -> str:
    lines: list[str] = []
    lines.append("# SPDX-FileCopyrightText: 2025 NXP")
    lines.append("# SPDX-License-Identifier: Apache-2.0")
    lines.append("# Auto-generated by gen_nxp_virtual_board.py")
    lines.append("# Build-only SoC coverage: keep requirements minimal")

    # Keep runtime I/O optional for build-only.
    lines.append("CONFIG_CONSOLE=n")
    lines.append("CONFIG_UART_CONSOLE=n")
    lines.append("CONFIG_SERIAL=n")
    lines.append("CONFIG_PRINTK=y")

    # NOTE: Do not set CONFIG_SOC_PART_NUMBER_* directly in defconfig.
    # These symbols are typically non-user-configurable (no prompt), and Zephyr
    # treats assigning them in configuration files as a fatal Kconfig warning.
    # The generated board Kconfig selects the part-number symbol instead.

    # Some NXP SoC headers unconditionally include <fsl_port.h>.
    # Ensure the MCUX SDK PORT driver is pulled in so the header is on the
    # include path.
    # if t.soc.startswith("mcxc") or t.soc.startswith("mcxe24") or t.soc in {
    #    "mcxw716c",
    #    "mcxw727c",
    # }:
    #    lines.append("CONFIG_PINCTRL=y")
    #    lines.append("CONFIG_PINCTRL_NXP_PORT=y")

    # MCXW23x defaults to MCUX_OS_TIMER unless PM is enabled. That combination
    # can leave SYS_CLOCK_HW_CYCLES_PER_SEC unset for build-only configs.
    # Force SysTick and enable it in DTS.
    if t.soc in {"mcxw235", "mcxw236"}:
        lines.append("CONFIG_CORTEX_M_SYSTICK=y")
        # Avoid depending on DT/Kconfig default plumbing for build-only.
        lines.append("CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC=32000000")

    # Some SoC families depend on the chosen timer to set SYS_CLOCK_HW_CYCLES_PER_SEC.
    # In a virtual board without explicit timer enablement, this can end up unset.
    # Force SysTick for *ARM Cortex-M* targets.
    if t.soc.startswith("mcxn"):
        lines.append("CONFIG_CORTEX_M_SYSTICK=y")

    # i.MX RT build-only coverage: ensure we always end up with a valid system clock.
    # For NXP i.MXRT11xx/118x/7xx, SYS_CLOCK_HW_CYCLES_PER_SEC is usually only
    # defaulted when CORTEX_M_SYSTICK or MCUX_OS_TIMER is selected.
    if t.soc.startswith("mimxrt") and (t.cpucluster not in {"hifi4", "hifi1", "f1"}):
        lines.append("CONFIG_CORTEX_M_SYSTICK=y")

    # i.MX RT: the SoC default enables the ROM boot header, which pulls in MCUX
    # boot header code that expects a board-provided "board.h".
    # For build-only virtual boards, disable the boot header.
    if t.soc.startswith("mimxrt") and (t.cpucluster not in {"hifi4", "hifi1", "f1"}):
        lines.append("CONFIG_NXP_IMXRT_BOOT_HEADER=n")

    # i.MXRT6xx (RT685 CM33) SoC init requires these board-level Kconfig values.
    if t.soc == "mimxrt685s" and t.cpucluster == "cm33":
        lines.append("CONFIG_XTAL_SYS_CLK_HZ=24000000")
        lines.append("CONFIG_SYSOSC_SETTLING_US=260")

    # RT595 F1 (Xtensa) needs ISR table generation and a smaller vector entry.

    # i.MX91 A55 selects HAS_MCUX_CACHE unconditionally, but HAS_MCUX is only
    # selected when CLOCK_CONTROL=y. Enable it to avoid Kconfig warnings aborting the build.
    if t.soc in {"mimx9111", "mimx9131"}:
        lines.append("CONFIG_CLOCK_CONTROL=y")

    if t.soc == "mimxrt595s" and t.cpucluster == "f1":
        lines.append("CONFIG_GEN_ISR_TABLES=y")
        lines.append("CONFIG_GEN_IRQ_VECTOR_TABLE=n")
        lines.append("CONFIG_XTENSA_SMALL_VECTOR_TABLE_ENTRY=y")
        lines.append("CONFIG_NXP_IMXRT_BOOT_HEADER=n")

    # i.MX93 A55 selects HAS_MCUX_CACHE unconditionally, but HAS_MCUX is only
    # selected when CLOCK_CONTROL=y. Enable it to avoid Kconfig warnings aborting the build.
    if t.soc == "mimx9352" and t.cpucluster == "a55":
        lines.append("CONFIG_CLOCK_CONTROL=y")

    # i.MX8M (A53) selects HAS_MCUX_CACHE/HAS_MCUX_RDC but only selects HAS_MCUX
    # when CLOCK_CONTROL=y. Enable it to avoid Kconfig warnings aborting the build.
    if t.soc in {"mimx8ml8", "mimx8mm6", "mimx8mn6"} and t.cpucluster == "a53":
        lines.append("CONFIG_CLOCK_CONTROL=y")

    # i.MX9 (i.MX95/i.MX943) SoC init expects SCMI plumbing; without it we get
    # undefined references to scmi_clock_* and DEVICE_DT_GET(scmi_clk).
    # For build-only virtual boards, enable the SCMI drivers but keep I/O disabled.
    if t.soc in {"mimx9596", "mimx94398"} and t.cpucluster in {"a55", "m33", "m7", "m7_0", "m7_1"}:
        lines.append("CONFIG_ARM_SCMI=y")
        lines.append("CONFIG_MBOX=y")
        # Ensure SCMI transport comes after its mailbox dependency.
        # Default MBOX_INIT_PRIORITY=40, so set SCMI init priorities above that.
        lines.append("CONFIG_MBOX_INIT_PRIORITY=40")
        lines.append("CONFIG_ARM_SCMI_SHMEM_INIT_PRIORITY=60")
        lines.append("CONFIG_ARM_SCMI_TRANSPORT_INIT_PRIORITY=60")
        # SCMI clock controller driver runs at CLOCK_CONTROL_INIT_PRIORITY.
        # Ensure it comes after SCMI core/transport.
        lines.append("CONFIG_CLOCK_CONTROL_INIT_PRIORITY=70")
        lines.append("CONFIG_CLOCK_CONTROL=y")

    # i.MXRT6xx HIFI4:
    # - ensure required vector/isr table settings so the small vector entries fit
    #   the fixed MEM_VECT_TEXT_SIZE (see soc/nxp/imxrt/imxrt6xx/hifi4/include/soc/memory.h)
    # - IMAGE_VECTOR_TABLE_OFFSET is a 'hex' Kconfig symbol; if left unset it expands
    #   to "0x" in generated configs.c and breaks the assembler.
    if t.soc == "mimxrt685s" and t.cpucluster == "hifi4":
        lines.append("CONFIG_GEN_ISR_TABLES=y")
        lines.append("CONFIG_GEN_IRQ_VECTOR_TABLE=n")
        lines.append("CONFIG_XTENSA_SMALL_VECTOR_TABLE_ENTRY=y")
        lines.append("CONFIG_NXP_IMXRT_BOOT_HEADER=n")
        lines.append("CONFIG_IMAGE_VECTOR_TABLE_OFFSET=0x0")

    lines.append("")
    return "\n".join(lines)


def render_target_yaml(
    board_name: str, t: Target, arch: str, supported: list[str] | None = None
) -> str:
    # Keep identifier format and vendor field consistent with Zephyr expectations.
    return common.render_target_yaml(
        board_name=board_name,
        t=t,
        arch=arch,
        vendor="nxp",
        supported=supported,
    )


@dataclass(frozen=True)
class PlatformInfo:
    identifier: str
    board_rel_dir: str  # relative to boards/nxp
    yaml_path: Path
    dts_path: Path | None
    supported: list[str]
    dtsi_include: str | None = None
    socs: list[tuple[str, str | None]] | None = None


def parse_board_platform_identifier(ident: str) -> tuple[str | None, str | None]:
    # Common NXP identifiers are of form:
    #   <board>/<soc>[/<cpucluster>[/...]]
    # We only care about the SoC and the optional cpucluster here.
    parts = [p for p in str(ident).split("/") if p]
    if len(parts) < 2:
        return None, None
    soc = parts[1]
    cpucluster = parts[2] if len(parts) >= 3 else None
    return soc, cpucluster


def guess_dtsi_include_from_board_dts(dts_path: Path) -> str | None:
    # Heuristic: take the first include that looks like an NXP SoC DTSI.
    inc_re = re.compile(r"^\s*#include\s+<([^>]+)>\s*$")
    try:
        for line in dts_path.read_text(encoding="utf-8", errors="ignore").splitlines():
            m = inc_re.match(line)
            if not m:
                continue
            inc = m.group(1).strip()
            if inc.endswith(".dtsi") and (inc.startswith("nxp/") or "/nxp_" in inc):
                return inc
    except OSError:
        return None
    return None


def collect_nxp_platform_db(zephyr_root: Path) -> dict[str, PlatformInfo]:
    """Collect platform metadata from existing boards under boards/nxp/."""

    db: dict[str, PlatformInfo] = {}
    boards_root = zephyr_root / "boards" / "nxp"
    if not boards_root.exists():
        return db

    for yml in sorted(boards_root.rglob("*.yaml"), key=str):
        try:
            data = common.load_yaml(yml)
        except Exception:
            continue
        if not isinstance(data, dict):
            continue
        ident = data.get("identifier")
        if not ident:
            continue

        soc, cpucluster = parse_board_platform_identifier(str(ident))

        supported = data.get("supported")
        if not isinstance(supported, list):
            supported_list: list[str] = []
        else:
            supported_list = [str(x) for x in supported]

        dts_candidate = yml.with_suffix(".dts")
        dts_path = dts_candidate if dts_candidate.exists() else None
        dtsi_include: str | None = (
            guess_dtsi_include_from_board_dts(dts_candidate) if dts_path else None
        )

        # boards/nxp/<board_rel_dir>/<file>
        board_rel_dir = str(yml.parent.relative_to(boards_root))

        # Try to infer the SoC(s) supported by this platform.
        # 1) from the identifier itself (<board>/<soc>[/<cpucluster>])
        # 2) otherwise from a sibling board.yml file.
        socs: list[tuple[str, str | None]] = []
        if soc:
            socs.append((soc, cpucluster))
        else:
            byml = yml.parent / "board.yml"
            if byml.exists():
                try:
                    by = common.load_yaml(byml)
                except Exception:
                    by = None
                if isinstance(by, dict) and isinstance(by.get("board"), dict):
                    for se in by["board"].get("socs", []) or []:
                        if not isinstance(se, dict) or not se.get("name"):
                            continue
                        soc_name = str(se["name"])
                        variants = se.get("variants")
                        if isinstance(variants, list) and variants:
                            for v in variants:
                                if not isinstance(v, dict):
                                    continue
                                cc = v.get("cpucluster")
                                socs.append((soc_name, str(cc) if cc else None))
                        else:
                            socs.append((soc_name, None))

        info = PlatformInfo(
            identifier=str(ident),
            board_rel_dir=board_rel_dir,
            yaml_path=yml,
            dts_path=dts_path,
            supported=supported_list,
            dtsi_include=dtsi_include,
            socs=socs,
        )

        db.setdefault(str(ident), info)

    return db


def platform_for_target(
    platform_db: dict[str, PlatformInfo],
    soc: str,
    cpucluster: str | None,
) -> PlatformInfo | None:
    # Prefer an exact match for any known identifier with this soc/cpucluster.
    # If multiple exist, choose the lexicographically smallest identifier for stability.
    candidates: list[PlatformInfo] = []
    for pi in platform_db.values():
        if not pi.socs:
            continue
        if (soc, cpucluster) in pi.socs:
            candidates.append(pi)
    if not candidates:
        return None
    return sorted(candidates, key=lambda x: x.identifier)[0]


def render_dts_from_master(
    *,
    t: Target,
    master: PlatformInfo,
    zephyr_root: Path,
    dtsi_include: str,
) -> str:
    """Generate DTS based on a master platform DTS, with minimal rewriting."""

    if not master.dts_path:
        raise ValueError(f"Master platform has no DTS: {master.identifier}")

    src = master.dts_path.read_text(encoding="utf-8", errors="ignore")

    # NOTE: Keep quoted includes as-is.
    # They are typically board-local files (e.g. "<board>-pinctrl.dtsi").
    # We copy those files next to the generated DTS so the C preprocessor
    # can resolve them via the "current directory" include semantics.
    out = src

    # Replace SoC DTSI include. Prefer replacing exactly the include we detected
    # from the master platform DTS.
    if master.dtsi_include:
        out = out.replace(f"#include <{master.dtsi_include}>", f"#include <{dtsi_include}>")
    else:
        # fallback: replace first nxp/*.dtsi include
        out = re.sub(
            r'^\s*#include\s+<nxp/[^>]+\.dtsi>\s*$',
            f"#include <{dtsi_include}>",
            out,
            count=1,
            flags=re.MULTILINE,
        )

    out = common.patch_model_and_compatible(
        out,
        model=f"NXP Virtual Board ({t.qualifier})",
        compat_soc=f"nxp,{t.soc}",
    )

    banner = (
        "/* Auto-generated by gen_nxp_virtual_board.py */\n"
        f"/* Master DTS template: boards/nxp/{master.board_rel_dir}/{master.dts_path.name} */\n"
    )
    return banner + out


def copy_master_quoted_includes(*, master: PlatformInfo, board_out_dir: Path) -> None:
    if not master.dts_path:
        return
    common.copy_quoted_includes_from_file(src_dts_path=master.dts_path, dst_dir=board_out_dir)


def resolve_master_platform_identifier(
    t: Target,
    master_platforms: dict[str, Any],
    overrides_master: dict[str, Any],
) -> str | None:
    """Resolve a master platform identifier for a target.

    Supports multiple shapes in nxp_virtual.json:

    1) "<series>": "<master_ident>"
    2) "<series>": {"<cpucluster>": "<master_ident>", "default": "..."}
    3) "<series>": {"<soc>": "<master_ident>"}
    4) "<series>": {"<soc>": {"<cpucluster>": "<master_ident>", "default": "..."}}

    (3)/(4) are required for i.MX families where different SoCs share a series
    but require different board pinctrl/overlays.
    """

    # Per-target/per-soc override first
    ov = overrides_master.get(t.qualifier) or overrides_master.get(t.soc)
    if ov:
        return str(ov)

    if not t.series:
        return None

    entry = master_platforms.get(t.series)
    if not entry:
        return None

    def _pick(val: Any) -> str | None:
        if not val:
            return None
        if isinstance(val, str):
            return val
        if isinstance(val, dict):
            # allow per-cpucluster selection, with optional "default"
            if t.cpucluster and t.cpucluster in val:
                return _pick(val[t.cpucluster])
            if "default" in val:
                return _pick(val["default"])
        return None

    if isinstance(entry, str):
        return entry

    if isinstance(entry, dict):
        # Prefer per-SoC mapping if present.
        if t.soc and t.soc in entry:
            picked = _pick(entry[t.soc])
            if picked:
                return picked

        # Fallback to per-cpucluster mapping.
        if t.cpucluster and t.cpucluster in entry:
            picked = _pick(entry[t.cpucluster])
            if picked:
                return picked

        # Final fallback.
        if "default" in entry:
            return _pick(entry["default"])

    return None


def render_dts_template_rt595_f1(t: Target) -> str:
    # Based on boards/nxp/mimxrt595_evk/mimxrt595_evk_mimxrt595s_f1.dts
    # Keep it minimal but valid for build-only.
    lines: list[str] = []
    lines.append(_SPDX_C_COPYRIGHT)
    lines.append(_SPDX_C_LICENSE)
    lines.append("/* Auto-generated by gen_nxp_virtual_board.py (template: rt595_f1) */")
    lines.append("/dts-v1/;")
    lines.append("#include <mem.h>")
    lines.append("#include <xtensa/xtensa.dtsi>")
    lines.append("")
    lines.append("/ {")
    lines.append(f"\tmodel = \"NXP Virtual Board ({t.qualifier})\";")
    lines.append("\tcompatible = \"nxp\";")
    lines.append("")
    lines.append("\tcpus {")
    lines.append("\t\t#address-cells = <1>;")
    lines.append("\t\t#size-cells = <0>;")
    lines.append("\t\tcpu0: cpu@0 {")
    lines.append("\t\t\tdevice_type = \"cpu\";")
    lines.append("\t\t\tcompatible = \"cdns,tensilica-xtensa-lx6\";")
    lines.append("\t\t\treg = <0>;")
    lines.append("\t\t};")
    lines.append("\t};")
    lines.append("")
    lines.append("\tsram0: memory@0 {")
    lines.append("\t\t#address-cells = <1>;")
    lines.append("\t\t#size-cells = <1>;")
    lines.append("\t\tdevice_type = \"memory\";")
    lines.append("\t\tcompatible = \"mmio-sram\";")
    lines.append("\t\treg = <0x0 DT_SIZE_K(512)>;")
    lines.append("")
    lines.append("\t\tadsp_data: memory@840000 {")
    lines.append("\t\t\treg = <0x840000 DT_SIZE_K(256)>;")
    lines.append("\t\t};")
    lines.append("\t};")
    lines.append("")
    lines.append("\tchosen {")
    lines.append("\t\tzephyr,sram = &adsp_data;")
    lines.append("\t};")
    lines.append("};")
    lines.append("")
    return "\n".join(lines)


def render_dts_template_imx6sx_m4(t: Target) -> str:
    """Minimal DTS template for i.MX6 SoloX M4 (mcimx6x/m4).

    Reuses the SoC DTSI but provides the DT_FLASH_ADDR/DT_SRAM_ADDR defines
    required by nxp_imx6sx_m4.dtsi.
    """

    lines: list[str] = []
    lines.append(_SPDX_C_COPYRIGHT)
    lines.append(_SPDX_C_LICENSE)
    lines.append("/* Auto-generated by gen_nxp_virtual_board.py (template: imx6sx_m4) */")
    lines.append("/dts-v1/;")
    lines.append("")
    lines.append("#include <mem.h>")
    lines.append("")
    lines.append("/* Required by nxp_imx6sx_m4.dtsi */")
    lines.append("#define DT_FLASH_SIZE\tDT_SIZE_K(512)")
    lines.append("#define DT_FLASH_ADDR\t84000000")
    lines.append("#define DT_SRAM_SIZE\tDT_SIZE_K(128)")
    lines.append("#define DT_SRAM_ADDR\t84080000")
    lines.append("")
    lines.append("#include <nxp/nxp_imx6sx_m4.dtsi>")
    lines.append("#include <nxp/nxp_imx/mimx6sx-pinctrl.dtsi>")
    lines.append("")

    lines.append("/ {")
    lines.append(f"\tmodel = \"NXP Virtual Board ({t.qualifier})\";")
    lines.append(f"\tcompatible = \"nxp,{t.soc}\";")
    lines.append("")
    lines.append("\tchosen {")
    lines.append("\t\tzephyr,flash = &flash;")
    lines.append("\t\tzephyr,sram = &sram;")
    lines.append("\t};")
    lines.append("};")
    lines.append("")
    return "\n".join(lines)


def render_dts_template_imx7d_m4(t: Target) -> str:
    """Minimal DTS template for i.MX7D M4 (mcimx7d/m4).

    The SoC DTSI references a large set of pinmux node labels which are defined
    in the NXP HAL pinctrl include; include it so dtc can resolve the labels.
    """

    lines: list[str] = []
    lines.append(_SPDX_C_COPYRIGHT)
    lines.append(_SPDX_C_LICENSE)
    lines.append("/* Auto-generated by gen_nxp_virtual_board.py (template: imx7d_m4) */")
    lines.append("/dts-v1/;")
    lines.append("")
    lines.append("#include <nxp/nxp_imx7d_m4.dtsi>")
    # Pinmux labels referenced by nxp_imx7d_m4.dtsi live in the NXP HAL DTS.
    # (Zephyr's DTS include paths include modules/hal/nxp/dts by default.)
    lines.append("#include <nxp/nxp_imx/mimx7d-pinctrl.dtsi>")
    lines.append("")
    lines.append("/ {")
    lines.append(f"\tmodel = \"NXP Virtual Board ({t.qualifier})\";")
    lines.append(f"\tcompatible = \"nxp,{t.soc}\";")
    lines.append("")
    lines.append("\tchosen {")
    lines.append("\t\tzephyr,flash = &tcml_code;")
    lines.append("\t\tzephyr,sram = &tcmu_sys;")
    lines.append("\t};")
    lines.append("};")
    lines.append("")
    return "\n".join(lines)


def render_dts(t: Target, dtsi_include: str, dts_labels: dict[str, str] | None = None) -> str:
    # Support explicit templates for targets that don't have a usable SoC DTSI.
    if dtsi_include.startswith("template:"):
        templ = dtsi_include.split(":", 1)[1]
        if templ == "rt595_f1":
            return render_dts_template_rt595_f1(t)
        if templ == "imx6sx_m4":
            return render_dts_template_imx6sx_m4(t)
        if templ == "imx7d_m4":
            return render_dts_template_imx7d_m4(t)
        raise ValueError(f"Unknown DTS template: {dtsi_include}")

    compat = f"nxp,{t.soc}"

    def _render_status_override(node_label: str, status: str) -> None:
        lines.append(f"&{node_label} {{")
        lines.append(f"\tstatus = \"{status}\";")
        lines.append("};")

    def _render_root_chosen(*, flash: str, sram: str, flash_ctrl: str | None = None) -> None:
        lines.append("/ {")
        lines.append("\tchosen {")
        lines.append(f"\t\tzephyr,flash = &{flash};")
        lines.append(f"\t\tzephyr,sram = &{sram};")
        if flash_ctrl:
            lines.append(f"\t\tzephyr,flash-controller = &{flash_ctrl};")
        lines.append("\t};")
        lines.append("};")

    lines: list[str] = []
    lines.append(_SPDX_C_COPYRIGHT)
    lines.append(_SPDX_C_LICENSE)
    lines.append("/* Auto-generated by gen_nxp_virtual_board.py */")
    lines.append("/dts-v1/;")
    lines.append("")
    lines.append(f"#include <{dtsi_include}>")

    # Some SoCs have pinmux node labels provided by the NXP HAL DTS includes.
    if t.soc == "mimx9352" and t.cpucluster in {"a55", "m33"}:
        lines.append("#include <nxp/nxp_imx/mimx9352cvuxk-pinctrl.dtsi>")

    if t.soc == "mimx9596" and t.cpucluster in {"a55", "m7"}:
        # i.MX95 pinctrl labels come from the NXP HAL DTS.
        # Use one known-good part-number include that matches symbols used by
        # the EVK DTSI (board-local pinctrl file is copied when using a master DTS).
        lines.append("#include <nxp/nxp_imx/mimx9596cvtxn-pinctrl.dtsi>")

    # Some SoC bindings require board-level properties.
    if t.soc == "mimx9596" and t.cpucluster in {"a55", "m7"}:
        # i.MX95 EVK pinctrl
        lines.append("#include <nxp/nxp_imx/mimx9596cvtxn-pinctrl.dtsi>")
    lines.append("")
    lines.append("/ {")
    lines.append(f"\tmodel = \"NXP Virtual Board ({t.qualifier})\";")
    lines.append(f"\tcompatible = \"{compat}\";")

    # Provide basic chosen nodes for some SoCs so the linker gets sane FLASH/RAM sizes.
    if t.soc.startswith("mcxn"):
        lines.append("")
        lines.append("\tchosen {")
        lines.append("\t\tzephyr,sram = &sram0;")
        lines.append("\t\tzephyr,flash = &flash;")
        lines.append("\t\tzephyr,flash-controller = &fmu;")
        lines.append("\t};")

    # MCXE24x SoCs: provide chosen flash/sram for XIP builds.
    if t.soc.startswith("mcxe24"):
        lines.append("")
        lines.append("\tchosen {")
        lines.append("\t\tzephyr,flash = &flash0;")
        lines.append("\t\tzephyr,sram = &sram_l;")
        lines.append("\t};")

    # MCXE31x SoCs: provide chosen flash/sram for XIP builds.
    if t.soc in {"mcxe315", "mcxe316"}:
        lines.append("")
        lines.append("\tchosen {")
        lines.append("\t\tzephyr,sram = &dtcm;")
        lines.append("\t\tzephyr,flash = &program_flash;")
        lines.append("\t\tzephyr,flash-controller = &flash;")
        lines.append("\t};")

    if t.soc in {"mcxe317", "mcxe31b"}:
        lines.append("")
        lines.append("\tchosen {")
        lines.append("\t\tzephyr,sram = &sram;")
        lines.append("\t\tzephyr,flash = &program_flash;")
        lines.append("\t\tzephyr,flash-controller = &flash;")
        lines.append("\t};")

    # i.MX93 A55 build-only needs a DDR region and a flash chosen node.
    if t.soc == "mimx9352" and t.cpucluster == "a55":
        lines.append("")
        lines.append("\tflash0: flash@0 {")
        lines.append("\t\tcompatible = \"soc-nv-flash\";")
        lines.append("\t\treg = <0 DT_SIZE_M(8)>;")
        lines.append("\t\terase-block-size = <4096>;")
        lines.append("\t\twrite-block-size = <1>;")
        lines.append("\t};")
        lines.append("")
        lines.append("\tdram: memory@d0000000 {")
        lines.append("\t\tdevice_type = \"memory\";")
        lines.append("\t\treg = <0xd0000000 DT_SIZE_M(16)>;")
        lines.append("\t};")
        lines.append("")
        lines.append("\tchosen {")
        lines.append("\t\tzephyr,flash = &flash0;")
        lines.append("\t\tzephyr,sram = &dram;")
        lines.append("\t};")

    # RW6xx has external flash in real boards; provide a dummy flash region.
    if t.soc in {"rw610", "rw612"}:
        lines.append("")
        lines.append("\tflash0: flash@0 {")
        lines.append("\t\tcompatible = \"soc-nv-flash\";")
        lines.append("\t\treg = <0 DT_SIZE_M(8)>;")
        lines.append("\t\terase-block-size = <4096>;")
        lines.append("\t\twrite-block-size = <1>;")
        lines.append("\t};")
        lines.append("")
        lines.append("\tchosen {")
        lines.append("\t\tzephyr,sram = &sram_data;")
        lines.append("\t\tzephyr,flash = &flash0;")
        lines.append("\t\tzephyr,flash-controller = &flexspi;")
        lines.append("\t};")

    # i.MX RT (ARM clusters) often need a board-provided flash node.
    if t.soc.startswith("mimxrt") and (t.cpucluster not in {"hifi4", "hifi1", "f1"}):
        lines.append("")
        lines.append("\tflash0: flash@0 {")
        lines.append("\t\tcompatible = \"soc-nv-flash\";")
        lines.append("\t\treg = <0 DT_SIZE_M(8)>;")
        lines.append("\t};")
        lines.append("")
        lines.append("\tchosen {")
        lines.append("\t\tzephyr,sram = &sram0;")
        lines.append("\t\tzephyr,flash = &flash0;")
        lines.append("\t};")

    lines.append("};")
    lines.append("")

    if t.soc.startswith("mcxc"):
        labels = dts_labels or {}
        sim_label = str(labels.get("sim", "sim"))
        osc_label = str(labels.get("osc", "osc"))
        cpu0_label = str(labels.get("cpu0", "cpu0"))

        # Required DT properties for MCXC clock setup
        lines.append(f"&{sim_label} {{")
        lines.append("\tpllfll-select = <KINETIS_SIM_PLLFLLSEL_MCGPLLCLK>;")
        lines.append("\ter32k-select = <KINETIS_SIM_ER32KSEL_OSC32KCLK>;")
        lines.append("};")
        lines.append("")

        # Required by nxp,mcxc-osc binding
        lines.append(f"&{osc_label} {{")
        lines.append("\tclock-frequency = <32768>;\t/* conservative default */")
        lines.append("\tmode = \"low-power\";")
        lines.append("};")
        lines.append("")

        # MCXC SoC init reads cpu0 clock-frequency from DT
        lines.append(f"&{cpu0_label} {{")
        lines.append("\tclock-frequency = <48000000>;")
        lines.append("};")
        lines.append("")

    # MCXN nx4x: SoC DTSI defines ENET without pinctrl (binding requires pinctrl-0).
    # For build-only coverage, disable it.
    if t.soc in {"mcxn947", "mcxn547"}:
        _render_status_override("enet", "disabled")
        lines.append("")

    # Enable SysTick DT node where SoC DT disables it.
    if t.soc.startswith("mcxn"):
        _render_status_override("os_timer", "disabled")
        lines.append("")
        _render_status_override("systick", "okay")
        lines.append("")

    if t.soc.startswith("mimxrt") and (t.cpucluster not in {"hifi4", "hifi1", "f1"}):
        # Enable OS timer so CONFIG_MCUX_OS_TIMER can be selected.
        _render_status_override("os_timer", "okay")
        lines.append("")

    # MCXW2xx/7xx:
    # These SoC DTSI files don't set zephyr,flash/zephyr,sram chosen nodes, but
    # XIP is enabled and core Zephyr Kconfig derives CONFIG_FLASH_SIZE and
    # CONFIG_SRAM_SIZE from those chosen nodes (arch/Kconfig).
    #
    # Real boards (e.g. frdm_mcxw23/frdm_mcxw71/frdm_mcxw72) provide these chosen
    # nodes at the board level. Do the same for build-only virtual boards.
    if t.soc in {"mcxw235", "mcxw236"}:
        _render_status_override("os_timer", "disabled")
        lines.append("")
        _render_status_override("systick", "okay")
        lines.append("")
        _render_root_chosen(flash="flash0", sram="sram0", flash_ctrl="iap")
        lines.append("")

    if t.soc in {"mcxw716c", "mcxw727c"}:
        _render_root_chosen(flash="flash", sram="stcm0", flash_ctrl="fmu")
        lines.append("")

    # MCXN dual-core CPU selection: mimic real boards by deleting the other core
    if t.cpucluster == "cpu0":
        lines.append("/ {")
        lines.append("\tcpus {")
        lines.append("\t\t/delete-node/ cpu@1;")
        lines.append("\t};")
        lines.append("};")
    elif t.cpucluster == "cpu1":
        lines.append("/ {")
        lines.append("\tcpus {")
        lines.append("\t\t/delete-node/ cpu@0;")
        lines.append("\t};")
        lines.append("};")

    lines.append("")
    return "\n".join(lines)


def apply_filters(targets: list[Target], filters: dict[str, Any], family_name: str) -> list[Target]:
    f = (filters or {}).get(family_name) or {}
    prefixes = f.get("prefixes")

    out = common.apply_prefix_filters(targets, prefixes)

    # Optional filter knobs:
    # - cpuclusters: keep only these cpuclusters
    # - exclude_cpuclusters: drop these cpuclusters
    cpuclusters = f.get("cpuclusters")
    if isinstance(cpuclusters, list) and cpuclusters:
        keep = {str(c) for c in cpuclusters}
        out = [t for t in out if t.cpucluster in keep]

    exclude_cpuclusters = f.get("exclude_cpuclusters")
    if isinstance(exclude_cpuclusters, list) and exclude_cpuclusters:
        drop = {str(c) for c in exclude_cpuclusters}
        out = [t for t in out if t.cpucluster not in drop]

    return out


def apply_cpucluster_overrides(targets: list[Target], overrides: dict[str, Any]) -> list[Target]:
    """Optionally restrict cpuclusters per-SoC.

    Config format:
      "overrides": {
        "cpuclusters": {
          "mcimx7d": ["m4"],
          "mimx94398": ["a55", "m33"]
        }
      }

    For SoCs listed in the mapping, only the listed cpuclusters are kept.
    Non-listed SoCs are left unchanged.
    """

    m = (overrides or {}).get("cpuclusters") or {}
    if not isinstance(m, dict) or not m:
        return targets

    allow: dict[str, set[str]] = {}
    for soc, clusters in m.items():
        if not isinstance(clusters, list):
            continue
        allow[str(soc)] = {str(c) for c in clusters}

    if not allow:
        return targets

    out: list[Target] = []
    for t in targets:
        allowed = allow.get(t.soc)
        if not allowed:
            out.append(t)
            continue
        if t.cpucluster in allowed:
            out.append(t)

    return out


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(allow_abbrev=False)
    ap.add_argument("--config", "--configuration", dest="config", required=True, type=Path)
    ap.add_argument("--zephyr-root", default=_default_zephyr_root(), type=Path)
    args = ap.parse_args(argv)

    zephyr_root: Path = args.zephyr_root.resolve()
    cfg = common.load_json(args.config)

    # Used to validate #include <...> DTSI paths we generate.
    dts_index = build_dts_include_index(zephyr_root)

    board_cfg = cfg["board"]
    board_name = str(board_cfg["name"])
    full_name = str(board_cfg.get("full_name", board_name))
    vendor = str(board_cfg.get("vendor", "nxp"))

    include_all_cpuclusters = bool(cfg.get("include_all_cpuclusters", True))
    include_part_number_variants = bool(cfg.get("include_part_number_variants", False))
    filters: dict[str, Any] = cfg.get("filters", {}) or {}

    # Optional: exclude specific SOC_PART_NUMBER_* variants from generation.
    #
    # Format:
    #   "exclude_part_number_variants": {
    #     "mimxrt1052": ["mimxrt1052cvl5a", "mimxrt1052dvl6a"],
    #     "mimx9596/m7": ["mimx9596avtxn"],
    #     "mimx8qx6/adsp": ["mimx8qx6cvldz"],
    #   }
    #
    # Keys can be:
    # - "<soc>"
    # - "<soc>/<cpucluster>"
    # Values can be:
    # - raw part number strings (case-insensitive)
    # - or full symbol names like "SOC_PART_NUMBER_<...>" (case-insensitive)
    exclude_part_number_variants: dict[str, Any] = cfg.get("exclude_part_number_variants", {}) or {}

    def _normalize_part(v: str) -> str:
        s = str(v).strip()
        s = s.removeprefix("SOC_PART_NUMBER_")
        return s.lower()

    def _excluded_parts_for(t: Target) -> set[str]:
        items: list[str] = []
        v1 = exclude_part_number_variants.get(t.qualifier)
        v2 = exclude_part_number_variants.get(t.soc)
        for v in (v1, v2):
            if isinstance(v, list):
                items.extend([str(x) for x in v])
            elif isinstance(v, str):
                items.append(v)
        return {_normalize_part(x) for x in items if str(x).strip()}

    out_root = zephyr_root / cfg["output"]["root"]
    board_out_dir = out_root / cfg["output"]["board_relpath"]

    overrides_dtsi: dict[str, str] = (cfg.get("overrides", {}) or {}).get("dtsi", {}) or {}
    overrides: dict[str, Any] = cfg.get("overrides", {}) or {}

    # Optional per-target DTS patching. Each entry is a list of strings; values that
    # look like DTS node references will have their overlay blocks removed from the
    # generated DTS. Non-DTS-looking values will be used to filter YAML `supported:`.
    overrides_patch_generated_dts: dict[str, Any] = overrides.get("patch_generated_dts", {}) or {}

    # Default DT node labels used in the emitted board DTS.
    dts_labels_default: dict[str, str] = cfg.get("dts_labels", {}) or {}
    overrides_dts_labels: dict[str, Any] = (cfg.get("overrides", {}) or {}).get(
        "dts_labels", {}
    ) or {}

    reuse_nxp_platforms = bool(cfg.get("reuse_nxp_platforms", True))
    platform_db: dict[str, PlatformInfo] = (
        collect_nxp_platform_db(zephyr_root) if reuse_nxp_platforms else {}
    )

    # SoC targets are sourced from Zephyr's soc.yml files.
    family_soc_yml = {
        "mcx": zephyr_root / "soc/nxp/mcx/soc.yml",
        "rw": zephyr_root / "soc/nxp/rw/soc.yml",
        "imxrt": zephyr_root / "soc/nxp/imxrt/soc.yml",
        "imx": zephyr_root / "soc/nxp/imx/soc.yml",
    }

    cfg_soc_ymls = cfg.get("soc_ymls")
    soc_ymls: list[Path]
    if cfg_soc_ymls:
        soc_ymls = [(zephyr_root / Path(str(p))).resolve() for p in (cfg_soc_ymls or [])]
    else:
        families: list[str] = list(cfg.get("families", []))
        soc_ymls = []
        for fam in families:
            p = family_soc_yml.get(fam)
            if not p or not p.exists():
                raise SystemExit(f"Missing soc.yml for family '{fam}': {p}")
            soc_ymls.append(p)

    # Optional legacy behavior: discover additional cpuclusters from Kconfig.soc.
    discover_cpuclusters_from_kconfig = bool(cfg.get("discover_cpuclusters_from_kconfig", False))
    family_kconfig_roots = {
        "mcx": zephyr_root / "soc/nxp/mcx",
        "rw": zephyr_root / "soc/nxp/rw",
        "imxrt": zephyr_root / "soc/nxp/imxrt",
        "imx": zephyr_root / "soc/nxp/imx",
    }

    targets: list[Target] = []
    for soc_yml in soc_ymls:
        if not soc_yml.exists():
            raise SystemExit(f"Missing soc.yml: {soc_yml}")

        # filters keys are typically the family folder name (e.g. "mcx", "imxrt")
        family_name = soc_yml.parent.name

        fam_targets = common.extract_targets_from_soc_yml(
            soc_yml, include_all_cpuclusters=include_all_cpuclusters
        )
        fam_targets = apply_filters(fam_targets, filters=filters, family_name=family_name)
        targets.extend(fam_targets)

    targets = apply_cpucluster_overrides(targets, overrides)

    # Optional: expand SoC targets into per-part-number board variants.
    # This is mainly useful for i.MX RT families where Kconfig.soc defines many
    # SOC_PART_NUMBER_* symbols and some downstream builds require selecting a
    # concrete part number.
    if include_part_number_variants:
        kconfig_soc_files: list[Path] = []
        for root in family_kconfig_roots.values():
            if not root or not root.exists():
                continue
            kconfig_soc_files.extend(sorted(root.rglob("Kconfig.soc"), key=str))

        part_numbers_by_soc = common.collect_kconfig_soc_part_numbers(kconfig_soc_files)

        expanded: list[Target] = []
        for t in targets:
            parts = part_numbers_by_soc.get(t.soc) or []
            if not parts:
                expanded.append(t)
                continue

            excluded = _excluded_parts_for(t)

            # Use the raw part-number string (without SOC_PART_NUMBER_ prefix)
            # as the board-variant name.
            for psym in parts:
                # Use lowercase for board variant identifiers so they align with
                # Zephyr conventions and the generated filename suffixes.
                pname = str(psym).removeprefix("SOC_PART_NUMBER_").lower()

                if pname in excluded:
                    continue

                expanded.append(
                    Target(soc=t.soc, cpucluster=t.cpucluster, series=t.series, variant=pname)
                )

        # De-dupe
        seenq: set[str] = set()
        dedup2: list[Target] = []
        for t in expanded:
            if t.qualifier in seenq:
                continue
            seenq.add(t.qualifier)
            dedup2.append(t)
        targets = dedup2

    if discover_cpuclusters_from_kconfig and include_all_cpuclusters:
        roots: list[Path] = []
        for soc_yml in soc_ymls:
            family_name = soc_yml.parent.name
            root = family_kconfig_roots.get(family_name)
            if root:
                roots.append(root)

        kconfig_symbols = common.collect_kconfig_soc_symbols(roots)
        for soc in sorted({t.soc for t in targets}):
            for c in sorted(common.extract_cpuclusters_from_kconfig_symbols(soc, kconfig_symbols)):
                targets.append(Target(soc=soc, cpucluster=c))

    # Final de-dupe
    seen: set[str] = set()
    dedup: list[Target] = []
    for t in targets:
        if t.qualifier in seen:
            continue
        seen.add(t.qualifier)
        dedup.append(t)
    targets = dedup

    master_platforms: dict[str, Any] = cfg.get("master_platforms", {}) or {}
    overrides_master_platform: dict[str, Any] = (cfg.get("overrides", {}) or {}).get(
        "master_platform", {}
    ) or {}

    common.ensure_empty_dir(board_out_dir)

    (board_out_dir / "board.yml").write_text(
        common.render_board_yml(board_name, full_name, vendor, targets), encoding="utf-8"
    )

    # The file name *must* be Kconfig.<boardname>
    (board_out_dir / f"Kconfig.{board_name}").write_text(
        render_kconfig_board(board_name, targets), encoding="utf-8"
    )

    # Emit per-target artifacts
    for t in targets:
        suffix = (
            f"{t.soc}"
            + (f"_{t.cpucluster}" if t.cpucluster else "")
            + (f"_{t.variant.lower()}" if t.variant else "")
        )

        # JSON override keys:
        # - "soc" (e.g. "rw612")
        # - "soc/cluster" (e.g. "mimx9352/a55")
        platform_info = platform_for_target(platform_db, t.soc, t.cpucluster)

        master_ident = resolve_master_platform_identifier(
            t, master_platforms, overrides_master_platform
        )
        master_info = platform_db.get(master_ident) if master_ident else None

        # DTSI override lookup should ignore the variant suffix.
        base_qual = f"{t.soc}/{t.cpucluster}" if t.cpucluster else t.soc

        dtsi_include = (
            overrides_dtsi.get(base_qual)
            or overrides_dtsi.get(t.soc)
            or (platform_info.dtsi_include if platform_info else None)
            or (master_info.dtsi_include if master_info else None)
            or default_dtsi(t)
        )

        dtsi_include = validate_and_fix_dtsi_include(
            t=t,
            dtsi_include=dtsi_include,
            dts_index=dts_index,
        )

        dts_labels_override = (
            overrides_dts_labels.get(t.qualifier) or overrides_dts_labels.get(t.soc) or {}
        )
        dts_labels: dict[str, str] = {
            **{str(k): str(v) for k, v in dts_labels_default.items()},
            **{str(k): str(v) for k, v in (dts_labels_override or {}).items()},
        }

        arch = guess_arch(t)

        dts_text: str
        if master_info and master_info.dts_path:
            copy_master_quoted_includes(master=master_info, board_out_dir=board_out_dir)
            try:
                dts_text = render_dts_from_master(
                    t=t,
                    master=master_info,
                    zephyr_root=zephyr_root,
                    dtsi_include=dtsi_include,
                )
            except Exception:
                # Robust fallback: keep generation working even if master DTS inlining fails.
                dts_text = render_dts(t, dtsi_include=dtsi_include, dts_labels=dts_labels)
        else:
            dts_text = render_dts(t, dtsi_include=dtsi_include, dts_labels=dts_labels)

        # Derive defconfig and, if needed, inherit the system clock from the resolved
        # master platform's *platform* defconfig.
        #
        # Important: many NXP boards do not have a "<board>_defconfig". Instead,
        # they use per-target defconfigs named after the platform YAML stem
        # (e.g. "mimxrt1170_evk_mimxrt1176_cm7_defconfig"). For those cases we
        # must look up the defconfig next to the master platform's *.yaml.
        base_defconfig = render_defconfig(t)
        gen_clk = extract_kconfig_value_from_text(
            base_defconfig, "CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC"
        )

        defconfig_text = base_defconfig
        # Inherit SYS_CLOCK_HW_CYCLES_PER_SEC from the resolved master platform's
        # defconfig if it is missing or set to 0.
        if (gen_clk is None) or _kconfig_value_is_zero(gen_clk):
            if master_info is not None:
                mdef = master_defconfig_path(master_info)
                try:
                    master_clk = extract_kconfig_value_from_defconfig(
                        mdef, "CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC"
                    )
                except (OSError, ValueError) as e:
                    master_clk = None
                    print(
                        f'''WARN: cannot read master platform defconfig for
                          '{master_info.identifier}' ({mdef}) to inherit
                          SYS_CLOCK_HW_CYCLES_PER_SEC: {e}'''
                    )

                if master_clk and not _kconfig_value_is_zero(master_clk):
                    defconfig_text = upsert_kconfig_line(
                        defconfig_text,
                        "CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC",
                        master_clk,
                    )
                    print(
                        f'''INFO: inherited SYS_CLOCK_HW_CYCLES_PER_SEC={master_clk}
                          from {master_info.identifier} ({mdef}) for {t.qualifier}'''
                    )
            elif master_ident:
                # No master platform in the DB; keep build-only generation robust.
                print(
                    f'''WARN: no master platform info for '{master_ident}'
                      to inherit SYS_CLOCK_HW_CYCLES_PER_SEC'''
                )

        patch_items: Any = (
            overrides_patch_generated_dts.get(base_qual)
            or overrides_patch_generated_dts.get(t.soc)
            or []
        )
        if isinstance(patch_items, str):
            patch_list: list[str] = [patch_items]
        elif isinstance(patch_items, list):
            patch_list = [str(x) for x in patch_items]
        else:
            patch_list = []

        # Normalize whitespace for dts-linter: no trailing blank lines, but ensure a final newline.
        dts_text = patch_generated_dts(t=t, dts_text=dts_text, remove=patch_list)
        dts_text = dts_text.rstrip() + "\n"

        (board_out_dir / f"{board_name}_{suffix}.dts").write_text(dts_text, encoding="utf-8")
        (board_out_dir / f"{board_name}_{suffix}_defconfig").write_text(
            defconfig_text, encoding="utf-8"
        )
        supported_feats = (
            platform_info.supported if platform_info and platform_info.supported else None
        ) or (master_info.supported if master_info and master_info.supported else None)
        if supported_feats and patch_list:
            # If the patch list contains non-DTS-node values, treat them as features
            # to remove from the YAML `supported:` list.
            rm = {x for x in patch_list if not _is_likely_dts_node_ref(x)}
            if rm:
                supported_feats = [x for x in supported_feats if x not in rm]

        (board_out_dir / f"{board_name}_{suffix}.yaml").write_text(
            render_target_yaml(
                board_name,
                t,
                arch=arch,
                supported=supported_feats,
            ),
            encoding="utf-8",
        )

    print(f"Generated board '{board_name}' with {len(targets)} targets")
    print(f"Output: {board_out_dir}")
    print(f"BOARD_ROOT: {out_root}")
    return 0
