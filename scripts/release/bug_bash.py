#!/usr/bin/env python3
# Copyright (c) 2021, Facebook
#
# SPDX-License-Identifier: Apache-2.0

"""Query the Top-Ten Bug Bashers

This script will query the top-ten Bug Bashers in a specified date window.

Usage:
    ./scripts/bug-bash.py -t ~/.ghtoken -b 2021-07-26 -e 2021-08-07
    GITHUB_TOKEN="..." ./scripts/bug-bash.py -b 2021-07-26 -e 2021-08-07
"""

import argparse
from datetime import datetime, timedelta
import operator
import os

# Requires PyGithub
from github import Github


def parse_args():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument('-a', '--all', dest='all',
                        help='Show all bugs squashed', action='store_true')
    parser.add_argument('-t', '--token', dest='tokenfile',
                        help='File containing GitHub token (alternatively, use GITHUB_TOKEN env variable)', metavar='FILE')
    parser.add_argument('-s', '--start', dest='start', help='start date (YYYY-mm-dd)',
                        metavar='START_DATE', type=valid_date_type, required=True)
    parser.add_argument('-e', '--end', dest='end', help='end date (YYYY-mm-dd)',
                        metavar='END_DATE', type=valid_date_type, required=True)

    args = parser.parse_args()

    if args.end < args.start:
        raise ValueError(
            'end date {} is before start date {}'.format(args.end, args.start))

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


class BugBashTally(object):
    def __init__(self, gh, start_date, end_date):
        """Create a BugBashTally object with the provided Github object,
        start datetime object, and end datetime object"""
        self._gh = gh
        self._repo = gh.get_repo('zephyrproject-rtos/zephyr')
        self._start_date = start_date
        self._end_date = end_date

        self._issues = []
        self._pulls = []

    def get_tally(self):
        """Return a dict with (key = user, value = score)"""
        tally = dict()
        for p in self.get_pulls():
            user = p.user.login
            tally[user] = tally.get(user, 0) + 1

        return tally

    def get_rev_tally(self):
        """Return a dict with (key = score, value = list<user>) sorted in
        descending order"""
        # there may be ties!
        rev_tally = dict()
        for user, score in self.get_tally().items():
            if score not in rev_tally:
                rev_tally[score] = [user]
            else:
                rev_tally[score].append(user)

        # sort in descending order by score
        rev_tally = dict(
            sorted(rev_tally.items(), key=operator.itemgetter(0), reverse=True))

        return rev_tally

    def get_top_ten(self):
        """Return a dict with (key = score, value = user) sorted in
        descending order"""
        top_ten = []
        for score, users in self.get_rev_tally().items():
            # do not sort users by login - hopefully fair-ish
            for user in users:
                if len(top_ten) == 10:
                    return top_ten

                top_ten.append(tuple([score, user]))

        return top_ten

    def get_pulls(self):
        """Return GitHub pull requests that squash bugs in the provided
        date window"""
        if self._pulls:
            return self._pulls

        self.get_issues()

        return self._pulls

    def get_issues(self):
        """Return GitHub issues representing bugs in the provided date
        window"""
        if self._issues:
            return self._issues

        cutoff = self._end_date + timedelta(1)
        issues = self._repo.get_issues(state='closed', labels=[
            'bug'], since=self._start_date)

        for i in issues:
            # the PyGithub API and v3 REST API do not facilitate 'until'
            # or 'end date' :-/
            if i.closed_at < self._start_date or i.closed_at > cutoff:
                continue

            ipr = i.pull_request
            if ipr is None:
                # ignore issues without a linked pull request
                continue

            prid = int(ipr.html_url.split('/')[-1])
            pr = self._repo.get_pull(prid)
            if not pr.merged:
                # pull requests that were not merged do not count
                continue

            self._pulls.append(pr)
            self._issues.append(i)

        return self._issues


# https://gist.github.com/monkut/e60eea811ef085a6540f
def valid_date_type(arg_date_str):
    """custom argparse *date* type for user dates values given from the
    command line"""
    try:
        return datetime.strptime(arg_date_str, "%Y-%m-%d")
    except ValueError:
        msg = "Given Date ({0}) not valid! Expected format, YYYY-MM-DD!".format(arg_date_str)
        raise argparse.ArgumentTypeError(msg)


def print_top_ten(top_ten):
    """Print the top-ten bug bashers"""
    for score, user in top_ten:
        # print tab-separated value, to allow for ./script ... > foo.csv
        print('{}\t{}'.format(score, user))


def main():
    args = parse_args()
    bbt = BugBashTally(Github(args.token), args.start, args.end)
    if args.all:
        # print one issue per line
        issues = bbt.get_issues()
        pulls = bbt.get_pulls()
        n = len(issues)
        m = len(pulls)
        assert n == m
        for i in range(0, n):
            print('{}\t{}\t{}'.format(
                issues[i].number, pulls[i].user.login, pulls[i].title))
    else:
        # print the top ten
        print_top_ten(bbt.get_top_ten())


if __name__ == '__main__':
    main()
