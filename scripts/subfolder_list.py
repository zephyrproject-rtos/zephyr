#!/usr/bin/env python3

import os
import argparse

def touch(trigger):
    # If no trigger file is provided then do a return.
    if(trigger is None):
        return

    if os.path.exists(trigger):
        os.utime(trigger, None)
    else:
        with open(trigger, 'w') as fp:
            fp.write("")


def main():
    parser = argparse.ArgumentParser(
        description='This script will walk the specified directory and write the file specified \
                     with the list of all sub-directories found. If to the output file already \
                     exists, the file will only be updated in case sub-directories has been added \
                     or removed since previous invocation.')

    parser.add_argument('-d', '--directory', required=True,
                        help='Directory to walk for sub-directory discovery')
    parser.add_argument('-o', '--out-file', required=True,
                        help='File to write containing a list of all directories found')
    parser.add_argument('-t', '--trigger-file', required=False,
                        help='Trigger file to be be touched to re-run CMake')

    args = parser.parse_args()

    dirlist = []
    dirlist.extend(args.directory)
    dirlist.extend(os.linesep)
    for root, dirs, files in os.walk(args.directory):
        for subdir in dirs:
            dirlist.extend(os.path.join(root, subdir))
            dirlist.extend(os.linesep)

    new      = ''.join(dirlist)
    existing = ''

    if os.path.exists(args.out_file):
        with open(args.out_file, 'r') as fp:
            existing = fp.read()

    if new != existing:
        touch(args.trigger_file)

    # Always write the file to ensure dependencies to changed files are updated
    with open(args.out_file, 'w') as fp:
        fp.write(new)


if __name__ == "__main__":
    main()
