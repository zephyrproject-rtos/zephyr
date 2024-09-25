#!/usr/bin/env python3
# Copyright (c) 2020-2023 Intel Corp.
# SPDX-License-Identifier: Apache-2.0

"""
Syntax of file:
    [
        {
            "version": "<commit>",
            "date": "<date>",
            "weekly: False,
        },
    ]
"""
import json
import argparse
import urllib.request
import os
import tempfile

from git import Git
from datetime import datetime

VERSIONS_FILE = "versions.json"


def parse_args():
    parser = argparse.ArgumentParser(
                description="Manage versions to be tested.", allow_abbrev=False)
    parser.add_argument('-l', '--list', action="store_true",
                        help="List all published versions")
    parser.add_argument('-u', '--update',
                        help="Update versions file from tree.")
    parser.add_argument('-L', '--latest', action="store_true",
                        help="Get latest published version")
    parser.add_argument('-w', '--weekly', action="store_true",
                        help="Mark as weekly")
    parser.add_argument('-W', '--list-weekly', action="store_true",
                        help="List weekly commits")
    parser.add_argument('-v', '--verbose', action="store_true",
                        help="Verbose output")
    return parser.parse_args()


def get_versions():
    data = None
    fo = tempfile.NamedTemporaryFile()
    if not os.path.exists('versions.json'):
        url = 'https://testing.zephyrproject.org/daily_tests/versions.json'
        urllib.request.urlretrieve(url, fo.name)
    with open(fo.name, "r") as fp:
        data = json.load(fp)
    return data

def handle_compat(item):
    item_compat = {}
    if isinstance(item, str):
        item_compat['version'] =  item
        item_compat['weekly'] = False
        item_compat['date'] = None
    else:
        item_compat = item

    return item_compat

def show_versions(weekly=False):
    data = get_versions()
    for item in data:
        item_compat = handle_compat(item)
        is_weekly = item_compat.get('weekly', False)
        if weekly and not is_weekly:
            continue
        wstr = ""
        datestr = ""
        if args.verbose:
            if is_weekly:
                wstr = "(marked for weekly testing)"
            if item_compat.get('date'):
                pdate = datetime.strptime(item_compat['date'], '%Y-%m-%dT%H:%M:%S.%f')
                date = pdate.strftime("%b %d %Y %H:%M:%S")
                datestr = f"published on {date}"
            print(f"- {item_compat['version']} {datestr} {wstr}")
        else:
            print(f"{item_compat['version']}")



def show_latest():
    data = get_versions()
    latest = data[-1]
    item_compat = handle_compat(latest)

    ver = item_compat.get("version")
    date = item_compat.get("date", False)
    is_weekly = item_compat.get('weekly')
    datestr = ""
    if date:
        datestr = f"published on {date}"
    if args.verbose:
        print(f"Latest version is {ver} {datestr}")
    if args.verbose and is_weekly:
        print("This version is marked for weekly testing.")

    if not args.verbose:
        print(f"{ver}")


def update(git_tree, is_weekly=False):
    g = Git(git_tree)
    today = datetime.now().strftime('%Y-%m-%dT%H:%M:%S.%f')
    version = g.describe("--abbrev=12")
    published = False
    data = get_versions()

    if not is_weekly:
        wday = datetime.today().strftime('%A')
        if wday == 'Monday':
            is_weekly = True

    found = list(filter(lambda item: (isinstance(item, dict) and
                        item.get('version') == version) or item == version, data))
    if found:
        published = True
        print("version already published")
    else:
        print(f"New version {version}, adding to file...")

    if data and not published:
        with open(VERSIONS_FILE, "w") as versions:
            item = {}
            item['version'] = version
            item['date'] = today
            item['weekly'] = is_weekly
            data.append(item)
            json.dump(data, versions)

def main():
    global args

    args = parse_args()
    if args.update:
        update(args.update, args.weekly)
    elif args.list or args.list_weekly:
        show_versions(weekly=args.list_weekly)
    elif args.latest:
        show_latest()
    else:
        print("You did not specify any options")

if __name__ == "__main__":
    main()
