#
# Copyright (c) 2024 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
#

'''
Pytest harness to test the output of the dictionary logging.
'''

import logging
import os
import re
import shlex
import subprocess

from twister_harness import DeviceAdapter

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")

logger = logging.getLogger(__name__)

def process_logs(dut: DeviceAdapter, build_dir):
    '''
    This grabs the encoded log from console and parse the log
    through the dictionary logging parser.

    Returns the decoded log lines.
    '''
    # Make sure the log parser script is there...
    parser_script = os.path.join(ZEPHYR_BASE, "scripts", "logging", "dictionary", "log_parser.py")
    assert os.path.isfile(parser_script)
    logger.info(f'Log parser script: {parser_script}')

    # And also the dictionary JSON file is there...
    dictionary_json = os.path.join(build_dir, "zephyr", "log_dictionary.json")
    assert os.path.isfile(dictionary_json)
    logger.info(f'Dictionary JSON: {dictionary_json}')

    # Read the encoded logs and save them to a file
    # as the log parser requires file as input.
    # Timeout is intentionally long. Twister will
    # timeout earlier with per-test timeout.
    handler_output = dut.readlines_until(regex = '.*##ZLOGV1##[0-9]+', timeout = 600.0)

    # Join all the output lines together
    handler_output = ''.join(ho.strip() for ho in handler_output)

    # Find the last dictionary logging block and extract it
    ridx = handler_output.rfind("##ZLOGV1##")
    encoded_logs = handler_output[ridx:]

    encoded_log_file = os.path.join(build_dir, "encoded.log")
    with open(encoded_log_file, 'w', encoding='utf-8') as fp:
        fp.write(encoded_logs)

    # Run the log parser
    cmd = [parser_script, '--hex', dictionary_json, encoded_log_file]
    logger.info(f'Running parser script: {shlex.join(cmd)}')
    result = subprocess.run(cmd, capture_output=True, text=True, check=True)
    assert result.returncode == 0

    # Grab the decoded log lines from stdout, print a copy and return it
    decoded_logs = result.stdout
    logger.info(f'Decoded logs: {decoded_logs}')

    return decoded_logs


def expected_regex_common():
    '''
    Return an array of compiled regular expression for matching
    the decoded log lines.
    '''
    return [
    # *** Booting Zephyr OS build <version> ***
    re.compile(r'.*[*][*][*] Booting Zephyr OS build [0-9a-z.-]+'),
    # Hello World! <board name>
    re.compile(r'[\s]+Hello World! [\w-]+'),
    # [        10] <err> hello_world: error string
    re.compile(r'[\s]+[\[][0-9,:\. ]+[\]] <err> hello_world: error string'),
    # [        10] <dbg> hello_world: main: debug string
    re.compile(r'[\s]+[\[][0-9,:\. ]+[\]] <dbg> hello_world: main: debug string'),
    # [        10] <inf> hello_world: info string
    re.compile(r'[\s]+[\[][0-9,:\. ]+[\]] <inf> hello_world: info string'),
    # [        10] <dbg> hello_world: main: int8_t 1, uint8_t 2
    re.compile(r'[\s]+[\[][0-9,:\. ]+[\]] <dbg> hello_world: main: int8_t 1, uint8_t 2'),
    # [        10] <dbg> hello_world: main: int16_t 16, uint16_t 17
    re.compile(r'[\s]+[\[][0-9,:\. ]+[\]] <dbg> hello_world: main: int16_t 16, uint16_t 17'),
    # [        10] <dbg> hello_world: main: int32_t 32, uint32_t 33
    re.compile(r'[\s]+[\[][0-9,:\. ]+[\]] <dbg> hello_world: main: int32_t 32, uint32_t 33'),
    # [        10] <dbg> hello_world: main: int64_t 64, uint64_t 65
    re.compile(r'[\s]+[\[][0-9,:\. ]+[\]] <dbg> hello_world: main: int64_t 64, uint64_t 65'),
    # [        10] <dbg> hello_world: main: char !
    re.compile(r'[\s]+[\[][0-9,:\. ]+[\]] <dbg> hello_world: main: char !'),
    # [        10] <dbg> hello_world: main: s str static str c str
    re.compile(r'[\s]+[\[][0-9,:\. ]+[\]] <dbg> hello_world: main: s str static str c str'),
    # [        10] <dbg> hello_world: main: d str dynamic str
    re.compile(r'[\s]+[\[][0-9,:\. ]+[\]] <dbg> hello_world: main: d str dynamic str'),
    # [        10] <dbg> hello_world: main: mixed str dynamic str --- dynamic str \
    # --- another dynamic str --- another dynamic str
    re.compile(r'[\s]+[\[][0-9,:\. ]+[\]] <dbg> hello_world: main: mixed str dynamic str '
                '--- dynamic str --- another dynamic str --- another dynamic str'),
    # [        10] <dbg> hello_world: main: mixed c/s ! static str dynamic str static str !
    re.compile(r'[\s]+[\[][0-9,:\. ]+[\]] <dbg> hello_world: main: mixed c/s ! static str '
                'dynamic str static str !'),
    # [        10] <dbg> hello_world: main: pointer 0x1085f9
    re.compile(r'[\s]+[\[][0-9,:\. ]+[\]] <dbg> hello_world: main: pointer 0x[0-9a-f]+'),
    # [        10] <dbg> hello_world: main: For HeXdUmP!
    re.compile(r'[\s]+[\[][0-9,:\. ]+[\]] <dbg> hello_world: main: For HeXdUmP!'),
    # 48 45 58 44 55 4d 50 21  20 48 45 58 44 55 4d 50 |HEXDUMP!  HEXDUMP
    re.compile(r'[\s]+[ ]+[0-9a-f ]{48,52}[|]HEXDUMP!  HEXDUMP'),
    # 40 20 48 45 58 44 55 4d  50 23                   |@ HEXDUM P#
    re.compile(r'[\s]+[ ]+[0-9a-f ]{48,52}[|]@ HEXDUM P#'),
    ]


def expected_regex_fpu():
    '''
    Return an array of additional compiled regular expression for matching
    the decoded log lines for FPU builds.
    '''
    return [
    # [        10] <dbg> hello_world: main: float 66.669998, double 68.690000
    re.compile(r'[\s]+[\[][0-9,:\. ]+[\]] <dbg> hello_world: main: '
               r'float 66[\.][0-9-\.]+, double 68[\.][0-9-\.]+'),
    ]


def regex_matching(decoded_logs, expected_regex):
    '''
    Given the decoded log lines and an array of compiled regular expression,
    match all of them and display whether a line is found or not.

    Return True if all regular expressions have corresponding matches,
    False otherwise.
    '''
    regex_results = [ex_re.search(decoded_logs) for ex_re in expected_regex]

    # Using 1:1 mapping between regex_results and expected_regex, so
    # cannot use enumeration.
    #
    # pylint: disable=consider-using-enumerate
    for idx in range(len(regex_results)):
        if regex_results[idx]:
            logger.info(f'Found: {regex_results[idx].group(0).strip()}')
        else:
            logger.info(f'NOT FOUND: {expected_regex[idx]}')

    return all(regex_results)


def test_logging_dictionary(dut: DeviceAdapter, is_fpu_build):
    '''
    Main entrance to setup test result validation.
    '''
    build_dir = dut.device_config.app_build_dir

    logger.info(f'FPU build? {is_fpu_build}')

    decoded_logs = process_logs(dut, build_dir)

    assert regex_matching(decoded_logs, expected_regex_common())

    if is_fpu_build:
        assert regex_matching(decoded_logs, expected_regex_fpu())
