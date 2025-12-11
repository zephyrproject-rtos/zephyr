# Copyright (c) 2025 Arduino SA
# SPDX-License-Identifier: Apache-2.0

import json
import os
import pickle
import sys

# fmt: off

# Indices for the fields in the trace entries
# Keep these in sync with the generation code in scripts/kconfig/kconfig.py!
(
    SYM_NAME,
    SYM_VIS,
    SYM_TYPE,
    SYM_VALUE,
    SYM_KIND,
    SYM_LOC
) = range(6)

EXPECTED_TRACES = [
    [
        "CONFIG_BOARD",
        "n",
        "string",
        "native_sim",
        "default",
        [ "zephyr/boards/Kconfig", None ] # only test the file name
    ],
    [
        "CONFIG_MAIN_FLAG",
        "n",
        "bool",
        "y",
        "select",
        [ "MAIN_FLAG_SELECT" ]
    ],
    [
        "CONFIG_MAIN_FLAG_DEPENDENCY",
        "y",
        "bool",
        "y",
        "default",
        [ "tests/kconfig/tracing/Kconfig", 14 ]
    ],
    [
        "CONFIG_MAIN_FLAG_SELECT",
        "y",
        "bool",
        "y",
        "assign",
        [ "tests/kconfig/tracing/prj.conf", 7 ]
    ],
    [
        "CONFIG_SECOND_CHOICE",
        "y",
        "bool",
        "y",
        "default",
        None
    ],
    [
        "CONFIG_UNSET_FLAG",
        "y",
        "bool",
        None,
        "unset",
        None
    ],
]

# fmt: on


def compare_entry(actual, expected):
    for field in SYM_NAME, SYM_VIS, SYM_TYPE, SYM_VALUE, SYM_KIND:
        if actual[field] != expected[field]:
            return False

    if expected[SYM_KIND] in ("imply", "select") or expected[SYM_LOC] is None:
        # list of strings or None, compare directly
        if actual[SYM_LOC] != expected[SYM_LOC]:
            return False
    else:
        # file reference, trim input path
        if not isinstance(actual[SYM_LOC], list | tuple) or len(actual[SYM_LOC]) != 2:
            return False

        expected_path, expected_line = expected[SYM_LOC]
        actual_path, actual_line = actual[SYM_LOC]

        if not os.path.normpath(actual_path).endswith(expected_path):
            return False

        if expected_line is not None and expected_line != actual_line:
            return False

    return True


def compare_traces(source, actual_list, expected_list):
    result = True
    for expected in expected_list:
        actual = next((sr for sr in actual_list if sr[0] == expected[0]), None)
        if not actual:
            print(f"Missing value traces for {expected[0]}")
            result = False
            continue

        if not compare_entry(actual, expected):
            print(f"{source}: ERROR: value traces mismatch for {expected[0]}:")
            print("  expected:", expected)
            print("    actual:", actual)
            result = False
            continue

    if result:
        print(f"{source}: value traces match expected values.")
    else:
        print(f"{source}: ERROR: Validation failed.")
        sys.exit(1)


def main():
    if len(sys.argv) != 2:
        print("Usage: validate_traces.py <dotconfig>")
        sys.exit(1)

    dotconfig = sys.argv[1]

    with open(dotconfig + "-trace.json") as f:
        traces = json.load(f)
        compare_traces("json", traces, EXPECTED_TRACES)

    with open(dotconfig + "-trace.pickle", 'rb') as f:
        traces = pickle.load(f)
        compare_traces("pickle", traces, EXPECTED_TRACES)


if __name__ == "__main__":
    main()
