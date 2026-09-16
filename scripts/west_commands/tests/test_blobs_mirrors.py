# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
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


@pytest.fixture
def blob_cmd():
    def _factory(mirrors_value):
        cmd = Blobs()
        cmd.config = FakeConfig(mirrors_value)
        return cmd

    return _factory


# mirrors config, url, expected get_valid_mirrors() result
TEST_CASES_GET_VALID_MIRRORS = [
    (
        {'https://github.com/': 'https://mirror.example.com/'},
        ORIGINAL_URL,
        ['https://mirror.example.com/example/repo.git'],
    ),
    (
        {
            'https://github.com/': 'https://mirror1.example.com/',
            'https://gitlab.com/': 'https://mirror2.example.com/',
        },
        ORIGINAL_URL,
        ['https://mirror1.example.com/example/repo.git'],
    ),
    (
        {
            'https://github.com/': 'https://mirror1.example.com/',
            'https://gitlab.com/': 'https://mirror2.example.com/',
        },
        'https://gitlab.com/example/repo.git',
        ['https://mirror2.example.com/example/repo.git'],
    ),
    (
        {'https://GitHub.com/': 'https://mirror.example.com/'},
        'https://GitHub.com/example/repo.git',
        ['https://mirror.example.com/example/repo.git'],
    ),
    (
        {'https://GitHub.com/': 'https://mirror.example.com/'},
        'HTTPS://github.COM/example/repo.git',
        [],
    ),
    (
        {
            'https://github.com/': [
                'https://mirror1.example.com/',
                'https://mirror2.example.com/',
            ]
        },
        ORIGINAL_URL,
        [
            'https://mirror1.example.com/example/repo.git',
            'https://mirror2.example.com/example/repo.git',
        ],
    ),
    (
        {'https://gitlab.com/': 'https://mirror.example.com/'},
        ORIGINAL_URL,
        [],
    ),
    # A more specific (longer) remote URL prefix's mirror comes before a
    # shorter, more general one that also matches, like git's "insteadOf".
    # Both are kept; the shorter match is not dropped. Key order in the
    # config doesn't matter since matches are sorted by prefix length.
    (
        {
            'https://github.com/': 'https://general-mirror.example.com/',
            'https://github.com/zephyrproject-rtos/': 'https://specific-mirror.example.com/',
        },
        'https://github.com/zephyrproject-rtos/zephyr',
        [
            'https://specific-mirror.example.com/zephyr',
            'https://general-mirror.example.com/zephyrproject-rtos/zephyr',
        ],
    ),
    (
        {
            'https://github.com/': 'https://general-mirror.example.com/',
            'https://github.com/zephyrproject-rtos/': 'https://specific-mirror.example.com/',
        },
        'https://github.com/other-org/repo',
        ['https://general-mirror.example.com/other-org/repo'],
    ),
    # Mirrors for the longest-matching remote still keep their relative
    # listed order among themselves, ahead of shorter matches.
    (
        {
            'https://github.com/': 'https://general-mirror.example.com/',
            'https://github.com/zephyrproject-rtos/': [
                'https://specific-mirror1.example.com/',
                'https://specific-mirror2.example.com/',
            ],
        },
        'https://github.com/zephyrproject-rtos/zephyr',
        [
            'https://specific-mirror1.example.com/zephyr',
            'https://specific-mirror2.example.com/zephyr',
            'https://general-mirror.example.com/zephyrproject-rtos/zephyr',
        ],
    ),
]


@pytest.mark.parametrize('test_case', TEST_CASES_GET_VALID_MIRRORS)
def test_get_valid_mirrors(blob_cmd, test_case):
    mirrors, url, expected = test_case
    cmd = blob_cmd(json.dumps(mirrors))

    assert cmd.get_valid_mirrors(url) == expected


# mirrors config that get_valid_mirrors() must reject with SystemExit
TEST_CASES_DIES_ON_INVALID_CONFIG = [
    'not valid json',
    # Must be a JSON object, not an array or a bare string.
    json.dumps(['https://github.com/', 'https://mirror.example.com/']),
    json.dumps('https://mirror.example.com/'),
    json.dumps({'https://github.com/': 1}),
    # A list-of-mirrors value must contain only strings.
    json.dumps({'https://github.com/': ['https://mirror.example.com/', 1]}),
]


@pytest.mark.parametrize('mirrors', TEST_CASES_DIES_ON_INVALID_CONFIG)
def test_get_valid_mirrors_dies_on_invalid_config(blob_cmd, mirrors):
    cmd = blob_cmd(mirrors)

    with pytest.raises(SystemExit):
        cmd.get_valid_mirrors(ORIGINAL_URL)


def test_get_valid_mirrors_reads_config_only_once(blob_cmd):
    mirrors = json.dumps({'https://github.com/': 'https://mirror.example.com/'})
    cmd = blob_cmd(mirrors)

    cmd.get_valid_mirrors(ORIGINAL_URL)
    cmd.get_valid_mirrors('https://github.com/other/repo.git')
    cmd.get_valid_mirrors('https://gitlab.com/example/repo.git')

    assert cmd.config.get_calls == 1


def test_download_blob_uses_mirror_first_then_falls_back(blob_cmd, monkeypatch, tmp_path):
    mirrors = json.dumps({'https://github.com/': 'https://mirror.example.com/'})
    cmd = blob_cmd(mirrors)

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
    cmd.download_blob(blob, tmp_path / 'blob.bin')

    assert attempted_urls == ['https://mirror.example.com/example/repo.git', ORIGINAL_URL]


def test_download_blob_tries_all_mirrors_in_order_then_falls_back(blob_cmd, monkeypatch, tmp_path):
    mirrors = json.dumps(
        {
            'https://github.com/': [
                'https://mirror1.example.com/',
                'https://mirror2.example.com/',
            ]
        }
    )
    cmd = blob_cmd(mirrors)

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
    cmd.download_blob(blob, tmp_path / 'blob.bin')

    assert attempted_urls == [
        'https://mirror1.example.com/example/repo.git',
        'https://mirror2.example.com/example/repo.git',
        ORIGINAL_URL,
    ]
