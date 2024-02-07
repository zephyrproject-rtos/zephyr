#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2021 Intel Corporation

# A script to generate twister options based on modified files.

import re, os
import argparse
import yaml
import fnmatch
import subprocess
import json
import logging
import sys
import glob
from pathlib import Path
from git import Repo
from west.manifest import Manifest

if "ZEPHYR_BASE" not in os.environ:
    exit("$ZEPHYR_BASE environment variable undefined.")

# These are globaly used variables. They are assigned in __main__ and are visible in further methods
# however, pylint complains that it doesn't recognized them when used (used-before-assignment).
zephyr_base = Path(os.environ['ZEPHYR_BASE'])
repository_path = zephyr_base
repo_to_scan = zephyr_base
args = None


logging.basicConfig(format='%(levelname)s: %(message)s', level=logging.INFO)

sys.path.append(os.path.join(zephyr_base, 'scripts'))
import list_boards

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

class Tag:
    """
    Represents an entry for a tag in tags.yaml.

    These attributes are available:

    name:
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
        return "<Tag {}>".format(self.name)

class Filters:
    def __init__(self, modified_files, ignore_path, alt_tags, testsuite_root,
                 pull_request=False, platforms=[], detailed_test_id=True):
        self.modified_files = modified_files
        self.testsuite_root = testsuite_root
        self.resolved_files = []
        self.twister_options = []
        self.full_twister = False
        self.all_tests = []
        self.tag_options = []
        self.pull_request = pull_request
        self.platforms = platforms
        self.detailed_test_id = detailed_test_id
        self.ignore_path = ignore_path
        self.tag_cfg_file = alt_tags

    def process(self):
        self.find_modules()
        self.find_tags()
        self.find_tests()
        if not self.platforms:
            self.find_archs()
            self.find_boards()
        self.find_excludes()

    def get_plan(self, options, integration=False, use_testsuite_root=True):
        fname = "_test_plan_partial.json"
        cmd = [f"{zephyr_base}/scripts/twister", "-c"] + options + ["--save-tests", fname ]
        if not self.detailed_test_id:
            cmd += ["--no-detailed-test-id"]
        if self.testsuite_root and use_testsuite_root:
            for root in self.testsuite_root:
                cmd+=["-T", root]
        if integration:
            cmd.append("--integration")

        logging.info(" ".join(cmd))
        _ = subprocess.call(cmd)
        with open(fname, newline='') as jsonfile:
            json_data = json.load(jsonfile)
            suites = json_data.get("testsuites", [])
            self.all_tests.extend(suites)
        if os.path.exists(fname):
            os.remove(fname)

    def find_modules(self):
        if 'west.yml' in self.modified_files:
            print(f"Manifest file 'west.yml' changed")
            print("=========")
            old_manifest_content = repo_to_scan.git.show(f"{args.commits[:-2]}:west.yml")
            with open("west_old.yml", "w") as manifest:
                manifest.write(old_manifest_content)
            old_manifest = Manifest.from_file("west_old.yml")
            new_manifest = Manifest.from_file("west.yml")
            old_projs = set((p.name, p.revision) for p in old_manifest.projects)
            new_projs = set((p.name, p.revision) for p in new_manifest.projects)
            logging.debug(f'old_projs: {old_projs}')
            logging.debug(f'new_projs: {new_projs}')
            # Removed projects
            rprojs = set(filter(lambda p: p[0] not in list(p[0] for p in new_projs),
                old_projs - new_projs))
            # Updated projects
            uprojs = set(filter(lambda p: p[0] in list(p[0] for p in old_projs),
                new_projs - old_projs))
            # Added projects
            aprojs = new_projs - old_projs - uprojs

            # All projs
            projs = rprojs | uprojs | aprojs
            projs_names = [name for name, rev in projs]

            logging.info(f'rprojs: {rprojs}')
            logging.info(f'uprojs: {uprojs}')
            logging.info(f'aprojs: {aprojs}')
            logging.info(f'project: {projs_names}')

            _options = []
            for p in projs_names:
                _options.extend(["-t", p ])

            if self.platforms:
                for platform in self.platforms:
                    _options.extend(["-p", platform])

            self.get_plan(_options, True)


    def find_archs(self):
        # we match both arch/<arch>/* and include/zephyr/arch/<arch> and skip common.
        archs = set()

        for f in self.modified_files:
            p = re.match(r"^arch\/([^/]+)\/", f)
            if not p:
                p = re.match(r"^include\/zephyr\/arch\/([^/]+)\/", f)
            if p:
                if p.group(1) != 'common':
                    archs.add(p.group(1))
                    # Modified file is treated as resolved, since a matching scope was found
                    self.resolved_files.append(f)

        _options = []
        for arch in archs:
            _options.extend(["-a", arch ])

        if _options:
            logging.info(f'Potential architecture filters...')
            if self.platforms:
                for platform in self.platforms:
                    _options.extend(["-p", platform])

                self.get_plan(_options, True)
            else:
                self.get_plan(_options, True)

    def find_boards(self):
        boards = set()
        all_boards = set()
        resolved = []

        for f in self.modified_files:
            if f.endswith(".rst") or f.endswith(".png") or f.endswith(".jpg"):
                continue
            p = re.match(r"^boards\/[^/]+\/([^/]+)\/", f)
            if p and p.groups():
                boards.add(p.group(1))
                resolved.append(f)

        roots = [zephyr_base]
        if repository_path != zephyr_base:
            roots.append(repository_path)

        # Look for boards in monitored repositories
        lb_args = argparse.Namespace(**{ 'arch_roots': roots, 'board_roots': roots})
        known_boards = list_boards.find_boards(lb_args)
        for b in boards:
            name_re = re.compile(b)
            for kb in known_boards:
                if name_re.search(kb.name):
                    all_boards.add(kb.name)

        # If modified file is catched by "find_boards" workflow (change in "boards" dir AND board recognized)
        # it means a proper testing scope for this file was found and this file can be removed
        # from further consideration
        for board in all_boards:
            self.resolved_files.extend(list(filter(lambda f: board in f, resolved)))

        _options = []
        if len(all_boards) > 20:
            logging.warning(f"{len(boards)} boards changed, this looks like a global change, skipping test handling, revert to default.")
            self.full_twister = True
            return

        for board in all_boards:
            _options.extend(["-p", board ])

        if _options:
            logging.info(f'Potential board filters...')
            self.get_plan(_options)

    def find_tests(self):
        tests = set()
        for f in self.modified_files:
            if f.endswith(".rst"):
                continue
            d = os.path.dirname(f)
            scope_found = False
            while not scope_found and d:
                head, tail = os.path.split(d)
                if os.path.exists(os.path.join(d, "testcase.yaml")) or \
                    os.path.exists(os.path.join(d, "sample.yaml")):
                    tests.add(d)
                    # Modified file is treated as resolved, since a matching scope was found
                    self.resolved_files.append(f)
                    scope_found = True
                elif tail == "common":
                    # Look for yamls in directories collocated with common

                    yamls_found = [yaml for yaml in glob.iglob(head + '/**/testcase.yaml', recursive=True)]
                    yamls_found.extend([yaml for yaml in glob.iglob(head + '/**/sample.yaml', recursive=True)])
                    if yamls_found:
                        for yaml in yamls_found:
                            tests.add(os.path.dirname(yaml))
                        self.resolved_files.append(f)
                        scope_found = True
                    else:
                        d = os.path.dirname(d)
                else:
                    d = os.path.dirname(d)

        _options = []
        for t in tests:
            _options.extend(["-T", t ])

        if len(tests) > 20:
            logging.warning(f"{len(tests)} tests changed, this looks like a global change, skipping test handling, revert to default")
            self.full_twister = True
            return

        if _options:
            logging.info(f'Potential test filters...({len(tests)} changed...)')
            if self.platforms:
                for platform in self.platforms:
                    _options.extend(["-p", platform])
            else:
                _options.append("--all")
            self.get_plan(_options, use_testsuite_root=False)

    def find_tags(self):

        with open(self.tag_cfg_file, 'r') as ymlfile:
            tags_config = yaml.safe_load(ymlfile)

        tags = {}
        for t,x in tags_config.items():
            tag = Tag()
            tag.exclude = True
            tag.name = t

            # tag._match_fn(path) tests if the path matches files and/or
            # files-regex
            tag._match_fn = _get_match_fn(x.get("files"), x.get("files-regex"))

            # Like tag._match_fn(path), but for files-exclude and
            # files-regex-exclude
            tag._exclude_match_fn = \
                _get_match_fn(x.get("files-exclude"), x.get("files-regex-exclude"))

            tags[tag.name] = tag

        for f in self.modified_files:
            for t in tags.values():
                if t._contains(f):
                    t.exclude = False

        exclude_tags = set()
        for t in tags.values():
            if t.exclude:
                exclude_tags.add(t.name)

        for tag in exclude_tags:
            self.tag_options.extend(["-e", tag ])

        if exclude_tags:
            logging.info(f'Potential tag based filters: {exclude_tags}')

    def find_excludes(self, skip=[]):
        with open(self.ignore_path, "r") as twister_ignore:
            ignores = twister_ignore.read().splitlines()
            ignores = filter(lambda x: not x.startswith("#"), ignores)

        found = set()
        files_not_resolved = list(filter(lambda x: x not in self.resolved_files, self.modified_files))

        for pattern in ignores:
            if pattern:
                found.update(fnmatch.filter(files_not_resolved, pattern))

        logging.debug(found)
        logging.debug(files_not_resolved)

        # Full twister run can be ordered by detecting great number of tests/boards changed
        # or if not all modified files were resolved (corresponding scope found)
        self.full_twister = self.full_twister or sorted(files_not_resolved) != sorted(found)

        if self.full_twister:
            _options = []
            logging.info(f'Need to run full or partial twister...')
            if self.platforms:
                for platform in self.platforms:
                    _options.extend(["-p", platform])

                _options.extend(self.tag_options)
                self.get_plan(_options)
            else:
                _options.extend(self.tag_options)
                self.get_plan(_options, True)
        else:
            logging.info(f'No twister needed or partial twister run only...')

def parse_args():
    parser = argparse.ArgumentParser(
                description="Generate twister argument files based on modified file",
                allow_abbrev=False)
    parser.add_argument('-c', '--commits', default=None,
            help="Commit range in the form: a..b")
    parser.add_argument('-m', '--modified-files', default=None,
            help="File with information about changed/deleted/added files.")
    parser.add_argument('-o', '--output-file', default="testplan.json",
            help="JSON file with the test plan to be passed to twister")
    parser.add_argument('-P', '--pull-request', action="store_true",
            help="This is a pull request")
    parser.add_argument('-p', '--platform', action="append",
            help="Limit this for a platform or a list of platforms.")
    parser.add_argument('-t', '--tests_per_builder', default=700, type=int,
            help="Number of tests per builder")
    parser.add_argument('-n', '--default-matrix', default=10, type=int,
            help="Number of tests per builder")
    parser.add_argument('--detailed-test-id', action='store_true',
            help="Include paths to tests' locations in tests' names.")
    parser.add_argument("--no-detailed-test-id", dest='detailed_test_id', action="store_false",
            help="Don't put paths into tests' names.")
    parser.add_argument('-r', '--repo-to-scan', default=None,
            help="Repo to scan")
    parser.add_argument('--ignore-path',
            default=os.path.join(zephyr_base, 'scripts', 'ci', 'twister_ignore.txt'),
            help="Path to a text file with patterns of files to be matched against changed files")
    parser.add_argument('--alt-tags',
            default=os.path.join(zephyr_base, 'scripts', 'ci', 'tags.yaml'),
            help="Path to a file describing relations between directories and tags")
    parser.add_argument(
            "-T", "--testsuite-root", action="append", default=[],
            help="Base directory to recursively search for test cases. All "
                "testcase.yaml files under here will be processed. May be "
                "called multiple times. Defaults to the 'samples/' and "
                "'tests/' directories at the base of the Zephyr tree.")

    # Include paths in names by default.
    parser.set_defaults(detailed_test_id=True)

    return parser.parse_args()


if __name__ == "__main__":

    args = parse_args()
    files = []
    errors = 0
    if args.repo_to_scan:
        repository_path = Path(args.repo_to_scan)
    if args.commits:
        repo_to_scan = Repo(repository_path)
        commit = repo_to_scan.git.diff("--name-only", args.commits)
        files = commit.split("\n")
    elif args.modified_files:
        with open(args.modified_files, "r") as fp:
            files = json.load(fp)

    if files:
        print("Changed files:\n=========")
        print("\n".join(files))
        print("=========")

    f = Filters(files, args.ignore_path, args.alt_tags, args.testsuite_root,
                args.pull_request, args.platform, args.detailed_test_id)
    f.process()

    # remove dupes and filtered cases
    dup_free = []
    dup_free_set = set()
    logging.info(f'Total tests gathered: {len(f.all_tests)}')
    for ts in f.all_tests:
        if ts.get('status') == 'filtered':
            continue
        n = ts.get("name")
        a = ts.get("arch")
        p = ts.get("platform")
        if ts.get('status') == 'error':
            logging.info(f"Error found: {n} on {p} ({ts.get('reason')})")
            errors += 1
        if (n, a, p,) not in dup_free_set:
            dup_free.append(ts)
            dup_free_set.add((n, a, p,))

    logging.info(f'Total tests to be run: {len(dup_free)}')
    with open(".testplan", "w") as tp:
        total_tests = len(dup_free)
        if total_tests and total_tests < args.tests_per_builder:
            nodes = 1
        else:
            nodes = round(total_tests / args.tests_per_builder)

        tp.write(f"TWISTER_TESTS={total_tests}\n")
        tp.write(f"TWISTER_NODES={nodes}\n")
        tp.write(f"TWISTER_FULL={f.full_twister}\n")
        logging.info(f'Total nodes to launch: {nodes}')

    header = ['test', 'arch', 'platform', 'status', 'extra_args', 'handler',
            'handler_time', 'used_ram', 'used_rom']

    # write plan
    if dup_free:
        data = {}
        data['testsuites'] = dup_free
        with open(args.output_file, 'w', newline='') as json_file:
            json.dump(data, json_file, indent=4, separators=(',',':'))

    sys.exit(errors)
