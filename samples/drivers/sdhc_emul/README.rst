.. zephyr:code-sample:: sdhc-emul
   :name: SDHC Emulator
   :relevant-api: sdhc_interface

   Emulate an SDHC host controller and card for testing without hardware.

Overview
********

The SDHC Emulator sample demonstrates a software-backed SDHC host controller
that works with the Zephyr FAT file system without requiring physical SD
card hardware. It is useful for continuous integration, debugging storage
stack issues, and rapid prototyping.

The sample formats a FAT disk on the emulated SD card, mounts it, creates a
text file, reads it back, and verifies the contents.

Supported Card Modes
********************

The emulator supports multiple card personalities selectable via devicetree:

.. list-table::
   :header-rows: 1

   * - Mode
     - Devicetree ``card-type``
     - Notable Commands / Features
   * - SD / SDHC
     - ``sdhc``
     - CMD8, ACMD41, ACMD6, ACMD51
   * - SDXC
     - ``sdxc``
     - CSD v2.0 with extended capacity
   * - eMMC
     - ``emmc``
     - CMD1, CMD6 (SWITCH), CMD8 (EXT_CSD), CMD21 (tuning), HS200 / HS400
   * - SDIO
     - ``sdio``
     - CMD5, CMD52 (CCCR/FBR), CMD53 (byte/block mode)

Architecture
************

The emulator can be built in two configurations:

* **Monolithic driver** (``zephyr,sdhc-emul``): combines the host controller
  and card emulation in a single device node. This sample uses the monolithic
  driver.
* **Split framework** (``zephyr,sdhc-emul-controller`` + ``zephyr,sdhc-emul-card``):
  separates the bus controller from the card peripheral, enabling multiple card
  personalities or multi-slot scenarios. The test suite uses the split
  framework.

Requirements
************

The sample runs on the ``native_sim`` board and requires no extra hardware.

Building and Running
********************

Build and run the sample as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/sdhc_emul
   :board: native_sim
   :goals: build run
   :compact:

After running, the console output should contain ``Zephyr SDHC emulator OK``
and end with ``PASS``.

Running Tests
*************

You can run this sample with twister using the included :file:`sample.yaml`:

.. code-block:: bash

   west twister -T samples/drivers/sdhc_emul --platform native_sim

Fault Injection
***************

The emulator provides test accessors to inject errors on specific commands.
This is useful for verifying error handling in upper-layer code:

.. code-block:: c

   sdhc_emul_set_fault(dev, 17);    /* Force CMD17 to return -EIO */
   sdhc_emul_clear_fault(dev);      /* Restore normal operation */

Extending the Emulator
**********************

To add a new card personality, update the CSD builder and command state machine
to mirror the respective SD / eMMC / SDIO standard. Adding a new SDIO function
involves updating the function buffer handling in the CMD52 / CMD53 logic.
