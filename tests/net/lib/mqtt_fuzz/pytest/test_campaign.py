# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: Apache-2.0
#
# Bounded libFuzzer campaign wrapper for `net.mqtt.fuzz.campaign`.
#
# Uses unlaunched_dut, not dut: NativeSimulatorAdapter.generate_command()
# hardcodes argv to the bare executable path with no arguments, so the dut
# fixture would launch the binary with no corpus/flags and it would fuzz
# forever with no bound -- exactly what this scenario must avoid.

import glob
import logging
import os
import re
import subprocess
from pathlib import Path

from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)

SOURCE_DIR = Path(__file__).resolve().parent.parent

EXECUTED_UNITS_FLOOR = 10_000


def test_mqtt_fuzz_campaign(unlaunched_dut: DeviceAdapter, fuzz_max_total_time, fuzz_max_len):
    build_dir = Path(unlaunched_dut.device_config.app_build_dir)
    exe = build_dir / "zephyr" / "zephyr.exe"
    assert exe.exists(), f"{exe} not found -- build failed?"

    work_corpus = build_dir / "fuzz_corpus"
    work_corpus.mkdir(exist_ok=True)
    artifacts = build_dir / "fuzz_artifacts"
    artifacts.mkdir(exist_ok=True)

    # Writable work_corpus FIRST: libFuzzer only writes new coverage-adding
    # inputs to the first corpus directory on the command line.  Any shared
    # $MQTT_FUZZ_CORPUS is listed second, used as read-only seed material, so
    # a CI run never mutates persistent storage.
    corpus_args = [str(work_corpus)]
    if "MQTT_FUZZ_CORPUS" in os.environ:
        corpus_args.append(os.environ["MQTT_FUZZ_CORPUS"])

    cmd = [
        str(exe),
        *corpus_args,
        f"-dict={SOURCE_DIR / 'mqtt5.dict'}",
        f"-max_total_time={fuzz_max_total_time}",
        f"-max_len={fuzz_max_len}",
        f"-artifact_prefix={artifacts}/",
        "-fork=1",
        "-print_final_stats=1",
    ]
    logger.info("running: %s", " ".join(cmd))

    result = subprocess.run(
        cmd,
        capture_output=True,
        text=True,
        timeout=fuzz_max_total_time + 300,
    )
    logger.info("campaign stdout:\n%s", result.stdout)

    def artifact_glob(pattern):
        return glob.glob(str(artifacts / pattern))

    findings = (
        artifact_glob("crash-*")
        + artifact_glob("leak-*")
        + artifact_glob("timeout-*")
        + artifact_glob("oom-*")
    )
    if findings:
        artifact = findings[0]
        pytest.fail(
            f"campaign produced {len(findings)} artifact(s); first: {artifact}\n"
            f"reproduce with: {exe} {artifact}\n"
            f"minimize with:  {exe} -minimize_crash=1 {artifact}\n"
            f"stdout:\n{result.stdout}"
        )

    match = re.search(r"stat::number_of_executed_units:\s*(\d+)", result.stdout)
    assert match, f"no stat::number_of_executed_units in stdout:\n{result.stdout}"
    executed_units = int(match.group(1))
    assert executed_units > EXECUTED_UNITS_FLOOR, (
        f"only {executed_units} executed units (floor {EXECUTED_UNITS_FLOOR}) -- "
        f"the target booted but did not actually fuzz\nstdout:\n{result.stdout}"
    )

    assert result.returncode == 0, (
        f"campaign exited {result.returncode} (expected clean -max_total_time "
        f"expiry)\nstdout:\n{result.stdout}"
    )
