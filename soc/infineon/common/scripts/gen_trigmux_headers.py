#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

r"""Generate the trigger multiplexer dt-bindings header of a part from the
trigger enums of the PDL device configuration header of the part in
hal_infineon.

Usage, from the zephyr directory of a west workspace:

  ./soc/infineon/common/scripts/gen_trigmux_headers.py --part <part> \
    --pdl <PDL device configuration header> [--output <directory>]

The header is written to <directory>/infineon-trigmux-<part>.h. Without
--output, the directory is include/zephyr/dt-bindings/mux/ of the
zephyr tree holding the script.
"""

import argparse
import re
import sys
from pathlib import Path

TRIGGER = re.compile(
    r"^\s+(?P<name>(?P<group>(?:PERI_[01]_)?TRIG_(?:IN|OUT)_(?:MUX|1TO1)_\d+)_\w*)"
    r"\s*=\s*(?P<value>0x[0-9A-Fa-f]+)u"
)

LICENSE = """/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
"""

DEFAULT_OUTPUT = Path(__file__).resolve().parents[4] / "include/zephyr/dt-bindings/mux"


def part_name(value):
    if re.fullmatch(r"[a-z][a-z0-9_]*", value) is None:
        raise argparse.ArgumentTypeError(
            f"'{value}' is not a part name made of lowercase letters, digits and underscores"
        )
    return value


def read_blocks(pdl_header):
    blocks = []

    for line in pdl_header.read_text(encoding="utf-8").splitlines():
        match = TRIGGER.match(line)
        if match is None:
            continue
        if not blocks or blocks[-1][0] != match["group"]:
            blocks.append((match["group"], []))
        blocks[-1][1].append((match["name"], match["value"]))

    return [entries for _, entries in blocks]


def render(part, blocks):
    prefix = f"{part.upper()}_"
    guard = f"ZEPHYR_INCLUDE_DT_BINDINGS_MUX_INFINEON_TRIGMUX_{part.upper()}_H_"
    parts = []

    for entries in blocks:
        width = max(len(prefix + name) for name, _ in entries)
        parts.append(
            "\n".join(f"#define {(prefix + name).ljust(width)} {value}" for name, value in entries)
        )

    return (
        f"{LICENSE}\n"
        f"#ifndef {guard}\n"
        f"#define {guard}\n\n"
        "#include <zephyr/dt-bindings/mux/infineon-trigmux.h>\n\n"
        + "\n\n".join(parts)
        + "\n\n#endif\n"
    )


def parse_args():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    parser.add_argument(
        "--pdl",
        type=Path,
        required=True,
        help="PDL device configuration header of the part, for example tviic2d6m_config.h",
    )
    parser.add_argument(
        "--part",
        type=part_name,
        required=True,
        help="Part name used for the header name and the macro prefix, for example cyt4dn",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT,
        help=f"Output directory (default: {DEFAULT_OUTPUT})",
    )
    return parser.parse_args()


def main():
    args = parse_args()

    if not args.pdl.is_file():
        sys.exit(f"error: {args.pdl} not found")

    blocks = read_blocks(args.pdl)
    if not blocks:
        sys.exit(f"error: no trigger IDs found in {args.pdl}")

    args.output.mkdir(parents=True, exist_ok=True)
    output = args.output / f"infineon-trigmux-{args.part}.h"
    output.write_text(render(args.part, blocks), encoding="utf-8")
    print(f"{output}: {sum(len(entries) for entries in blocks)} trigger IDs")


if __name__ == "__main__":
    main()
