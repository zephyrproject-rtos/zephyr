#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

import re

from twister_harness import DeviceAdapter


def test_rramc_wakeup(dut: DeviceAdapter):
    """
    PyTest code for samples/boards/nordic/nrf_sys_event, sample configurations:
    - sample.boards.nordic.nrf_sys_event.rramc_wakeup,
    - sample.boards.nordic.nrf_sys_event.rramc_wakeup.ppi.
    Parse logs from serial port. If the Register Event API was used correctly,
    code execution shall be faster by ~14 us when event was registered.
    If the API works correctly, RRAMC is woken up just before event occures.
    Thus, there is no delay resulting from RRAMC getting ready.
    """

    TIMEOUT = 5
    MIN_DIFF = 10  # Minimal difference to pass the check

    # Get output from serial port
    output = "\n".join(
        dut.readlines_until(
            regex="All done",
            print_output=True,
            timeout=TIMEOUT,
        )
    )

    # Get execution times
    t_not_registered_str = re.search(
        r"Alarm set for 1000 us, execution took:(.+) \(no event registered\)", output
    ).group(1)
    assert t_not_registered_str is not None, "Timing for no event registered was NOT found"
    t_not_registered = int(t_not_registered_str)

    t_registered_str = re.search(
        r"Alarm set for 1000 us, execution took:(.+) \(event registered\)", output
    ).group(1)
    assert t_registered_str is not None, "Timing for event registered was NOT found"
    t_registered = int(t_registered_str)

    # Check if registering event results in faster code execution
    assert t_not_registered > t_registered + MIN_DIFF, (
        f"{t_not_registered} is NOT larger than {t_registered} + {MIN_DIFF}"
    )
