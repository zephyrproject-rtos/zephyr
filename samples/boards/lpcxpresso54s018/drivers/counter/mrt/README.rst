.. _mrt_sample:

LPC54S018 Multi-Rate Timer (MRT) Sample
#######################################

Overview
********

This sample demonstrates the use of the LPC54S018 Multi-Rate Timer (MRT) peripheral.
The MRT provides multiple independent timer channels that can generate interrupts 
at different rates.

The sample:

- Configures MRT channel 0 for a 1-second periodic interrupt
- Configures MRT channel 1 for a 500ms period without interrupt
- Demonstrates timer callbacks and value reading

Building and Running
********************

.. code-block:: console

   west build -b lpcxpresso54s018 samples/drivers/counter/mrt
   west flash

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build v3.5.0 ***
   [00:00:00.000,000] <inf> mrt_sample: LPC54S018 MRT (Multi-Rate Timer) Sample
   [00:00:00.000,000] <inf> mrt_sample: MRT frequency: 96000000 Hz
   [00:00:00.000,000] <inf> mrt_sample: MRT channel 0 started with 1 second period
   [00:00:00.000,000] <inf> mrt_sample: MRT channel 1 started with 500ms period
   [00:00:00.250,000] <inf> mrt_sample: Channel 1 counter value: 24000000
   [00:00:00.500,000] <inf> mrt_sample: Channel 1 counter value: 48000000
   [00:00:00.750,000] <inf> mrt_sample: Channel 1 counter value: 24000000
   [00:00:01.000,000] <inf> mrt_sample: MRT timer expired!
   [00:00:01.000,000] <inf> mrt_sample: Timer expired, restarting...

Special Features
****************

The LPC54S018 MRT has several special features:

1. **24-bit counters**: Unlike some implementations with 31-bit counters,
   the LPC54S018 MRT uses 24-bit counters (16.7M max count at 1MHz)

2. **Shared interrupt**: All channels share a single interrupt with individual
   status flags

3. **Auto-clear mode**: Can be configured to automatically clear counter on match

4. **One-shot or repeat mode**: Each channel can operate in single-shot or
   continuous mode