#!/usr/bin/env python3
# Copyright (c) 2022, Meta
#
# SPDX-License-Identifier: Apache-2.0

"""Query issues in a release branch

This script searches for issues referenced via pull-requests in a release
branch in order to simplify tracking changes such as automated backports,
manual backports, security fixes, and stability fixes.

A formatted report is printed to standard output either in JSON or
reStructuredText.

Since an issue is required for all changes to release branches, merged PRs
must have at least one instance of the phrase "Fixes #1234" in the body. This
script will throw an error if a PR has been made without an associated issue.

Usage:
    ./scripts/release/list_backports.py \
        -t ~/.ghtoken \
        -b v2.7-branch \
        -s 2021-12-15 -e 2022-04-22 \
        -P 45074 -P 45868 -P 44918 -P 41234 -P 41174 \
        -j | jq . | tee /tmp/backports.json

    GITHUB_TOKEN="<secret>" \
    ./scripts/release/list_backports.py \
        -b v3.0-branch \
        -p 43381 \
        -j | jq . | tee /tmp/backports.json
"""

import argparse
from datetime import datetime, timedelta
import io
import json
import logging
import os
import re
import sys

# Requires PyGithub
from github import Github


# https://gist.github.com/monkut/e60eea811ef085a6540f
def valid_date_type(arg_date_str):
    """custom argparse *date* type for user dates values given from the
    command line"""
    try:
        return datetime.strptime(arg_date_str, "%Y-%m-%d")
    except ValueError:
        msg = "Given Date ({0}) not valid! Expected format, YYYY-MM-DD!".format(arg_date_str)
        raise argparse.ArgumentTypeError(msg)


def parse_args():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument('-t', '--token', dest='tokenfile',
                        help='File containing GitHub token (alternatively, use GITHUB_TOKEN env variable)', metavar='FILE')
    parser.add_argument('-b', '--base', dest='base',
                        help='branch (base) for PRs (e.g. v2.7-branch)', metavar='BRANCH', required=True)
    parser.add_argument('-j', '--json', dest='json', action='store_true',
                        help='print output in JSON rather than RST')
    parser.add_argument('-s', '--start', dest='start', help='start date (YYYY-mm-dd)',
                        metavar='START_DATE', type=valid_date_type)
    parser.add_argument('-e', '--end', dest='end', help='end date (YYYY-mm-dd)',
                        metavar='END_DATE', type=valid_date_type)
    parser.add_argument("-o", "--org", default="zephyrproject-rtos",
                        help="Github organization")
    parser.add_argument('-p', '--include-pull', dest='includes',
                        help='include pull request (can be specified multiple times)',
                        metavar='PR', type=int, action='append', default=[])
    parser.add_argument('-P', '--exclude-pull', dest='excludes',
                        help='exlude pull request (can be specified multiple times, helpful for version bumps and release notes)',
                        metavar='PR', type=int, action='append', default=[])
    parser.add_argument("-r", "--repo", default="zephyr",
                        help="Github repository")

    args = parser.parse_args()

    if args.includes:
        if getattr(args, 'start'):
            logging.error(
                'the --start argument should not be used with --include-pull')
            return None
        if getattr(args, 'end'):
            logging.error(
                'the --end argument should not be used with --include-pull')
            return None
    else:
        if not getattr(args, 'start'):
            logging.error(
                'if --include-pr PR is not used, --start START_DATE is required')
            return None

        if not getattr(args, 'end'):
            setattr(args, 'end', datetime.now())

        if args.end < args.start:
            logging.error(
                f'end date {args.end} is before start date {args.start}')
            return None

    if args.tokenfile:
        with open(args.tokenfile, 'r') as file:
            token = file.read()
            token = token.strip()
    else:
        if 'GITHUB_TOKEN' not in os.environ:
            raise ValueError('No credentials specified')
        token = os.environ['GITHUB_TOKEN']

    setattr(args, 'token', token)

    return args


class Backport(object):
    def __init__(self, repo, base, pulls):
        self._base = base
        self._repo = repo
        self._issues = []
        self._pulls = pulls

        self._pulls_without_an_issue = []
        self._pulls_with_invalid_issues = {}

    @staticmethod
    def by_date_range(repo, base, start_date, end_date, excludes):
        """Create a Backport object with the provided repo,
        base, start datetime object, and end datetime objects, and
        list of excluded PRs"""

        pulls = []

        unfiltered_pulls = repo.get_pulls(
            base=base, state='closed')
        for p in unfiltered_pulls:
            if not p.merged:
                # only consider merged backports
                continue

            if p.closed_at < start_date or p.closed_at >= end_date + timedelta(1):
                # only concerned with PRs within time window
                continue

            if p.number in excludes:
                # skip PRs that have been explicitly excluded
                continue

            pulls.append(p)

        # paginated_list.sort() does not exist
        pulls = sorted(pulls, key=lambda x: x.number)

        return Backport(repo, base, pulls)

    @staticmethod
    def by_included_prs(repo, base, includes):
        """Create a Backport object with the provided repo,
        base, and list of included PRs"""

        pulls = []

        for i in includes:
            try:
                p = repo.get_pull(i)
            except Exception:
                p = None

            if not p:
                logging.error(f'{i} is not a valid pull request')
                return None

            if p.base.ref != base:
                logging.error(
                    f'{i} is not a valid pull request for base {base} ({p.base.label})')
                return None

            pulls.append(p)

        # paginated_list.sort() does not exist
        pulls = sorted(pulls, key=lambda x: x.number)

        return Backport(repo, base, pulls)

    @staticmethod
    def sanitize_title(title):
        # TODO: sanitize titles such that they are suitable for both JSON and ReStructured Text
        # could also automatically fix titles like "Automated backport of PR #1234"
        return title

    def print(self):
        for i in self.get_issues():
            title = Backport.sanitize_title(i.title)
            # * :github:`38972` - logging: Cleaning references to tracing in logging
            print(f'* :github:`{i.number}` - {title}')

    def print_json(self):
        issue_objects = []
        for i in self.get_issues():
            obj = {}
            obj['id'] = i.number
            obj['title'] = Backport.sanitize_title(i.title)
            obj['url'] = f'https://github.com/{self._repo.organization.login}/{self._repo.name}/pull/{i.number}'
            issue_objects.append(obj)

        print(json.dumps(issue_objects))

    def get_pulls(self):
        return self._pulls

    def get_issues(self):
        """Return GitHub issues fixed in the provided date window"""
        if self._issues:
            return self._issues

        issue_map = {}
        self._pulls_without_an_issue = []
        self._pulls_with_invalid_issues = {}

        for p in self._pulls:
            # check for issues in this pr
            issues_for_this_pr = {}
            with io.StringIO(p.body) as buf:
                for line in buf.readlines():
                    line = line.strip()
                    match = re.search(r"^Fixes[:]?\s*#([1-9][0-9]*).*", line)
                    if not match:
                        match = re.search(
                            rf"^Fixes[:]?\s*https://github\.com/{self._repo.organization.login}/{self._repo.name}/issues/([1-9][0-9]*).*", line)
                    if not match:
                        continue
                    issue_number = int(match[1])
                    issue = self._repo.get_issue(issue_number)
                    if not issue:
                        if not self._pulls_with_invalid_issues[p.number]:
                            self._pulls_with_invalid_issues[p.number] = [
                                issue_number]
                        else:
                            self._pulls_with_invalid_issues[p.number].append(
                                issue_number)
                        logging.error(
                            f'https://github.com/{self._repo.organization.login}/{self._repo.name}/pull/{p.number} references invalid issue number {issue_number}')
                        continue
                    issues_for_this_pr[issue_number] = issue

            # report prs missing issues later
            if len(issues_for_this_pr) == 0:
                logging.error(
                    f'https://github.com/{self._repo.organization.login}/{self._repo.name}/pull/{p.number} does not have an associated issue')
                self._pulls_without_an_issue.append(p)
                continue

            # FIXME: when we have upgrade to python3.9+, use "issue_map | issues_for_this_pr"
            issue_map = {**issue_map, **issues_for_this_pr}

        issues = list(issue_map.values())

        # paginated_list.sort() does not exist
        issues = sorted(issues, key=lambda x: x.number)

        self._issues = issues

        return self._issues

    def get_pulls_without_issues(self):
        if self._pulls_without_an_issue:
            return self._pulls_without_an_issue

        self.get_issues()

        return self._pulls_without_an_issue

    def get_pulls_with_invalid_issues(self):
        if self._pulls_with_invalid_issues:
            return self._pulls_with_invalid_issues

        self.get_issues()

        return self._pulls_with_invalid_issues


def main():
    args = parse_args()

    if not args:
        return os.EX_DATAERR

    try:
        gh = Github(args.token)
    except Exception:
        logging.error('failed to authenticate with GitHub')
        return os.EX_DATAERR

    try:
        repo = gh.get_repo(args.org + '/' + args.repo)
    except Exception:
        logging.error('failed to obtain Github repository')
        return os.EX_DATAERR

    bp = None
    if args.includes:
        bp = Backport.by_included_prs(repo, args.base, set(args.includes))
    else:
        bp = Backport.by_date_range(repo, args.base,
                                    args.start, args.end, set(args.excludes))

    if not bp:
        return os.EX_DATAERR

    pulls_with_invalid_issues = bp.get_pulls_with_invalid_issues()
    if pulls_with_invalid_issues:
        logging.error('The following PRs link to invalid issues:')
        for (p, lst) in pulls_with_invalid_issues:
            logging.error(
                f'\nhttps://github.com/{repo.organization.login}/{repo.name}/pull/{p.number}: {lst}')
        return os.EX_DATAERR

    pulls_without_issues = bp.get_pulls_without_issues()
    if pulls_without_issues:
        logging.error(
            'Please ensure the body of each PR to a release branch contains "Fixes #1234"')
        logging.error('The following PRs are lacking associated issues:')
        for p in pulls_without_issues:
            logging.error(
                f'https://github.com/{repo.organization.login}/{repo.name}/pull/{p.number}')
        return os.EX_DATAERR

    if args.json:
        bp.print_json()
    else:
        bp.print()

    return os.EX_OK


if __name__ == '__main__':
    sys.exit(main())
