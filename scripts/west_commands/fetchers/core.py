# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from abc import ABC, abstractmethod
from pathlib import Path
from typing import List, Type

class ZephyrBlobFetcher(ABC):

    @staticmethod
    def get_fetchers() -> List[Type['ZephyrBlobFetcher']]:
        '''Get a list of all currently defined fetcher classes.'''
        return ZephyrBlobFetcher.__subclasses__()

    @classmethod
    @abstractmethod
    def schemes(cls) -> List[str]:
        '''Return this fetcher's schemes.'''

    @abstractmethod
    def fetch(self, url: str, path: Path):
        ''' Fetch a blob and store it '''
