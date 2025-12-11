# Copyright (c) 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
import logging

import pytest
import yaml
from camera_shield.main import Application
from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)


@pytest.fixture
def test_config(request):
    return request.config.getoption("--config")


def get_prompt(config):
    '''
    get_prompt: get prompt string
    '''
    with open(config) as yaml_file:
        data = yaml.safe_load(yaml_file)

    return data.get('test', {})


def test_display_harness(dut: DeviceAdapter, test_config):
    if not test_config:
        pytest.skip('test_config not provided')

    testcase_config = get_prompt(test_config)
    assert testcase_config != {}, "test config not exist"

    dut.readlines_until(
        regex=testcase_config.get("prompt", 'uart:~$'),
        timeout=testcase_config.get("timeout", 30),
        print_output=True,
    )
    app = Application(test_config)
    result = app.run()
    logging.info(result)
    assert sorted(testcase_config.get("expect", ['PASS'])) == sorted(result)
