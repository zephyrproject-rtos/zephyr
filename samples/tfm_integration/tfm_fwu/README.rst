.. zephyr:code-sample:: tfm_fwu
   :name: TF-M Firmware Update

   Perform a firmware update using the PSA Firmware Update API with TF-M.

Overview
********

This TF-M integration example demonstrates how to use the PSA Firmware Update
API to perform a firmware update on a device running TF-M on the secure side
and Zephyr on the non-secure side.

The sample includes two applications:

- The main application writes a new firmware image to the update partition
  using the PSA FWU API, then triggers a reboot to apply the update.
- The swapped application is the new firmware image that boots after the
  update is applied.

For simplicity, the signed image of the swapped application is embedded
directly into the main application binary at build time.

Building and Running
********************

This sample requires sysbuild and runs on platforms with TF-M support.

.. zephyr-app-commands::
   :zephyr-app: samples/tfm_integration/tfm_fwu
   :board: mps2/an521/cpu0/ns
   :goals: build

Sample Output
*************

.. code-block:: console

   TF-M FWU on mps2
   Installing new firmware for component 1
   Starting FWU
   Writing ... bytes
   Finishing FWU
   Installing FWU
   FWU installed, reboot required
   Rebooting to apply the new firmware...

After reboot, the swapped application prints:

.. code-block:: console

   Swapped application booted on mps2
   FWU completed successfully
