# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0

"""
Check that the dmic_i2s pipeline actually processes audio on native_sim.

The native_sim DMIC driver reads its input from a file and the native_sim I2S
driver writes its output to one, so the whole pipeline can be run headless and
the result compared against the expected samples. The sample applies a gain of
CONFIG_SAMPLE_AUDIO_GAIN_PERCENT, so the output must equal the input scaled by
that gain and saturated to the sample range.
"""

import logging
import struct
import time
from pathlib import Path

import pytest
from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)

# The sample negotiates 16-bit stereo on native_sim.
SAMPLE_MAX = 2**15 - 1
SAMPLE_MIN = -(2**15)
RUN_SECONDS = 15


def _build_input(path: Path, frames: int = 8000) -> bytes:
    """
    Write a stereo 16-bit ramp that sweeps the full sample range.

    Covering the extremes matters: a gain above unity must saturate at full
    scale, and an intermediate that is too narrow wraps around instead.
    """
    step = (SAMPLE_MAX - SAMPLE_MIN) // (frames - 1)
    samples = []
    for i in range(frames):
        value = SAMPLE_MIN + i * step
        samples.append(value)  # left
        samples.append(-value if value != SAMPLE_MIN else SAMPLE_MAX)  # right
    data = struct.pack(f'<{len(samples)}h', *samples)
    path.write_bytes(data)
    return data


def _apply_gain(data: bytes, gain_percent: int) -> bytes:
    gain_fixed = (gain_percent * 65536) // 100
    samples = struct.unpack(f'<{len(data) // 2}h', data)
    scaled = []
    for sample in samples:
        value = (sample * gain_fixed) >> 16
        scaled.append(max(SAMPLE_MIN, min(SAMPLE_MAX, value)))
    return struct.pack(f'<{len(scaled)}h', *scaled)


def test_pipeline_applies_gain(unlaunched_dut: DeviceAdapter, tmp_path: Path):
    """Run the pipeline and compare its output against the expected samples."""
    gain_percent = int(
        unlaunched_dut.device_config.build_dir.joinpath('zephyr', '.config')
        .read_text()
        .split('CONFIG_SAMPLE_AUDIO_GAIN_PERCENT=')[1]
        .split('\n')[0]
    )
    logger.info('pipeline gain is %d%%', gain_percent)

    input_file = tmp_path / 'input.pcm'
    output_file = tmp_path / 'output.pcm'
    source = _build_input(input_file)

    # launch() only generates the command when it is still empty, so build it
    # first and then append the native_sim arguments.
    unlaunched_dut.generate_command()
    unlaunched_dut.command += [
        f'--dmic0_file={input_file}',
        f'--i2s_tx_tx={output_file}',
        f'-stop_at={RUN_SECONDS}',
    ]

    unlaunched_dut.launch()
    try:
        unlaunched_dut.readlines_until(regex='Capture started', timeout=30.0)
        # Capture starts long before enough audio has been played out, so wait
        # for the sink to have written at least one full pass of the input.
        deadline = time.time() + RUN_SECONDS * 4
        while time.time() < deadline:
            if output_file.exists() and output_file.stat().st_size >= len(source):
                break
            time.sleep(0.5)
    finally:
        unlaunched_dut.close()

    assert output_file.exists(), 'pipeline produced no I2S output file'
    produced = output_file.read_bytes()
    assert len(produced) >= len(source), (
        f'pipeline produced {len(produced)} bytes, expected at least {len(source)}'
    )

    # The DMIC driver loops its input file, so compare the first pass only.
    expected = _apply_gain(source, gain_percent)
    produced = produced[: len(expected)]

    if produced != expected:
        mismatches = sum(1 for a, b in zip(produced, expected, strict=True) if a != b)
        pytest.fail(
            f'{mismatches} of {len(expected)} output bytes differ from '
            f'input scaled by {gain_percent}%'
        )
