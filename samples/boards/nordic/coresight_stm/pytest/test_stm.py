#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

import logging
import re
import subprocess
from pathlib import Path
from time import sleep

import psutil
from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)

SB_CONFIG_APP_CPUPPR_RUN = None
SB_CONFIG_APP_CPUFLPR_RUN = None

# https://github.com/zephyrproject-rtos/zephyr/blob/main/drivers/misc/coresight/nrf_etr.c#L102
STM_M_ID = {
    "sec": 33,
    "app": 34,
    "rad": 35,
    "mod": 36,
    "sys": 44,
    "flpr": 45,
    "ppr": 46,
    "hw": 128,
}


def _analyse_autoconf(filepath: str) -> None:
    global SB_CONFIG_APP_CPUPPR_RUN
    global SB_CONFIG_APP_CPUFLPR_RUN

    SB_CONFIG_APP_CPUPPR_RUN = False
    SB_CONFIG_APP_CPUFLPR_RUN = False

    # Parse contents of {BUILD_DIR}/_sysbuild/autoconf.h
    with open(f"{filepath}", errors="ignore") as autoconf:
        for line in autoconf:
            if "SB_CONFIG_APP_CPUPPR_RUN 1" in line:
                SB_CONFIG_APP_CPUPPR_RUN = True
                continue
            if "SB_CONFIG_APP_CPUFLPR_RUN 1" in line:
                SB_CONFIG_APP_CPUFLPR_RUN = True
    logger.debug(f"{SB_CONFIG_APP_CPUPPR_RUN=}")
    logger.debug(f"{SB_CONFIG_APP_CPUFLPR_RUN=}")


def _check_benchmark_results(output: str, core: str) -> None:
    """
    Use regular expressions to parse 'output' string.
    Search for benchmark results related to 'core' coprocessor.
    """

    latency_msg_0_str = re.search(
        rf"{core}: Timing for log message with 0 arguments: (.+)us", output
    ).group(1)
    assert latency_msg_0_str is not None, "Timing for log message with 0 arguments NOT found"

    latency_msg_1_str = re.search(
        rf"{core}: Timing for log message with 1 argument: (.+)us", output
    ).group(1)
    assert latency_msg_1_str is not None, "Timing for log message with 1 argument NOT found"

    latency_msg_2_str = re.search(
        rf"{core}: Timing for log message with 2 arguments: (.+)us", output
    ).group(1)
    assert latency_msg_2_str is not None, "Timing for log message with 2 arguments NOT found"

    latency_msg_3_str = re.search(
        rf"{core}: Timing for log message with 3 arguments: (.+)us", output
    ).group(1)
    assert latency_msg_3_str is not None, "Timing for log message with 3 arguments NOT found"

    latency_msg_string_str = re.search(
        rf"{core}: Timing for log_message with string: (.+)us", output
    ).group(1)
    assert latency_msg_string_str is not None, "Timing for log_message with string NOT found"

    latency_tracepoint_str = re.search(
        rf"{core}: Timing for tracepoint: (.+)us", output
    ).group(1)
    assert latency_tracepoint_str is not None, "Timing for tracepoint NOT found"

    latency_tracepoint_d32_str = re.search(
        rf"{core}: Timing for tracepoint_d32: (.+)us", output
    ).group(1)
    assert latency_tracepoint_d32_str is not None, "Timing for tracepoint_d32 NOT found"


# nrfutil starts children processes
# when subprocess.terminate(nrfutil_process) is executed, only the parent terminates
# this blocks serial port for other uses
def _kill(proc):
    try:
        for child in psutil.Process(proc.pid).children(recursive=True):
            child.kill()
        proc.kill()
    except Exception as e:
        logger.exception(f'Could not kill nrfutil - {e}')


def _nrfutil_dictionary_from_serial(
    dut: DeviceAdapter,
    decoded_file_name: str = "output.txt",
    collect_time: float = 60.0,
) -> None:
    UART_PATH = dut.device_config.serial
    UART_BAUDRATE = dut.device_config.baud
    dut.close()

    logger.debug(f"Using serial: {UART_PATH}")

    if Path(f"{decoded_file_name}").exists():
        logger.warning("Output file already exists!")

    # prepare database config string
    BUILD_DIR = str(dut.device_config.build_dir)
    logger.debug(f"{BUILD_DIR=}")
    config_str = f"{STM_M_ID['app']}:{BUILD_DIR}/coresight_stm/zephyr/log_dictionary.json"
    config_str = config_str + f",{STM_M_ID['rad']}:{BUILD_DIR}/remote_rad/zephyr/log_dictionary.json"
    if SB_CONFIG_APP_CPUPPR_RUN:
        config_str = config_str + f",{STM_M_ID['ppr']}:{BUILD_DIR}/remote_ppr/zephyr/log_dictionary.json"
    if SB_CONFIG_APP_CPUFLPR_RUN:
        config_str = config_str + f",{STM_M_ID['flpr']}:{BUILD_DIR}/remote_flpr/zephyr/log_dictionary.json"
    logger.debug(f"{config_str=}")

    cmd = (
        "nrfutil trace stm --database-config "
        f"{config_str} "
        f"--input-serialport {UART_PATH} --baudrate {UART_BAUDRATE} "
        f"--output-ascii {decoded_file_name}"
    )
    try:
        # run nrfutil trace in background non-blocking
        logger.info(f"Executing:\n{cmd}")
        proc = subprocess.Popen(cmd.split(), stdout=subprocess.DEVNULL)
    except OSError as exc:
        logger.error(f"Unable to start nrfutil trace:\n{cmd}\n{exc}")
    try:
        proc.wait(collect_time)
    except subprocess.TimeoutExpired:
        pass
    finally:
        _kill(proc)


def test_STM_decoded(dut: DeviceAdapter):
    """
    Run sample.boards.nrf.coresight_stm from samples/boards/nrf/coresight_stm sample.
    Both Application and Radio cores use STM for logging.
    STM proxy (Application core) decodes logs from all domains.
    After reset, coprocessors execute code in expected way and Application core
    outputs STM traces on UART port.
    """
    BUILD_DIR = str(dut.device_config.build_dir)
    autoconf_file = f"{BUILD_DIR}/_sysbuild/autoconf.h"

    # nrf54h20 prints immediately after it is flashed.
    # Wait a bit to skipp logs from previous test.
    sleep(4)

    # Get output from serial port
    output = "\n".join(dut.readlines())

    # set SB_CONFIG_APP_CPUPPR_RUN, SB_CONFIG_APP_CPUFLPR_RUN
    _analyse_autoconf(autoconf_file)

    # check that LOGs from Application core are present
    _check_benchmark_results(
        output=output,
        core='app',
    )

    # check that LOGs from Radio core are present
    _check_benchmark_results(
        output=output,
        core='rad',
    )

    if SB_CONFIG_APP_CPUPPR_RUN:
        # check that LOGs from PPR core are present
        _check_benchmark_results(
            output=output,
            core='ppr',
        )

    if SB_CONFIG_APP_CPUFLPR_RUN:
        # check that LOGs from FLPR core are present
        _check_benchmark_results(
            output=output,
            core='flpr',
        )


def test_STM_dictionary_mode(dut: DeviceAdapter):
    """
    Run sample.boards.nrf.coresight_stm.dict from samples/boards/nrf/coresight_stm sample.
    Both Application and Radio cores use STM for logging.
    STM proxy (Application core) prints on serial port raw logs from all domains.
    Nrfutil trace is used to decode STM logs.
    After reset, coprocessors execute code in expected way and Application core
    outputs STM traces on UART port.
    """
    BUILD_DIR = str(dut.device_config.build_dir)
    test_filename = f"{BUILD_DIR}/coresight_stm_dictionary.txt"
    autoconf_file = f"{BUILD_DIR}/_sysbuild/autoconf.h"
    COLLECT_TIMEOUT = 10.0

    # set SB_CONFIG_APP_CPUPPR_RUN, SB_CONFIG_APP_CPUFLPR_RUN
    # this information is needed to build nrfutil database-config
    _analyse_autoconf(autoconf_file)

    # use nrfutil trace to decode logs
    _nrfutil_dictionary_from_serial(
        dut=dut,
        decoded_file_name=f"{test_filename}",
        collect_time=COLLECT_TIMEOUT,
    )

    # read decoded logs
    with open(f"{test_filename}", errors="ignore") as decoded_file:
        decoded_file_content = decoded_file.read()

    # if nothing in decoded_file, stop test
    assert(
        len(decoded_file_content) > 0
    ), f"File {test_filename} is empty"

    # check that LOGs from Application core are present
    _check_benchmark_results(
        output=decoded_file_content,
        core='app',
    )

    # check that LOGs from Radio core are present
    _check_benchmark_results(
        output=decoded_file_content,
        core='rad',
    )

    if SB_CONFIG_APP_CPUPPR_RUN:
        # check that LOGs from PPR core are present
        _check_benchmark_results(
            output=decoded_file_content,
            core='ppr',
        )

    if SB_CONFIG_APP_CPUFLPR_RUN:
        # check that LOGs from FLPR core are present
        _check_benchmark_results(
            output=decoded_file_content,
            core='flpr',
        )
