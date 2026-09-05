.. zephyr:code-sample:: safety-test-sensor-chain
   :name: Safety test sensor chain
   :relevant-api: safety_test_api sensor_interface

   Run a safety test at every init level, ending at a temperature reading you
   can falsify externally.

Overview
********

Most safety samples show you a test that passes. This one shows you a chain.

It registers one test at each of the five ``safety_test`` init levels. Each
test checks something the next one depends on, so by the time you reach the
temperature reading at the end, everything underneath it has already been
checked:

.. code-block:: none

   EARLY           cpu_core           the CPU adds and shifts correctly
                      |
   PRE_KERNEL_1    flash_integrity    the code in flash is intact
                      |
   PRE_KERNEL_2    ram_march          the RAM it uses is sound
                      |
   POST_KERNEL     clock_xcheck       the bus clock runs at the right speed
                      |
   POST_KERNEL     sensor_comm        the sensor answers on the bus
                      |
   APPLICATION     sensor_plausible   the reading is believable

The last link is the point. Pinch the sensor with your finger, the reading
climbs out of its plausible range, and a real failure travels through the real
failure hook and the real safe-state handler. Nothing is faked.

What you need
*************

An ``frdm_mcxa156`` board and a USB cable. The P3T1755 temperature sensor is
already on the board and talks over I3C, so there is no shield to fit and
nothing to wire up.

Building and running
********************

This sample needs **two build passes**. Build it once the normal way and the
flash test will report an error, on purpose. The next section explains why.

.. code-block:: console

   west build -p -b frdm_mcxa156 samples/subsys/safety_test/sensor_chain -d build

   CRC=$(python3 samples/subsys/safety_test/sensor_chain/scripts/compute_text_crc.py \
           --elf build/zephyr/zephyr.elf)

   west build -p -b frdm_mcxa156 samples/subsys/safety_test/sensor_chain -d build \
           -- -DCONFIG_SAMPLE_IMAGE_CRC=$CRC

   west flash -d build

Then open a serial terminal at 115200 8N1.

For the strict version, which halts on a failure instead of carrying on, add
``-DEXTRA_CONF_FILE=strict.conf`` to both build commands.

How the flash test works, and why it takes two passes
*****************************************************

``flash_integrity`` checks that the code in flash is the code you built. It
does that by computing a CRC-32 over the whole ``.text`` region at boot and
comparing it against the value it expects.

The awkward part is where that expected value comes from. You cannot simply
write it into a source file, because the moment you do, the code changes, and
so does its checksum. Chasing your own tail.

The way out is to keep the expected value **outside the region being checked**:

.. code-block:: none

   flash layout          checked?
   -------------------   --------
   .text                 yes  <- the CRC covers exactly this
   .rodata               no   <- the expected value lives here

Because the value sits in ``.rodata`` and the checksum only covers ``.text``,
writing the value in cannot change any byte the checksum looks at. So:

1. **First pass.** Build with ``CONFIG_SAMPLE_IMAGE_CRC`` left at its default
   of 0. The image links, and the ``.text`` region is now fixed.
2. **Compute.** ``scripts/compute_text_crc.py`` reads the ELF and prints the
   checksum of that region.
3. **Second pass.** Build again, passing that value in. The ``.text`` region
   comes out byte for byte identical, so the value you just computed is still
   correct.

Two Kconfig settings this test depends on
=========================================

Both are already in :file:`prj.conf`, and both are easy to get wrong if you
copy this test into your own application.

``CONFIG_CRC=y``
   Without it the CRC library is not compiled at all and the build fails to
   link.

``CONFIG_CRC_HW_HANDLER=n``
   This one is less obvious. The board's devicetree points ``zephyr,crc`` at
   the CRC peripheral, which turns the hardware CRC handler on by default. That
   handler replaces the software ``crc32_ieee()`` with a version that drives the
   CRC hardware -- and that hardware is a POST_KERNEL device, while this test
   runs at PRE_KERNEL_1, long before it exists. Leave it on and the board hangs
   during boot with nothing on the console.

Adding this test to your own application
========================================

If you want the same check somewhere else:

1. Copy ``scripts/compute_text_crc.py``.
2. Add a ``SAMPLE_IMAGE_CRC``-style hex option to your :file:`Kconfig`, with a
   default of 0.
3. Store the expected value in a ``volatile const uint32_t`` initialised from
   that option. The ``volatile`` matters: without it the compiler folds the
   value straight into the comparison, and the check quietly becomes "is this
   number equal to itself", which passes on a corrupt image.
4. Add ``CONFIG_CRC=y``, and turn off the hardware handler if your board
   enables it.
5. Wrap your build in the two-pass sequence above, or put it in a small script
   so nobody has to remember.

What you should see
*******************

.. code-block:: none

   safety_test sensor chain sample
   boot results:
     clock_xcheck       POST_KERNEL   PASS       10115 us
     cpu_core           EARLY         PASS           0 us
     flash_integrity    PRE_KERNEL_1  PASS           0 us
     ram_march          PRE_KERNEL_2  PASS         132 us
     sensor_comm        POST_KERNEL   PASS          78 us
     sensor_plausible   APPLICATION   PASS          79 us
   boot summary: 6 total, 6 passed, 0 failed, 0 skipped, 0 not run
   boot chain: PASS
   cycle 1: 6 visited, 5 passed, 0 failed, 1 skipped, 0 over budget, temp 28750 mC, history 1

Then a ``cycle`` line every two seconds.

Some of this output looks odd until you know why:

**Why do two tests say 0 us?**
   ``cpu_core`` and ``flash_integrity`` run before the system timer starts, so
   there is no clock to measure them with. The subsystem does not guess. The
   same tests report real numbers on the runtime cycles, where the timer is
   running (about 2 us and 9530 us).

**Why is one test always skipped?**
   ``ram_march`` writes patterns over a block of RAM, which destroys whatever
   was in it. That is fine at boot, when nothing is there yet. At runtime it
   would wipe the sample's own temperature history, so it is marked destructive
   and the runtime sweep refuses to run it. You are watching that flag do its
   job, once every two seconds.

**Why is the temperature an integer?**
   It is in millidegrees. ``28750`` means 28.750 degrees Celsius. The sample
   avoids floating point entirely, so it prints whole numbers.

Making a test fail
******************

This is the part worth doing.

1. Flash the normal build and wait for a few ``cycle`` lines.
2. Pinch the P3T1755 package between finger and thumb and hold it. Warming it
   through the plastic is enough; you do not need to touch the pins.
3. Within a few cycles the reading passes the top of its allowed range and
   ``sensor_plausible`` fails with ``-34`` (``-ERANGE``).

What happens next depends on which build you flashed:

.. list-table::
   :header-rows: 1

   * - Build
     - What the board does
   * - normal
     - Logs the failure, lights the red LED, keeps running. Let go and it
       recovers on the next cycle.
   * - ``strict.conf``
     - Logs the failure, then halts. The cycle lines stop. Press reset.
   * - ``CONFIG_SAMPLE_SAFE_STATE_ACTION_RESET=y``
     - Logs the failure, then cold reboots. See the note below.

That difference is one line of configuration, not one line of code. The
sample's safe-state handler always says "carry on". In the strict build the
subsystem refuses to accept that answer and halts anyway. That is the whole
point of ``CONFIG_SAFETY_TEST_STRICT_CRITICAL``: an application cannot wave a
critical failure through, no matter what its handler returns.

One more thing to try: after a failure, get the board to restart and the next
boot should tell you what killed the previous one, read back from a small
record kept in uninitialised RAM.

Press RESET after a halt, or have the sample reboot itself instead of halting.
For the second, add ``-DCONFIG_SAMPLE_SAFE_STATE_ACTION_RESET=y`` to both build
passes:

.. code-block:: console

   west build -p -b frdm_mcxa156 samples/subsys/safety_test/sensor_chain -d build \
           -- -DCONFIG_SAMPLE_SAFE_STATE_ACTION_RESET=y

   CRC=$(python3 samples/subsys/safety_test/sensor_chain/scripts/compute_text_crc.py \
           --elf build/zephyr/zephyr.elf)

   west build -p -b frdm_mcxa156 samples/subsys/safety_test/sensor_chain -d build \
           -- -DCONFIG_SAMPLE_SAFE_STATE_ACTION_RESET=y -DCONFIG_SAMPLE_IMAGE_CRC=$CRC

Both passes need the same options. If they differ, the ``.text`` region differs
too and the checksum from the first pass will not match the second.

LEDs
****

.. list-table::
   :header-rows: 1

   * - LED
     - Meaning
   * - Red
     - A test has failed at some point. Stays on once lit.
   * - Green
     - Every critical boot test passed.
   * - Blue
     - Blinks once per runtime cycle. If it stops, the board stopped.

Configuration
*************

.. list-table::
   :header-rows: 1

   * - Option
     - Default
     - What it does
   * - ``CONFIG_SAMPLE_TEMP_MIN_MC``
     - 10000
     - Bottom of the plausible range, in millidegrees
   * - ``CONFIG_SAMPLE_TEMP_MAX_MC``
     - 35000
     - Top of the range. Lower it if your finger is not enough
   * - ``CONFIG_SAMPLE_CLOCK_TOLERANCE_PCT``
     - 10
     - How far the reference clock may drift before it fails
   * - ``CONFIG_SAMPLE_CLOCK_WINDOW_MS``
     - 10
     - How long the clock check measures for
   * - ``CONFIG_SAMPLE_RUNTIME_PERIOD_MS``
     - 2000
     - Gap between runtime cycles
   * - ``CONFIG_SAMPLE_IMAGE_CRC``
     - 0
     - Set by the second build pass. Do not edit by hand
   * - ``CONFIG_SAMPLE_SAFE_STATE_ACTION_*``
     - CONTINUE
     - What the safe-state handler returns: CONTINUE, RESET or HALT

What this sample does not do
****************************

These are deliberate. A real product would do each of them differently, and it
is better to say so than to let the sample look more finished than it is.

**The CPU test checks arithmetic, not registers.**
   It runs known numbers through add, subtract, multiply, divide and the bit
   operations, and checks the answers. A certified CPU test checks the register
   file itself, which cannot be done in C. Chip vendors supply those routines in
   assembly.

**The flash test proves the image is unchanged, not that it is the right image.**
   The expected checksum ships inside the very image it checks. If someone
   replaces the whole image, checksum included, the test is happy. Catching
   that needs a signature checked by something the attacker cannot replace,
   which is what a secure bootloader is for.

**The clock tolerance is a guess.**
   Ten percent was chosen because it does not produce false failures, not
   because a datasheet says so. The low speed oscillator it compares against is
   not a precision part. Before you rely on this test, look up your oscillator's
   real accuracy and set the number from that. As written it catches gross
   faults (a wrong divider, a clock that never started) and nothing finer.

One last note. The sensor is in continuous conversion mode, so reading it is
just a bus transfer. If you switch it to one-shot mode to save power, every
read gains a 12 ms wait, and this sample reads it twice per cycle.
