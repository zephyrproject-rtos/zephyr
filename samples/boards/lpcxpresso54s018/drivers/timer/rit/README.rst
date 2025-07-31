.. _rit_sample:

LPC54S018 Repetitive Interrupt Timer (RIT) Sample
##################################################

Overview
********

This sample demonstrates the use of the LPC54S018 Repetitive Interrupt Timer (RIT).
The RIT is a 48-bit counter/timer that can generate repetitive interrupts.

The sample demonstrates:

- Basic timer configuration and operation
- Timer callbacks
- Reading current timer value
- Using mask functionality
- Dynamic period reconfiguration

Building and Running
********************

.. code-block:: console

   west build -b lpcxpresso54s018 samples/drivers/timer/rit
   west flash

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build v3.5.0 ***
   [00:00:00.000,000] <inf> rit_sample: LPC54S018 RIT (Repetitive Interrupt Timer) Sample
   [00:00:00.000,000] <inf> rit_sample: RIT frequency: 96000000 Hz
   [00:00:00.000,000] <inf> rit_sample: RIT started with 1 second period
   [00:00:00.250,000] <inf> rit_sample: Current timer value: 0x5b8d800
   [00:00:01.000,000] <inf> rit_sample: RIT timer expired! Count: 1
   [00:00:01.250,000] <inf> rit_sample: Current timer value: 0x5b8d800
   [00:00:02.000,000] <inf> rit_sample: RIT timer expired! Count: 2
   [00:00:03.000,000] <inf> rit_sample: Setting mask to skip every other bit...
   [00:00:03.000,000] <inf> rit_sample: RIT timer expired! Count: 3

Features
********

The LPC54S018 RIT provides:

1. **48-bit counter**: Full 48-bit counter for long period timing
2. **Compare and mask registers**: Flexible interrupt generation
3. **Auto-clear mode**: Automatically clear counter on match
4. **Debug mode control**: Can halt/continue during debug

API Usage
*********

The RIT driver provides the following APIs:

- ``rit_configure()``: Configure timer period and options
- ``rit_start()``: Start the timer
- ``rit_stop()``: Stop the timer
- ``rit_get_value()``: Read current 48-bit counter value
- ``rit_set_mask()``: Set mask for compare operation
- ``rit_get_frequency()``: Get timer input frequency