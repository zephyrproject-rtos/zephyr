# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import asyncio
import logging
import re

logger = logging.getLogger(__name__)


async def device_power_on(device) -> None:
    while True:
        try:
            await device.power_on()
            break
        except Exception:
            continue


def _search_messages(result, messages, read_lines):
    for message in messages:
        for line in read_lines:
            if re.search(message, line):
                result[message] = True
                if False not in result.values():
                    return True
                break

    return False


async def wait_for_shell_response(dut, regex: list[str] | str, max_wait_sec=20):
    found = False
    lines = []

    logger.debug('wait_for_shell_response')

    messages = [regex] if isinstance(regex, str) else regex
    result = dict.fromkeys(messages, False)

    try:
        for _ in range(0, max_wait_sec):
            read_lines = dut.readlines(print_output=True)
            lines += read_lines
            for line in read_lines:
                logger.debug(f'DUT response: {str(line)}')

            found = _search_messages(result, messages, read_lines)
            if found is True:
                break
            await asyncio.sleep(1)
    except Exception as e:
        logger.error(f'{e}!', exc_info=True)
        raise e

    for key in result:
        logger.debug(f'Expected DUT response: "{key}", Matched: {result[key]}')

    return found, lines


async def send_cmd_to_iut(
    shell, dut, cmd, response=None, expect_to_find_resp=True, max_wait_sec=20, shell_ret=False
):
    found = False
    lines = ''
    shell_cmd_ret = shell.exec_command(cmd)
    if response is not None:
        shell_cmd_ret_str = str(shell_cmd_ret)
        if response in shell_cmd_ret_str:
            found = True
            lines = shell_cmd_ret_str
        else:
            found, lines = await wait_for_shell_response(dut, response, max_wait_sec)
    else:
        found = True
    assert found is expect_to_find_resp
    return lines


def check_shell_response(lines: list[str], regex: str) -> bool:
    found = False
    try:
        for line in lines:
            if re.search(regex, line):
                found = True
                break
    except Exception as e:
        logger.error(f'{e}!', exc_info=True)
        raise e

    return found
