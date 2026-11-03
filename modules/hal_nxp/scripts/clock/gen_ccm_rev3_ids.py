#!/usr/bin/env python3
#
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0

"""
Generate the Zephyr nxp,imx-ccm-rev3 clock-identifier binding header for a SoC
from that SoC's MCUXpresso SDK fsl_clock.h.

The header defines the IMX_CCM_LPCG_* / IMX_CCM_ROOT_* / IMX_CCM_MUX_* macros
that peripheral devicetree nodes pass to the rev3 clock controller. Their values
must equal the HAL enumerations (clock_lpcg_t, clock_root_t,
clock_root_mux_source_t) the controller forwards them to, so they are generated
straight from fsl_clock.h rather than maintained by hand.

The parsing framework is shared across every rev3 SoC; each SoC contributes a
small configuration (see socs/) describing its devicetree sub-directory, its
output header name and any naming exceptions. RT266x is the first such
configuration.

The header lives in Zephyr's include/zephyr/dt-bindings/clock directory beside
the SoC-agnostic imx_ccm_rev3.h packed-cell definitions. The HAL fsl_clock.h
remains an explicitly supplied input; normal builds consume the committed
binding header without running this maintenance tool.

Usage:
    gen_ccm_rev3_ids.py --soc rt266x \\
        --fsl-clock <path to devices/.../drivers/fsl_clock.h> \\
        [--output <header to write; defaults to the Zephyr binding path above>] \\
        [--source-version <hal_nxp commit / SDK release, stamped into header>] \\
        [--check]   # do not write; exit 1 on drift against the committed header
"""

import argparse
import importlib
import re
import sys
from pathlib import Path

# --- fsl_clock.h enum parsing (rev3-family, SoC-agnostic) --------------------


def _enum_body(text, enum_name):
    """Return the body between `typedef enum _<enum_name>` and `} <enum_name>_t`,
    or None if that enum is not present (some rev3 SoCs omit clock_lpcg)."""
    m = re.search(
        rf"typedef\s+enum\s+_{re.escape(enum_name)}\b(.*?)\}}\s*{re.escape(enum_name)}_t",
        text,
        re.DOTALL,
    )
    return m.group(1) if m else None


def _valued_entries(body):
    """[(enumerator, int value)] for entries with an explicit = <int>.

    The value is taken from the source, never inferred from position: some roots
    (e.g. NPU, USB1) appear in more than one mux group, each numbering its
    sources from 0, so a positional counter would be wrong.
    """
    out = []
    for name, val in re.findall(r"(kCLOCK_[A-Za-z0-9_]+)\s*=\s*(0[xX][0-9A-Fa-f]+|\d+)", body):
        out.append((name, int(val, 0)))
    return out


def _is_marker(name):
    """Range markers (_START/_END) and invalid sentinels are not exported."""
    n = name.upper()
    return n.endswith("_START") or n.endswith("_END") or n.endswith("INVALID")


def driver_version(fsl_text):
    """Return the FSL clock driver version as 'major.minor.patch', or None.

    Some rev3 SoCs (e.g. i.MX95) ship fsl_clock.h without the version macro;
    the caller records 'n/a' in that case.
    """
    m = re.search(
        r"#define\s+FSL_CLOCK_DRIVER_VERSION\s+\(\s*MAKE_VERSION\s*\(\s*"
        r"(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)\s*\)",
        fsl_text,
    )
    return ".".join(m.groups()) if m else None


def generate_macros(fsl_text, cfg):
    """Return ordered [(macro, value)] for LPCG, ROOT, MUX per the SoC config."""
    macros = []

    def entries(enum, required):
        body = _enum_body(fsl_text, enum)
        if body is None:
            if required:
                raise ValueError(f"required enum _{enum} not found in fsl_clock.h")
            return []
        return _valued_entries(body)

    # LPCG: kCLOCK_<TAIL> -> IMX_CCM_LPCG_<TAIL upper>
    # Optional: some rev3 SoCs (e.g. i.MX95) expose no clock_lpcg enum.
    for name, val in entries("clock_lpcg", required=False):
        if _is_marker(name):
            continue
        macros.append(("IMX_CCM_LPCG_" + cfg.lpcg_tail(name).upper(), val))

    # ROOT: kCLOCK_Root_<TAIL> -> IMX_CCM_ROOT_<TAIL upper>
    for name, val in entries("clock_root", required=True):
        if _is_marker(name):
            continue
        macros.append(("IMX_CCM_ROOT_" + cfg.root_tail(name).upper(), val))

    # MUX: kCLOCK_<ROOT>_ClockRoot_<SOURCE> -> IMX_CCM_MUX_<ROOT>_<SOURCE>
    for name, val in entries("clock_root_mux_source", required=True):
        if _is_marker(name):
            continue
        parsed = cfg.mux_name(name)
        if parsed is None:
            continue
        macros.append(("IMX_CCM_MUX_" + parsed, val))

    return macros


# --- header rendering --------------------------------------------------------


def render_header(macros, cfg, source_version, driver_ver):
    guard = f"ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_IMX_CCM_REV3_{cfg.soc.upper()}_H_"
    src = source_version or "unversioned -- pass --source-version to record it"
    lines = [
        "/*",
        " * SPDX-FileCopyrightText: Copyright 2026 NXP",
        " *",
        " * SPDX-License-Identifier: Apache-2.0",
        " */",
        "",
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        "/**",
        " * @file",
        f" * @brief i.MX {getattr(cfg, 'display', cfg.soc)} clock identifiers "
        "for the nxp,imx-ccm-rev3 controller.",
        " *",
        " * Three independent identifier spaces:",
        " *",
    ]
    lines += [(" * " + ln).rstrip() for ln in cfg.id_space_doc.splitlines()]
    lines += [
        " *",
        " * Peripheral nodes combine a gate and a root with @c IMX_CCM_CLK();",
        " * clock-root child nodes of the controller take a root identifier in @c reg",
        " * and a mux value in @c clock-mux.",
        " *",
        " * Generated from fsl_clock.h (clock_lpcg_t, clock_root_t,",
        " * clock_root_mux_source_t) by",
        " * modules/hal_nxp/scripts/clock/gen_ccm_rev3_ids.py, so the values cannot drift from",
        " * the HAL enumerations the driver passes them to. Do not edit by hand;",
        " * regenerate against a newer device release instead. The _START/_END range",
        " * markers and the invalid sentinels are intentionally not exported -- use",
        " * @c IMX_CCM_GATE_NONE / @c IMX_CCM_ROOT_NONE from",
        " * <zephyr/dt-bindings/clock/imx_ccm_rev3.h> instead.",
        " *",
        f" * FSL clock driver version: {driver_ver or 'n/a'}",
        f" * Source: {src}",
        " */",
        "",
        "#include <zephyr/dt-bindings/clock/imx_ccm_rev3.h>",
        "",
    ]
    # group by prefix, preserving generation order within each group
    for prefix, title in (
        ("IMX_CCM_LPCG_", "Peripheral clock gates (LPCG), from clock_lpcg_t."),
        ("IMX_CCM_ROOT_", "Clock roots, from clock_root_t."),
        ("IMX_CCM_MUX_", "Per-root mux source selectors, from clock_root_mux_source_t."),
    ):
        group = [(n, v) for n, v in macros if n.startswith(prefix)]
        if not group:
            continue
        width = max(len(n) for n, _ in group)
        lines.append(f"/* {title} */")
        for n, v in group:
            lines.append(f"#define {n.ljust(width)} {v}")
        lines.append("")
    lines.append(f"#endif /* {guard} */")
    return "\n".join(lines) + "\n"


def default_output(cfg):
    """Zephyr binding path for this SoC's generated id header."""
    zephyr = Path(__file__).resolve().parents[4]
    return zephyr / "include/zephyr/dt-bindings/clock" / f"imx_ccm_rev3_{cfg.soc}.h"


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("--soc", required=True, help="SoC config name under socs/ (e.g. rt266x)")
    ap.add_argument("--fsl-clock", required=True, type=Path, help="Path to fsl_clock.h")
    ap.add_argument(
        "--output",
        default=None,
        type=Path,
        help="header to write (default: Zephyr include/zephyr/dt-bindings/clock/)",
    )
    ap.add_argument(
        "--source-version",
        default=None,
        help="hal_nxp commit / SDK release, stamped into the header",
    )
    ap.add_argument(
        "--check",
        action="store_true",
        help="do not write; exit 1 if the target differs from freshly generated content",
    )
    args = ap.parse_args()

    try:
        cfg = importlib.import_module(f"socs.{args.soc}").CONFIG
    except ModuleNotFoundError:
        sys.exit(f"error: no SoC config socs/{args.soc}.py")

    output = args.output if args.output is not None else default_output(cfg)

    fsl_text = args.fsl_clock.read_text(encoding="utf-8")
    macros = generate_macros(fsl_text, cfg)
    driver_ver = driver_version(fsl_text)
    content = render_header(macros, cfg, args.source_version, driver_ver)

    if args.check:
        existing = output.read_text(encoding="utf-8") if output.exists() else ""
        if existing != content:
            sys.exit(
                f"error: {output} is out of date "
                f"(fsl_clock.h driver version {driver_ver or 'n/a'}); "
                f"regenerate with gen_ccm_rev3_ids.py"
            )
        print(f"{output}: up to date ({len(macros)} macros, driver version {driver_ver or 'n/a'})")
        return

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(content, encoding="utf-8")
    print(f"wrote {output} ({len(macros)} macros)")


if __name__ == "__main__":
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    main()
