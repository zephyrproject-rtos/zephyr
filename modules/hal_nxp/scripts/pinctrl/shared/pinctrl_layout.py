# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

"""
Resolve pinctrl output directories and board include paths.

Binding headers live in ``include/zephyr/dt-bindings/pinctrl/nxp`` under their
family/series directories. Node-bearing DTSI files live in ``dts/arm/nxp``
under ``<family>/<series>/pinctrl`` (or ``<family>/pinctrl`` for flat families).
Generators share this mapping so regeneration and board includes stay aligned.

Authority for "does <family>/<series> exist" is the zephyr DTS side layout, NOT
the SoC tree. A snapshot of the existing series dirs is embedded below; refresh
it with:

    for fam in imx imxrt kinetis lpc mcx rw s32; do \\
        for d in zephyr/dts/arm/nxp/$fam/*/; do \\
            b=$(basename "$d"); [ "$b" = common ] && continue; echo "$fam/$b"; \\
        done; done | sort

kinetis is treated specially: it is a series-organized family, so a kinetis part
ALWAYS goes into its derived series dir even when that dir does not yet exist on
the DTS side (k0x / km3x / kv3x are allowed new dirs). Every other family uses a
series dir only when ``<family>/<series>`` already exists on the DTS side;
otherwise the part uses its family directory (imx / s32 / rw are flat there).
"""

import pathlib
import re

# Snapshot of series dirs that exist on the zephyr DTS side
# (dts/arm/nxp/<family>/<series>/). See module docstring for refresh command.
_DTS_EXISTING_SERIES = {
    "imxrt/imxrt10xx",
    "imxrt/imxrt118x",
    "imxrt/imxrt11xx",
    "imxrt/imxrt266x",
    "imxrt/imxrt5xx",
    "imxrt/imxrt6xx",
    "imxrt/imxrt7xx",
    "kinetis/k2x",
    "kinetis/k32lx",
    "kinetis/k6x",
    "kinetis/k8x",
    "kinetis/ke1xf",
    "kinetis/ke1xz",
    "kinetis/kl2x",
    "kinetis/kv5x",
    "kinetis/kwx",
    "lpc/lpc11u6x",
    "lpc/lpc51u68",
    "lpc/lpc54xxx",
    "lpc/lpc55xxx",
    "lpc/lpc84x",
    "mcx/mcxa",
    "mcx/mcxc",
    "mcx/mcxe",
    "mcx/mcxl",
    "mcx/mcxn",
    "mcx/mcxw",
    # imx, s32, rw are FLAT on the DTS side (no series dirs).
}


def _series_kinetis(p):
    # order matters
    if p.startswith("K32L"):
        return "k32lx"
    if p.startswith("MK02"):
        return "k0x"
    if p.startswith("MK22"):
        return "k2x"
    if re.match(r"MK2[4678]", p):
        return "k2x"
    if re.match(r"MK6[3456]", p):
        return "k6x"
    if re.match(r"MK8[02]", p):
        return "k8x"
    if p.startswith("MKM35"):
        return "km3x"
    if p.startswith("MKV31"):
        return "kv3x"
    if p.startswith("MKV5"):
        return "kv5x"
    if p.startswith("MKL2"):
        return "kl2x"
    if p.startswith("MKW"):
        return "kwx"
    if re.match(r"MKE1[2-8]", p):
        if p[5:6] == "F":
            return "ke1xf"
        if p[5:6] == "Z":
            return "ke1xz"
    return None


def _series_lpc(p):
    if p.startswith("LPC51U68"):
        return "lpc51u68"
    if p.startswith("LPC54"):
        return "lpc54xxx"
    if p.startswith("LPC55"):
        return "lpc55xxx"
    return None


def _series_mcx(p):
    for letter, dirn in (
        ("MCXA", "mcxa"),
        ("MCXC", "mcxc"),
        ("MCXE", "mcxe"),
        ("MCXL", "mcxl"),
        ("MCXN", "mcxn"),
        ("MCXW", "mcxw"),
    ):
        if p.startswith(letter):
            return dirn
    return None


def _series_imxrt(p):
    if re.match(r"MIMXRT118", p):
        return "imxrt118x"  # MUST precede RT11
    if re.match(r"MIMXRT10", p):
        return "imxrt10xx"
    if re.match(r"MIMXRT11", p):
        return "imxrt11xx"
    if re.match(r"MIMXRT26", p):
        return "imxrt266x"
    if re.match(r"MIMXRT5", p):
        return "imxrt5xx"
    if re.match(r"MIMXRT6", p):
        return "imxrt6xx"
    if re.match(r"MIMXRT7", p):
        return "imxrt7xx"
    return None


def _family_and_series(part_num):
    """
    Map a bare SOC part number to (family, series). ``series`` may be None when
    it cannot be derived (the caller then falls back to a flat family layout).
    """
    p = part_num.upper()
    # i.MX RT (matches IMXRT... and MIMXRT...) before plain i.MX.
    if re.match(r"M?IMXRT", p):
        return "imxrt", _series_imxrt("MIMXRT" + p.split("IMXRT", 1)[1])
    if re.match(r"M?IMX", p):
        return "imx", None  # i.MX application processors: flat
    if p.startswith("LPC"):
        return "lpc", _series_lpc(p)
    if p.startswith("MCX"):
        return "mcx", _series_mcx(p)
    if p.startswith("RW"):
        return "rw", None  # flat on DTS side
    if p.startswith("S32"):
        return "s32", None  # flat on DTS side
    if p.startswith("MK") or p.startswith("K32"):
        return "kinetis", _series_kinetis(p)
    return None, None


def resolve_subdir(part_num, suffix=".h"):
    """
    Return a family/series directory, with pinctrl/ appended only for DTSI.
    Return None when the family is unknown.
    """
    if suffix not in (".h", ".dtsi"):
        raise ValueError(f"Unsupported pinctrl file suffix: {suffix}")
    family, series = _family_and_series(part_num)
    if family is None:
        return None
    if series is not None and (family == "kinetis" or f"{family}/{series}" in _DTS_EXISTING_SERIES):
        family = f"{family}/{series}"
    return f"{family}/pinctrl" if suffix == ".dtsi" else family


def output_root(suffix):
    """Return the committed Zephyr output tree for a pinctrl file type."""
    zephyr = pathlib.Path(__file__).resolve().parents[5]
    if suffix == ".h":
        return zephyr / "include/zephyr/dt-bindings/pinctrl/nxp"
    if suffix == ".dtsi":
        return zephyr / "dts/arm/nxp"
    raise ValueError(f"Unsupported pinctrl file suffix: {suffix}")


def include_path(part_num, suffix):
    """Return the board include matching a generated part's destination."""
    subdir = resolve_subdir(part_num, suffix)
    if subdir is None:
        raise ValueError(f"Unknown pinctrl family for part: {part_num}")
    if suffix == ".h":
        prefix = "zephyr/dt-bindings/pinctrl/nxp"
        part_num = part_num.upper()
    else:
        prefix = "nxp"
        part_num = part_num.lower()
    return f"{prefix}/{subdir}/{part_num}-pinctrl{suffix}"
