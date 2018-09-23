# Zephyr launcher which is interoperable with:
#
# 1. "mono-repo" Zephyr installations that have 'make flash'
#    etc. supplied by a copy of some west code in scripts/meta.
#
# 2. "multi-repo" Zephyr installations where west is provided in a
#    separate Git repository elsewhere.
#
# This is basically a copy of the "wrapper" functionality in the west
# bootstrap script for the multi-repo case, plus a fallback onto the
# copy in scripts/meta/west for mono-repo installs.

import os
import subprocess
import sys

if sys.version_info < (3,):
    sys.exit('fatal error: you are running Python 2')

# Top-level west directory, containing west itself and the manifest.
WEST_DIR = 'west'
# Subdirectory to check out the west source repository into.
WEST = 'west'
# File inside of WEST_DIR which marks it as the top level of the
# Zephyr project installation.
#
# (The WEST_DIR name is not distinct enough to use when searching for
# the top level; other directories named "west" may exist elsewhere,
# e.g. zephyr/doc/west.)
WEST_MARKER = '.west_topdir'


class WestError(RuntimeError):
    pass


class WestNotFound(WestError):
    '''Neither the current directory nor any parent has a West installation.'''


def find_west_topdir(start):
    '''Find the top-level installation directory, starting at ``start``.

    If none is found, raises WestNotFound.'''
    cur_dir = start

    while True:
        if os.path.isfile(os.path.join(cur_dir, WEST_DIR, WEST_MARKER)):
            return cur_dir

        parent_dir = os.path.dirname(cur_dir)
        if cur_dir == parent_dir:
            # At the root
            raise WestNotFound('Could not find a West installation '
                               'in this or any parent directory')
        cur_dir = parent_dir


def append_to_pythonpath(directory):
    pp = os.environ.get('PYTHONPATH')
    os.environ['PYTHONPATH'] = ':'.join(([pp] if pp else []) + [directory])


def wrap(topdir, argv):
    # Replace the wrapper process with the "real" west

    # sys.argv[1:] strips the argv[0] of the wrapper script itself
    west_git_repo = os.path.join(topdir, WEST_DIR, WEST)
    argv = ([sys.executable,
             os.path.join(west_git_repo, 'src', 'west', 'main.py')] +
            argv)

    try:
        append_to_pythonpath(os.path.join(west_git_repo, 'src'))
        subprocess.check_call(argv)
    except subprocess.CalledProcessError as e:
        sys.exit(e.returncode)


def run_scripts_meta_west():
    try:
        subprocess.check_call([sys.executable,
                               os.path.join(os.environ['ZEPHYR_BASE'],
                                            'scripts', 'meta', 'west',
                                            'main.py')] + sys.argv[1:])
    except subprocess.CalledProcessError as e:
        sys.exit(e.returncode)


def main():
    try:
        topdir = find_west_topdir(__file__)
    except WestNotFound:
        topdir = None

    if topdir is not None:
        wrap(topdir, sys.argv[1:])
    else:
        run_scripts_meta_west()


if __name__ == '__main__':
    main()
