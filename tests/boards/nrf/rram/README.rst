.. _rram_test:

nRF RRAM driver tests
#####################

Integration tests for the nRF RRAM driver (``[zephyr/drivers/flash/soc_flash_nrf_rram.c]``)
on physical nrf54l15dk hardware.

Coverage matrix
***************

============================  ==============================================
Test                          What it validates
============================  ==============================================
test_flash_throttling         ``CONFIG_SOC_FLASH_NRF_THROTTLING`` produces
                              the expected delay (vs the analytical model
                              built from ``CONFIG_NRF_RRAM_THROTTLING_DELAY``
                              and ``CONFIG_NRF_RRAM_THROTTLING_DATA_BLOCK``).
                              Skipped when throttling is disabled.

test_flatten_throughput       ``flash_flatten()`` dispatches to the
                              driver's ``api->fill`` callback (one bulk
                              memset + one buffer commit) instead of
                              falling back to ``flash_fill()``'s chunked
                              write loop. Catches a regression to the
                              generic emulation that was historically
                              about 5x slower.

test_flatten_subpage          Sub-page flatten of a write-line aligned
                              range works because ``api->fill`` is
                              constrained by ``write_block_size`` (16 B),
                              not by the page-aligned constraint that
                              still applies to ``api->erase``. Unaligned
                              ranges are still rejected with ``-EINVAL``.

test_skip_if_filled           With ``CONFIG_NRF_RRAM_FILL_SKIP_IF_FILLED``
                              a flatten of an already-erased region must
                              be at least 4x faster (microsecond
                              resolution) than a flatten of a dirty region
                              of the same size, because the optimisation
                              skips the physical write entirely.

test_multislot_stress         ``write_op()`` reentry across radio time
                              slots when ``RADIO_SYNC_TICKER`` is selected,
                              and large single-call ``rram_write()``
                              when ``RADIO_SYNC_NONE`` is selected. Loops
                              20 iterations over a 1 KiB region (two
                              512 B slots in the radio-sync variants).
============================  ==============================================

Variants
********

Defined in ``testcase.yaml``:

========================================  ==========================================
Variant                                   Description
========================================  ==========================================
``boards.nrf.rram.throttling``            Default - ``THROTTLING=y``,
                                          ``RADIO_SYNC_NONE``.

``boards.nrf.rram.skip_if_filled``        Default plus
                                          ``NRF_RRAM_FILL_SKIP_IF_FILLED=y``.

``boards.nrf.rram.radio_sync``            Brings up BLE LL with non-connectable
                                          advertising, routing every flash op
                                          through ``write_op()``. See
                                          ``overlay-radio_sync.conf`` for the
                                          rationale behind pinning
                                          ``WRITE_BUFFER_SIZE = 32``.

``boards.nrf.rram.radio_sync_skip``       Combines ``radio_sync`` and
                                          ``skip_if_filled``.
========================================  ==========================================

Building and running
********************

Default variant:

.. code-block:: console

   west build -b nrf54l15dk/nrf54l15/cpuapp -p always tests/boards/nrf/rram
   west flash

Other variants pull their overlay through ``EXTRA_CONF_FILE``:

.. code-block:: console

   west build -b nrf54l15dk/nrf54l15/cpuapp -p always \
       tests/boards/nrf/rram -- \
       -DEXTRA_CONF_FILE=overlay-radio_sync_skip.conf
   west flash
