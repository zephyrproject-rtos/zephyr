#!/usr/bin/env python3

# Copyright 2025 Google LLC
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import sys

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

    return parser.parse_args(argv)


def main(argv):
    args = parse_args(argv)

    token = os.environ.get('GITHUB_TOKEN', None)
    gh = github.Github(token)

    print_rate_limit(gh, "zephyrproject-rtos")

    repo = gh.get_repo("zephyrproject-rtos/zephyr")
    pr = repo.get_pull(args.pull_request)

    for label in pr.get_labels():
        if label.name in DNM_LABELS:
            print(f"Pull request is labeled as \"{label.name}\".")
            print("This workflow fails so that the pull request cannot be merged.")
            sys.exit(1)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
