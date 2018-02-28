# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path

class ZephyrMixin(object):
    __slots__ = []

    @staticmethod
    def zephyr_path():
        return Path(__file__).resolve().parents[2]
