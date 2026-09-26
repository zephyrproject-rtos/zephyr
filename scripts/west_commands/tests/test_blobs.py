# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

'''Tests for the blobs command's cache and download handling.'''

import hashlib
import sys
from pathlib import Path
from unittest import mock

import pytest

from blobs import Blobs
from fetchers.core import ZephyrBlobException

WANTED = b'the real blob'
WANTED_SHA = hashlib.sha256(WANTED).hexdigest()
# What a server answers with when it wants a login instead of serving a file.
SIGN_IN_PAGE = b'<html>please sign in</html>'


@pytest.fixture
def cmd():
    '''A Blobs command with its logging silenced.'''
    command = Blobs()
    for name in ('dbg', 'wrn', 'inf', 'err'):
        setattr(command, name, mock.Mock())
    return command


def make_blob(tmp_path, urls=('https://example.invalid/blob.bin',)):
    return {
        'path': 'blob.bin',
        'abspath': str(tmp_path / 'out' / 'blob.bin'),
        'sha256': WANTED_SHA,
        'url': list(urls),
        'module': 'demo',
        'type': 'img',
    }


def fetcher_writing(payload, seen=None):
    '''A fetcher class that reports success and writes the given bytes.

    Passing a list as 'seen' records the URL each call was handed, which is
    how a test tells "went on to the next URL" apart from "asked the first
    one twice".
    '''

    class Fetcher:
        def fetch(self, cmd, blob, path):
            if seen is not None:
                seen.append(blob['url'])
            Path(path).write_bytes(payload)

    return Fetcher


def fetcher_failing():
    class Fetcher:
        def fetch(self, cmd, blob, path):
            raise ZephyrBlobException('no answer from the server')

    return Fetcher


def patch_fetchers(*classes):
    '''Hand out the given fetcher classes, one per call, then repeat the last.'''
    remaining = list(classes)

    def get_fetcher_cls(_scheme):
        return remaining.pop(0) if len(remaining) > 1 else remaining[0]

    module = mock.Mock()
    module.get_fetcher_cls = get_fetcher_cls
    return mock.patch.dict(sys.modules, {'fetchers': module})


def test_get_cached_blob_finds_the_suffixed_name(cmd, tmp_path):
    cache = tmp_path / 'cache'
    cache.mkdir()
    cached = cache / f'blob.bin.{WANTED_SHA}'
    cached.write_bytes(WANTED)

    assert cmd.get_cached_blob(make_blob(tmp_path), [cache]) == cached


def test_get_cached_blob_prefers_the_suffixed_name(cmd, tmp_path):
    '''Both names can be cached at once; the suffixed one is the auto-cache's.'''
    cache = tmp_path / 'cache'
    cache.mkdir()
    (cache / 'blob.bin').write_bytes(WANTED)
    suffixed = cache / f'blob.bin.{WANTED_SHA}'
    suffixed.write_bytes(WANTED)

    assert cmd.get_cached_blob(make_blob(tmp_path), [cache]) == suffixed


def test_get_cached_blob_finds_the_plain_name(cmd, tmp_path):
    cache = tmp_path / 'cache'
    cache.mkdir()
    cached = cache / 'blob.bin'
    cached.write_bytes(WANTED)

    assert cmd.get_cached_blob(make_blob(tmp_path), [cache]) == cached


def test_get_cached_blob_ignores_a_file_with_the_wrong_checksum(cmd, tmp_path):
    '''The name says which blob it is; only the checksum says it is that blob.'''
    cache = tmp_path / 'cache'
    cache.mkdir()
    (cache / f'blob.bin.{WANTED_SHA}').write_bytes(SIGN_IN_PAGE)

    assert cmd.get_cached_blob(make_blob(tmp_path), [cache]) is None


def test_get_cached_blob_skips_a_directory_that_is_not_there(cmd, tmp_path):
    present = tmp_path / 'present'
    present.mkdir()
    cached = present / 'blob.bin'
    cached.write_bytes(WANTED)

    dirs = [tmp_path / 'absent', present]
    assert cmd.get_cached_blob(make_blob(tmp_path), dirs) == cached


def test_get_cached_blob_returns_none_when_nothing_is_cached(cmd, tmp_path):
    cache = tmp_path / 'cache'
    cache.mkdir()

    assert cmd.get_cached_blob(make_blob(tmp_path), [cache]) is None


def test_download_blob_moves_on_to_the_next_url(cmd, tmp_path):
    blob = make_blob(tmp_path, urls=('https://a.invalid/b', 'https://b.invalid/b'))
    dest = tmp_path / 'out' / 'blob.bin'

    with patch_fetchers(fetcher_failing(), fetcher_writing(WANTED)):
        cmd.download_blob(blob, dest)

    assert dest.read_bytes() == WANTED


def test_download_blob_moves_on_when_the_body_is_wrong(cmd, tmp_path):
    '''A 200 with the wrong body is a reason to try the next URL too.

    Nothing raises here: the first URL answers. What sends download_blob()
    on is the checksum of what it was handed, which is the half of the
    docstring's promise the failing fetcher above cannot reach.
    '''
    seen = []
    blob = make_blob(tmp_path, urls=('https://a.invalid/b', 'https://b.invalid/b'))
    dest = tmp_path / 'out' / 'blob.bin'

    with patch_fetchers(fetcher_writing(SIGN_IN_PAGE, seen), fetcher_writing(WANTED, seen)):
        cmd.download_blob(blob, dest)

    assert dest.read_bytes() == WANTED
    assert seen == ['https://a.invalid/b', 'https://b.invalid/b']


def test_download_blob_uses_the_fetcher_the_blob_names(cmd, tmp_path):
    '''A blob may name its fetcher rather than leave it to the URL scheme.'''
    seen = []

    def get_fetcher_cls(scheme):
        seen.append(scheme)
        return fetcher_writing(WANTED)

    module = mock.Mock()
    module.get_fetcher_cls = get_fetcher_cls
    blob = make_blob(tmp_path) | {'fetcher': 'git-lfs'}

    with mock.patch.dict(sys.modules, {'fetchers': module}):
        cmd.download_blob(blob, tmp_path / 'out' / 'blob.bin')

    assert seen == ['git-lfs']


def test_download_blob_raises_when_no_url_answers(cmd, tmp_path):
    blob = make_blob(tmp_path, urls=('https://a.invalid/b', 'https://b.invalid/b'))

    with patch_fetchers(fetcher_failing()), pytest.raises(ZephyrBlobException):
        cmd.download_blob(blob, tmp_path / 'out' / 'blob.bin')


def test_download_blob_leaves_a_mismatched_download_in_place(cmd, tmp_path):
    '''A 200 with the wrong body is a download, not a failure to download.

    download_blob() deliberately does not raise here: verify_blob() reports
    the mismatch later, with the advice it has for it.
    '''
    blob = make_blob(tmp_path)
    dest = tmp_path / 'out' / 'blob.bin'

    with patch_fetchers(fetcher_writing(SIGN_IN_PAGE)):
        cmd.download_blob(blob, dest)

    assert dest.read_bytes() == SIGN_IN_PAGE


def test_handle_auto_cache_uses_what_is_already_cached(cmd, tmp_path):
    cache = tmp_path / 'cache'
    cache.mkdir()
    cached = cache / f'blob.bin.{WANTED_SHA}'
    cached.write_bytes(WANTED)

    with mock.patch.object(cmd, 'download_blob') as download:
        assert cmd.handle_auto_cache(make_blob(tmp_path), cache) == cached
    download.assert_not_called()


def test_handle_auto_cache_downloads_what_is_missing(cmd, tmp_path):
    cache = tmp_path / 'cache'

    with patch_fetchers(fetcher_writing(WANTED)):
        got = cmd.handle_auto_cache(make_blob(tmp_path), cache)

    assert got == cache / f'blob.bin.{WANTED_SHA}'
    assert got.read_bytes() == WANTED


def test_handle_auto_cache_hands_back_a_mismatched_download(cmd, tmp_path):
    '''Having an auto-cache must not change how a bad download is reported.

    get_cached_blob() checks the checksum, so it finds nothing after a
    download that did not match, and the path has to come from somewhere:
    fetch_blob() copies it to the blob's place and verify_blob() reports the
    mismatch, exactly as when no cache is configured.
    '''
    cache = tmp_path / 'cache'

    with patch_fetchers(fetcher_writing(SIGN_IN_PAGE)):
        got = cmd.handle_auto_cache(make_blob(tmp_path), cache)

    assert got == cache / f'blob.bin.{WANTED_SHA}'
    assert got.read_bytes() == SIGN_IN_PAGE
