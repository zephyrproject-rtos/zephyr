# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import importlib
import logging
import os
from pathlib import Path

from fetchers.core import ZephyrBlobFetcher

_logger = logging.getLogger('fetchers')

def _import_fetcher_module(fetcher_name):
    try:
        importlib.import_module(f'fetchers.{fetcher_name}')
    except ImportError as ie:
        # Fetchers are supposed to gracefully handle failures when they
        # import anything outside of stdlib, but they sometimes do
        # not. Catch ImportError to handle this.
        _logger.warning(f'The module for fetcher "{fetcher_name}" '
                        f'could not be imported ({ie}). This most likely '
                        'means it is not handling its dependencies properly. '
                        'Please report this to the zephyr developers.')

# We import these here to ensure the BlobFetcher subclasses are
# defined; otherwise, BlobFetcher.get_fetchers() won't work.

# Those do not contain subclasses of ZephyrBlobFetcher
name_blocklist = ['__init__', 'core']

fetchers_dir = Path(__file__).parent.resolve()
for f in [f for f in os.listdir(fetchers_dir)]:
    file = fetchers_dir / Path(f)
    if file.suffix == '.py' and file.stem not in name_blocklist:
        _import_fetcher_module(file.stem)

def get_fetcher_cls(scheme):
    '''Get a fetcher's class object, given a scheme.'''
    for cls in ZephyrBlobFetcher.get_fetchers():
        if scheme in cls.schemes():
            return cls
    raise ValueError('unknown fetcher for scheme "{}"'.format(scheme))

__all__ = ['ZephyrBlobFetcher', 'get_fetcher_cls']
