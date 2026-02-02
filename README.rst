Zephyr RTOS Port for ESP32-S3-BOX-3 🚀
=========================================

This repository branch provides board support and reference samples for
running Zephyr RTOS on the **ESP32-S3-BOX-3** platform.

The port focuses on enabling display, touch, Wi-Fi, Bluetooth, persistent
storage, and LVGL-based user interfaces on the ESP32-S3-BOX-3 while keeping
all changes aligned with upstream Zephyr design principles.

Branch 🌿
--------

- **Branch name:** ``zephyr-esp32s3-box-3``
- **Base:** Upstream Zephyr ``main``

All changes are maintained on top of upstream Zephyr with minimal
divergence and clear separation between board, driver, and sample code.

Hardware Overview 🔧
-------------------

ESP32-S3-BOX-3 features:

- 🖥️ ESP32-S3 dual-core MCU
- 📺 SPI-connected LCD panel
- 👆 Capacitive touch controller
- 📶 Wi-Fi (802.11 b/g/n)
- 📡 Bluetooth Low Energy (BLE)
- 💾 External flash with MCUboot support

Supported Features ✅
--------------------

The following functionality is enabled and validated in this port:

- 📋 ESP32-S3-BOX-3 board definition
- 🎨 LVGL graphics stack
- 📺 SPI LCD display support
- 👆 Capacitive touch controller integration
- 📡 Bluetooth Low Energy (BLE)
- 📶 Wi-Fi station mode
- 🔗 BLE-based Wi-Fi provisioning
- 💾 NVS-backed persistent storage
- 🥾 MCUboot-compatible flash partitioning
- ⚙️ Application CPU and LP core partition layout

Board Support 🛠️
---------------

Board files are located under:

::

  boards/espressif/esp32s3_box3/

Board documentation is available in:

::

  boards/espressif/esp32s3_box3/README.md

Custom devicetree overlays are used by samples to enable additional
features such as custom flash partitioning and NVS storage without
modifying the base board devicetree.

Flash Partitioning 💾
--------------------

Some samples use a custom flash layout provided via devicetree overlay:

- 🥾 MCUboot boot partition
- 🔄 Dual application image slots
- ⚙️ App CPU and LP core partitions
- 💾 Dedicated NVS storage partition
- 🔄 Scratch partition for image swap
- 🐛 Coredump partition

The NVS partition is automatically mounted at ``/nvs`` using Zephyr's
``fstab`` mechanism for persistent application data.

Build Instructions 🔨
--------------------

Prerequisites:

- 🛠️ Zephyr SDK installed
- 🌿 West tool initialized
- 🔧 ESP32 toolchain available
- 🐍 Python 3.x

Initialize the workspace:

::

  west init -m https://github.com/notionext/zephyr
  west update

Build a sample:

::

  west build -b esp32s3_box3 samples/boards/espressif/wifi_ble_lvgl

Flash to device:

::

  west flash

Monitor the output:

::

  west espressif monitor

Usage Instructions 📋
******************

**Device Operation:**

1. **Power On** 🔌: Device shows "WiFi BLE Provisioning Ready" on white background

2. **Auto-Connect** 🔄: If credentials are stored, device automatically connects to WiFi

3. **Manual Provisioning** 📱:

   - Connect to device via BLE (advertised as "ESP32_S3_BOX3_BLE")
   - Write to WiFi credentials characteristic (UUID: `12345678-1234-1234-1234-123456789abd`)
   - Send JSON format:

   .. code-block:: json

      {
        "ssid": "YourWiFiSSID",
        "password": "YourWiFiPassword", 
        "save_credentials": true
      }

**Service UUIDs:**

- 🔧 Service: `12345678-1234-1234-1234-123456789abc`
- 🔑 Credentials Characteristic: `12345678-1234-1234-1234-123456789abd`
- 📊 Status Characteristic: `12345678-1234-1234-1234-123456789abe`

   .. raw:: html

      <img src="doc/_static/images/wifi_ble_lvgl_3.png" alt="WiFi BLE LVGL Success State" width="400">

**Python Provisioning Script** 🐍

The included `provision_wifi.py` script provides a complete provisioning solution:

**Installation:**

::

   pip install bleak

**Usage:**

::

   # Basic usage
   python3 provision_wifi.py --ssid "MyNetwork" --password "MyPassword"
   
   # Don't save credentials to device storage
   python3 provision_wifi.py --ssid "MyNetwork" --password "MyPassword" --no-save
   
   # Scan for devices only
   python3 provision_wifi.py --scan-only

4. **Success State** ✅: "WiFi Connected Successfully!" appears in green with "DEVICE IP" button

   .. raw:: html

      <img src="doc/_static/images/wifi_ble_lvgl.png" alt="WiFi BLE LVGL Success State" width="400">


5. **View IP** 🌐: Touch "DEVICE IP" to see the assigned IP address

   .. raw:: html

      <img src="doc/_static/images/wifi_ble_lvgl_2.png" alt="WiFi BLE LVGL Success State" width="400">

6. **Navigation** ⬅️: Use "BACK" button to return to main screen

Notes on Bluetooth and Wi-Fi 📡
------------------------------

- 📡 BLE is used for provisioning and control-plane communication
- 📶 Wi-Fi operates in station mode
- 🔐 Credentials are stored securely in NVS
- 🔄 BLE and Wi-Fi can operate concurrently with LVGL UI

Status 📊
--------

This port is intended for:

- 🔍 Platform evaluation
- ✅ Feature validation
- 🚀 Product prototyping
- 📖 Reference implementation for ESP32-S3-BOX-3

Some components may still be under active development and are not yet
submitted upstream.

References 📊
--------
- samples/boards/espressif/lcd_lvgl
- samples/boards/espressif/lcd_lvgl_touch
- samples/boards/espressif/wifi_ble_lvgl

License 📄
---------

This project is licensed under the Apache License, Version 2.0.
See the ``LICENSE`` file for details.

