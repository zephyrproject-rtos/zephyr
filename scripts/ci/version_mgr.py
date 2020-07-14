#!/usr/bin/env python3
# Copyright (c) 2020 Intel Corp.
# SPDX-License-Identifier: Apache-2.0

import json
import argparse
import urllib.request

from git import Git

VERSIONS_FILE = "versions.json"

def parse_args():
    parser = argparse.ArgumentParser(
                description="Manage versions to be tested.")
    parser.add_argument('-l', '--list', action="store_true",
            help="List all published versions")
    parser.add_argument('-u', '--update',
            help="Update versions file from tree.")
    parser.add_argument('-L', '--latest', action="store_true",
            help="Get latest published version")
    return parser.parse_args()

def get_versions():
    data = None
    url = 'https://testing.zephyrproject.org/daily_tests/versions.json'
    urllib.request.urlretrieve(url, 'versions.json')
    with open("versions.json", "r") as fp:
        data = json.load(fp)

    return data

def show_versions():
    data = get_versions()
    for v in data:
        print(f"- {v}")

def show_latest():
    data = get_versions()
    print(data[-1])

def update(git_tree):
    g = Git(git_tree)
    version = g.describe()
    published = False
    data = get_versions()
    if version in data:
        published = True
        print("version already published")
    else:
        print(f"New version {version}, adding to file...")

    if data and not published:
        with open(VERSIONS_FILE, "w") as versions:
            data.append(version)
            json.dump(data, versions)

def main():
    args = parse_args()
    if args.update:
        update(args.update)
    elif args.list:
        show_versions()
    elif args.latest:
        show_latest()

if __name__ == "__main__":
    main()
