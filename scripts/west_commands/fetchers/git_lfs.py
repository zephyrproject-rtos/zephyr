# Copyright (c) 2025 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

import os
import sys
from urllib.parse import parse_qsl, urlsplit, urlunsplit

import requests
from west import log

from fetchers.core import ZephyrBlobFetcher


class GitLFSFetcher(ZephyrBlobFetcher):
    """
    Fetcher for Git LFS (Large File Storage) objects.

    Extracts LFS information from custom URLs that use the "lfs-http(s)" scheme, and turns it into
    a request to the Git LFS API to download the actual file.

    The URL format expected by the fetcher is "lfs-<url>?<params>", where '<params>' are the LFS
    object id (oid) and size encoded as query parameters, and '<url>' is the Git LFS API endpoint
    URL.
    """

    @classmethod
    def schemes(cls):
        return ['lfs-http', 'lfs-https']

    def fetch(self, url, path):
        url = urlsplit(url)
        lfs_data = dict(parse_qsl(url.query))
        endpoint = urlunsplit(
            (url.scheme.removeprefix("lfs-"), url.netloc, url.path, '', url.fragment)
        )

        log.dbg(f'GitLFSFetcher fetching {lfs_data["oid"]} from {endpoint}')

        # Perform a batch request to get the download URL of the LFS object
        # see https://github.com/git-lfs/git-lfs/blob/main/docs/api/batch.md#requests
        headers = {
            "Accept": "application/vnd.git-lfs+json",
            "Content-Type": "application/vnd.git-lfs+json",
        }

        request = {
            "operation": "download",
            "transfers": ["basic"],
            "objects": [
                {
                    "oid": lfs_data["oid"],
                    "size": int(lfs_data["size"], 10),
                }
            ],
        }

        try:
            resp = requests.post(endpoint + "/objects/batch", json=request, headers=headers)
            resp.raise_for_status()  # Raises an HTTPError for bad status codes (4xx or 5xx)
        except requests.exceptions.HTTPError as e:
            log.err(f'HTTP error occurred: {e}')
            sys.exit(os.EX_NOHOST)
        except requests.exceptions.RequestException as e:
            log.err(f'An error occurred: {e}')
            sys.exit(os.EX_DATAERR)

        # Download the LFS object
        # see https://github.com/git-lfs/git-lfs/blob/main/docs/api/batch.md#successful-responses
        for obj in resp.json().get("objects", []):
            if obj["oid"] == lfs_data["oid"]:
                if "error" in obj:
                    log.err(f'Failed to download LFS object {lfs_data["oid"]}: {obj["error"]}')
                    sys.exit(os.EX_DATAERR)

                download = obj["actions"]["download"]
                log.dbg(f'GitLFSFetcher fetching {download["href"]} to {path}')
                try:
                    resp = requests.get(download["href"], headers=download.get("headers", {}))
                    resp.raise_for_status()  # Raises an HTTPError for bad status codes (4xx or 5xx)
                except requests.exceptions.HTTPError as e:
                    log.err(f'HTTP error occurred: {e}')
                    sys.exit(os.EX_NOHOST)
                except requests.exceptions.RequestException as e:
                    log.err(f'An error occurred: {e}')
                    sys.exit(os.EX_DATAERR)
                with open(path, "wb") as f:
                    f.write(resp.content)
                break
        else:
            log.err(f'Failed to download LFS object {lfs_data["oid"]}')
            sys.exit(os.EX_DATAERR)
