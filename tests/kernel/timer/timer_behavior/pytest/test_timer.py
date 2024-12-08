# Copyright (c) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import logging

from math import ceil

from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)


def do_analysis(test, stats, stats_count, config, sys_clock_hw_cycles_per_sec):
    logger.info('====================================================')
    logger.info(f'periodic timer behaviour using {test} mechanism:')

    test_period = int(config['TIMER_TEST_PERIOD'])
    test_samples = int(config['TIMER_TEST_SAMPLES'])

    seconds = (test_period * test_samples) / 1_000_000

    periods_sec = test_period / 1_000_000
    ticks_per_sec = int(config['SYS_CLOCK_TICKS_PER_SEC'])
    periods_tick = ceil(ticks_per_sec * periods_sec)

    expected_period = test_period * sys_clock_hw_cycles_per_sec / 1_000_000
    cyc_per_tick = sys_clock_hw_cycles_per_sec / ticks_per_sec
    expected_period_drift = ((periods_tick * cyc_per_tick - expected_period) /
                             sys_clock_hw_cycles_per_sec * 1_000_000)
    expected_total_drift = expected_period_drift * test_samples / 1_000_000

    period_max_drift = (int(config['TIMER_EXTERNAL_TEST_PERIOD_MAX_DRIFT_PPM'])
                        / 1_000_000)
    min_bound = (test_period - period_max_drift * test_period +
                 expected_period_drift) / 1_000_000
    max_bound = (test_period + period_max_drift * test_period +
                 expected_period_drift) / 1_000_000

    min_cyc = 1. / sys_clock_hw_cycles_per_sec
    max_stddev = int(config['TIMER_TEST_MAX_STDDEV']) / 1_000_000
    # Max STDDEV cannot be lower than clock single cycle
    max_stddev = max(min_cyc, max_stddev)

    max_drift_ppm = int(config['TIMER_EXTERNAL_TEST_MAX_DRIFT_PPM'])
    time_diff = stats['total_time'] - seconds - expected_total_drift

    logger.info(f'min:            {stats["min"] * 1_000_000:.6f} us')
    logger.info(f'max:            {stats["max"] * 1_000_000:.6f} us')
    logger.info(f'mean:           {stats["mean"] * 1_000_000:.6f} us')
    logger.info(f'variance:       {stats["var"] * 1_000_000:.6f} us')
    logger.info(f'stddev:         {stats["stddev"] * 1_000_000:.6f} us')
    logger.info(f'total time:     {stats["total_time"] * 1_000_000:.6f} us')
    logger.info(f'expected drift: {seconds * max_drift_ppm} us')
    logger.info(f'real drift:     {time_diff * 1_000_000:.6f} us')
    logger.info('====================================================')

    logger.info('RECORD: {'
                f'"testcase":"jitter_drift_timer"'
                f', "mechanism":"{test}_external", "stats_count": {stats_count}, ' +
                ', '.join(['"{}_us":{:.6f}'.format(k, v * 1_000_000) for k,v in stats.items()]) +
                f', "expected_total_time_us":{seconds * 1_000_000:.6f}'
                f', "expected_total_drift_us":{seconds * max_drift_ppm:.6f}'
                f', "total_drift_us": {time_diff * 1_000_000:.6f}'
                f', "min_bound_us":{min_bound * 1_000_000:.6f}'
                f', "max_bound_us":{max_bound * 1_000_000:.6f}'
                f', "expected_period_cycles":{expected_period:.0f}'
                f', "MAX_STD_DEV":{max_stddev * 1_000_000:.0f}'
                f', "sys_clock_hw_cycles_per_sec":{sys_clock_hw_cycles_per_sec}, ' +
                ', '.join(['"CONFIG_{}":{}'.format(k, str(config[k]).rstrip()) for k in [
                                            'SYS_CLOCK_HW_CYCLES_PER_SEC',
                                            'SYS_CLOCK_TICKS_PER_SEC',
                                            'TIMER_TEST_PERIOD',
                                            'TIMER_TEST_SAMPLES',
                                            'TIMER_EXTERNAL_TEST_PERIOD_MAX_DRIFT_PPM',
                                            'TIMER_EXTERNAL_TEST_MAX_DRIFT_PPM'
                                                                                 ]
                          ]) + '}'
              )

    assert stats['stddev'] < max_stddev
    assert stats['min'] >= min_bound
    assert stats['max'] <= max_bound
    assert abs(time_diff) < seconds * max_drift_ppm / 1_000_000


def wait_sync_point(dut: DeviceAdapter, point):
    dut.readlines_until(regex=f"===== {point} =====")


def test_flash(dut: DeviceAdapter, tool, tool_options, config,
               sys_clock_hw_cycles_per_sec):
    tool = __import__(tool)

    test_period = int(config['TIMER_TEST_PERIOD'])
    test_samples = int(config['TIMER_TEST_SAMPLES'])
    seconds = (test_period * test_samples) / 1_000_000

    tests = ["builtin", "startdelay"]
    for test in tests:
        wait_sync_point(dut, test)
        stats, stats_count = tool.run(seconds, tool_options)
        assert stats_count
        do_analysis(test, stats, stats_count, config, sys_clock_hw_cycles_per_sec)

    # Let the running test's image output to be fully captured from device.
    dut.readlines()
