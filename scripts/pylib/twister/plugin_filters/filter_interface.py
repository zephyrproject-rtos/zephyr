#!/usr/bin/env python3
#
# Copyright (c) 2025 Baumer Group. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0


class FilterInterface:
    """
    Interface for Plugin Filters.
    Helps users to greatly reduce the filtering scope with self-written code.
    """

    exclude_reason: str
    include_reason: str

    def setup(self, *_args, **kwargs):
        """
        Initialises the Filter object. The arguments and keyword arguments given in the command
        will be passed to the Filters by this method.
        """
        self.exclude_reason = kwargs.get('exclude_reason', '')
        self.include_reason = kwargs.get('include_reason', '')

    def exclude(self, _suite) -> bool:
        """
        Examines a given Test Suite and decides if it must be excluded from the test plan.

        Returns:
            bool: True, if the suite should be skipped
        """
        return False

    def include(self, _suite) -> bool:
        """
        Examines a given Test Suite and decides if it must be included in the test plan.

        Returns:
            bool: True, if the suite should not be skipped
        """
        return False

    def teardown(self):
        """
        Simple hook to execute some code after the filter has been used
        """
