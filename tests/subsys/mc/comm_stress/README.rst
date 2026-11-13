.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

MC communication stress tests
=============================

These tests check long messages, lost or damaged frames, and timeouts. The
simulator and QEMU tests use an in-memory link. No external ``micro_core``
library is needed.

Native simulator on the host
----------------------------

Run from this Zephyr directory in a west workspace. Set ``ZEPHYR_BASE``
explicitly when an environment such as nRF Connect SDK selects another
Zephyr checkout::

   unset PYTHONHOME PYTHONPATH
   export ZEPHYR_BASE="$PWD"
   export ZEPHYR_TOOLCHAIN_VARIANT=host
   west twister -p native_sim/native/64 -s mc.comm.stress.native -i \
     -O ../build/mc-stress-native

Run directly to see the ztest cases in the terminal::

   west build -b native_sim/native/64 tests/subsys/mc/comm_stress \
     -d ../build/mc-stress-native-direct --pristine
   west build -d ../build/mc-stress-native-direct -t run

QEMU x86 and x86-64
-------------------

Set ``ZEPHYR_SDK_INSTALL_DIR`` to a Zephyr SDK with the x86 GNU target and
QEMU host tools. The selected SDK must contain ``x86_64-zephyr-elf-gcc``.
If it does not, install a separate Zephyr SDK bundle as described in
``doc/develop/toolchains/zephyr_sdk.rst`` and run
``./setup.sh -t x86_64-zephyr-elf -h`` inside the extracted bundle. The nRF
Connect SDK toolchain may contain only selected targets. From this Zephyr
directory, run each QEMU platform separately::

   unset PYTHONHOME PYTHONPATH
   export ZEPHYR_BASE="$PWD"
   export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
   export ZEPHYR_SDK_INSTALL_DIR=/path/to/zephyr-sdk
   west twister -p qemu_x86/atom -s mc.comm.stress.qemu_x86 -i \
     -O ../build/mc-stress-qemu-x86
   west twister -p qemu_x86_64/atom -s mc.comm.stress.qemu_x86_64 -i \
     -O ../build/mc-stress-qemu-x86_64

The ``Zephyr version`` line printed by Twister must identify this checkout.
Each command runs eight tests.
An nRF Connect terminal can set ``PYTHONHOME`` or ``PYTHONPATH`` to its Python
3.12 libraries while ``west`` uses a different Python version. Clearing those
variables prevents ``AssertionError: SRE module mismatch`` before Twister starts.

To see every ztest case directly in the terminal, build and run the x86 image
without Twister::

   west build -b qemu_x86/atom tests/subsys/mc/comm_stress \
     -d ../build/mc-stress-x86-direct --pristine
   west build -d ../build/mc-stress-x86-direct -t run

For x86-64, use a separate build directory::

   west build -b qemu_x86_64/atom tests/subsys/mc/comm_stress \
     -d ../build/mc-stress-x86-64-direct --pristine
   west build -d ../build/mc-stress-x86-64-direct -t run

The ``run`` target prints each ``START`` and ``PASS`` line, followed by the
suite summary. After ``PROJECT EXECUTION SUCCESSFUL``, press ``Ctrl+A`` and
then ``X`` to exit QEMU.

For either QEMU board, a successful direct run shows these case names and
summary lines (durations and separators omitted)::

   Running TESTSUITE mc_comm_stress
   START - test_01_invalid_creation
    PASS - test_01_invalid_creation
   START - test_02_invalid_argument
    PASS - test_02_invalid_argument
   START - test_03_valid_creation
    PASS - test_03_valid_creation
   START - test_04_singly_direction
    PASS - test_04_singly_direction
   START - test_05_singly_repetitive
    PASS - test_05_singly_repetitive
   START - test_06_singly_low_lossy
    PASS - test_06_singly_low_lossy
   START - test_07_singly_high_lossy
    PASS - test_07_singly_high_lossy
   START - test_08_singly_timed_out
    PASS - test_08_singly_timed_out
   TESTSUITE mc_comm_stress succeeded
   SUITE PASS - 100.00% [mc_comm_stress]: pass = 8, fail = 0, skip = 0, total = 8
   PROJECT EXECUTION SUCCESSFUL

Zephyr prints separators, per-case durations, and a summary around these
lines. The x86 build can also print devicetree and test random generator
warnings before QEMU starts; they do not change the ztest result.

Each QEMU run starts one emulated board. The sender and receiver are two
``mc_comm`` instances inside that board, connected by the in-memory test
transport. The serial console appears in the terminal running QEMU; there is
no separate graphical window or receiver console. The tests print ztest case
results, not individual sender and receiver payloads.

The x86 board is selected by Twister or ``west build -b``. The default
``prj.conf`` leaves ``CONFIG_TEST_MC_STRESS_UART`` disabled, so the test
``CMakeLists.txt`` builds ``transport_native.c``, the in-memory link used by
both QEMU boards. No x86-specific CMake option is needed.

Twister prints a summary in the terminal. The ``-i`` option prints detailed
logs only when a test fails. After a successful run, the ``handler.log`` file
shows ``START`` and ``PASS`` for each case, including single-direction,
repetitive, low-loss, high-loss, and timeout transfers. For example::

   find ../build/mc-stress-native -name handler.log -exec cat {} +
   find ../build/mc-stress-qemu-x86 -name handler.log -exec cat {} +
   find ../build/mc-stress-qemu-x86_64 -name handler.log -exec cat {} +

A build error occurs before the cases run and produces no ``handler.log``.
The combined command for the communication, message, and stress suites is in
``doc/services/micro_core.rst``.

nRF54L15 DK UART test
---------------------

Build one image for each board from the Zephyr directory::

   west build -b nrf54l15dk/nrf54l15/cpuapp \
     tests/subsys/mc/comm_stress -d build/mc_receiver --pristine \
     -- -DCONF_FILE=prj_receiver.conf
   west build -b nrf54l15dk/nrf54l15/cpuapp \
     tests/subsys/mc/comm_stress -d build/mc_sender --pristine \
     -- -DCONF_FILE=prj_sender.conf

Connect P1.04 (TX) on each board to P1.05 (RX) on the other board. Connect
their grounds. UART20 uses 1 Mbaud. Disable VCOM1 on both boards because it
shares the UART20 pins. Keep VCOM0 for both consoles.

Flash the receiver first, then the sender. Replace the device IDs with your
board IDs::

   west flash -d build/mc_receiver --dev-id <receiver-id>
   west flash -d build/mc_sender --dev-id <sender-id>

The receiver waits up to five minutes at the first transfer for the sender
to start. Open both VCOM0 consoles at 115200 baud. To rerun, open both
monitors and reset the receiver, then the sender promptly. Both logs should print
``PROJECT EXECUTION SUCCESSFUL``. The test with high frame loss may take
several minutes.

Use only one monitor per console. Two readers on the same serial port split
its bytes, leaving missing characters in both logs. On Linux,
``fuser -v /dev/ttyACM*`` shows processes using serial ports and
``ls -l /dev/serial/by-id/`` identifies each board's VCOM0 port. UART20's
1 Mbaud setting applies only to the wire between the boards.
