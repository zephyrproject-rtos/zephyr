#!/usr/bin/env python3

# Copyright (c) 2024 Vestas Wind Systems A/S
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import sys

from get_maintainer import Maintainers
from github.GithubException import UnknownObjectException
from github_helpers import get_github_object


def parse_args():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=__doc__, allow_abbrev=False)

    parser.add_argument(
        "-m", "--maintainers",
        metavar="MAINTAINERS_FILE",
        help="Maintainers file to load. If not specified, MAINTAINERS.yml in "
             "the top-level repository directory is used, and must exist. "
             "Paths in the maintainers file will always be taken as relative "
             "to the top-level directory.")

    return parser.parse_args()

def main() -> None:
    args = parse_args()
    zephyr_repo = get_github_object().get_repo('zephyrproject-rtos/zephyr')
    maintainers = Maintainers(args.maintainers)
    gh = get_github_object()
    gh_users = []
    notfound = []
    noncollabs = []

    for area in maintainers.areas.values():
        gh_users = list(set(gh_users + area.maintainers + area.collaborators))

    gh_users.sort()

    print('Checking maintainer and collaborator user accounts on GitHub:')
    for gh_user in gh_users:
        try:
            print('.', end='', flush=True)
            gh.get_user(gh_user)

            if not zephyr_repo.has_in_collaborators(gh_user):
                noncollabs.append(gh_user)
        except UnknownObjectException:
            notfound.append(gh_user)
    print('\n')

    if notfound:
        print('The following GitHub user accounts do not exist:')
        print('\n'.join(notfound))
    else:
        print('No non-existing user accounts found')

    if noncollabs:
        print('The following GitHub user accounts are not collaborators:')
        print('\n'.join(noncollabs))
    else:
        print('No non-collaborator user accounts found')

    if notfound or noncollabs:
        sys.exit(1)

if __name__ == '__main__':
    main()
