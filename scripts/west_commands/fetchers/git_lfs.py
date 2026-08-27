# Copyright (c) 2025 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

import requests

from fetchers.core import ZephyrBlobException, ZephyrBlobFetcher


class GitLFSFetcher(ZephyrBlobFetcher):
    """
    Fetcher for Git LFS (Large File Storage) objects.

    Sends a request to a Git LFS endpoint to get the URL of the requested blob before downloading
    the blob itself. The ``sha256`` and ``size`` fields from the blob metadata are used for the
    ``oid`` and ``size`` parameters of the LFS API, and the ``url`` field is used as the LFS API
    URL.
    """

    @classmethod
    def schemes(cls):
        return ['lfs']

    def fetch(self, west_command, blob, path):
        url = blob["url"]
        oid = blob["sha256"]

        if "size" in blob:
            size = blob["size"]
        else:
            raise ZephyrBlobException(f"'size' field missing for blob {path}, unable to fetch")

        west_command.dbg(f'GitLFSFetcher fetching {oid} from {url}')

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
                    "oid": oid,
                    "size": size,
                }
            ],
        }

        try:
            resp = requests.post(url + "/objects/batch", json=request, headers=headers)
            resp.raise_for_status()  # Raises an HTTPError for bad status codes (4xx or 5xx)
        except requests.exceptions.HTTPError as e:
            raise ZephyrBlobException(f'HTTP error occurred: {e}') from e
        except requests.exceptions.RequestException as e:
            raise ZephyrBlobException(f'An error occurred: {e}') from e

        # Download the LFS object
        # see https://github.com/git-lfs/git-lfs/blob/main/docs/api/batch.md#successful-responses
        for obj in resp.json().get("objects", []):
            if obj["oid"] == oid:
                if "error" in obj:
                    raise ZephyrBlobException(
                        f'Failed to download LFS object {oid}: {obj["error"]}'
                    )

                try:
                    download = obj["actions"]["download"]
                except KeyError as e:
                    raise ZephyrBlobException(f'Unexpected response from LFS server: {obj}') from e

                west_command.dbg(f'GitLFSFetcher fetching {download["href"]} to {path}')
                try:
                    resp = requests.get(download["href"], headers=download.get("header", {}))
                    resp.raise_for_status()  # Raises an HTTPError for bad status codes (4xx or 5xx)
                except requests.exceptions.HTTPError as e:
                    raise ZephyrBlobException(f'HTTP error occurred: {e}') from e
                except requests.exceptions.RequestException as e:
                    raise ZephyrBlobException(f'An error occurred: {e}') from e
                with open(path, "wb") as f:
                    f.write(resp.content)
                break
        else:
            raise ZephyrBlobException(f'Failed to download LFS object {oid}')
