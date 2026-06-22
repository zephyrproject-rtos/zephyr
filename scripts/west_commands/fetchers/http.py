# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import requests

from fetchers.core import ZephyrBlobException, ZephyrBlobFetcher

class HTTPFetcher(ZephyrBlobFetcher):

    @classmethod
    def schemes(cls):
        return ['http', 'https']

    def fetch(self, west_command, blob, path):
        url = blob['url']
        west_command.dbg(f'HTTPFetcher fetching {url} to {path}')
        try:
            resp = requests.get(url)
            resp.raise_for_status()  # Raises an HTTPError for bad status codes (4xx or 5xx)
        except requests.exceptions.HTTPError as e:
            raise ZephyrBlobException(f'HTTP error occurred: {e}') from e
        except requests.exceptions.RequestException as e:
            raise ZephyrBlobException(f'An error occurred: {e}') from e
        with open(path, "wb") as f:
            f.write(resp.content)
