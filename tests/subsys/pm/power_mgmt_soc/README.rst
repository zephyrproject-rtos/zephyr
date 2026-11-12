.. SPDX-FileCopyrightText: Copyright 2026 NXP
.. SPDX-License-Identifier: Apache-2.0

SoC power management tests
##########################

The default ``pm.soc`` scenario exercises the board's configured power states.

Suspend-to-RAM
**************

The opt-in ``pm.soc.s2ram`` and ``pm.soc.s2ram.no_fpu`` scenarios exercise
retained execution on ``frdm_mcxw71/mcxw716c`` and
``frdm_mcxw72/mcxw727c``. They select the S2RAM multithread test instead of the
ordinary tests; default board configuration and ordinary test discovery are
unchanged.

For example, build the FP-enabled scenario with:

.. code-block:: console

   west build -b frdm_mcxw71/mcxw716c tests/subsys/pm/power_mgmt_soc \
     -d build/s2ram-w71 -- -DTEST_S2RAM=ON -DEXTRA_CONF_FILE=s2ram.conf \
     -DDTC_OVERLAY_FILE=mcxw7x_s2ram.overlay \
     -DCONFIG_FPU=y -DCONFIG_FPU_SHARING=y

Use the W72 board target for the other board. Set both ``CONFIG_FPU`` and
``CONFIG_FPU_SHARING`` to ``n`` for the no-FP scenario.

The overlay places kernel/application RAM in 32 KiB at ``0x30010000``, outside
the conservative low-64-KiB S2RAM exclusion. Use standalone secure application
core0, internal-flash XIP, no NBU firmware and software CRC dispatch. Factory
ROM must permit fast wake without NPX/FLW reconstruction.

The ``powerdown`` DT node maps hardware Power Down to Zephyr Suspend-to-RAM.
The ``s2ram_reserved`` node describes both excluded address aliases; the SoC
uses it for SRAM retention and checks every allocated final-ELF section against
those ranges. This exclusion is enforced only for S2RAM builds.

The shared exit-latency budget is 3 ms, including SoC/PM restoration. The
`MCXW71 data sheet <https://www.nxp.com/docs/en/data-sheet/MCXW71.pdf>`_
(Rev. 3, Table 19) specifies a 234.59 us maximum hardware Power Down-to-Active
delay; the `MCXW72 data sheet <https://www.nxp.com/docs/en/data-sheet/MCXW72.pdf>`_
(Rev. 5, Table 19) gives 158.20 us typical without a maximum. These hardware
figures do not cover the complete Zephyr restore path.

Separate characterization at 96 MHz with the 32768 Hz LPTMR observed upper
exit estimates of 550 us on W71 and 1069 us on W72 over ten genuine recoveries
per FP/no-FP profile. Each estimate includes one counter quantum. The 3 ms
budget rounds twice the larger observation up to a whole millisecond; it is
not an all-voltage/temperature worst-case guarantee. Applications must increase
it for slower device-resume paths. The one-second minimum residency remains
a conservative admission choice, not a measured energy break-even point.
The forced sleeps below do not test default-policy admission.

Ten forced 500 ms sleeps must each invoke the real architecture suspend once
and return zero. The test checks retained data, BSS, heap and main/worker stack
patterns, completed sleep time and worker progress after wake. Each successful
iteration prints ``PM S2RAM cycle N PASS`` before the final ztest result.
Skipped or aborted entries fail rather than counting as successful PM events.

A custom policy stays active except for the forced transition. MCXW board
support drains the default UART1 console before entry and restores its pinctrl
and cached UART configuration from the test thread after recovery. No extra
UART connection or button is required; LPTMR0 supplies timed wake.

This is a retained-execution regression, not full SoC acceptance. It does not
inject entry aborts, verify every FP/core register, probe every memory bank or
qualify actual power-on-reset/ECC behavior. FP/no-FP builds exercise their
respective production paths, not an exact FP-register preservation assertion.
Other MAIN-domain peripherals require their own context restoration.
