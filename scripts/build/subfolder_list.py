#!/usr/bin/env python3
#
# Copyright (c) 2017 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

"""Write subfolder list to a file

This script will walk the specified directory and write the file specified with
the list of all sub-directories found. If the output file already exists, the
file will only be updated in case sub-directories have been added or removed
since the previous invocation.

"""

import argparse
import os


def parse_args():
    """Parse command line arguments and options"""
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )

    parser.add_argument(
        "-d",
        "--directory",
        required=True,
        action="append",
        help="Directory to walk for sub-directory discovery \
                              (can be specified multiple times)",
    )
    parser.add_argument(
        "-c",
        "--create-links",
        required=False,
        help="Create links for each directory found in \
                              directory given",
    )
    parser.add_argument(
        "-o",
        "--out-file",
        required=True,
        help="File to write containing a list of all \
                              directories found",
    )
    parser.add_argument(
        "-t",
        "--trigger-file",
        required=False,
        help="Trigger file to be touched to re-run CMake",
    )

    args = parser.parse_args()

    return args


def get_subfolder_list(directories, create_links=None):
    """Return subfolder list of directories"""
    dirlist = []
    used_link_names = set()

    for directory in directories:
        if create_links is not None:
            if not os.path.exists(create_links):
                os.makedirs(create_links)
            symbase = os.path.basename(directory)

            # Ensure unique link name for the base directory
            linkname = symbase
            counter = 1
            while linkname in used_link_names:
                linkname = f"{symbase}_{counter}"
                counter += 1
            used_link_names.add(linkname)

            symlink = create_links + os.path.sep + linkname
            if not os.path.exists(symlink):
                os.symlink(directory, symlink)
            dirlist.append(symlink)
        else:
            dirlist.append(directory)

        for root, dirs, _ in os.walk(directory, topdown=True):
            dirs.sort()
            for subdir in dirs:
                if create_links is not None:
                    targetdirectory = os.path.join(root, subdir)
                    reldir = os.path.relpath(targetdirectory, directory)
                    base_linkname = (
                        os.path.basename(directory) + "_" + reldir.replace(os.path.sep, "_")
                    )

                    # Ensure unique link name for subdirectories
                    linkname = base_linkname
                    counter = 1
                    while linkname in used_link_names:
                        linkname = f"{base_linkname}_{counter}"
                        counter += 1
                    used_link_names.add(linkname)

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

    with open(out_file, "w", encoding="utf-8") as out_file_fo:
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
        with open(trigger, "w") as trigger_fo:
            trigger_fo.write("")


def main():
    """Parse command line arguments and take respective actions"""
    args = parse_args()

    directories = sorted(set(args.directory))
    dirs = get_subfolder_list(directories, args.create_links)
    gen_out_file(args.out_file, dirs)

    # Always touch trigger file to ensure json files are updated
    touch(args.trigger_file)


if __name__ == "__main__":
    main()
