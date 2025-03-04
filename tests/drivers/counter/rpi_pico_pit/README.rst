RPI Pico Pit Counter Tests
##########################

Tests the RPI Pico PIT counter driver. The testsuite exercises most of the drivers functionality and
includes tests for erroneous inputs.

Hardware Setup
==============
- A Raspberry Pi Pico board
- Other RP2040 boards should work as well, but no overlay files for these are supplied

Sample Output
=============

.. code-block:: console


   *** Booting Zephyr OS build v4.0.0-4357-g042761cf36bd ***
   Running TESTSUITE counter_pico_pit
   ===================================================================
   START - test_counter_wraps
   PASS - test_counter_wraps in 0.111 seconds
   ===================================================================
   START - test_no_value_change_after_stop
   PASS - test_no_value_change_after_stop in 0.301 seconds
   ===================================================================
   START - test_set_top_value
   PASS - test_set_top_value in 0.001 seconds
   ===================================================================
   START - test_top_value_interrupt_set
   PASS - test_top_value_interrupt_set in 0.201 seconds
   ===================================================================
   START - test_top_value_interrupt_unset
   PASS - test_top_value_interrupt_unset in 0.241 seconds
   ===================================================================
   START - test_top_value_no_counter_value_reset
   PASS - test_top_value_no_counter_value_reset in 0.051 seconds
   ===================================================================
   START - test_top_value_no_value_reset_new_top_value_smaller_than_counter_value
   PASS - test_top_value_no_value_reset_new_top_value_smaller_than_counter_value in 0.051 seconds
   ===================================================================
   START - test_top_value_zero_ticks
   E: rpi_pico_pit_channel: Top value should be greater than 0 and have a maximum value of 65535
   PASS - test_top_value_zero_ticks in 0.009 seconds
   ===================================================================
   START - test_two_pit_channels_top_interrupts
   PASS - test_two_pit_channels_top_interrupts in 0.211 seconds
   ===================================================================
   START - test_value_increase_over_time
   PASS - test_value_increase_over_time in 0.051 seconds
   ===================================================================
   TESTSUITE counter_pico_pit succeeded

   ------ TESTSUITE SUMMARY START ------

   SUITE PASS - 100.00% [counter_pico_pit]: pass = 10, fail = 0, skip = 0, total = 10 duration = 1.228 seconds
   - PASS - [counter_pico_pit.test_counter_wraps] duration = 0.111 seconds
   - PASS - [counter_pico_pit.test_no_value_change_after_stop] duration = 0.301 seconds
   - PASS - [counter_pico_pit.test_set_top_value] duration = 0.001 seconds
   - PASS - [counter_pico_pit.test_top_value_interrupt_set] duration = 0.201 seconds
   - PASS - [counter_pico_pit.test_top_value_interrupt_unset] duration = 0.241 seconds
   - PASS - [counter_pico_pit.test_top_value_no_counter_value_reset] duration = 0.051 seconds
   - PASS - [counter_pico_pit.test_top_value_no_value_reset_new_top_value_smaller_than_counter_value] duration = 0.051 seconds
   - PASS - [counter_pico_pit.test_top_value_zero_ticks] duration = 0.009 seconds
   - PASS - [counter_pico_pit.test_two_pit_channels_top_interrupts] duration = 0.211 seconds
   - PASS - [counter_pico_pit.test_value_increase_over_time] duration = 0.051 seconds

   ------ TESTSUITE SUMMARY END ------

   ===================================================================
   RunID: c2854719f8c282344687e97f964e7fdc
   PROJECT EXECUTION SUCCESSFUL
