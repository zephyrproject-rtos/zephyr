# Copyright (c) 2026
#
# SPDX-License-Identifier: Apache-2.0

import json

import pytest
import zephyr_module

import fetchers
from blobs import Blobs
from fetchers.core import ZephyrBlobException

ORIGINAL_URL = 'https://github.com/example/repo.git'


class FakeConfig:
    '''Minimal stand-in for west.configuration.Configuration in tests.

    Tracks how many times get() is called, so tests can verify that
    'blobs.mirrors' is only read (and parsed) once per Blobs() instance.
    '''

    def __init__(self, mirrors_value):
        self.mirrors_value = mirrors_value
        self.get_calls = 0

    def get(self, key, *a, **kw):
        if key == 'blobs.mirrors':
            self.get_calls += 1
            return self.mirrors_value
        return None

    def getboolean(self, key, default=False):
        return default


def _blob_cmd_with_config(mirrors_value):
    '''Build a Blobs() instance whose self.config.get('blobs.mirrors')
    returns the given raw config string (or None if unset).
    '''
    blob_cmd = Blobs()
    blob_cmd.config = FakeConfig(mirrors_value)
    return blob_cmd


def test_get_valid_mirrors_object_form_single_mirror():
    # {"<remote>": "<mirror>"}
    mirrors = json.dumps({'https://github.com/': 'https://mirror.example.com/'})
    blob_cmd = _blob_cmd_with_config(mirrors)

    assert blob_cmd.get_valid_mirrors(ORIGINAL_URL) == [
        'https://mirror.example.com/example/repo.git'
    ]


def test_get_valid_mirrors_object_form_multiple_remotes():
    # {"<remote1>": "<mirror1>", "<remote2>": "<mirror2>"}
    mirrors = json.dumps(
        {
            'https://github.com/': 'https://mirror1.example.com/',
            'https://gitlab.com/': 'https://mirror2.example.com/',
        }
    )
    blob_cmd = _blob_cmd_with_config(mirrors)

    assert blob_cmd.get_valid_mirrors(ORIGINAL_URL) == [
        'https://mirror1.example.com/example/repo.git'
    ]
    assert blob_cmd.get_valid_mirrors('https://gitlab.com/example/repo.git') == [
        'https://mirror2.example.com/example/repo.git'
    ]


def test_get_valid_mirrors_matches_remote_case_insensitively():
    mirrors = json.dumps({'https://GitHub.com/': 'https://mirror.example.com/'})
    blob_cmd = _blob_cmd_with_config(mirrors)

    assert blob_cmd.get_valid_mirrors('HTTPS://github.COM/example/repo.git') == [
        'https://mirror.example.com/example/repo.git'
    ]


def test_get_valid_mirrors_object_form_list_of_mirrors():
    # {"<remote>": ["<mirror1>", "<mirror2>"]}
    mirrors = json.dumps(
        {
            'https://github.com/': [
                'https://mirror1.example.com/',
                'https://mirror2.example.com/',
            ]
        }
    )
    blob_cmd = _blob_cmd_with_config(mirrors)

    assert blob_cmd.get_valid_mirrors(ORIGINAL_URL) == [
        'https://mirror1.example.com/example/repo.git',
        'https://mirror2.example.com/example/repo.git',
    ]


def test_get_valid_mirrors_when_no_match():
    mirrors = json.dumps([{'https://gitlab.com/': 'https://mirror.example.com/'}])
    blob_cmd = _blob_cmd_with_config(mirrors)

    assert blob_cmd.get_valid_mirrors(ORIGINAL_URL) == []


def test_get_valid_mirrors_dies_on_invalid_json():
    blob_cmd = _blob_cmd_with_config('not valid json')

    with pytest.raises(SystemExit):
        blob_cmd.get_valid_mirrors(ORIGINAL_URL)


def test_get_valid_mirrors_dies_on_unsupported_top_level_type():
    # Neither a JSON object nor a JSON list.
    mirrors = json.dumps('https://mirror.example.com/')
    blob_cmd = _blob_cmd_with_config(mirrors)

    with pytest.raises(SystemExit):
        blob_cmd.get_valid_mirrors(ORIGINAL_URL)


def test_get_valid_mirrors_dies_on_malformed_list_entry():
    # Entries must be single-key objects; a two-key entry is ambiguous.
    mirrors = json.dumps(
        [
            {'https://github.com/': 'https://mirror1.example.com/', 'extra': 'key'},
            {'https://gitlab.com/': 'https://mirror2.example.com/'},
        ]
    )
    blob_cmd = _blob_cmd_with_config(mirrors)

    with pytest.raises(SystemExit):
        blob_cmd.get_valid_mirrors(ORIGINAL_URL)


def test_get_valid_mirrors_dies_on_non_string_entry():
    # Keys/values must both be strings.
    mirrors = json.dumps([{'https://github.com/': 1}])
    blob_cmd = _blob_cmd_with_config(mirrors)

    with pytest.raises(SystemExit):
        blob_cmd.get_valid_mirrors(ORIGINAL_URL)


def test_get_valid_mirrors_dies_on_non_string_entry_in_object_form_list():
    # A list-of-mirrors value in object form must contain only strings.
    mirrors = json.dumps({'https://github.com/': ['https://mirror.example.com/', 1]})
    blob_cmd = _blob_cmd_with_config(mirrors)

    with pytest.raises(SystemExit):
        blob_cmd.get_valid_mirrors(ORIGINAL_URL)


def test_get_valid_mirrors_dies_on_non_http_mirror():
    # Mirrors must be fetchable over http(s); ssh (and other schemes) cannot
    # be used to download blobs.
    mirrors = json.dumps([{'https://github.com/': 'ssh://git@example.com/mirror/'}])
    blob_cmd = _blob_cmd_with_config(mirrors)

    with pytest.raises(SystemExit):
        blob_cmd.get_valid_mirrors(ORIGINAL_URL)


def test_get_valid_mirrors_returns_all_matches_in_order():
    mirrors = json.dumps(
        [
            {'https://github.com/': 'https://mirror1.example.com/'},
            {'https://github.com/': 'https://mirror2.example.com/'},
        ]
    )
    blob_cmd = _blob_cmd_with_config(mirrors)

    assert blob_cmd.get_valid_mirrors(ORIGINAL_URL) == [
        'https://mirror1.example.com/example/repo.git',
        'https://mirror2.example.com/example/repo.git',
    ]


def test_get_valid_mirrors_orders_longest_matching_remote_first():
    # A more specific (longer) remote URL prefix's mirror comes before a
    # shorter, more general one that also matches, like git's "insteadOf".
    # Both are kept; the shorter match is not dropped.
    mirrors = json.dumps(
        {
            'https://github.com/': 'https://general-mirror.example.com/',
            'https://github.com/zephyrproject-rtos/': 'https://specific-mirror.example.com/',
        }
    )
    blob_cmd = _blob_cmd_with_config(mirrors)

    assert blob_cmd.get_valid_mirrors('https://github.com/zephyrproject-rtos/zephyr') == [
        'https://specific-mirror.example.com/zephyr',
        'https://general-mirror.example.com/zephyrproject-rtos/zephyr',
    ]
    assert blob_cmd.get_valid_mirrors('https://github.com/other-org/repo') == [
        'https://general-mirror.example.com/other-org/repo'
    ]


def test_get_valid_mirrors_keeps_listed_order_within_same_remote():
    # Mirrors for the longest-matching remote still keep their relative
    # listed order among themselves, ahead of shorter matches.
    mirrors = json.dumps(
        [
            {'https://github.com/': 'https://general-mirror.example.com/'},
            {'https://github.com/zephyrproject-rtos/': 'https://specific-mirror1.example.com/'},
            {'https://github.com/zephyrproject-rtos/': 'https://specific-mirror2.example.com/'},
        ]
    )
    blob_cmd = _blob_cmd_with_config(mirrors)

    assert blob_cmd.get_valid_mirrors('https://github.com/zephyrproject-rtos/zephyr') == [
        'https://specific-mirror1.example.com/zephyr',
        'https://specific-mirror2.example.com/zephyr',
        'https://general-mirror.example.com/zephyrproject-rtos/zephyr',
    ]


def test_get_valid_mirrors_reads_config_only_once():
    mirrors = json.dumps([{'https://github.com/': 'https://mirror.example.com/'}])
    blob_cmd = _blob_cmd_with_config(mirrors)

    blob_cmd.get_valid_mirrors(ORIGINAL_URL)
    blob_cmd.get_valid_mirrors('https://github.com/other/repo.git')
    blob_cmd.get_valid_mirrors('https://gitlab.com/example/repo.git')

    assert blob_cmd.config.get_calls == 1


def test_download_blob_uses_mirror_first_then_falls_back(monkeypatch, tmp_path):
    mirrors = json.dumps([{'https://github.com/': 'https://mirror.example.com/'}])
    blob_cmd = _blob_cmd_with_config(mirrors)

    attempted_urls = []

    class FakeFetcher:
        def fetch(self, _cmd, blob, path):
            attempted_urls.append(blob['url'])
            if blob['url'].startswith('https://mirror.example.com/'):
                raise ZephyrBlobException('mirror unavailable')
            path.write_bytes(b'ok')

    monkeypatch.setattr(fetchers, 'get_fetcher_cls', lambda _scheme: FakeFetcher)
    monkeypatch.setattr(
        zephyr_module, 'get_blob_status', lambda _path, _sha: zephyr_module.BLOB_PRESENT
    )

    blob = {'url': ORIGINAL_URL, 'sha256': 'dummy', 'path': 'blobs/blob.bin'}
    blob_cmd.download_blob(blob, tmp_path / 'blob.bin')

    assert attempted_urls == ['https://mirror.example.com/example/repo.git', ORIGINAL_URL]


def test_download_blob_tries_all_mirrors_in_order_then_falls_back(monkeypatch, tmp_path):
    mirrors = json.dumps(
        [
            {'https://github.com/': 'https://mirror1.example.com/'},
            {'https://github.com/': 'https://mirror2.example.com/'},
        ]
    )
    blob_cmd = _blob_cmd_with_config(mirrors)

    attempted_urls = []

    class FakeFetcher:
        def fetch(self, _cmd, blob, path):
            attempted_urls.append(blob['url'])
            if blob['url'] != ORIGINAL_URL:
                raise ZephyrBlobException('mirror unavailable')
            path.write_bytes(b'ok')

    monkeypatch.setattr(fetchers, 'get_fetcher_cls', lambda _scheme: FakeFetcher)
    monkeypatch.setattr(
        zephyr_module, 'get_blob_status', lambda _path, _sha: zephyr_module.BLOB_PRESENT
    )

    blob = {'url': ORIGINAL_URL, 'sha256': 'dummy', 'path': 'blobs/blob.bin'}
    blob_cmd.download_blob(blob, tmp_path / 'blob.bin')

    assert attempted_urls == [
        'https://mirror1.example.com/example/repo.git',
        'https://mirror2.example.com/example/repo.git',
        ORIGINAL_URL,
    ]
