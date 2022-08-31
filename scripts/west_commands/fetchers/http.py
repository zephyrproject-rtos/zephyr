# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import requests

from west import log

from fetchers.core import ZephyrBlobFetcher

class HTTPFetcher(ZephyrBlobFetcher):

    @classmethod
    def schemes(cls):
        return ['http', 'https']

    def fetch(self, url, path):
        log.dbg(f'HTTPFetcher fetching {url} to {path}')
        resp = requests.get(url)
        open(path, "wb").write(resp.content)
