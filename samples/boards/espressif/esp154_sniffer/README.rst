esp154_sniffer
##############

Skeleton IEEE 802.15.4 frame sniffer application for Espressif chips using the
Zephyr IEEE 802.15.4 radio driver.

Supported targets (intended)
****************************

- ESP32-H2
- ESP32-C6
- ESP32-C5

What it does
************

- Starts the IEEE 802.15.4 radio selected by ``zephyr,ieee802154``
- Enables promiscuous mode if supported by the driver
- Sets a default channel (see ``prj.conf``)
- Prints every received frame as a hex dump along with LQI and RSSI (if provided)

Build
*****

Example (adjust board name as needed)::

  west build -b esp32c6_devkitc apps/esp154_sniffer -p always

If you don't have ``west`` available in PATH, you can configure a build with
CMake directly (requires Zephyr's Python dependencies installed)::

  ZEPHYR_BASE=$PWD cmake -B build-esp154 -S apps/esp154_sniffer -GNinja -DBOARD=esp32c6_devkitc

