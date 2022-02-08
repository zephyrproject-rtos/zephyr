#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2021 Intel Corporation

# A script to generate twister options based on modified files.

import re, os
import argparse
import glob
import yaml
import json
import fnmatch
import subprocess
import csv
import logging
from git import Repo

if "ZEPHYR_BASE" not in os.environ:
    exit("$ZEPHYR_BASE environment variable undefined.")

repository_path = os.environ['ZEPHYR_BASE']
logging.basicConfig(format='%(levelname)s: %(message)s', level=logging.INFO)

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
    def __init__(self, modified_files, pull_request=False, platforms=[]):
        self.modified_files = modified_files
        self.twister_options = []
        self.full_twister = False
        self.all_tests = []
        self.tag_options = []
        self.pull_request = pull_request
        self.platforms = platforms


    def process(self):
        self.find_tags()
        self.find_excludes()
        self.find_tests()
        if not self.platforms:
            self.find_archs()
        self.find_boards()

    def get_plan(self, options, integration=False):
        fname = "_test_plan_partial.csv"
        cmd = ["scripts/twister", "-c"] + options + ["--save-tests", fname ]
        if integration:
            cmd.append("--integration")

        logging.info(" ".join(cmd))
        _ = subprocess.call(cmd)
        with open(fname, newline='') as csvfile:
            csv_reader = csv.reader(csvfile, delimiter=',')
            _ = next(csv_reader)
            for e in csv_reader:
                self.all_tests.append(e)
        if os.path.exists(fname):
            os.remove(fname)

    def find_archs(self):
        # we match both arch/<arch>/* and include/arch/<arch> and skip common.
        # Some architectures like riscv require special handling, i.e. riscv
        # directory covers 2 architectures known to twister: riscv32 and riscv64.
        archs = set()

        for f in self.modified_files:
            p = re.match(r"^arch\/([^/]+)\/", f)
            if not p:
                p = re.match(r"^include\/arch\/([^/]+)\/", f)
            if p:
                if p.group(1) != 'common':
                    if p.group(1) == 'riscv':
                        archs.add('riscv32')
                        archs.add('riscv64')
                    else:
                        archs.add(p.group(1))

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
                self.get_plan(_options, False)

    def find_boards(self):
        boards = set()
        all_boards = set()

        for f in self.modified_files:
            if f.endswith(".rst") or f.endswith(".png") or f.endswith(".jpg"):
                continue
            p = re.match(r"^boards\/[^/]+\/([^/]+)\/", f)
            if p and p.groups():
                boards.add(p.group(1))

        for b in boards:
            suboards = glob.glob("boards/*/%s/*.yaml" %(b))
            for subboard in suboards:
                name = os.path.splitext(os.path.basename(subboard))[0]
                if name:
                    all_boards.add(name)

        _options = []
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
            while d:
                if os.path.exists(os.path.join(d, "testcase.yaml")) or \
                    os.path.exists(os.path.join(d, "sample.yaml")):
                    tests.add(d)
                    break
                else:
                    d = os.path.dirname(d)

        _options = []
        for t in tests:
            _options.extend(["-T", t ])

        if _options:
            logging.info(f'Potential test filters...')
            if self.platforms:
                for platform in self.platforms:
                    _options.extend(["-p", platform])
            else:
                _options.append("--all")
            self.get_plan(_options)

    def find_tags(self):

        tag_cfg_file = os.path.join(repository_path, 'scripts', 'ci', 'tags.yaml')
        with open(tag_cfg_file, 'r') as ymlfile:
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
            logging.info(f'Potential tag based filters...')

    def find_excludes(self):
        with open("scripts/ci/twister_ignore.txt", "r") as twister_ignore:
            ignores = twister_ignore.read().splitlines()
            ignores = filter(lambda x: not x.startswith("#"), ignores)

        found = set()
        files = list(filter(lambda x: x, self.modified_files))

        for pattern in ignores:
            if pattern:
                found.update(fnmatch.filter(files, pattern))

        logging.debug(found)
        logging.debug(files)

        if sorted(files) != sorted(found):
            _options = []
            logging.info(f'Need to run full or partial twister...')
            self.full_twister = True
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
                description="Generate twister argument files based on modified file")
    parser.add_argument('-c', '--commits', default=None,
            help="Commit range in the form: a..b")
    parser.add_argument('-m', '--modified-files', default=None,
            help="File with information about changed/deleted/added files.")
    parser.add_argument('-o', '--output-file', default="testplan.csv",
            help="CSV file with the test plan to be passed to twister")
    parser.add_argument('-P', '--pull-request', action="store_true",
            help="This is a pull request")
    parser.add_argument('-p', '--platform', action="append",
            help="Limit this for a platform or a list of platforms.")
    parser.add_argument('-t', '--tests_per_builder', default=700, type=int,
            help="Number of tests per builder")
    parser.add_argument('-n', '--default-matrix', default=10, type=int,
            help="Number of tests per builder")

    return parser.parse_args()


if __name__ == "__main__":

    args = parse_args()
    if args.commits:
        repo = Repo(repository_path)
        commit = repo.git.diff("--name-only", args.commits)
        files = commit.split("\n")
    elif args.modified_files:
        with open(args.modified_files, "r") as fp:
            files = json.load(fp)

    print("Changed files:\n=========")
    print("\n".join(files))
    print("=========")

    f = Filters(files, args.pull_request, args.platform)
    f.process()

    # remove dupes and filtered cases
    dup_free = []
    dup_free_set = set()
    for x in f.all_tests:
        if x[3] == 'skipped':
            continue
        if tuple(x) not in dup_free_set:
            dup_free.append(x)
            dup_free_set.add(tuple(x))

    logging.info(f'Total tests to be run: {len(dup_free)}')
    with open(".testplan", "w") as tp:
        total_tests = len(dup_free)
        nodes = round(total_tests / args.tests_per_builder)
        if total_tests % args.tests_per_builder != total_tests:
            nodes = nodes + 1

        if nodes > 5:
            nodes = args.default_matrix

        tp.write(f"TWISTER_TESTS={total_tests}\n")
        tp.write(f"TWISTER_NODES={nodes}\n")

    header = ['test', 'arch', 'platform', 'status', 'extra_args', 'handler',
            'handler_time', 'ram_size', 'rom_size']

    # write plan
    if dup_free:
        with open(args.output_file, 'w', newline='') as csv_file:
            writer = csv.writer(csv_file)
            writer.writerow(header)
            writer.writerows(dup_free)
