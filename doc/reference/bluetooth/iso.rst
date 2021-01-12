.. _bluetooth_iso:

Bluetooth Isochronous Channels
##############################

The Bluetooth Isochronous channels :option:`CONFIG_BT_ISO` is a feature
introduced in Bluetooth 5.2 core specification. It can be used to create streams
(e.g. audio) using either Connected Isochronous Stream (CIS) or Broadcast
Isochronous Stream (BIS).

Shell
*****

ISO  shell module implements commands corresponding to the procedures related to
CIS, in the future it might support BIS procedures as well.

.. toctree::
   :maxdepth: 1

   shell/iso.rst

API reference
**************

.. doxygengroup:: bt_iso
   :project: Zephyr
   :members:
