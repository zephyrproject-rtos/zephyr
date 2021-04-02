#!/usr/bin/env python3
#
# Copyright (c) 2021 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""
Abstract Class for Dictionary-based Logging Parsers
"""

import abc


class LogParser(abc.ABC):
    """Abstract class of log parser"""
    def __init__(self, database):
        self.database = database

    @abc.abstractmethod
    def parse_log_data(self, logdata, debug=False):
        """Parse log data"""
        return None
