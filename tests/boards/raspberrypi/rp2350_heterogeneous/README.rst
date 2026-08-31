RP2350 heterogeneous hardware test
##################################

This test launches a SRAM-resident Hazard3 Zephyr image on CPU1 from a
Cortex-M33 CPU0 image. It verifies the active architecture, hart ID, repeated
remote timer wakeups, and 128 unique mailbox request/response exchanges. Each
four-response burst is held in the RP2350 SIO FIFO before the CPU0 interrupt is
unmasked, so losing queued words fails deterministically.

The images own separate 256 KiB SRAM regions and separate flash partitions.
SRAM bank 8 contains only the test status structure. The test exclusively owns
the SIO inter-processor FIFO; applications using this AMP arrangement must
provide equivalent ownership rules and must not combine the mailbox driver
with other FIFO users such as an SMP scheduler or Pico SDK multicore lockout.

Run on a USB-connected Pico 2 or Pico 2 W with no debug probe:

.. code-block:: console

   $ ./tests/boards/raspberrypi/rp2350_heterogeneous/scripts/run_hardware_test.py

The helper discovers the board, creates a persistent Twister hardware map,
builds both images, requests BOOTSEL from a previous test image when needed,
flashes the merged UF2, and runs the ztest harness. For lab setups with an SWD
probe, the same test also uses the normal Twister ``--west-flash`` flow and the
board's multi-image OpenOCD configuration.
