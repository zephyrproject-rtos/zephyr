# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import logging
import pytest
import re
from twister_harness import Shell

logger = logging.getLogger(__name__)

TESTDATA = [
    ('mpu mtest 1', 'The value is: 0x.*'),
    ('mpu mtest 2', 'The value is: 0x.*'),
]
@pytest.mark.parametrize('command,expected', TESTDATA)
def test_shell_harness(shell: Shell, command, expected):
    logger.info('send "help" command')
    lines = shell.exec_command(command)
    match = False
    for line in lines:
        if re.match(expected, line):
            match = True
            break

    assert match, 'expected response not found'
    logger.info('response is valid')
