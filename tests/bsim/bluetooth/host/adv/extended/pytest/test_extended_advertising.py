# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
import logging

from twister_harness_bsim.bsim_helper import BsimHelper

logger = logging.getLogger(__name__)


def test_advertising(bsim: BsimHelper):
    """Extended advertising test

    Broadcasting Only: a Bluetooth LE broadcaster advertises with extended
    advertising, and a scanner scans the extended advertisement packets.
    """
    bsim.run_dut(dut_index=0, args="-d=0 -RealEncryption=0 -testid=ext_adv_advertiser -rs=23")

    bsim.run_dut(dut_index=1, args="-d=1 -RealEncryption=0 -testid=ext_adv_scanner -rs=6")

    bsim.start_bsim(args="-D=2 -sim_length=10e6")
    bsim.wait_for_bsim(timeout=20)

    # Read the output after the simulation has completed to ensure all logs are captured
    bsim.flush_duts_output(print_output=True)


def test_advertising_connectable(bsim: BsimHelper):
    """Extended advertising test

    Connectable: In addition to broadcasting advertisements, it is connectable
    and restarts advertisements once disconnected. The scanner/central scans
    for the packets and establishes the connection, to then disconnect
    shortly-after.
    """
    bsim.run_dut(dut_index=0, args="-d=0 -RealEncryption=0 -testid=ext_adv_conn_advertiser -rs=23")

    bsim.run_dut(dut_index=1, args="-d=1 -RealEncryption=0 -testid=ext_adv_conn_scanner -rs=6")

    bsim.start_bsim(args="-D=2 -sim_length=10e6")
    bsim.wait_for_bsim(timeout=20)

    bsim.flush_duts_output(print_output=True)


def test_advertising_connectable_x5(bsim: BsimHelper):
    """Extended advertising test

    Connectable X5: In addition to broadcasting advertisements, it is connectable
    and restarts advertisements once disconnected. The scanner/central scans
    for the packets and establishes the connection, to then disconnect
    shortly-after. This is repeated over 5 times.
    """
    bsim.run_dut(
        dut_index=0, args="-d=0 -RealEncryption=0 -testid=ext_adv_conn_advertiser_x5 -rs=23"
    )

    bsim.run_dut(dut_index=1, args="-d=1 -RealEncryption=0 -testid=ext_adv_conn_scanner_x5 -rs=6")

    bsim.start_bsim(args="-D=2 -sim_length=10e6")
    bsim.wait_for_bsim(timeout=20)

    bsim.flush_duts_output(print_output=True)
