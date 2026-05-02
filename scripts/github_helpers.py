# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

'''
Generic GitHub helper routines which may be useful to other scripts.

This file is not meant to be run directly, but rather to be imported
as a module from other scripts.
'''

# Note that the type annotations are not currently checked by mypy.
# Unless that changes, they serve as documentation, rather than
# guarantees from a type checker.

# stdlib
import getpass
import os
import netrc
import sys
from typing import Dict

# third party
import github

def get_github_credentials(ask: bool = True) -> Dict[str, str]:
    '''Get credentials for constructing a github.Github object.

    This function tries to get github.com credentials from these
    places, in order:

    1. a ~/.netrc file, if one exists
    2. a GITHUB_TOKEN environment variable
    3. if the 'ask' kwarg is truthy, from the user on the
       at the command line.

    On failure, RuntimeError is raised.

    Scripts often need credentials because anonym access to
    api.github.com is rate limited more aggressively than
    authenticated access. Scripts which use anonymous access are
    therefore more likely to fail due to rate limiting.

    The return value is a dict which can be passed to the
    github.Github constructor as **kwargs.

    :param ask: if truthy, the user will be prompted for credentials
                if none are found from other sources
    '''

    try:
        nrc = netrc.netrc()
    except (FileNotFoundError, netrc.NetrcParseError):
        nrc = None

    if nrc is not None:
        auth = nrc.authenticators('github.com')
        if auth is not None:
            return {'login_or_token': auth[0], 'password': auth[2]}

    token = os.environ.get('GITHUB_TOKEN')
    if token:
        return {'login_or_token': token}

    if ask:
        print('Missing GitHub credentials:\n'
              '~/.netrc file not found or has no github.com credentials, '
              'and GITHUB_TOKEN is not set in the environment. '
              'Please give your GitHub token.',
              file=sys.stderr)
        token = getpass.getpass('token: ')
        return {'login_or_token': token}

    raise RuntimeError('no credentials found')

def get_github_object(ask: bool = True) -> github.Github:
    '''Get a github.Github object, created with credentials.

    :param ask: passed to get_github_credentials()
    '''
    return github.Github(**get_github_credentials())
