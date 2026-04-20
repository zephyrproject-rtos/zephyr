# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
import logging

from test_demo import run_ble_connect  # pylint: disable=no-name-in-module
from twister_harness import Shell
from twister_harness_bsim.bsim_helper import BsimHelper

logger = logging.getLogger(__name__)


def test_ble_connect_bsim(bsim: BsimHelper):
    """Test BLE connection between two DUTs in BSIM simulation using the shell interface."""
    bsim.verbosity_level = "0"
    logger.info("Run two DUTs")
    # -d=$dev_id       : Which device number in that simulation is this one
    # -RealEncryption=1: Actually encrypt RADIO traffic
    # -mro=10e3        : Synchronize with the phy every 10ms to be more fluid in interactive use
    bsim.run_dut(dut_index=0, args="-d=1 -RealEncryption=1 -mro=10e3")
    bsim.run_dut(dut_index=1, args="-d=2 -RealEncryption=1 -mro=10e3")

    logger.info("Start device handbrake to force real-time simulation")
    # -r=1       : run at 1x of real time (actual real time)
    # -pp=10e3   : Hold down the simulation every 10ms (so it feels fluid for interactive use)
    bsim.start_device_handbrake(args="-d=0 -r=1 -pp=10e3")

    logger.info("Start BSIM simulation")
    bsim.start_bsim(args="-D=3")

    logger.info("Get ready shells for both DUTs")
    shells: list[Shell] = bsim.get_ready_shells()

    logger.info("Run BLE connection test")
    run_ble_connect(shells)
