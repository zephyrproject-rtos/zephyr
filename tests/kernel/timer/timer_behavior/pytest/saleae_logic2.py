# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

# Sample code showing an external tool Python module helper for Saleae Logic 2
# compatible logic analyzer.
# To use it, the Saleae Logic 2 Automation server must be enabled. For more
# information on it, check
# https://saleae.github.io/logic2-automation/getting_started.html

import numpy as np
import tempfile

from pathlib import Path
from saleae import automation
from saleae.automation import (CaptureConfiguration, LogicDeviceConfiguration,
DigitalTriggerCaptureMode, DigitalTriggerType)

def do_collection(device_id, port, channel, sample_rate, threshold_volts,
                  seconds, output_dir):
    with automation.Manager.connect(port=port) as manager:

        device_configuration = LogicDeviceConfiguration(
            enabled_digital_channels=[channel],
            digital_sample_rate=sample_rate,
            digital_threshold_volts=threshold_volts,
        )

        capture_mode = DigitalTriggerCaptureMode(DigitalTriggerType.RISING,
                                                 channel,
                                                 after_trigger_seconds=seconds)
        capture_configuration = CaptureConfiguration(capture_mode=capture_mode)

        with manager.start_capture(
                device_id=device_id,
                device_configuration=device_configuration,
                capture_configuration=capture_configuration) as capture:

            capture.wait()

            capture.export_raw_data_csv(directory=output_dir,
                                        digital_channels=[channel])

def do_analysis(output_dir):
    file_name = Path(output_dir) / 'digital.csv'
    all_data = np.loadtxt(file_name, delimiter=',', skiprows=1, usecols=0)

    # Pre trigger data is negative on CSV
    non_negative = all_data[all_data >= 0]

    # Last sample is just captured at last moment of capture, not related
    # to gpio toggle. Discard it
    data = non_negative[:-1]

    diff = np.diff(data)

    mean = np.mean(diff)
    std = np.std(diff)
    var = np.var(diff)
    minimum = np.min(diff)
    maximum = np.max(diff)
    total_time = data[-1]

    return {'mean': mean, 'stddev': std, 'var': var, 'min': minimum,
            'max': maximum, 'total_time': total_time}


# options should be a string of the format:
# [device-id=<device_id>,]port=<saleae_logic2_server_port>,
# channel=<channel_number>,sample-rate=<sample-rate>,
# threshold-volts=<threshold-volts>
def run(seconds, options):
    options = [i for p in options.split(',') for i in p.split('=')]
    options = dict(zip(options[::2], options[1::2]))

    device_id = options.get('device-id')
    port = int(options.get('port'))
    channel = int(options.get('channel'))
    sample_rate = int(options.get('sample-rate'))
    threshold_volts = float(options.get('threshold-volts'))

    with tempfile.TemporaryDirectory() as output_dir:
        output_dir = tempfile.mkdtemp()
        # Add one second to ensure all data is captured
        do_collection(device_id, port, channel, sample_rate, threshold_volts,
                      seconds + 1, output_dir)

    return do_analysis(output_dir)
