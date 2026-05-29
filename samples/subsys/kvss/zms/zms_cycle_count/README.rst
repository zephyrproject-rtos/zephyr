.. zephyr:code-sample:: zms-cycle-count
   :name: ZMS Cycle Count Verification
   :relevant-api: zms_high_level_api

   Verify that ZMS sector cycle counters are tracked correctly across garbage
   collection cycles, including past the uint8_t wraparound boundary.

Overview
********

 This sample exercises and verifies the ZMS sector cycle counting APIs:

 * :c:func:`zms_get_num_cycles`
 * :c:func:`zms_get_sector_num_cycles`
 * :c:func:`zms_sector_use_next`

 It checks that the recycle count returned by ZMS matches the expected value
 after many forced sector advances, and that the count keeps growing correctly
 once it exceeds the original ``uint8_t`` cycle counter range (256), which is
 where the legacy field used to wrap around.

 The sample runs in three phases:

 #. **Phase 1** -- Mounts the partition, clears it, and performs 30 sector
    advances. The cycle count is checked against the expected value
    (``base + PHASE1_ITERATIONS / SECTOR_COUNT``).
 #. **Phase 2** -- Performs additional sector advances until the relative
    cycle count exceeds 256, validating that the counter keeps incrementing
    past the historical ``uint8_t`` wraparound point.
 #. **Phase 3** -- Reads the per-sector cycle count via
    :c:func:`zms_get_sector_num_cycles` for every sector, ensures none of them
    are zero, and verifies that an out-of-range sector index returns
    ``-EINVAL``.

 On success the sample prints ``PASS: cycle count verification succeeded``.

Requirements
************

* A board with flash support or the ``native_sim`` target

Building and Running
********************

The sample can be built for several platforms. It has been tested on
``native_sim``, ``qemu_x86`` and ``nrf7002dk/nrf5340/cpuapp``.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/kvss/zms/zms_cycle_count
   :goals: build
   :compact:

Sample Output
=============

The exact ``base`` value depends on the existing state of the storage
partition (it grows by one full cycle each time the sample is re-run against
persistent flash). The example below shows a fresh-flash run where
``base=1``; subsequent runs simply start from a higher base and end at a
higher final value, but the relative growth and the ``PASS`` line are the
same.

.. code-block:: console

   *** Booting Zephyr OS build v4.4.0-rc2 ***
   [00:00:00.000,000] <inf> fs_zms: 3 Sectors of 4096 bytes
   [00:00:00.000,000] <inf> fs_zms: alloc wra: 0, fc0
   [00:00:00.000,000] <inf> fs_zms: data wra: 0, 0
   [00:00:00.000,000] <inf> fs_zms: 3 Sectors of 4096 bytes
   [00:00:00.000,000] <inf> fs_zms: alloc wra: 0, fc0
   [00:00:00.000,000] <inf> fs_zms: data wra: 0, 0
   After mount: num_cycles=1 (base=1)
   --- Phase 1: 30 sector advances ---
   After sector_use_next[1]: num_cycles=1
   After sector_use_next[2]: num_cycles=2
   After sector_use_next[3]: num_cycles=2
   ...
   After sector_use_next[30]: num_cycles=11
   Phase 1 expected=11, got=11
   --- Phase 2: 741 sector advances to exceed 256 cycles above base ---
   Phase 2 progress [3/741]: num_cycles=12
   Phase 2 progress [6/741]: num_cycles=13
   ...
   Phase 2 progress [741/741]: num_cycles=258
   Final expected=258, got=258 (total_advances=771)
   --- Phase 3: per-sector cycle count verification ---
   Sector 0: cycles=258
   Sector 1: cycles=258
   Sector 2: cycles=257
   PASS: cycle count verification succeeded (num_cycles=258 > 255)
