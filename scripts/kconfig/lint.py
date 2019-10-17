#!/usr/bin/env python3

# Copyright (c) 2019 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

"""
Linter for the Zephyr Kconfig files. Pass --help to see
available checks. By default, all checks are enabled.

Some of the checks rely on heuristics and can get tripped up
by things like preprocessor magic, so manual checking is
still needed. 'git grep' is handy.

Requires west, because the checks need to see Kconfig files
and source code from modules.
"""

import argparse
import os
import re
import shlex
import subprocess
import sys
import tempfile

TOP_DIR = os.path.join(os.path.dirname(__file__), "..", "..")

sys.path.insert(0, os.path.join(TOP_DIR, "scripts", "kconfig"))
import kconfiglib


def main():
    init_kconfig()

    args = parse_args()
    if args.checks:
        checks = args.checks
    else:
        # Run all checks if no checks were specified
        checks = (check_always_n,
                  check_unused,
                  check_pointless_menuconfigs,
                  check_defconfig_only_definition)

    first = True
    for check in checks:
        if not first:
            print()
        first = False
        check()


def parse_args():
    # args.checks is set to a list of check functions to run

    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawTextHelpFormatter,
        description=__doc__)

    parser.add_argument(
        "-n", "--check-always-n",
        action="append_const", dest="checks", const=check_always_n,
        help="""\
List symbols that can never be anything but n/empty. These
are detected as symbols with no prompt or defaults that
aren't selected or implied.
""")

    parser.add_argument(
        "-u", "--check-unused",
        action="append_const", dest="checks", const=check_unused,
        help="""\
List symbols that might be unused.

Heuristic:

 - Isn't referenced in Kconfig
 - Isn't referenced as CONFIG_<NAME> outside Kconfig
   (besides possibly as CONFIG_<NAME>=<VALUE>)
 - Isn't selecting/implying other symbols
 - Isn't a choice symbol

C preprocessor magic can trip up this check.""")

    parser.add_argument(
        "-m", "--check-pointless-menuconfigs",
        action="append_const", dest="checks", const=check_pointless_menuconfigs,
        help="""\
List symbols defined with 'menuconfig' where the menu is
empty due to the symbol not being followed by stuff that
depends on it""")

    parser.add_argument(
        "-d", "--check-defconfig-only-definition",
        action="append_const", dest="checks", const=check_defconfig_only_definition,
        help="""\
List symbols that are only defined in Kconfig.defconfig
files. A common base definition should probably be added
somewhere for such symbols, and the type declaration ('int',
'hex', etc.) removed from Kconfig.defconfig.""")

    return parser.parse_args()


def check_always_n():
    print_header("Symbols that can't be anything but n/empty")
    for sym in kconf.unique_defined_syms:
        if not has_prompt(sym) and not is_selected_or_implied(sym) and \
           not has_defaults(sym):
            print(name_and_locs(sym))


def check_unused():
    print_header("Symbols that look unused")
    referenced = referenced_sym_names()
    for sym in kconf.unique_defined_syms:
        if not is_selecting_or_implying(sym) and not sym.choice and \
           sym.name not in referenced:
            print(name_and_locs(sym))


def check_pointless_menuconfigs():
    print_header("menuconfig symbols with empty menus")
    for node in kconf.node_iter():
        if node.is_menuconfig and not node.list and \
           isinstance(node.item, kconfiglib.Symbol):
            print("{0.item.name:40} {0.filename}:{0.linenr}".format(node))


def check_defconfig_only_definition():
    print_header("Symbols only defined in Kconfig.defconfig files")
    for sym in kconf.unique_defined_syms:
        if all("defconfig" in node.filename for node in sym.nodes):
            print(name_and_locs(sym))


def print_header(s):
    print(s + "\n" + len(s)*"=")


def init_kconfig():
    global kconf

    os.environ.update(
        srctree=TOP_DIR,
        CMAKE_BINARY_DIR=modules_file_dir(),
        KCONFIG_DOC_MODE="1",
        ZEPHYR_BASE=TOP_DIR,
        SOC_DIR="soc",
        ARCH_DIR="arch",
        BOARD_DIR="boards/*/*",
        ARCH="*")

    kconf = kconfiglib.Kconfig()


def modules_file_dir():
    # Creates Kconfig.modules in a temporary directory and returns the path to
    # the directory. Kconfig.modules brings in Kconfig files from modules.

    tmpdir = tempfile.mkdtemp()
    run((os.path.join("scripts", "zephyr_module.py"),
         "--kconfig-out", os.path.join(tmpdir, "Kconfig.modules")))
    return tmpdir


def referenced_sym_names():
    # Returns the names of all symbols referenced inside and outside the
    # Kconfig files (that we can detect), without any "CONFIG_" prefix

    return referenced_in_kconfig() | referenced_outside_kconfig()


def referenced_in_kconfig():
    # Returns the names of all symbols referenced inside the Kconfig files

    return {ref.name
            for node in kconf.node_iter()
                for ref in node.referenced
                    if isinstance(ref, kconfiglib.Symbol)}


def referenced_outside_kconfig():
    # Returns the names of all symbols referenced outside the Kconfig files

    regex = r"\bCONFIG_[A-Z0-9_]+\b"

    res = set()

    # 'git grep' all modules
    for modpath in run(("west", "list", "-f{abspath}")).splitlines():
        for line in run(("git", "grep", "-h", "--extended-regexp", regex),
                        cwd=modpath).splitlines():
            # Don't record lines starting with "CONFIG_FOO=" or "# CONFIG_FOO="
            # as references, so that symbols that are only assigned in .config
            # files are not included
            if re.match(r"[\s#]*CONFIG_[A-Z0-9_]+=.*", line):
                continue

            # Could pass --only-matching to git grep as well, but it was added
            # pretty recently (2018)
            for match in re.findall(regex, line):
                res.add(match[7:])  # Strip "CONFIG_"

    return res


def has_prompt(sym):
    return any(node.prompt for node in sym.nodes)


def is_selected_or_implied(sym):
    return sym.rev_dep is not kconf.n or sym.weak_rev_dep is not kconf.n


def has_defaults(sym):
    return bool(sym.defaults)


def is_selecting_or_implying(sym):
    return sym.selects or sym.implies


def name_and_locs(sym):
    # Returns a string with the name and definition location(s) for 'sym'

    return "{:40} {}".format(
        sym.name,
        ", ".join("{0.filename}:{0.linenr}".format(node) for node in sym.nodes))


def run(cmd, cwd=TOP_DIR):
    # Runs 'cmd' with subprocess, returning the decoded stdout output. 'cwd' is
    # the working directory. It defaults to the top-level Zephyr directory.

    cmd_s = " ".join(shlex.quote(word) for word in cmd)

    try:
        process = subprocess.Popen(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=cwd)
    except FileNotFoundError:
        sys.exit("'{}' not found".format(cmd[0]))
    except OSError as e:
        sys.exit("Failed to run '{}': {}".format(cmd_s, e))

    stdout, stderr = process.communicate()
    if process.returncode or stderr:
        sys.exit("'{}' exited with status {}.\n\nstdout:\n{}\n\nstderr:\n{}"
                 .format(cmd_s, process.returncode,
                         stdout.decode("utf-8"), stderr.decode("utf-8")))

    return stdout.decode("utf-8")


if __name__ == "__main__":
    main()
