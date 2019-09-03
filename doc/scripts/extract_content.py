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
import collections
import fnmatch
import os
from os import path
import re
import shutil
import sys

# directives to parse for included files
DIRECTIVES = ["figure", "include", "image", "literalinclude"]

# A simple namedtuple for a generated output file.
#
# - src: source file, what file should be copied (in source directory)
# - dst: destination file, path it should be copied to (in build directory)
Output = collections.namedtuple('Output', 'src dst')

# Represents the content which must be extracted from the Zephyr tree,
# as well as the output directories needed to contain it.
#
# - outputs: list of Output objects for extracted content.
# - output_dirs: set of directories which must exist to contain
#                output destination files.
Content = collections.namedtuple('Content', 'outputs output_dirs')


def src_deps(zephyr_base, src_file, dest, src_root):
    # - zephyr_base: the ZEPHYR_BASE directory containing src_file
    # - src_file: path to a source file in the documentation
    # - dest: path to the top-level output/destination directory
    # - src_root: path to the Sphinx top-level source directory
    #
    # Return a list of Output objects which contain src_file's
    # additional dependencies, as they should be copied into
    # dest. Output paths inside dest are based on each
    # dependency's relative path from zephyr_base.

    # Inspect only .rst files for directives referencing other files
    # we'll need to copy (as configured in the DIRECTIVES variable)
    if not src_file.endswith(".rst"):
        return []

    # Load the file's contents, bailing on decode errors.
    try:
        with open(src_file, encoding="utf-8") as f:
            content = [x.strip() for x in f.readlines()]
    except UnicodeDecodeError as e:
        # pylint: disable=unsubscriptable-object
        sys.stderr.write(
            "Malformed {} in {}\n"
            "  Context: {}\n"
            "  Problematic data: {}\n"
            "  Reason: {}\n".format(
                e.encoding, src_file,
                e.object[max(e.start - 40, 0):e.end + 40],
                e.object[e.start:e.end],
                e.reason))
        return []

    # Source file's directory.
    src_dir = path.dirname(src_file)
    # Destination directory for any dependencies.
    dst_dir = path.join(dest, path.relpath(src_dir, start=zephyr_base))

    # Find directives in the content which imply additional
    # dependencies. We assume each such directive takes a single
    # argument, which is a (relative) path to the additional
    # dependency file.
    directives = "|".join(DIRECTIVES)
    pattern = re.compile(r"\.\.\s+(?P<directive>%s)::\s+(?P<dep_rel>.*)" %
                         directives)
    deps = []
    for l in content:
        m = pattern.match(l)
        if not m:
            continue

        dep_rel = m.group('dep_rel')  # relative to src_dir or absolute
        dep_src = path.abspath(path.join(src_dir, dep_rel))
        if path.isabs(dep_rel):
            # Not a relative path, check if it's absolute if we have been
            # provided with a sphinx source directory root
            if not src_root:
                print("Absolute path to file:", dep_rel, "\n  referenced by:",
                      src_file, "with no --sphinx-src-root", file=sys.stderr)
                continue
            # Make it really relative
            dep_rel = '.' + dep_rel
            dep_src = path.abspath(path.join(src_root, dep_rel))
            if path.isfile(dep_src):
                # File found, but no need to copy it since it's part
                # of Sphinx's top-level source directory
                continue
        if not path.isfile(dep_src):
            print("File not found:", dep_src, "\n  referenced by:",
                  src_file, file=sys.stderr)
            continue

        dep_dst = path.abspath(path.join(dst_dir, dep_rel))
        deps.append(Output(dep_src, dep_dst))

    return deps


def find_content(zephyr_base, src, dest, fnfilter, ignore, src_root):
    # Create a list of Outputs to copy over, and new directories we
    # might need to make to contain them. Don't copy any files or
    # otherwise modify dest.
    outputs = []
    output_dirs = set()
    for dirpath, dirnames, filenames in os.walk(path.join(zephyr_base, src)):
        # Limit the rest of the walk to subdirectories that aren't ignored.
        dirnames[:] = [d for d in dirnames if not
                       path.normpath(path.join(dirpath, d)).startswith(ignore)]

        # If the current directory contains no matching files, keep going.
        sources = fnmatch.filter(filenames, fnfilter)
        if not sources:
            continue

        # There are sources here; track that the output directory
        # needs to exist.
        dst_dir = path.join(dest, path.relpath(dirpath, start=zephyr_base))
        output_dirs.add(path.abspath(dst_dir))

        # Initialize an Output for each source file, as well as any of
        # that file's additional dependencies. Make sure output
        # directories for dependencies are tracked too.
        for src_rel in sources:
            src_abs = path.join(dirpath, src_rel)
            deps = src_deps(zephyr_base, src_abs, dest, src_root)

            for depdir in (path.dirname(d.dst) for d in deps):
                output_dirs.add(depdir)

            outputs.extend(deps)
            outputs.append(Output(src_abs,
                                  path.abspath(path.join(dst_dir, src_rel))))

    return Content(outputs, output_dirs)


def extract_content(content):
    # Ensure each output subdirectory exists.
    for d in content.output_dirs:
        os.makedirs(d, exist_ok=True)

    # Create each output file. Use copy2() to avoid updating
    # modification times unnecessarily, as this triggers documentation
    # rebuilds.
    for output in content.outputs:
        shutil.copy2(output.src, output.dst)


def main():
    parser = argparse.ArgumentParser(
        description='''Recursively copy documentation files from ZEPHYR_BASE to
        a destination folder, along with files referenced in those .rst files
        by a configurable list of directives: {}. The ZEPHYR_BASE environment
        variable is used to determine source directories to copy files
        from.'''.format(DIRECTIVES))

    parser.add_argument('--outputs',
                        help='If given, save input/output files to this path')
    parser.add_argument('--just-outputs', action='store_true',
                        help='''Skip extraction and just list outputs.
                        Cannot be given without --outputs.''')
    parser.add_argument('--ignore', action='append',
                        help='''Source directories to ignore when copying
                        files. This may be given multiple times.''')
    parser.add_argument('--sphinx-src-root',
                        help='''If given, absolute paths for dependencies are
                        resolved using this root, which is the Sphinx top-level
                        source directory as passed to sphinx-build.''')

    parser.add_argument('content_config', nargs='+',
                        help='''A glob:source:destination specification
                        for content to extract. The "glob" is a documentation
                        file name pattern to include, "source" is a source
                        directory to search for such files in, and
                        "destination" is the directory to copy it into.''')
    args = parser.parse_args()

    if "ZEPHYR_BASE" not in os.environ:
        sys.exit("ZEPHYR_BASE environment variable undefined.")
    zephyr_base = os.environ["ZEPHYR_BASE"]

    if not args.ignore:
        ignore = ()
    else:
        ignore = tuple(path.normpath(ign) for ign in args.ignore)

    if args.just_outputs and not args.outputs:
        sys.exit('--just-outputs cannot be given without --outputs')

    content_config = [cfg.split(':', 2) for cfg in args.content_config]
    outputs = set()
    for fnfilter, source, dest in content_config:
        content = find_content(zephyr_base, source, dest, fnfilter, ignore,
                               args.sphinx_src_root)
        if not args.just_outputs:
            extract_content(content)
        outputs |= set(content.outputs)
    if args.outputs:
        with open(args.outputs, 'w') as f:
            for o in outputs:
                print(o.src, file=f, end='\n')
                print(o.dst, file=f, end='\n')


if __name__ == "__main__":
    main()
