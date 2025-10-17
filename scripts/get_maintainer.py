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
from tabulate import tabulate

from yaml import load, YAMLError
try:
    # Use the speedier C LibYAML parser if available
    from yaml import CSafeLoader as SafeLoader
except ImportError:
    from yaml import SafeLoader


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
        description=__doc__, allow_abbrev=False)

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

    areas_parser = subparsers.add_parser(
        "areas",
        help="List areas and maintainers")
    areas_parser.add_argument(
        "maintainer",
        metavar="MAINTAINER",
        nargs="?",
        help="List all areas maintained by maintainer.")


    area_parser = subparsers.add_parser(
        "area",
        help="List area(s) by name")
    area_parser.add_argument(
        "name",
        metavar="AREA",
        nargs="?",
        help="List all areas with the given name.")

    area_parser.set_defaults(cmd_fn=Maintainers._area_cmd)

    # New arguments for filtering
    areas_parser.add_argument(
        "--without-maintainers",
        action="store_true",
        help="Exclude areas that have maintainers")
    areas_parser.add_argument(
        "--without-collaborators",
        action="store_true",
        help="Exclude areas that have collaborators")

    areas_parser.set_defaults(cmd_fn=Maintainers._areas_cmd)

    orphaned_parser = subparsers.add_parser(
        "orphaned",
        help="List orphaned files (files that do not appear in any area)")
    orphaned_parser.add_argument(
        "path",
        metavar="PATH",
        nargs="?",
        help="Limit to files under PATH")
    orphaned_parser.set_defaults(cmd_fn=Maintainers._orphaned_cmd)

    count_parser = subparsers.add_parser(
        "count",
        help="Count areas, unique maintainers, and / or unique collaborators")
    count_parser.add_argument(
        "-a",
        "--count-areas",
        action="store_true",
        help="Count the number of areas")
    count_parser.add_argument(
        "-c",
        "--count-collaborators",
        action="store_true",
        help="Count the number of unique collaborators")
    count_parser.add_argument(
        "-n",
        "--count-maintainers",
        action="store_true",
        help="Count the number of unique maintainers")
    count_parser.add_argument(
        "-o",
        "--count-unmaintained",
        action="store_true",
        help="Count the number of unmaintained areas")
    count_parser.set_defaults(cmd_fn=Maintainers._count_cmd)

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
        if (filename is not None) and (pathlib.Path(filename).exists()):
            self.filename = pathlib.Path(filename)
            self._toplevel = self.filename.parent
        else:
            self._toplevel = pathlib.Path(_git("rev-parse", "--show-toplevel"))
            self.filename = self._toplevel / "MAINTAINERS.yml"

        self.areas = {}
        for area_name, area_dict in _load_maintainers(self.filename).items():
            area = Area()
            area.name = area_name
            area.status = area_dict.get("status")
            area.maintainers = area_dict.get("maintainers", [])
            area.collaborators = area_dict.get("collaborators", [])
            area.inform = area_dict.get("inform", [])
            area.labels = area_dict.get("labels", [])
            area.tests = area_dict.get("tests", [])
            area.tags = area_dict.get("tags", [])
            area.description = area_dict.get("description")

            # area._match_fn(path) tests if the path matches files and/or
            # files-regex
            area._match_fn = \
                _get_match_fn(area_dict.get("files"),
                              area_dict.get("files-regex"))

            # Like area._match_fn(path), but for files-exclude and
            # files-regex-exclude
            area._exclude_match_fn = \
                _get_match_fn(area_dict.get("files-exclude"),
                              area_dict.get("files-regex-exclude"))

            self.areas[area_name] = area

    def name2areas(self, name):
        """
        Returns a list of Area instances for the areas that match 'name'.
        """
        return [area for area in self.areas.values() if area.name == name]

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
        # Final '--' is to make sure 'commits' is interpreted as a commit range
        # rather than a path. That might give better error messages.
        for path in _git("diff", "--name-only", commits, "--").splitlines():
            res.update(self.path2areas(path))
        return res

    def __repr__(self):
        return "<Maintainers for '{}'>".format(self.filename)

    #
    # Command-line subcommands
    #

    def _area_cmd(self, args):
        # 'area' subcommand implementation

        res = set()
        areas = self.name2areas(args.name)
        res.update(areas)
        _print_areas(res)

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

    def _areas_cmd(self, args):
        # 'areas' subcommand implementation
        def multiline(items):
            # Each item on its own line, empty string if none
            return "\n".join(items) if items else ""

        table = []
        for area in self.areas.values():
            maintainers = multiline(area.maintainers)
            collaborators = multiline(area.collaborators)

            # Filter based on new arguments
            if getattr(args, "without_maintainers", False) and area.maintainers:
                continue
            if getattr(args, "without_collaborators", False) and area.collaborators:
                continue

            if args.maintainer:
                if args.maintainer in area.maintainers:
                    table.append([
                        area.name,
                        maintainers,
                        collaborators
                    ])
            else:
                table.append([
                    area.name,
                    maintainers,
                    collaborators
                ])
        if table:
            print(tabulate(
                table,
                headers=["Area", "Maintainers", "Collaborators"],
                tablefmt="grid",
                stralign="left",
                disable_numparse=True
            ))

    def _count_cmd(self, args):
        # 'count' subcommand implementation

        if not (args.count_areas or args.count_collaborators or args.count_maintainers or args.count_unmaintained):
            # if no specific count is provided, print them all
            args.count_areas = True
            args.count_collaborators = True
            args.count_maintainers = True
            args.count_unmaintained = True

        unmaintained = 0
        collaborators = set()
        maintainers = set()

        for area in self.areas.values():
            if area.status == 'maintained':
                maintainers = maintainers.union(set(area.maintainers))
            elif area.status == 'odd fixes':
                unmaintained += 1
            collaborators = collaborators.union(set(area.collaborators))

        if args.count_areas:
            print('{:14}\t{}'.format('areas:', len(self.areas)))
        if args.count_maintainers:
            print('{:14}\t{}'.format('maintainers:', len(maintainers)))
        if args.count_collaborators:
            print('{:14}\t{}'.format('collaborators:', len(collaborators)))
        if args.count_unmaintained:
            print('{:14}\t{}'.format('unmaintained:', unmaintained))

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

        return self._match_fn and self._match_fn(path) and not \
            (self._exclude_match_fn and self._exclude_match_fn(path))

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
\ttests: {}
\ttags: {}
\tdescription: {}""".format(area.name,
                            area.status,
                            ", ".join(area.maintainers),
                            ", ".join(area.collaborators),
                            ", ".join(area.inform),
                            ", ".join(area.labels),
                            ", ".join(area.tests),
                            ", ".join(area.tags),
                            area.description or ""))


def _get_match_fn(globs, regexes):
    # Constructs a single regex that tests for matches against the globs in
    # 'globs' and the regexes in 'regexes'. Parts are joined with '|' (OR).
    # Returns the search() method of the compiled regex.
    #
    # Returns None if there are neither globs nor regexes, which should be
    # interpreted as no match.

    if not (globs or regexes):
        return None

    regex = ""

    if globs:
        glob_regexes = []
        for glob in globs:
            # Construct a regex equivalent to the glob
            glob_regex = glob.replace(".", "\\.").replace("*", "[^/]*") \
                             .replace("?", "[^/]")

            if not glob.endswith("/"):
                # Require a full match for globs that don't end in /
                glob_regex += "$"

            glob_regexes.append(glob_regex)

        # The glob regexes must anchor to the beginning of the path, since we
        # return search(). (?:) is a non-capturing group.
        regex += "^(?:{})".format("|".join(glob_regexes))

    if regexes:
        if regex:
            regex += "|"
        regex += "|".join(regexes)

    return re.compile(regex).search


def _load_maintainers(path):
    # Returns the parsed contents of the maintainers file 'filename', also
    # running checks on the contents. The returned format is plain Python
    # dicts/lists/etc., mirroring the structure of the file.

    with open(path, encoding="utf-8") as f:
        try:
            yaml = load(f, Loader=SafeLoader)
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
               "labels", "description", "tests", "tags"}

    ok_status = {"maintained", "odd fixes", "unmaintained", "obsolete"}
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

        if not area_dict.get("maintainers") and area_dict.get("status") == "maintained":
            ferr("maintained area '{}' with no maintainers".format(area_name))

        for list_name in "maintainers", "collaborators", "inform", "files", \
                         "files-regex", "labels", "tags", "tests":
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
                        if all(path.is_dir() for path in paths):
                            ferr("glob pattern '{}' in '{}' in area '{}' "
                                     "matches only directories, but has no "
                                     "trailing '/'"
                                     .format(glob_pattern, files_key,
                                             area_name))

        for files_regex_key in "files-regex", "files-regex-exclude":
            if files_regex_key in area_dict:
                for regex in area_dict[files_regex_key]:
                    try:
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
