# Copyright (c) 2026
#
# SPDX-License-Identifier: Apache-2.0

import os
import subprocess

import pytest
import zephyr_module

import fetchers
from blobs import Blobs
from fetchers.core import ZephyrBlobException

ORIGINAL_URL = 'https://github.com/example/repo.git'


@pytest.fixture(autouse=True)
def isolated_git_env(monkeypatch, tmp_path):
    '''Run the real `git` binary against an isolated, empty config instead of
    this machine's ~/.gitconfig or the repo's own local config, so tests
    exercise the actual `get_rewritten_url` subprocess call deterministically.
    '''
    monkeypatch.chdir(tmp_path)
    monkeypatch.setenv('GIT_CONFIG_NOSYSTEM', 'true')
    monkeypatch.setenv('GIT_CONFIG_GLOBAL', os.devnull)
    monkeypatch.setenv('GIT_CONFIG_COUNT', '0')
    monkeypatch.delenv('GIT_CONFIG_KEY_0', raising=False)
    monkeypatch.delenv('GIT_CONFIG_VALUE_0', raising=False)


@pytest.fixture(autouse=True)
def _print_git_config(isolated_git_env):
    # Visible with `pytest -s`; shows the actual insteadOf config each test runs against.
    yield
    print('\n--- git config --list (insteadOf entries) ---')
    result = subprocess.run(
        ['git', 'config', '--list'], capture_output=True, text=True, check=False
    )
    lines = [line for line in result.stdout.splitlines() if 'insteadof' in line.lower()]
    print('\n'.join(lines) if lines else '(none)')
    print('--- end git config --list ---\n')


def _set_insteadof(monkeypatch, prefix, replacement):
    monkeypatch.setenv('GIT_CONFIG_COUNT', '1')
    monkeypatch.setenv('GIT_CONFIG_KEY_0', f'url.{replacement}.insteadof')
    monkeypatch.setenv('GIT_CONFIG_VALUE_0', prefix)


def test_get_rewritten_url_when_match(monkeypatch):
    _set_insteadof(monkeypatch, 'https://github.com/', 'https://mirror.example.com/')
    blob_cmd = Blobs()

    assert blob_cmd.get_rewritten_url(ORIGINAL_URL) == 'https://mirror.example.com/example/repo.git'


def test_get_rewritten_url_when_no_match(monkeypatch):
    # An insteadOf rule exists, but its prefix doesn't match this url.
    _set_insteadof(monkeypatch, 'https://gitlab.com/', 'https://mirror.example.com/')
    blob_cmd = Blobs()

    assert blob_cmd.get_rewritten_url(ORIGINAL_URL) is None


def test_get_rewritten_url_when_nothing_defined():
    # No url.*.insteadOf configuration exists at all (isolated_git_env leaves it empty).
    blob_cmd = Blobs()

    assert blob_cmd.get_rewritten_url(ORIGINAL_URL) is None


def test_get_rewritten_url_when_it_fails(monkeypatch, tmp_path):
    # A malformed config file makes every git invocation fail.
    bad_config = tmp_path / 'bad-gitconfig'
    bad_config.write_text('this is [ not valid config')
    monkeypatch.setenv('GIT_CONFIG_GLOBAL', str(bad_config))
    blob_cmd = Blobs()

    assert blob_cmd.get_rewritten_url(ORIGINAL_URL) is None


def test_get_rewritten_url_ignores_ssh_rewrite(monkeypatch):
    # SSH rewrite rules are ignored since blob downloads require HTTP/HTTPS.
    _set_insteadof(monkeypatch, 'https://github.com/', 'git@github.com:')
    blob_cmd = Blobs()

    assert blob_cmd.get_rewritten_url(ORIGINAL_URL) is None


def test_download_blob_uses_mirror_first_then_falls_back(monkeypatch, tmp_path):
    _set_insteadof(monkeypatch, 'https://github.com/', 'https://mirror.example.com/')
    blob_cmd = Blobs()

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


def test_download_blob_uses_original_url_when_rewrite_fails(monkeypatch, tmp_path):
    bad_config = tmp_path / 'bad-gitconfig'
    bad_config.write_text('this is [ not valid config')
    monkeypatch.setenv('GIT_CONFIG_GLOBAL', str(bad_config))
    blob_cmd = Blobs()

    attempted_urls = []

    class FakeFetcher:
        def fetch(self, _cmd, blob, path):
            attempted_urls.append(blob['url'])
            path.write_bytes(b'ok')

    monkeypatch.setattr(fetchers, 'get_fetcher_cls', lambda _scheme: FakeFetcher)
    monkeypatch.setattr(
        zephyr_module, 'get_blob_status', lambda _path, _sha: zephyr_module.BLOB_PRESENT
    )

    blob = {'url': ORIGINAL_URL, 'sha256': 'dummy', 'path': 'blobs/blob.bin'}
    blob_cmd.download_blob(blob, tmp_path / 'blob.bin')

    assert attempted_urls == [ORIGINAL_URL]


def test_download_blob_does_not_duplicate_url_when_no_match(monkeypatch, tmp_path):
    # No insteadOf rule matches; the url should only be attempted once.
    blob_cmd = Blobs()

    attempted_urls = []

    class FakeFetcher:
        def fetch(self, _cmd, blob, path):
            attempted_urls.append(blob['url'])
            path.write_bytes(b'ok')

    monkeypatch.setattr(fetchers, 'get_fetcher_cls', lambda _scheme: FakeFetcher)
    monkeypatch.setattr(
        zephyr_module, 'get_blob_status', lambda _path, _sha: zephyr_module.BLOB_PRESENT
    )

    blob = {'url': ORIGINAL_URL, 'sha256': 'dummy', 'path': 'blobs/blob.bin'}
    blob_cmd.download_blob(blob, tmp_path / 'blob.bin')

    assert attempted_urls == [ORIGINAL_URL]
