.. _external_module_heatshrink:

Heatshrink
##########

Introduction
************

`Heatshrink`_ is a data compression/decompression library optimized for
very low memory environments. It uses `LZSS`_ encoding with a small sliding
window, making it suitable for memory-constrained microcontrollers.

Heatshrink is used by the Delta Firmware Update subsystem to decompress
bsdiff delta patches on-target.

Heatshrink is licensed under the ISC license.

Usage with Zephyr
*****************

To pull in heatshrink as a Zephyr :ref:`module <modules>`, either add it
as a West project in the :file:`west.yaml` file or pull it in by adding a
submanifest (e.g. ``zephyr/submanifests/heatshrink.yaml``) file with the
following content and run ``west update``:

.. code-block:: yaml

   manifest:
     projects:
       - name: heatshrink
         url: https://github.com/kickmaker/heatshrink
         revision: c47356093c4ea079749214de2864186a578668a0
         path: modules/lib/heatshrink

Reference
*********

.. _Heatshrink:
   https://github.com/atomicobject/heatshrink
