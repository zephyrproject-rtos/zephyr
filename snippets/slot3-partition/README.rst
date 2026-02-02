.. _snippet-slot3-partition:

Slot3 partition snippet (slot3-partition)
#########################################

.. code-block:: console

   west build -S slot3-partition [...]

Overview
********

This snippet changes the chosen flash partition to be the ``slot3_partition`` node, this can be
used to build for the mutable slot in 2-stage MCUboot project.

Requirements
************

Partition mapping correct setup in devicetree, for example:

.. literalinclude:: ../../dts/vendor/nordic/nrf52840_partition.dtsi
   :language: dts
   :dedent:
   :lines: 15-
