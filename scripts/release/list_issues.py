#!/usr/bin/env python3
#
# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

# Lists all closes issues since a given date

import argparse
import sys
import os
import re
import time
import threading
import requests


args = None


class Spinner:
    busy = False
    delay = 0.1

    @staticmethod
    def spinning_cursor():
        while 1:
            for cursor in '|/-\\':
                yield cursor

    def __init__(self, delay=None):
        self.spinner_generator = self.spinning_cursor()
        if delay and float(delay):
            self.delay = delay

    def spinner_task(self):
        while self.busy:
            sys.stdout.write(next(self.spinner_generator))
            sys.stdout.flush()
            time.sleep(self.delay)
            sys.stdout.write('\b')
            sys.stdout.flush()

    def __enter__(self):
        self.busy = True
        threading.Thread(target=self.spinner_task).start()

    def __exit__(self, exception, value, tb):
        self.busy = False
        time.sleep(self.delay)
        if exception is not None:
            return False


class Issues:
    def __init__(self, org, repo, token):
        self.repo = repo
        self.org = org
        self.issues_url = "https://github.com/%s/%s/issues" % (
            self.org, self.repo)
        self.github_url = 'https://api.github.com/repos/%s/%s' % (
            self.org, self.repo)

        self.api_token = token
        self.headers = {}
        self.headers['Authorization'] = 'token %s' % self.api_token
        self.headers['Accept'] = 'application/vnd.github.golden-comet-preview+json'
        self.items = []

    def get_pull(self, pull_nr):
        url = ("%s/pulls/%s" % (self.github_url, pull_nr))
        response = requests.get("%s" % (url), headers=self.headers)
        if response.status_code != 200:
            raise RuntimeError(
                "Failed to get issue due to unexpected HTTP status code: {}".format(
                    response.status_code)
            )
        item = response.json()
        return item

    def get_issue(self, issue_nr):
        url = ("%s/issues/%s" % (self.github_url, issue_nr))
        response = requests.get("%s" % (url), headers=self.headers)
        if response.status_code != 200:
            return None

        item = response.json()
        return item

    def list_issues(self, url):
        response = requests.get("%s" % (url), headers=self.headers)
        if response.status_code != 200:
            raise RuntimeError(
                "Failed to get issue due to unexpected HTTP status code: {}".format(
                    response.status_code)
            )
        self.items = self.items + response.json()

        try:
            print("Getting more items...")
            next_issues = response.links["next"]
            if next_issues:
                next_url = next_issues['url']
                self.list_issues(next_url)
        except KeyError:
            pass

    def issues_since(self, date, state="closed"):
        self.list_issues("%s/issues?state=%s&since=%s" %
                         (self.github_url, state, date))

    def pull_requests(self, base='v1.14-branch', state='closed'):
        self.list_issues("%s/pulls?state=%s&base=%s" %
                         (self.github_url, state, base))


def parse_args():
    global args

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("-o", "--org", default="zephyrproject-rtos",
                        help="Github organisation")

    parser.add_argument("-r", "--repo", default="zephyr",
                        help="Github repository")

    parser.add_argument("-f", "--file", required=True,
                        help="Name of output file.")

    parser.add_argument("-s", "--issues-since",
                        help="""List issues since date where date
        is in the format 2019-09-01.""")

    parser.add_argument("-b", "--issues-in-pulls",
                        help="List issues in pulls for a given branch")

    parser.add_argument("-c", "--commits-file",
                        help="""File with all commits (git log a..b) to
        be parsed for fixed bugs.""")

    args = parser.parse_args()


def main():
    parse_args()

    token = os.environ.get('GITHUB_TOKEN', None)
    if not token:
        sys.exit("""Github token not set in environment,
set the env. variable GITHUB_TOKEN please and retry.""")

    i = Issues(args.org, args.repo, token)

    if args.issues_since:
        i.issues_since(args.issues_since)
        count = 0
        with open(args.file, "w") as f:
            for issue in i.items:
                if 'pull_request' not in issue:
                    # * :github:`8193` - STM32 config BUILD_OUTPUT_HEX fail
                    f.write("* :github:`{}` - {}\n".format(
                        issue['number'], issue['title']))
                    count = count + 1
    elif args.issues_in_pulls:
        i.pull_requests(base=args.issues_in_pulls)
        count = 0

        bugs = set()
        backports = []
        for issue in i.items:
            if not isinstance(issue['body'], str):
                continue
            match = re.findall(r"(Fixes|Closes|Fixed|close):? #([0-9]+)",
                               issue['body'], re.MULTILINE)
            if match:
                for mm in match:
                    bugs.add(mm[1])
            else:
                match = re.findall(
                    r"Backport #([0-9]+)", issue['body'], re.MULTILINE)
                if match:
                    backports.append(match[0])

        # follow PRs to their origin (backports)
        with Spinner():
            for p in backports:
                item = i.get_pull(p)
                match = re.findall(r"(Fixes|Closes|Fixed|close):? #([0-9]+)",
                                   item['body'], re.MULTILINE)
                for mm in match:
                    bugs.add(mm[1])

        # now open commits
        if args.commits_file:
            print("Open commits file and parse for fixed bugs...")
            with open(args.commits_file, "r") as commits:
                content = commits.read()
                match = re.findall(r"(Fixes|Closes|Fixed|close):? #([0-9]+)",
                                   str(content), re.MULTILINE)
                for mm in match:
                    bugs.add(mm[1])

        print("Create output file...")
        with Spinner():
            with open(args.file, "w") as f:
                for m in sorted(bugs):
                    item = i.get_issue(m)
                    if item:
                        # * :github:`8193` - STM32 config BUILD_OUTPUT_HEX fail
                        f.write("* :github:`{}` - {}\n".format(
                                item['number'], item['title']))


if __name__ == '__main__':
    main()
