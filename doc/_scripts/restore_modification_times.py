#!/usr/bin/env python3
#
# Copyright (c) 2019, Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0


import argparse
import shutil
import os
import sys
import filecmp
import logging


def main():
    parser = argparse.ArgumentParser(
        description="""Maintains a shadow copy of generated files. Restores their previous
	modification times when their content hasn't changed. This stops
	build tools assuming generated files have changed when their
	content has not.  Doxygen for instance doesn't support
	incremental builds and regenerates (XML,...) files which seem
	new even when they haven't changed at all. This breaks
	incremental builds for tools processing its output further.
	Skips: %s.""" % filecmp.DEFAULT_IGNORES
    )

    parser.add_argument("newer", help="Location of the generated files monitored")
    parser.add_argument("shadow_dir", help="backup location", nargs='?')
    parser.add_argument("-l", "--loglevel", help="python logging level",
                        default="ERROR")

    args = parser.parse_args()

    # At the INFO level, running twice back to back should print nothing
    # the second time.
    logging.basicConfig(level=getattr(logging, args.loglevel))

    # Strip any trailing slash
    args.newer = os.path.normpath(args.newer)

    if args.shadow_dir is None:
        args.shadow_dir = args.newer + "_shadow_files_stats"

    os.makedirs(args.shadow_dir, exist_ok=True)

    save_filestats_restore_mtimes(filecmp.dircmp(args.newer, args.shadow_dir))


def save_filestats_restore_mtimes(dcmp):
    "left = newer, right = shadow backup"

    for same_f in dcmp.same_files:
        restore_older_mtime(os.path.join(dcmp.left, same_f),
                            os.path.join(dcmp.right, same_f))

    for name in dcmp.left_only + dcmp.diff_files:
        logging.info("Saving new object(s) to %s ",
                     os.path.join(dcmp.right, name))
        rsync(os.path.join(dcmp.left, name),
              os.path.join(dcmp.right, name))

    for name in dcmp.right_only:
        obsolete = os.path.join(dcmp.right, name)
        if os.path.isdir(obsolete) and not os.path.islink(obsolete):
            logging.info("Cleaning up dir %s ", obsolete)
            shutil.rmtree(obsolete)
        else:
            logging.info("Cleaning up file or link %s ", obsolete)
            os.remove(obsolete)

    for sub_dcmp in dcmp.subdirs.values():
        logging.debug("Recursing into %s", sub_dcmp.left)
        save_filestats_restore_mtimes(sub_dcmp)


def restore_older_mtime(newer, shadow):
    newer_stat = os.lstat(newer)
    newer_mtime = newer_stat.st_mtime_ns
    shadow_mtime = os.lstat(shadow).st_mtime_ns
    if shadow_mtime == newer_mtime:
        logging.debug("Nothing to do for %s ", newer)
        return
    if shadow_mtime < newer_mtime:
        logging.debug("Restoring mtime of unchanged %s ", newer)
        os.utime(newer, ns=(newer_stat.st_atime_ns, shadow_mtime))
        return
    if shadow_mtime > newer_mtime:
        logging.error("Newer modified time on shadow file %s!", shadow)
        sys.exit("Corrupted shadow, aborting.")


def rsync(src, dest):
    if os.path.islink(src):
        linkto = os.readlink(src)
        os.symlink(linkto, dest)
    elif os.path.isdir(src):
        shutil.copytree(src, dest, symlinks=True)
    else:
        shutil.copy2(src, dest)


if __name__ == "__main__":
    main()
