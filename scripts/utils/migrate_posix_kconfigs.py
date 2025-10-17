"""
Utility script to migrate Zephyr-based projects to normative POSIX Kconfig options.

This script should be used for migrating from versions of Zephyr older than v3.7.0 to Zephyr
version v3.7.0 or later.

Usage::

    python $ZEPHYR_BASE/scripts/utils/migrate_posix_kconfigs.py --root root_path --dry-run

The utility will process c, cpp, h, hpp, rst, conf, CMakeLists.txt,
yml, yaml and Kconfig files.


Copyright (c) 2022 Nordic Semiconductor ASA
Copyright (c) 2024 Tenstorrent AI ULC
SPDX-License-Identifier: Apache-2.0
"""

import argparse
import re
import sys
from pathlib import Path

ZEPHYR_BASE = Path(__file__).parents[2]

FILE_PATTERNS = (
    r".+\.c", r".+\.cpp", r".+\.hpp", r".+\.h", r".+\.rst", r".+\.conf",
    r".+\.yml", r".+\.yaml", r"CMakeLists.txt", r"Kconfig(\..+)?"
)

REPLACEMENTS = {
    "EVENTFD_MAX": "ZVFS_EVENTFD_MAX",
    "FNMATCH": "POSIX_C_LIB_EXT",
    "GETENTROPY": "POSIX_C_LIB_EXT",
    "GETOPT": "POSIX_C_LIB_EXT",
    "MAX_PTHREAD_COUNT": "POSIX_THREAD_THREADS_MAX",
    "MAX_PTHREAD_KEY_COUNT": "POSIX_THREAD_KEYS_MAX",
    "MAX_TIMER_COUNT": "POSIX_TIMER_MAX",
    "MSG_COUNT_MAX": "POSIX_MQ_OPEN_MAX",
    "POSIX_CLOCK": "POSIX_TIMERS",
    "POSIX_CONFSTR": "POSIX_SINGLE_PROCESS",
    "POSIX_ENV": "POSIX_SINGLE_PROCESS",
    "POSIX_FS": "POSIX_FILE_SYSTEM",
    "POSIX_LIMITS_RTSIG_MAX": "POSIX_RTSIG_MAX",
    "POSIX_MAX_FDS": "ZVFS_OPEN_MAX",
    "POSIX_MAX_OPEN_FILES": "ZVFS_OPEN_MAX",
    "POSIX_MQUEUE": "POSIX_MESSAGE_PASSING",
    "POSIX_PUTMSG": "XOPEN_STREAMS",
    "POSIX_SIGNAL": "POSIX_SIGNALS",
    "POSIX_SYSCONF": "POSIX_SINGLE_PROCESS",
    "POSIX_SYSLOG": "XSI_SYSTEM_LOGGING",
    "POSIX_UNAME": "POSIX_SINGLE_PROCESS",
    "PTHREAD": "POSIX_THREADS",
    "PTHREAD_BARRIER": "POSIX_BARRIERS",
    "PTHREAD_COND": "POSIX_THREADS",
    "PTHREAD_IPC": "POSIX_THREADS",
    "PTHREAD_KEY": "POSIX_THREADS",
    "PTHREAD_MUTEX": "POSIX_THREADS",
    "PTHREAD_RWLOCK": "POSIX_RW_LOCKS",
    "PTHREAD_SPINLOCK": "POSIX_SPIN_LOCKS",
    "TIMER": "POSIX_TIMERS",
    "TIMER_DELAYTIMER_MAX": "POSIX_DELAYTIMER_MAX",
    "SEM_NAMELEN_MAX": "POSIX_SEM_NAME_MAX",
    "SEM_VALUE_MAX": "POSIX_SEM_VALUE_MAX",
}

MESSAGES = {
    "POSIX_CLOCK":
        "POSIX_CLOCK is a one-to-many replacement. If this simple substitution is not "
        "sufficient, it's best to try a combination of POSIX_CLOCK_SELECTION, POSIX_CPUTIME, "
        "POSIX_MONOTONIC_CLOCK, POSIX_TIMERS, and POSIX_TIMEOUTS.",
    "POSIX_MAX_FDS":
        "A read-only version of this symbol is POSIX_OPEN_MAX, which is of course, the standard "
        "symbol. ZVFS_OPEN_MAX may be set by the user. Consider using POSIX_MAX_FDS if the "
        "use-case is read-only.",
}


def process_file(path):
    modified = False
    output = []

    try:
        with open(path) as f:
            lines = f.readlines()

            lineno = 1
            for line in lines:
                longest = ""
                length = 0
                for m in REPLACEMENTS:
                    if re.match(".*" + m + ".*", line) and len(m) > length:
                        length = len(m)
                        longest = m

                # TIMER is just way too frequent an occurrence. Skip it.
                skip = {"TIMER"}
                if length != 0:
                    if longest in skip:
                        pass
                    else:
                        modified = True
                        old = line
                        line = line.replace(longest, REPLACEMENTS[longest])
                        msg = MESSAGES.get(longest)
                        if msg:
                            print(f"{path}:{lineno}:old:{old.strip()}")
                            print(f"{path}:{lineno}:new:{line.strip()}")

                lineno += 1
                output.append(line)

        if modified is False or args.dry_run:
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
        "-r",
        "--root",
        type=Path,
        required=True,
        help="Zephyr-based project path")
    parser.add_argument(
        "-d",
        "--dry-run",
        action="store_true",
        help="log potential changes without modifying files")
    args = parser.parse_args()

    process_tree(args.root)
