"""
Utility script to migrate Zephyr-based projects to new MCUmgr Kconfig options

Usage::

    python $ZEPHYR_BASE/scripts/utils/migrate_mcumgr_kconfigs.py -r root_path

The utility will process c, cpp, h, hpp, rst, conf, CMakeLists.txt,
yml, yaml and Kconfig files.


Copyright (c) 2022 Nordic Semiconductor ASA
SPDX-License-Identifier: Apache-2.0
"""

import argparse
from pathlib import Path
import re
import sys


ZEPHYR_BASE = Path(__file__).parents[2]

FILE_PATTERNS = (
    r".+\.c", r".+\.cpp", r".+\.hpp", r".+\.h", r".+\.rst", r".+\.conf",
    r".+\.yml", r".+\.yaml", r"CMakeLists.txt", r"Kconfig(\..+)?"
)

REPLACEMENTS = {
    "MCUMGR_SMP_BT_AUTHEN" : "MCUMGR_TRANSPORT_BT_PERM_RW_AUTHEN",
    "MCUMGR_TRANSPORT_BT_AUTHEN" : "MCUMGR_TRANSPORT_BT_PERM_RW_AUTHEN",
}

def process_file(path):
    modified = False
    output = []

    try:
        with open(path) as f:
            lines = f.readlines()

            for line in lines:
                longest = ""
                length = 0
                for m in REPLACEMENTS:
                    if re.match(".*" + m + ".*", line) and len(m) > length:
                        length = len(m)
                        longest = m

                if length != 0:
                    modified = True
                    line = line.replace(longest, REPLACEMENTS[longest])

                output.append(line)

        if modified is False:
            return

        with open(path, "w") as f:
            f.writelines(output)

    except UnicodeDecodeError:
        print(f"Unable to read lines from {path}", file=sys.stderr)
    except Exception as e:
        print(f"Failed with exception {e}", e)

def process_tree(project):
    for p in project.glob("**/*"):
        for fp in FILE_PATTERNS:
            cfp = re.compile(".+/" + fp + "$")
            if re.match(cfp, str(p)) is not None:
                process_file(p)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument(
        "-r", "--root", type=Path, required=True, help="Zephyr-based project path"
    )
    args = parser.parse_args()

    process_tree(args.root)
