#!/usr/bin/env python3
#
# Copyright (c) 2017 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

"""Write subfolder list to a file

This script will walk the specified directories and write the file specified with
the list of all sub-directories found. If the output file already exists, the
file will only be updated in case sub-directories have been added or removed
since the previous invocation.

"""

import argparse
import hashlib
import os


def parse_args():
    """Parse command line arguments and options"""
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )

    parser.add_argument(
        '-d',
        '--directory',
        required=True,
        action='append',
        help='Directory to walk for sub-directory discovery (may be repeated)',
    )
    parser.add_argument(
        '-c',
        '--create-links',
        required=False,
        help='Create links for each directory found in directory given',
    )
    parser.add_argument(
        '-o',
        '--out-file',
        required=True,
        help='File to write containing a list of all directories found',
    )
    parser.add_argument(
        '--exclude-directory',
        action='append',
        default=[],
        help='Subdirectory to omit during discovery (may be repeated)',
    )
    parser.add_argument(
        '-t', '--trigger-file', required=False, help='Trigger file to be touched to re-run CMake'
    )

    args = parser.parse_args()

    return args


def get_subfolder_list(directory, create_links=None, exclude_dirs=()):
    """Return directories, pruning exclusions below explicit scan roots."""
    if not os.path.isdir(directory):
        return []

    dirlist = []
    excluded_dirs = {os.path.realpath(path) for path in exclude_dirs}
    if create_links is not None:
        excluded_dirs.add(os.path.realpath(create_links))

    if create_links is not None:
        if not os.path.exists(create_links):
            os.makedirs(create_links)
        symbase = os.path.basename(directory)
        symlink = create_links + os.path.sep + symbase
        if not os.path.exists(symlink):
            os.symlink(directory, symlink)
        dirlist.append(symlink)
    else:
        dirlist.append(directory)

    for root, dirs, _ in os.walk(directory, topdown=True):
        dirs[:] = sorted(
            subdir
            for subdir in dirs
            if subdir != '.git'
            and os.path.realpath(os.path.join(root, subdir)) not in excluded_dirs
        )
        for subdir in dirs:
            if create_links is not None:
                targetdirectory = os.path.join(root, subdir)
                reldir = os.path.relpath(targetdirectory, directory)
                linkname = symbase + '_' + reldir.replace(os.path.sep, '_')
                symlink = create_links + os.path.sep + linkname
                if not os.path.exists(symlink):
                    os.symlink(targetdirectory, symlink)
                dirlist.append(symlink)
            else:
                dirlist.append(os.path.join(root, subdir))

    return dirlist


def gen_out_file(out_file, dirs):
    """Generate file with the list of directories

    File won't be updated if it already exists and has the same content

    """
    dirs_nl = "\n".join(dirs) + "\n"

    if os.path.exists(out_file):
        with open(out_file, encoding="utf-8") as out_file_fo:
            out_file_dirs = out_file_fo.read()

        if out_file_dirs == dirs_nl:
            return

    with open(out_file, 'w', encoding="utf-8") as out_file_fo:
        out_file_fo.writelines(dirs_nl)


def touch(trigger):
    """Touch the trigger file

    If no trigger file is provided then do a return.

    """
    if trigger is None:
        return

    if os.path.exists(trigger):
        os.utime(trigger, None)
    else:
        with open(trigger, 'w') as trigger_fo:
            trigger_fo.write("")


def main():
    """Parse command line arguments and take respective actions"""
    args = parse_args()

    exclude_dirs = args.exclude_directory
    if args.create_links is not None:
        exclude_dirs = [*exclude_dirs, args.create_links]

    dirs = []
    for directory in sorted(set(args.directory)):
        links = args.create_links
        if links is not None:
            # Keep each root's links separate and stable when roots are added or removed.
            root_id = hashlib.sha256(os.path.abspath(directory).encode()).hexdigest()
            links = os.path.join(links, root_id)
        dirs.extend(get_subfolder_list(directory, links, exclude_dirs))
    gen_out_file(args.out_file, dirs)

    # Always touch trigger file to ensure json files are updated
    touch(args.trigger_file)


if __name__ == "__main__":
    main()
