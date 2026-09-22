.. _snippet-slot1-partition:

Slot1 partition snippet (slot1-partition)
#########################################

.. code-block:: console

   west build -S slot1-partition [...]

Overview
********

This snippet changes the chosen flash partition to be the ``slot1_partition`` node, this can be
used to build for the alternate slot in MCUboot direct-xip or firmware loader modes.

Requirements
************

Partition mapping correct setup in devicetree, for example:

.. literalinclude:: ../../dts/vendor/nordic/nrf52840_partition.dtsi
   :language: dts
   :dedent:
   :lines: 15-
