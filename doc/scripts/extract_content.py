#!/usr/bin/env python3
#
# Copyright (c) 2018, Foundries.io Ltd
# Copyright (c) 2018, Nordic Semiconductor ASA
# Copyright (c) 2017, Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

# Internal script used by the documentation's build system to create
# the "final" docs tree which is then compiled by Sphinx.
#
# This works around the fact that Sphinx needs a single documentation
# root directory, while Zephyr's documentation files are spread around
# the tree.

import argparse
import filecmp
import fnmatch
import os
import re
import shutil
import sys

# directives to parse for included files
DIRECTIVES = ["figure", "include", "image", "literalinclude"]


def copy_if_different(src, dst):
    # Copies 'src' as 'dst', but only if dst does not exist or if itx contents
    # differ from src.This avoids unnecessary # timestamp updates, which
    # trigger documentation rebuilds.
    if os.path.exists(dst) and filecmp.cmp(src, dst):
        return
    shutil.copyfile(src, dst)


def get_files(zephyr_base, src, dest, fnfilter, ignore):
    matches = []
    for root, dirnames, filenames in os.walk('%s/%s' % (zephyr_base, src)):
        # Append any matching files.
        for filename in fnmatch.filter(filenames, fnfilter):
            matches.append(os.path.join(root, filename))

        # Limit the rest of the walk to subdirectories that aren't ignored.
        dirnames[:] = [d for d in dirnames if not
                       os.path.join(root, d).startswith(ignore)]

    for src_file in matches:
        frel = src_file.replace(zephyr_base, "").strip("/")
        dir = os.path.dirname(frel)
        if not os.path.exists(os.path.join(dest, dir)):
            os.makedirs(os.path.join(dest, dir))

        copy_if_different(src_file, os.path.join(dest, frel))

        # Inspect only .rst files for directives referencing other files
        # we'll need to copy (as configured in the DIRECTIVES variable)
        if not fnmatch.fnmatch(src_file, "*.rst"):
            continue

        try:
            with open(src_file, encoding="utf-8") as f:
                content = f.readlines()

            content = [x.strip() for x in content]
            directives = "|".join(DIRECTIVES)
            pattern = re.compile("\s*\.\.\s+(%s)::\s+(.*)" % directives)
            for l in content:
                m = pattern.match(l)
                if m:
                    inf = m.group(2)
                    ind = os.path.dirname(inf)
                    if not os.path.exists(os.path.join(dest, dir, ind)):
                        os.makedirs(os.path.join(dest, dir, ind))

                    src = os.path.join(zephyr_base, dir, inf)
                    dst = os.path.join(dest, dir, inf)
                    try:
                        copy_if_different(src, dst)

                    except FileNotFoundError:
                        print("File not found:", inf, "\n  referenced by:",
                              src_file, file=sys.stderr)

        except UnicodeDecodeError as e:
            sys.stderr.write(
                "Malformed {} in {}\n"
                "  Context: {}\n"
                "  Problematic data: {}\n"
                "  Reason: {}\n".format(
                    e.encoding, src_file,
                    e.object[max(e.start - 40, 0):e.end + 40],
                    e.object[e.start:e.end],
                    e.reason))

        f.close()


def main():
    parser = argparse.ArgumentParser(
        description='''Recursively copy .rst files from the origin folder(s) to
        the destination folder, plus files referenced in those .rst files by a
        configurable list of directives: {}. The ZEPHYR_BASE environment
        variable is used to determine source directories to copy files
        from.'''.format(DIRECTIVES))

    parser.add_argument('-a', '--all', action='store_true',
                        help='''Copy all files (recursively) in the specified
                        source folder(s).''')
    parser.add_argument('--ignore', action='append',
                        help='''Source directories to ignore when copying
                        files. This may be given multiple times.''')
    parser.add_argument('dest', nargs=1)
    parser.add_argument('src', nargs='+')
    args = parser.parse_args()

    if "ZEPHYR_BASE" not in os.environ:
        sys.exit("ZEPHYR_BASE environment variable undefined.")
    zephyr_base = os.environ["ZEPHYR_BASE"]

    dest = args.dest[0]
    if not args.ignore:
        ignore = ()
    else:
        ignore = tuple(os.path.normpath(ign) for ign in args.ignore)
    if args.all:
        fnfilter = '*'
    else:
        fnfilter = '*.rst'

    for d in args.src:
        get_files(zephyr_base, d, dest, fnfilter, ignore)


if __name__ == "__main__":
    main()
