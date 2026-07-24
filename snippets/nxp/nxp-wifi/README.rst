.. _snippet-nxp-wifi:

NXP Wi-Fi (nxp-wifi)
####################

Overview
********

This snippet enables NXP Wi-Fi platform support including:

- NXP Wi-Fi driver (``CONFIG_WIFI_NXP``)
- Zero-copy TX/RX for improved throughput
- Power management integration
- Performance-optimized thread priorities and buffer sizes
- Code/data relocation to SRAM for faster execution

Supported Boards
****************

- ``frdm_rw612`` / ``rd_rw612_bga`` (integrated Wi-Fi)
- ``mimxrt1060_evk`` (external module: IW416/IW612/IW610 via SDIO)

Usage
*****

.. code-block:: console

   west build -b rd_rw612_bga <sample> -S nxp-wifi

Typically used together with ``nxp-wifi-hostap`` for WPA supplicant support:

.. code-block:: console

   west build -b rd_rw612_bga <sample> -S nxp-wifi -S nxp-wifi-hostap
