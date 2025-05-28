# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import os
import requests
import sys

from west import log

from fetchers.core import ZephyrBlobFetcher

class HTTPFetcher(ZephyrBlobFetcher):

    @classmethod
    def schemes(cls):
        return ['http', 'https']

    def fetch(self, url, path):
        log.dbg(f'HTTPFetcher fetching {url} to {path}')
        try:
            resp = requests.get(url)
            resp.raise_for_status()  # Raises an HTTPError for bad status codes (4xx or 5xx)
        except requests.exceptions.HTTPError as e:
            log.err(f'HTTP error occurred: {e}')
            sys.exit(os.EX_NOHOST)
        except requests.exceptions.RequestException as e:
            log.err(f'An error occurred: {e}')
            sys.exit(os.EX_DATAERR)
        with open(path, "wb") as f:
            f.write(resp.content)
