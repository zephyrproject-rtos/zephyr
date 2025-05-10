.. zephyr:code-sample:: esp32-ethernet
   :name: Ethernet

   Demonstrates basic Ethernet functionality using the ESP32 Ethernet Kit and Zephyr's network stack.

Overview
********

This sample demonstrates basic Ethernet functionality on supported Espressif boards, specifically the
``esp32_ethernet_kit``. It initializes the Ethernet MAC controller, enables the external PHY and MDIO bus,
and configures the network stack to use DHCP for obtaining an IP address. The sample also enables the
Zephyr network shell, allowing you to test networking features such as DNS resolution and ICMP ping.

Requirements
------------

- A supported Espressif board with Ethernet capabilities (e.g., ESP32-Ethernet-Kit)
- An external Ethernet PHY properly connected (e.g., LAN8720)
- Zephyr development environment setup (see https://docs.zephyrproject.org/latest/getting_started/index.html)
- An Ethernet cable and a DHCP-enabled router/network

Configuration
-------------

The sample is pre-configured using:

- **prj.conf**
  Enables the ESP32 Ethernet driver, DHCPv4, DNS resolver, and the network shell.
- **esp32_ethernet_kit_procpu.overlay**
  Activates the Devicetree nodes for the Ethernet MAC (``&eth``), PHY (``&phy``), and MDIO bus (``&mdio``).

No further configuration is needed for a standard ESP32-Ethernet-Kit board.

Building and Running
--------------------

To build and flash the sample, run the following commands from the Zephyr workspace:

.. code-block:: bash

   west build -b esp32_ethernet_kit/esp32/procpu samples/boards/espressif/ethernet
   west flash

> **Note:** Replace ``esp32_ethernet_kit/esp32/procpu`` with your specific board target if it differs.
