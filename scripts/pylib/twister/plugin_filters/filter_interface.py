#!/usr/bin/env python3
#
# Copyright (c) 2025 Baumer Group. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

class FilterInterface:
    def setup(self, *args, **kwargs):
        """
        Initialises the Filter object. The arguments and keyword arguments given in the command
        will be passed to the Filters by this method.
        """

    def exclude(self, suite) -> bool:
        """
        Examines a given Test Suite and decides if it must be excluded from the test plan.

        Returns:
            bool: True, if the suite should be skipped
        """

    def include(self, suite) -> bool:
        """
        Examines a given Test Suite and decides if it must be included in the test plan.

        Returns:
            bool: True, if the suite should not be skipped
        """

    def teardown(self):
        """
        Simple hook to execute some code after the filter has been used
        """
