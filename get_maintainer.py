#!/usr/bin/env python3

# Copyright (c) 2019 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

"""
Lists maintainers for files or commits. Similar in function to
scripts/get_maintainer.pl from Linux, but geared towards GitHub. The mapping is
in MAINTAINERS.yml.

The comment at the top of MAINTAINERS.yml in Zephyr documents the file format.

See the help texts for the various subcommands for more information. They can
be viewed with e.g.

    ./get_maintainer.py path --help

This executable doubles as a Python library. Identifiers not prefixed with '_'
are part of the library API. The library documentation can be viewed with this
command:

    $ pydoc get_maintainer
"""

import argparse
import operator
import os
import pathlib
import re
import shlex
import subprocess
import sys

from yaml import load, YAMLError
try:
    # Use the speedier C LibYAML parser if available
    from yaml import CLoader as Loader
except ImportError:
    from yaml import Loader


def _main():
    # Entry point when run as an executable

    args = _parse_args()
    try:
        args.cmd_fn(Maintainers(args.maintainers), args)
    except (MaintainersError, GitError) as e:
        _serr(e)


def _parse_args():
    # Parses arguments when run as an executable

    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=__doc__)

    parser.add_argument(
        "-m", "--maintainers",
        metavar="MAINTAINERS_FILE",
        help="Maintainers file to load. If not specified, MAINTAINERS.yml in "
             "the top-level repository directory is used, and must exist. "
             "Paths in the maintainers file will always be taken as relative "
             "to the top-level directory.")

    subparsers = parser.add_subparsers(
        help="Available commands (each has a separate --help text)")

    id_parser = subparsers.add_parser(
        "path",
        help="List area(s) for paths")
    id_parser.add_argument(
        "paths",
        metavar="PATH",
        nargs="*",
        help="Path to list areas for")
    id_parser.set_defaults(cmd_fn=Maintainers._path_cmd)

    commits_parser = subparsers.add_parser(
        "commits",
        help="List area(s) for commit range")
    commits_parser.add_argument(
        "commits",
        metavar="COMMIT_RANGE",
        nargs="*",
        help="Commit range to list areas for (default: HEAD~..)")
    commits_parser.set_defaults(cmd_fn=Maintainers._commits_cmd)

    list_parser = subparsers.add_parser(
        "list",
        help="List files in areas")
    list_parser.add_argument(
        "area",
        metavar="AREA",
        nargs="?",
        help="Name of area to list files in. If not specified, all "
             "non-orphaned files are listed (all files that do not appear in "
             "any area).")
    list_parser.set_defaults(cmd_fn=Maintainers._list_cmd)

    orphaned_parser = subparsers.add_parser(
        "orphaned",
        help="List orphaned files (files that do not appear in any area)")
    orphaned_parser.add_argument(
        "path",
        metavar="PATH",
        nargs="?",
        help="Limit to files under PATH")
    orphaned_parser.set_defaults(cmd_fn=Maintainers._orphaned_cmd)

    args = parser.parse_args()
    if not hasattr(args, "cmd_fn"):
        # Called without a subcommand
        sys.exit(parser.format_usage().rstrip())

    return args


class Maintainers:
    """
    Represents the contents of a maintainers YAML file.

    These attributes are available:

    areas:
        A dictionary that maps area names to Area instances, for all areas
        defined in the maintainers file

    filename:
        The path to the maintainers file
    """
    def __init__(self, filename=None):
        """
        Creates a Maintainers instance.

        filename (default: None):
            Path to the maintainers file to parse. If None, MAINTAINERS.yml in
            the top-level directory of the Git repository is used, and must
            exist.
        """
        self._toplevel = pathlib.Path(_git("rev-parse", "--show-toplevel"))

        if filename is None:
            self.filename = self._toplevel / "MAINTAINERS.yml"
        else:
            self.filename = pathlib.Path(filename)

        self.areas = {}
        for area_name, area_dict in _load_maintainers(self.filename).items():
            area = Area()
            area.name = area_name
            area.status = area_dict.get("status")
            area.maintainers = area_dict.get("maintainers", [])
            area.collaborators = area_dict.get("collaborators", [])
            area.inform = area_dict.get("inform", [])
            area.labels = area_dict.get("labels", [])
            area.description = area_dict.get("description")
            area._files = area_dict.get("files")
            area._files_exclude = area_dict.get("files-exclude")
            area._files_regex = area_dict.get("files-regex")
            area._files_regex_exclude = area_dict.get("files-regex-exclude")
            self.areas[area_name] = area

    def path2areas(self, path):
        """
        Returns a list of Area instances for the areas that contain 'path',
        taken as relative to the current directory
        """
        # Make directory paths end in '/' so that foo/bar matches foo/bar/.
        # Skip this check in _contains() itself, because the isdir() makes it
        # twice as slow in cases where it's not needed.
        is_dir = os.path.isdir(path)

        # Make 'path' relative to the repository root and normalize it.
        # normpath() would remove a trailing '/', so we add it afterwards.
        path = os.path.normpath(os.path.join(
            os.path.relpath(os.getcwd(), self._toplevel),
            path))

        if is_dir:
            path += "/"

        return [area for area in self.areas.values()
                if area._contains(path)]

    def commits2areas(self, commits):
        """
        Returns a set() of Area instances for the areas that contain files that
        are modified by the commit range in 'commits'. 'commits' could be e.g.
        "HEAD~..", to inspect the tip commit
        """
        res = set()
        # Final '--' is to disallow a path for 'commits', so that
        # './get_maintainers.py some/file' errors out instead of doing nothing.
        # That makes forgetting -f easier to notice.
        for path in _git("diff", "--name-only", commits, "--").splitlines():
            res.update(self.path2areas(path))
        return res

    def __repr__(self):
        return "<Maintainers for '{}'>".format(self.filename)

    #
    # Command-line subcommands
    #

    def _path_cmd(self, args):
        # 'path' subcommand implementation

        for path in args.paths:
            if not os.path.exists(path):
                _serr("'{}': no such file or directory".format(path))

        res = set()
        orphaned = []
        for path in args.paths:
            areas = self.path2areas(path)
            res.update(areas)
            if not areas:
                orphaned.append(path)

        _print_areas(res)
        if orphaned:
            if res:
                print()
            print("Orphaned paths (not in any area):\n" + "\n".join(orphaned))

    def _commits_cmd(self, args):
        # 'commits' subcommand implementation

        commits = args.commits or ("HEAD~..",)
        _print_areas({area for commit_range in commits
                           for area in self.commits2areas(commit_range)})

    def _list_cmd(self, args):
        # 'list' subcommand implementation

        if args.area is None:
            # List all files that appear in some area
            for path in _ls_files():
                for area in self.areas.values():
                    if area._contains(path):
                        print(path)
                        break
        else:
            # List all files that appear in the given area
            area = self.areas.get(args.area)
            if area is None:
                _serr("'{}': no such area defined in '{}'"
                      .format(args.area, self.filename))

            for path in _ls_files():
                if area._contains(path):
                    print(path)

    def _orphaned_cmd(self, args):
        # 'orphaned' subcommand implementation

        if args.path is not None and not os.path.exists(args.path):
            _serr("'{}': no such file or directory".format(args.path))

        for path in _ls_files(args.path):
            for area in self.areas.values():
                if area._contains(path):
                    break
            else:
                print(path)  # We get here if we never hit the 'break'


class Area:
    """
    Represents an entry for an area in MAINTAINERS.yml.

    These attributes are available:

    status:
        The status of the area, as a string. None if the area has no 'status'
        key. See MAINTAINERS.yml.

    maintainers:
        List of maintainers. Empty if the area has no 'maintainers' key.

    collaborators:
        List of collaborators. Empty if the area has no 'collaborators' key.

    inform:
        List of people to inform on pull requests. Empty if the area has no
        'inform' key.

    labels:
        List of GitHub labels for the area. Empty if the area has no 'labels'
        key.

    description:
        Text from 'description' key, or None if the area has no 'description'
        key
    """
    def _contains(self, path):
        # Returns True if the area contains 'path', and False otherwise

        # Test exclusions first

        if self._files_exclude is not None:
            for glob in self._files_exclude:
                if _glob_match(glob, path):
                    return False

        if self._files_regex_exclude is not None:
            for regex in self._files_regex_exclude:
                if re.search(regex, path):
                    return False

        if self._files is not None:
            for glob in self._files:
                if _glob_match(glob, path):
                    return True

        if self._files_regex is not None:
            for regex in self._files_regex:
                if re.search(regex, path):
                    return True

        return False

    def __repr__(self):
        return "<Area {}>".format(self.name)


def _print_areas(areas):
    first = True
    for area in sorted(areas, key=operator.attrgetter("name")):
        if not first:
            print()
        first = False

        print("""\
{}
\tstatus: {}
\tmaintainers: {}
\tcollaborators: {}
\tinform: {}
\tlabels: {}
\tdescription: {}""".format(area.name,
                            area.status,
                            ", ".join(area.maintainers),
                            ", ".join(area.collaborators),
                            ", ".join(area.inform),
                            ", ".join(area.labels),
                            area.description or ""))


def _glob_match(glob, path):
    # Returns True if 'path' matches the pattern 'glob' from a
    # 'files(-exclude)' entry in MAINTAINERS.yml

    match_fn = re.match if glob.endswith("/") else re.fullmatch
    regex = glob.replace(".", "\\.").replace("*", "[^/]*").replace("?", "[^/]")
    return match_fn(regex, path)


def _load_maintainers(path):
    # Returns the parsed contents of the maintainers file 'filename', also
    # running checks on the contents. The returned format is plain Python
    # dicts/lists/etc., mirroring the structure of the file.

    with open(path, encoding="utf-8") as f:
        try:
            yaml = load(f, Loader=Loader)
        except YAMLError as e:
            raise MaintainersError("{}: YAML error: {}".format(path, e))

        _check_maintainers(path, yaml)
        return yaml


def _check_maintainers(maints_path, yaml):
    # Checks the maintainers data in 'yaml', which comes from the maintainers
    # file at maints_path, which is a pathlib.Path instance

    root = maints_path.parent

    def ferr(msg):
        _err("{}: {}".format(maints_path, msg))  # Prepend the filename

    if not isinstance(yaml, dict):
        ferr("empty or malformed YAML (not a dict)")

    ok_keys = {"status", "maintainers", "collaborators", "inform", "files",
               "files-exclude", "files-regex", "files-regex-exclude",
               "labels", "description"}

    ok_status = {"maintained", "odd fixes", "orphaned", "obsolete"}
    ok_status_s = ", ".join('"' + s + '"' for s in ok_status)  # For messages

    for area_name, area_dict in yaml.items():
        if not isinstance(area_dict, dict):
            ferr("malformed entry for area '{}' (not a dict)"
                 .format(area_name))

        for key in area_dict:
            if key not in ok_keys:
                ferr("unknown key '{}' in area '{}'"
                     .format(key, area_name))

        if "status" in area_dict and \
           area_dict["status"] not in ok_status:
            ferr("bad 'status' key on area '{}', should be one of {}"
                 .format(area_name, ok_status_s))

        if not area_dict.keys() & {"files", "files-regex"}:
            ferr("either 'files' or 'files-regex' (or both) must be specified "
                 "for area '{}'".format(area_name))

        for list_name in "maintainers", "collaborators", "inform", "files", \
                         "files-regex", "labels":
            if list_name in area_dict:
                lst = area_dict[list_name]
                if not (isinstance(lst, list) and
                        all(isinstance(elm, str) for elm in lst)):
                    ferr("malformed '{}' value for area '{}' -- should "
                         "be a list of strings".format(list_name, area_name))

        for files_key in "files", "files-exclude":
            if files_key in area_dict:
                for glob_pattern in area_dict[files_key]:
                    # This could be changed if it turns out to be too slow,
                    # e.g. to only check non-globbing filenames. The tuple() is
                    # needed due to pathlib's glob() returning a generator.
                    paths = tuple(root.glob(glob_pattern))
                    if not paths:
                        ferr("glob pattern '{}' in '{}' in area '{}' does not "
                             "match any files".format(glob_pattern, files_key,
                                                      area_name))
                    if not glob_pattern.endswith("/"):
                        for path in paths:
                            if path.is_dir():
                                ferr("glob pattern '{}' in '{}' in area '{}' "
                                     "matches a directory, but has no "
                                     "trailing '/'"
                                     .format(glob_pattern, files_key,
                                             area_name))

        for files_regex_key in "files-regex", "files-regex-exclude":
            if files_regex_key in area_dict:
                for regex in area_dict[files_regex_key]:
                    try:
                        # This also caches the regex in the 're' module, so we
                        # don't need to worry
                        re.compile(regex)
                    except re.error as e:
                        ferr("bad regular expression '{}' in '{}' in "
                             "'{}': {}".format(regex, files_regex_key,
                                               area_name, e.msg))

        if "description" in area_dict and \
           not isinstance(area_dict["description"], str):
            ferr("malformed 'description' value for area '{}' -- should be a "
                 "string".format(area_name))


def _git(*args):
    # Helper for running a Git command. Returns the rstrip()ed stdout output.
    # Called like git("diff"). Exits with SystemError (raised by sys.exit()) on
    # errors.

    git_cmd = ("git",) + args
    git_cmd_s = " ".join(shlex.quote(word) for word in git_cmd)  # For errors

    try:
        git_process = subprocess.Popen(
            git_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    except FileNotFoundError:
        _giterr("git executable not found (when running '{}'). Check that "
                "it's in listed in the PATH environment variable"
                .format(git_cmd_s))
    except OSError as e:
        _giterr("error running '{}': {}".format(git_cmd_s, e))

    stdout, stderr = git_process.communicate()
    if git_process.returncode:
        _giterr("error running '{}'\n\nstdout:\n{}\nstderr:\n{}".format(
            git_cmd_s, stdout.decode("utf-8"), stderr.decode("utf-8")))

    return stdout.decode("utf-8").rstrip()


def _ls_files(path=None):
    cmd = ["ls-files"]
    if path is not None:
        cmd.append(path)
    return _git(*cmd).splitlines()


def _err(msg):
    raise MaintainersError(msg)


def _giterr(msg):
    raise GitError(msg)


def _serr(msg):
    # For reporting errors when get_maintainer.py is run as a script.
    # sys.exit() shouldn't be used otherwise.
    sys.exit("{}: error: {}".format(sys.argv[0], msg))


class MaintainersError(Exception):
    "Exception raised for MAINTAINERS.yml-related errors"


class GitError(Exception):
    "Exception raised for Git-related errors"


if __name__ == "__main__":
    _main()
