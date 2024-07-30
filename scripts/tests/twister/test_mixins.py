#!/usr/bin/env python3
# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Tests for the mixins class
"""

import os
import pytest


def test_disable_pytest_test_collection(test_data):
    test_path = os.path.join(test_data, 'mixins')

    return_code = pytest.main([test_path])

    assert return_code == 5
