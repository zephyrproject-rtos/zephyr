# Copyright (c) 2025 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
import logging
import re

import pytest
import yaml
from twister_harness import Shell

logger = logging.getLogger(__name__)


@pytest.fixture
def testdata_path(request):
    return request.config.getoption("--testdata")


def get_next_commands(testdata_path):
    with open(testdata_path) as yaml_file:
        data = yaml.safe_load(yaml_file)
    for entry in data:
        yield entry['command'], entry.get('expected', None)


def test_shell_harness(shell: Shell, testdata_path):
    if not testdata_path:
        pytest.skip('testdata not provided')
    for command, expected in get_next_commands(testdata_path):
        logger.info('send command: %s', command)
        lines = shell.exec_command(command)
        if not expected:
            logger.debug('no expected response')
            continue
        match = False
        for line in lines:
            if re.match(expected, line):
                match = True
                break
        assert match, 'expected response not found'
        logger.info('response is valid')
