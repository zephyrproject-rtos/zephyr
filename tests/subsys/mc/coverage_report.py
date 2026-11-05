# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Summarize Micro Core production line coverage from Twister's gcovr XML."""

import argparse
import xml.etree.ElementTree as ET

SOURCES = ("subsys/micro_core/comm.c", "subsys/micro_core/msg.c")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("xml", help="Twister coverage/coverage.xml")
    parser.add_argument("--min-line-percent", type=float, default=0.0)
    args = parser.parse_args()
    root = ET.parse(args.xml).getroot()
    failed = False

    for source in SOURCES:
        matches = [item for item in root.findall(".//class")
                   if item.get("filename") == source]
        if len(matches) != 1:
            parser.error(f"expected one coverage entry for {source}, found {len(matches)}")
        lines = matches[0].findall("./lines/line")
        if not lines:
            parser.error(f"no executable lines found for {source}")
        covered = sum(int(line.get("hits", "0")) > 0 for line in lines)
        percent = 100.0 * covered / len(lines)
        print(f"{source}: {covered}/{len(lines)} lines ({percent:.2f}%)")
        if percent < args.min_line_percent:
            failed = True

    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
