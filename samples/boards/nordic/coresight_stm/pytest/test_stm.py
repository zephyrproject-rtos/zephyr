#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

import logging
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path
from time import sleep

import psutil
from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)

SB_CONFIG_APP_CPUPPR_RUN = None
SB_CONFIG_APP_CPUFLPR_RUN = None

# See definition of stm_m_id[] and stm_m_name[] in
# https://github.com/zephyrproject-rtos/zephyr/blob/main/drivers/misc/coresight/nrf_etr.c
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


@dataclass
class STMLimits:
    log_0_arg: float | None
    log_1_arg: float | None
    log_2_arg: float | None
    log_3_arg: float | None
    log_str: float | None
    tracepoint: float | None
    tracepoint_d32: float | None
    tolerance: float | None


def analyse_autoconf(filepath: str) -> None:
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


def check_console_output(output: str, core: str) -> None:
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


def check_benchmark_results(output: str, core: str, constraints: STMLimits) -> None:
    """
    Use regular expressions to parse 'output' string.
    Search for benchmark results related to 'core' coprocessor.
    Check that benchamrk results are lower than limits provided in 'constraints'.
    """

    cfg = {
        "log message with 0 arguments": {
            "regex": rf"{core}: Timing for log message with 0 arguments: (.+)us",
            "expected": constraints.log_0_arg,
        },
        "log message with 1 argument": {
            "regex": rf"{core}: Timing for log message with 1 argument: (.+)us",
            "expected": constraints.log_1_arg,
        },
        "log message with 2 arguments": {
            "regex": rf"{core}: Timing for log message with 2 arguments: (.+)us",
            "expected": constraints.log_2_arg,
        },
        "log message with 3 arguments": {
            "regex": rf"{core}: Timing for log message with 3 arguments: (.+)us",
            "expected": constraints.log_3_arg,
        },
        "log message with string": {
            "regex": rf"{core}: Timing for log_message with string: (.+)us",
            "expected": constraints.log_str,
        },
        "tracepoint": {
            "regex": rf"{core}: Timing for tracepoint: (.+)us",
            "expected": constraints.tracepoint,
        },
        "tracepoint_d32": {
            "regex": rf"{core}: Timing for tracepoint_d32: (.+)us",
            "expected": constraints.tracepoint_d32,
        },
    }

    for check in cfg:
        observed_str = re.search(cfg[check]["regex"], output).group(1)
        assert observed_str is not None, f"Timing for {check} NOT found"
        # check value
        observed = float(observed_str)
        threshold = cfg[check]["expected"] * (1 + constraints.tolerance)
        assert (
            observed < threshold
        ), f"{core}: Timing for {check} - {observed} us exceeds {threshold} us"


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


def nrfutil_dictionary_from_serial(
    dut: DeviceAdapter,
    decoded_file_name: str = "output.txt",
    collect_time: float = 60.0,
) -> None:
    UART_PATH = dut.device_config.serial
    dut.close()

    logger.debug(f"Using serial: {UART_PATH}")

    try:
        Path(f"{decoded_file_name}").unlink()
        logger.info("Old output file was deleted")
    except FileNotFoundError:
        pass

    # prepare database config string
    BUILD_DIR = str(dut.device_config.build_dir)
    logger.debug(f"{BUILD_DIR=}")
    config_str = f"{STM_M_ID['app']}:{BUILD_DIR}/coresight_stm/zephyr/log_dictionary.json"
    config_str += f",{STM_M_ID['rad']}:{BUILD_DIR}/remote_rad/zephyr/log_dictionary.json"
    if SB_CONFIG_APP_CPUPPR_RUN:
        config_str += f",{STM_M_ID['ppr']}:{BUILD_DIR}/remote_ppr/zephyr/log_dictionary.json"
    if SB_CONFIG_APP_CPUFLPR_RUN:
        config_str += f",{STM_M_ID['flpr']}:{BUILD_DIR}/remote_flpr/zephyr/log_dictionary.json"
    logger.debug(f"{config_str=}")

    cmd = (
        "nrfutil trace stm --database-config "
        f"{config_str} "
        f"--input-serialport {UART_PATH} "
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


def test_sample_STM_decoded(dut: DeviceAdapter):
    """
    Run sample.boards.nrf.coresight_stm from samples/boards/nordic/coresight_stm sample.
    Application, Radio, PPR and FLPR cores use STM for logging.
    STM proxy (Application core) decodes logs from all cores.
    After reset, coprocessors execute code in expected way and Application core
    outputs STM traces on UART port.
    """
    BUILD_DIR = str(dut.device_config.build_dir)
    autoconf_file = f"{BUILD_DIR}/_sysbuild/autoconf.h"

    # nrf54h20 prints immediately after it is flashed.
    # Wait a bit to skipp logs from previous test.
    sleep(5)

    # Get output from serial port
    output = "\n".join(dut.readlines())

    # set SB_CONFIG_APP_CPUPPR_RUN, SB_CONFIG_APP_CPUFLPR_RUN
    analyse_autoconf(autoconf_file)

    # check that LOGs from Application core are present
    check_console_output(
        output=output,
        core='app',
    )

    # check that LOGs from Radio core are present
    check_console_output(
        output=output,
        core='rad',
    )

    if SB_CONFIG_APP_CPUPPR_RUN:
        # check that LOGs from PPR core are present
        check_console_output(
            output=output,
            core='ppr',
        )

    if SB_CONFIG_APP_CPUFLPR_RUN:
        # check that LOGs from FLPR core are present
        check_console_output(
            output=output,
            core='flpr',
        )


def test_STM_decoded(dut: DeviceAdapter):
    """
    Run boards.nrf.coresight_stm from tests/boards/nrf/coresight_stm test suite.
    Application, Radio, PPR and FLPR cores use STM for logging.
    STM proxy (Application core) decodes logs from all cores.
    After reset, coprocessors execute code in expected way and Application core
    outputs STM traces on UART port.
    """
    BUILD_DIR = str(dut.device_config.build_dir)
    autoconf_file = f"{BUILD_DIR}/_sysbuild/autoconf.h"

    app_constraints = STMLimits(
        # all values in us
        log_0_arg=1.8,
        log_1_arg=2.1,
        log_2_arg=2.0,
        log_3_arg=2.1,
        log_str=4.5,
        tracepoint=0.5,
        tracepoint_d32=0.5,
        tolerance=0.5,  # 50 %
    )
    rad_constraints = STMLimits(
        # all values in us
        log_0_arg=4.6,
        log_1_arg=5.0,
        log_2_arg=5.2,
        log_3_arg=5.6,
        log_str=6.3,
        tracepoint=0.5,
        tracepoint_d32=0.5,
        tolerance=0.5,
    )
    ppr_constraints = STMLimits(
        # all values in us
        log_0_arg=25.7,
        log_1_arg=27.1,
        log_2_arg=27.3,
        log_3_arg=30.4,
        log_str=65.7,
        tracepoint=0.55,
        tracepoint_d32=0.25,
        tolerance=0.5,
    )
    flpr_constraints = STMLimits(
        # all values in us
        log_0_arg=1.3,
        log_1_arg=1.6,
        log_2_arg=1.6,
        log_3_arg=1.7,
        log_str=3.0,
        tracepoint=0.5,
        tracepoint_d32=0.5,
        tolerance=0.5,
    )

    # nrf54h20 prints immediately after it is flashed.
    # Wait a bit to skipp logs from previous test.
    sleep(5)

    # Get output from serial port
    output = "\n".join(dut.readlines())

    # set SB_CONFIG_APP_CPUPPR_RUN, SB_CONFIG_APP_CPUFLPR_RUN
    analyse_autoconf(autoconf_file)

    # check results on Application core
    check_benchmark_results(output=output, core='app', constraints=app_constraints)

    # check results on Radio core
    check_benchmark_results(output=output, core='rad', constraints=rad_constraints)

    if SB_CONFIG_APP_CPUPPR_RUN:
        # check results on PPR core
        check_benchmark_results(output=output, core='ppr', constraints=ppr_constraints)

    if SB_CONFIG_APP_CPUFLPR_RUN:
        # check results on FLPR core
        check_benchmark_results(output=output, core='flpr', constraints=flpr_constraints)


def test_sample_STM_dictionary_mode(dut: DeviceAdapter):
    """
    Run sample.boards.nrf.coresight_stm.dict from samples/boards/nordic/coresight_stm sample.
    Application, Radio, PPR and FLPR cores use STM for logging.
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
    analyse_autoconf(autoconf_file)

    # use nrfutil trace to decode logs
    nrfutil_dictionary_from_serial(
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
    check_console_output(
        output=decoded_file_content,
        core='app',
    )

    # check that LOGs from Radio core are present
    check_console_output(
        output=decoded_file_content,
        core='rad',
    )

    if SB_CONFIG_APP_CPUPPR_RUN:
        # check that LOGs from PPR core are present
        check_console_output(
            output=decoded_file_content,
            core='ppr',
        )

    if SB_CONFIG_APP_CPUFLPR_RUN:
        # check that LOGs from FLPR core are present
        check_console_output(
            output=decoded_file_content,
            core='flpr',
        )


def test_STM_dictionary_mode(dut: DeviceAdapter):
    """
    Run boards.nrf.coresight_stm.dict from tests/boards/nrf/coresight_stm test suite.
    Application, Radio, PPR and FLPR cores use STM for logging.
    STM proxy (Application core) prints on serial port raw logs from all domains.
    Nrfutil trace is used to decode STM logs.
    After reset, coprocessors execute code in expected way and Application core
    outputs STM traces on UART port.
    """
    BUILD_DIR = str(dut.device_config.build_dir)
    test_filename = f"{BUILD_DIR}/coresight_stm_dictionary.txt"
    autoconf_file = f"{BUILD_DIR}/_sysbuild/autoconf.h"
    COLLECT_TIMEOUT = 10.0

    app_constraints = STMLimits(
        # all values in us
        log_0_arg=0.7,
        log_1_arg=0.8,
        log_2_arg=0.8,
        log_3_arg=1.3,
        log_str=3.2,
        tracepoint=0.5,
        tracepoint_d32=0.5,
        tolerance=0.5,  # 50 %
    )
    rad_constraints = STMLimits(
        # all values in us
        log_0_arg=0.8,
        log_1_arg=0.9,
        log_2_arg=1.0,
        log_3_arg=1.5,
        log_str=3.6,
        tracepoint=0.5,
        tracepoint_d32=0.5,
        tolerance=0.5,
    )
    ppr_constraints = STMLimits(
        # all values in us
        log_0_arg=7.5,
        log_1_arg=8.5,
        log_2_arg=8.6,
        log_3_arg=17.4,
        log_str=45.2,
        tracepoint=0.5,
        tracepoint_d32=0.5,
        tolerance=0.5,
    )
    flpr_constraints = STMLimits(
        # all values in us
        log_0_arg=0.3,
        log_1_arg=0.4,
        log_2_arg=0.5,
        log_3_arg=0.8,
        log_str=2.6,
        tracepoint=0.5,
        tracepoint_d32=0.5,
        tolerance=0.5,  # 50 %
    )

    # set SB_CONFIG_APP_CPUPPR_RUN, SB_CONFIG_APP_CPUFLPR_RUN
    # this information is needed to build nrfutil database-config
    analyse_autoconf(autoconf_file)

    # use nrfutil trace to decode logs
    nrfutil_dictionary_from_serial(
        dut=dut,
        decoded_file_name=f"{test_filename}",
        collect_time=COLLECT_TIMEOUT,
    )

    # read decoded logs
    with open(f"{test_filename}", errors="ignore") as decoded_file:
        decoded_file_content = decoded_file.read()

    # if nothing in decoded_file, stop test
    assert len(decoded_file_content) > 0, f"File {test_filename} is empty"

    # check results on Application core
    check_benchmark_results(output=decoded_file_content, core='app', constraints=app_constraints)

    # check results on Radio core
    check_benchmark_results(output=decoded_file_content, core='rad', constraints=rad_constraints)

    if SB_CONFIG_APP_CPUPPR_RUN:
        # check results on PPR core
        check_benchmark_results(
            output=decoded_file_content, core='ppr', constraints=ppr_constraints
        )

    if SB_CONFIG_APP_CPUFLPR_RUN:
        # check results on FLPR core
        check_benchmark_results(
            output=decoded_file_content, core='flpr', constraints=flpr_constraints
        )
