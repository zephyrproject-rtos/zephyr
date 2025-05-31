.. zephyr:code-sample:: nxp_s32_mbox
   :name: NXP S32 MBOX

   Perform NXP S32 mailbox communication using the MBOX API with data.

Overview
********

The sample supports ping-pong loopback data on a mbox channel within one core.

Building and Running
********************

Building and running the sample, example for s32z2xxdc2/s32z270/rtu0:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/s32/mbox
   :board: s32z2xxdc2/s32z270/rtu0
   :goals: build flash

Results from running the sample, example for s32z2xxdc2/s32z270/rtu0:

.. code-block:: console

   *** Booting Zephyr OS build v4.1.0-3644-ga4ee89a96597 ***
   mbox_data demo started
   send (on channel 0) value: 0
   received (on channel 0) value: 0
   send (on channel 0) value: 1
   received (on channel 0) value: 1
   ...
   send (on channel 0) value: 98
   received (on channel 0) value: 98
   send (on channel 0) value: 99
   received (on channel 0) value: 99
   mbox_data demo ended
