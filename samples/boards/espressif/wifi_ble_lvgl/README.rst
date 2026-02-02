WiFi BLE Provisioning with Minimalistic LVGL UI
###############################################

Overview
********

This sample demonstrates a clean and minimalistic WiFi BLE provisioning system with a modern LVGL user interface. The application provides a streamlined two-screen UI experience for WiFi credential provisioning through Bluetooth Low Energy (BLE) with persistent storage using NVS (Non-Volatile Storage).

Features
********

* **Minimalistic LVGL UI**: Clean white interface with modern design principles
* **Two-Screen Navigation**:
  
  - **Main Screen**: Status display with centered "Device IP" button
  - **IP Display Screen**: Shows device IP address with back navigation
  
* **BLE WiFi Provisioning**: Receive WiFi credentials via BLE in JSON format
* **NVS Persistent Storage**: Store and retrieve WiFi credentials across reboots
* **Real-time Status Updates**: Color-coded visual feedback for connection status
* **Touch Debouncing**: Smooth navigation with proper touch handling
* **Factory Reset**: Hardware button support for clearing stored credentials
* **Auto-reconnect**: Automatically connects using stored credentials on boot

UI Design
*********

**Modern Minimalistic Theme:**
- Background: Pure White (#FFFFFF)
- Primary Button: Modern Blue (#007BFF)
- Text Colors:
  - Default: Black (#000000)

**Screen Flow:**
1. **Main Screen**: 
   - Clean white background
   - Centered status text with color-coded feedback
   - Modern blue "DEVICE IP" button (appears after WiFi connection)
   - All content perfectly centered for optimal user experience

2. **IP Display Screen**: 
   - Shows device IP address in black text
   - Blue "BACK" button for navigation
   - Minimalistic layout with centered content

**Touch Interaction:**
- 300ms debounce timing for smooth navigation
- Visual feedback with subtle button shadows
- Responsive touch areas with proper coordinate handling

Requirements
************

* ESP32-S3 based board with display and touch support
* LVGL display driver with touch input
* BLE capability
* NVS storage partition
* Python 3.7+ with `bleak` library for provisioning script

Supported Boards
****************

* ESP32-S3-BOX-3 (primary target)

Building and Running
********************

1. Build the application:

   .. code-block:: console

      west build -b esp32s3_box3 samples/boards/espressif/wifi_ble_provisioning_lvgl

2. Flash to device:

   .. code-block:: console

      west flash

3. Monitor the output:

   .. code-block:: console

      west espressif monitor

Usage Instructions
******************

**Device Operation:**

1. **Power On**: Device shows "WiFi BLE Provisioning Ready" on white background
2. **Auto-Connect**: If credentials are stored, device automatically connects to WiFi
3. **Manual Provisioning**: If no credentials or connection fails, use BLE provisioning
4. **Success State**: "WiFi Connected Successfully!" appears in green with "DEVICE IP" button
5. **View IP**: Touch "DEVICE IP" to see the assigned IP address
6. **Navigation**: Use "BACK" button to return to main screen

**Status Indicators:**
- **Black Text**: Ready for provisioning
- **Orange Text**: Processing (receiving credentials or connecting)
- **Green Text**: Successfully connected to WiFi
- **Red Text**: Connection failed

**BLE Provisioning with Python Script:**

The included `provision_wifi.py` script provides an easy way to provision the device:

.. code-block:: console

   python3 provision_wifi.py --ssid "YourWiFiNetwork" --password "YourPassword"

**Script Features:**
- Automatic device discovery
- Service and characteristic validation
- Retry logic for reliability
- Detailed error reporting
- Connection status verification

**Manual BLE Provisioning:**

1. Connect to device via BLE (advertised as "ESP32_S3_BOX3_BLE")
2. Write to WiFi credentials characteristic (UUID: `12345678-1234-1234-1234-123456789abd`)
3. Send JSON format:

   .. code-block:: json

      {
        "ssid": "YourWiFiSSID",
        "password": "YourWiFiPassword", 
        "save_credentials": true
      }

**Service UUIDs:**
- Service: `12345678-1234-1234-1234-123456789abc`
- Credentials Characteristic: `12345678-1234-1234-1234-123456789abd`
- Status Characteristic: `12345678-1234-1234-1234-123456789abe`

Python Provisioning Script
***************************

The included `provision_wifi.py` script provides a complete provisioning solution:

**Installation:**

.. code-block:: console

   pip install bleak

**Usage:**

.. code-block:: console

   # Basic usage
   python3 provision_wifi.py --ssid "MyNetwork" --password "MyPassword"
   
   # Don't save credentials to device storage
   python3 provision_wifi.py --ssid "MyNetwork" --password "MyPassword" --no-save
   
   # Scan for devices only
   python3 provision_wifi.py --scan-only

**Features:**
- Automatic retry logic (up to 3 attempts)
- Service discovery and validation
- Connection status verification
- Detailed error reporting and debugging
- Support for both write types (with/without response)

Factory Reset
*************

Press the hardware button 5 times quickly within 5 seconds to perform a factory reset:

* Clears all stored WiFi credentials from NVS
* Disconnects from current WiFi network
* Reboots the device
* Returns to provisioning mode

Sample Output
*************

**Successful Provisioning:**

.. code-block:: console

   *** Booting Zephyr OS build v3.5.0 ***
   [00:00:00.712,000] <inf> main: WiFi BLE Provisioning with LVGL UI Example
   [00:00:00.712,000] <inf> main: Board: esp32s3_box3
   [00:00:00.918,000] <inf> ui_manager: Main screen created
   [00:00:00.920,000] <inf> ui_manager: IP display screen created
   [00:00:01.027,000] <inf> nvs_storage: NVS storage initialized successfully
   [00:00:01.027,000] <inf> wifi_manager: WiFi manager initialized
   [00:00:01.059,000] <inf> ble_gatt: GATT service registered with UUIDs
   [00:00:01.060,000] <inf> ble_gatt: Advertising successfully started
   [00:00:14.943,000] <inf> ble_gatt: Connected XX:XX:XX:XX:XX:XX (public)
   [00:00:16.592,000] <inf> ble_gatt: === BLE Write Handler Called ===
   [00:00:16.592,000] <inf> ble_gatt: Received data (73 bytes): '{"ssid": "MyNetwork", "password": "MyPassword", "save_credentials": true}'
   [00:00:16.596,000] <inf> wifi_manager: WiFi connection request submitted successfully
   [00:00:21.687,000] <inf> wifi_manager: IPv4 address: 192.168.1.100
   [00:00:21.694,000] <inf> ui_manager: Device IP button is now visible

**Python Script Output:**

.. code-block:: console

   ESP32 WiFi BLE Provisioning Tool
   ========================================
   SSID: MyNetwork
   Password: **********
   Save credentials: True
   
   Scanning for ESP32_S3_BOX3_BLE...
   Found device: ESP32_S3_BOX3_BLE (XX:XX:XX:XX:XX:XX)
   Connecting to XX:XX:XX:XX:XX:XX...
   Connected successfully!
   Testing GATT operations...
   Initial status read successful: {'status': 'disconnected', 'message': 'WiFi is not connected'}
   Sending credentials: {"ssid": "MyNetwork", "password": "MyPassword", "save_credentials": true}
   Data length: 73 bytes
   Found WiFi service: 12345678-1234-1234-1234-123456789abc
   Found characteristic: 12345678-1234-1234-1234-123456789abd (properties: ['write-without-response', 'write'])
   Found credentials characteristic with properties: ['write-without-response', 'write']
   Attempting to write credentials...
   Credentials sent successfully!
   Check the device display for connection status
   
   Waiting for connection result...
   Device status: {'status': 'connected', 'message': 'WiFi connected successfully'}
   ✅ WiFi provisioning successful!
   
   ✅ Provisioning completed!
   Check the device display for the final status and IP address.

Architecture
************

The application uses a modular, event-driven architecture:

**Core Components:**

* **UI Manager** (`ui_manager.c/h`):
  - LVGL interface management
  - Touch input handling with debouncing
  - Screen transitions and event processing
  - Color-coded status feedback

* **WiFi Manager** (`wifi_manager.c/h`):
  - Asynchronous WiFi connection handling
  - Network event processing
  - IP address management
  - Connection status reporting

* **BLE GATT Server** (`ble_gatt.c/h`):
  - BLE service and characteristic definitions
  - Credential reception and parsing
  - Work queue for WiFi operations
  - Connection management

* **NVS Storage** (`nvs_storage.c/h`):
  - Persistent credential storage
  - Factory reset functionality
  - Data validation and error handling

* **Main Application** (`main.c`):
  - System initialization and coordination
  - Status monitoring thread
  - Factory reset button handling

**Key Design Decisions:**

* **Asynchronous WiFi**: Non-blocking connection prevents UI freezing
* **Event-Driven UI**: Message queue system for responsive interface
* **Touch Debouncing**: 300ms debounce prevents accidental double-taps
* **Modular Design**: Clear separation of concerns for maintainability
* **Error Handling**: Comprehensive error reporting and recovery

Configuration
*************

Key configuration options in `prj.conf`:

.. code-block:: ini

   # LVGL Configuration
   CONFIG_LV_Z_MEM_POOL_SIZE=16384
   CONFIG_LV_Z_VDB_SIZE=32
   CONFIG_LV_COLOR_DEPTH_16=y
   
   # Memory Configuration
   CONFIG_HEAP_MEM_POOL_SIZE=71680
   CONFIG_MAIN_STACK_SIZE=4096
   
   # BLE Configuration
   CONFIG_BT_DEVICE_NAME="ESP32_S3_BOX3_BLE"
   CONFIG_BT_L2CAP_TX_MTU=247
   
   # WiFi Configuration
   CONFIG_WIFI_ESP32=y
   CONFIG_NET_DHCPV4=y

Troubleshooting
***************

**Display Issues:**
- Verify display initialization in logs
- Check LVGL memory pool size (increase if needed)
- Ensure touch controller (GT911) is detected

**Touch Issues:**
- Check touch coordinates in logs
- Verify button areas are calculated correctly
- Adjust debounce timing if needed

**BLE Connection Issues:**
- Ensure device appears as "ESP32_S3_BOX3_BLE" in BLE scan
- Check service UUIDs match exactly
- Verify Python `bleak` library is installed
- Try the retry mechanism in the Python script

**WiFi Connection Issues:**
- Verify network is 2.4GHz (5GHz not supported)
- Check SSID and password are correct
- Monitor logs for detailed WiFi error messages
- Ensure network allows new device connections

**Memory Issues:**
- Monitor heap usage in logs
- Increase `CONFIG_HEAP_MEM_POOL_SIZE` if needed
- Check for memory leaks in touch handling

**Performance Issues:**
- Verify UI thread priority settings
- Check LVGL timer handler frequency
- Monitor system load and stack usage
