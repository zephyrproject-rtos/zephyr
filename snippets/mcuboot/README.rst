.. _snippet-mcuboot:

MCUboot snippet (mcuboot)
#########################

.. code-block:: console

   west build --no-sysbuild -S mcuboot [...]

Overview
********

This snippet changes the chosen flash partition to be the ``slot0_partition`` node, this can be
used to build for a MCUboot-enabled application, especially if the board target defaults to
building in a partition slot without a bootloader by default, and also sets the
``CONFIG_BOOTLOADER_MCUBOOT`` Kconfig to indicate MCUboot is enabled.

Requirements
************

Partition mapping correct setup in devicetree, for example:

.. literalinclude:: ../../dts/vendor/nordic/nrf52840_partition.dtsi
   :language: dts
   :dedent:
   :lines: 15-
