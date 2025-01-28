#!/usr/bin/env python3

# Copyright 2025 Google LLC
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import sys

import github

DNM_LABELS = ["DNM", "TSC", "Architecture Review", "dev-review"]


def parse_args(argv):
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )

    parser.add_argument("-p", "--pr", required=True, type=int, help="The PR number")

    return parser.parse_args(argv)


def main(argv):
    args = parse_args(argv)

    token = os.environ.get('GITHUB_TOKEN', None)
    gh = github.Github(token)
    repo = gh.get_repo("zephyrproject-rtos/zephyr")
    pr = repo.get_pull(args.pr)

    print(f"working on {pr.html_url}")

    want_approvers = None
    body = pr.body
    for line in body.splitlines():
        if not line.startswith("WANT_APPROVAL="):
            continue
        _, names = line.split("=", 2)
        want_approvers = set(names.split(","))
        print(f"want_approvers: {want_approvers}")

    if not want_approvers:
        print("no WANT_APPROVAL line")
        sys.exit(0)

    approvers = set()
    for review in pr.get_reviews():
        if review.user:
            if review.state == 'APPROVED':
                approvers.add(review.user.login)
            elif review.state in ['DISMISSED', 'CHANGES_REQUESTED']:
                approvers.discard(review.user.login)

    print(f"approvers: {approvers}")

    if want_approvers.issubset(approvers):
        print("all approvers approved")
        sys.exit(0)
    else:
        print("missing approvers")
        sys.exit(1)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
