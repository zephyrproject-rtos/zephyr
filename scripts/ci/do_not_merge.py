#!/usr/bin/env python3

# Copyright 2025 Google LLC
# SPDX-License-Identifier: Apache-2.0

import argparse
import datetime
import os
import sys
import time

import github

DNM_LABELS = ["DNM", "DNM (manifest)", "TSC", "Architecture Review", "dev-review"]


def print_rate_limit(gh, org):
    response = gh.get_organization(org)
    for header, value in response.raw_headers.items():
        if header.startswith("x-ratelimit"):
            print(f"{header}: {value}")


def parse_args(argv):
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )

    parser.add_argument("-p", "--pull-request", required=True, type=int, help="The PR number")
    parser.add_argument("-o", "--org", default="zephyrproject-rtos", help="Github organization")
    parser.add_argument("-r", "--repo", default="zephyr", help="Github repository")

    return parser.parse_args(argv)


WAIT_FOR_WORKFLOWS = set({"Manifest"})
WAIT_FOR_DELAY_S = 60


def workflow_delay(repo, pr):
    print(f"PR is at {pr.head.sha}")

    while True:
        runs = repo.get_workflow_runs(head_sha=pr.head.sha)

        completed = set()
        for run in runs:
            print(f"{run.name}: {run.status} {run.conclusion} {run.html_url}")
            if run.status == "completed":
                completed.add(run.name)

        if WAIT_FOR_WORKFLOWS.issubset(completed):
            return

        ts = datetime.datetime.now()
        print(f"wait: {ts} completed={completed}")
        time.sleep(WAIT_FOR_DELAY_S)


def main(argv):
    args = parse_args(argv)

    auth = github.Auth.Token(os.environ.get('GITHUB_TOKEN', None))
    gh = github.Github(auth=auth)

    print_rate_limit(gh, args.org)

    repo = gh.get_repo(f"{args.org}/{args.repo}")
    pr = repo.get_pull(args.pull_request)

    workflow_delay(repo, pr)

    print(f"pr: {pr.html_url}")

    fail = False

    for label in pr.get_labels():
        print(f"label: {label.name}")

        if label.name in DNM_LABELS:
            print(f"Pull request is labeled as \"{label.name}\".")
            fail = True

    if not pr.body:
        print("Pull request is description is empty.")
        fail = True

    if fail:
        print("This workflow fails so that the pull request cannot be merged.")
        sys.exit(1)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
