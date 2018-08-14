#!/usr/bin/env python3
#
# Copyright (c) 2018, Nordic Semiconductor ASA
# Copyright (c) 2017, Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

# Very quick script to move docs from different places into the doc directory
# to fix the website and external links

import argparse
import errno
import filecmp
import fnmatch
import os
import re
import shutil
import sys

# directives to parse for included files
DIRECTIVES = ["figure","include","image","literalinclude"]

if "ZEPHYR_BASE" not in os.environ:
    sys.stderr.write("$ZEPHYR_BASE environment variable undefined.\n")
    exit(1)
ZEPHYR_BASE = os.environ["ZEPHYR_BASE"]

if "ZEPHYR_BUILD" in os.environ:
    ZEPHYR_BUILD = os.environ["ZEPHYR_BUILD"]
else:
    ZEPHYR_BUILD = None

def copy_if_different(src, dst):
    # Copies 'src' as 'dst', but only if dst does not exist or if itx contents
    # differ from src.This avoids unnecessary # timestamp updates, which
    # trigger documentation rebuilds.
    if os.path.exists(dst) and filecmp.cmp(src, dst):
        return
    shutil.copyfile(src, dst)

def get_files(all, dest, dir):
    matches = []
    for root, dirnames, filenames in os.walk('%s/%s' %(ZEPHYR_BASE, dir)):
        if ZEPHYR_BUILD:
            if os.path.normpath(root).startswith(os.path.normpath(ZEPHYR_BUILD)):
                # Build folder, skip it
                continue

        for filename in fnmatch.filter(filenames, '*' if all else '*.rst'):
            matches.append(os.path.join(root, filename))
    for file in matches:
        frel = file.replace(ZEPHYR_BASE,"").strip("/")
        dir=os.path.dirname(frel)
        if not os.path.exists(os.path.join(dest, dir)):
            os.makedirs(os.path.join(dest, dir))

        copy_if_different(file, os.path.join(dest, frel))

        # Inspect only .rst files for directives referencing other files
        # we'll need to copy (as configured in the DIRECTIVES variable)
        if not fnmatch.fnmatch(file, "*.rst"):
            continue

        try:
            with open(file, encoding="utf-8") as f:
                content = f.readlines()

            content = [x.strip() for x in content]
            directives = "|".join(DIRECTIVES)
            pattern = re.compile("\s*\.\.\s+(%s)::\s+(.*)" %directives)
            for l in content:
                m = pattern.match(l)
                if m:
                    inf = m.group(2)
                    ind = os.path.dirname(inf)
                    if not os.path.exists(os.path.join(dest, dir, ind)):
                        os.makedirs(os.path.join(dest, dir, ind))

                    src = os.path.join(ZEPHYR_BASE, dir, inf)
                    dst = os.path.join(dest, dir, inf)
                    try:
                        copy_if_different(src, dst)

                    except FileNotFoundError:
                        sys.stderr.write("File not found: %s\n  reference by %s\n" % (inf, file))

        except UnicodeDecodeError as e:
            sys.stderr.write(
                "Malformed {} in {}\n"
                "  Context: {}\n"
                "  Problematic data: {}\n"
                "  Reason: {}\n".format(
                    e.encoding, file,
                    e.object[max(e.start - 40, 0):e.end + 40],
                    e.object[e.start:e.end],
                    e.reason))

        f.close()

def main():

    parser = argparse.ArgumentParser(description='Recursively copy .rst files '
                                     'from the origin folder(s) to the '
                                     'destination folder, plus files referenced '
                                     'in those .rst files by a configurable '
                                     'list of directives: {}.'.format(DIRECTIVES))

    parser.add_argument('-a', '--all', action='store_true', help='Copy all files '
                        '(recursively) in the specified source folder(s).')
    parser.add_argument('dest', nargs=1)
    parser.add_argument('src', nargs='+')
    args = parser.parse_args()

    dest = args.dest[0]

    for d in args.src:
        get_files(args.all, dest, d)

if __name__ == "__main__":
    main()
