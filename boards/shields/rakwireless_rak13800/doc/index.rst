.. _rakwireless_rak13800:

RAK13800 WisBlock Ethernet Module
#################################

Overview
********

RAK13800 is a WisBlock IO module carrying a WIZnet W5100S 10/100BASE-T Ethernet controller
with an RJ-45 jack and magnetics. The controller is strapped for SPI and reaches the
WisBlock Core over the IO slot SPI bus.

More information about the module can be found at `RAK13800 WisBlock Ethernet Module`_.

Requirements
************

RAK13800 requires a WisBlock Base Board and a WisBlock Core module. It mounts on the
WisBlock IO slot, so the base board shield must be listed first.

Pin Assignments
***************

+--------+-------------------+
| Signal | WisBlock IO pin   |
+========+===================+
| CS     | 25 (SPI_CS)       |
+--------+-------------------+
| SCLK   | 26 (SPI_CLK)      |
+--------+-------------------+
| MISO   | 27 (SPI_MISO)     |
+--------+-------------------+
| MOSI   | 28 (SPI_MOSI)     |
+--------+-------------------+
| RESET  | 31 (IO3)          |
+--------+-------------------+
| INT    | 38 (IO6)          |
+--------+-------------------+

The module also routes RESET to the WisBlock RESET pin and CS to IO5 through resistors
that are left unpopulated, so those paths are not described here.

MAC Address
***********

The W5100S has no MAC address of its own and the module carries no EEPROM, so the shield
asks for a random locally administered address with ``zephyr,random-mac-address``. The
address changes on every boot. An application that needs a stable address overrides it:

.. code-block:: devicetree

   &w5100s_rak13800 {
       /delete-property/ zephyr,random-mac-address;
       local-mac-address = [02 00 00 00 00 01];
   };

Programming
***********

List the base board shield before this one:

.. zephyr-app-commands::
   :zephyr-app: samples/net/dhcpv4_client
   :board: rak4631/nrf52840
   :shield: rakwireless_rak19007,rakwireless_rak13800
   :goals: build flash

References
**********

.. target-notes::

.. _RAK13800 WisBlock Ethernet Module:
   https://docs.rakwireless.com/product-categories/wisblock/rak13800
