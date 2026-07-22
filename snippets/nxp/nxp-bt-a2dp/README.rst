.. _snippet-nxp-bt-a2dp:

NXP Bluetooth A2DP (nxp-bt-a2dp)
#################################

Overview
********

This snippet provides NXP-specific Bluetooth A2DP streaming performance
tuning. It increases L2CAP TX buffers, heap, stack sizes, and HCI buffer
counts for high-throughput audio streaming.

Usage
*****

Use together with ``prj-ble.conf`` or ``prj-ble-classic.conf``:

.. code-block:: console

   west build -b rd_rw612_bga <sample> -S nxp-bt-a2dp -- \
     -DEXTRA_CONF_FILE="prj-ble-classic.conf prj-wifi.conf"
