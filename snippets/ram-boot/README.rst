.. _snippet-ram-boot:

RAM Boot Snippet (ram-boot)
#################################

.. code-block:: console

   west build -S ram-boot [...]

Overview
********

This snippet enables some of the supported boards/SoCs to boot from RAM.
This is useful for both development and deployment (for example, on a board without an SPI NOR flash).
Often these images can be loaded via a bootloader from the controlling device.
Such images can also be later processed to make device's bootloader to copy them from flash to RAM.
