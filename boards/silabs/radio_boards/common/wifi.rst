.. _siwx917_wifi_features:

SiWx917 Wi-Fi Features (Alpha)
##############################

Overview
========

This document provides an overview of the 917 Wi-Fi module's capabilities and
features, focusing on its STA and Soft-AP modes.


STA Mode Features
=================

The STA (Station) mode of the 917 Wi-Fi module provides robust functionality
for connecting to wireless networks as a client. Below are the key features:


Scan
----

The STA mode supports network scanning to identify available access points:

   - **Active Scanning**: Actively probes for nearby networks.
   - **Passive Scanning**: Listens for beacon frames to discover networks.
   - **Background (BG) Scanning**: Supports background scanning to identify
     better access points without interrupting the active connection.

The module supports up to **11 scan results** at a time, ensuring efficient
discovery of nearby access points.

The module supports only one SSID that can be specified for SSID filtering.

Active and Passive scanning are performed when the device is not in associated
state.

BG Scanning is performed when the device is in associated state.

If channel is not provided, the device will scan all the valid channels.

The default active dwell time is 100 ms.

The default passive dwell time is 400 ms.


Security
--------

The module incorporates advanced security measures to ensure a safe and
reliable connection. It supports the following security modes:

   - **Open**: No encryption, suitable for unsecured networks.
   - **WPA (Wi-Fi Protected Access)**: Provides improved security over WEP.
   - **WPA2 (Wi-Fi Protected Access II)**: Industry-standard encryption for
     most networks.
   - **WPA3 (Wi-Fi Protected Access III)**: Enhanced encryption and protection
     against modern threats.
   - **WPA2-SHA256**: Enhanced WPA2 security using SHA256 for improved
     cryptographic strength.
   - **WPA3 Transition Mode (WPA2+WPA3)**: Ensures compatibility with networks
     that use both WPA2 and WPA3 security standards.

These modes ensure that the module can connect securely to a wide range of
networks while maintaining strong data protection.


Link Modes
----------

To ensure compatibility with diverse networks, the module supports multiple
Wi-Fi link modes:

   - **802.11b (Wi-Fi 0 and Wi-Fi 1)**: Legacy mode with speeds up to 11 Mbps.
   - **802.11g (Wi-Fi 3)**: Enhanced performance up to 54 Mbps.
   - **802.11n (Wi-Fi 4)**: High throughput with MIMO technology.
   - **802.11ax (Wi-Fi 6)**: Next-generation efficiency, capacity, and
     performance.


Power Save
----------

The module implements power-saving mechanisms to optimize energy consumption.

It supports the following legacy power-saving modes:

   - **DTIM (Delivery Traffic Indication Message) Based**: Reduces power
     consumption by using DTIM intervals to wake up only when buffered data is
     available at the access point.
   - **Listen Interval Based**: Allows the module to wake up periodically based
     on a defined listen interval to check for traffic. The Listen Interval
     will be in units of Beacon Interval.

The device supports (Power save) Listen Interval ranging from 100 ms - 1000 ms.
Accordingly, the Listen Interval needs to be configured in units of
Beacon Interval. If the Listen Interval is configured more than 1000 ms, then
it takes the value as 1000 ms. The Listen Interval based wakeup will take
effect once the device is in connected state. If the Listen Interval is set to
zero, the driver will automatically switch to DTIM wakeup. If power save is
disabled by firmware than driver will take the previously configured
Listen Interval when it is enabled.

This (Power save) Listen Interval shall be less than the Listen Interval
advertised in (Re)Association Request frame.

It also supports the below modes to retrieve buffered packets:

   - **PS-Poll (Power Save Polling)**: Enables the device to retrieve buffered
     packets from the access point when it has entered power save mode.
   - **QoS Null Data**: Allows the device to send QoS Null frames to retrieve
     buffered packets from the access point in a power-efficient manner.

The device enables its proprietary mode Enhanced Max PSP feature to retrieve
buffered packets by default for improved performance.

If this feature is disabled through disabling the config flag
``CONFIG_WIFI_SILABS_SIWX91X_ENHANCED_MAX_PSP``, then QoS Null frame gets
enabled. Both QoS Null frame and Enhanced Max PSP are mutually exclusive.

After disconnection, PS gets disabled, thus, PS needs to be enabled again for
new connection, if required.

The Monitor Interval is defined as the time from the last packet received
(except ACK). If there is no packet received till this interval, then the
device goes to sleep. The default value of Monitor Interval is 50 ms if 0 is
configured. This is used for QoS Null Data or Enhanced Max PSP. The device
supports maximum value of Monitor Interval as 1000 ms.

Currently, ``timeout_ms`` is not supported in the device.


Management Frame Protection
---------------------------

Support for **802.11w** ensures protection for management frames, safeguarding
against:

   - Deauthentication attacks.
   - Disassociation attacks.

802.11w is enabled by default.

**Recommended MFP Values for different security modes:**

   +---------------+-------------+-------------+--------------+
   | Security Mode | MFP Disable | MFP Capable | MFP Required |
   +===============+=============+=============+==============+
   | Open          | Yes         |             |              |
   +---------------+-------------+-------------+--------------+
   | WPA           | Yes         |             |              |
   +---------------+-------------+-------------+--------------+
   | WPA + WPA2    |             | Yes         | Optional     |
   +---------------+-------------+-------------+--------------+
   | WPA2          |             | Yes         | Optional     |
   +---------------+-------------+-------------+--------------+
   | WPA2 + WPA3   |             | Yes         | No           |
   +---------------+-------------+-------------+--------------+
   | WPA3          |             | Yes         | Yes          |
   +---------------+-------------+-------------+--------------+


BG Scan and Roaming
-------------------

Roaming enables a Wi-Fi STA (Station) to seamlessly transition from one
Access Point (AP) to another within the same network without losing its
connection. This ensures uninterrupted connectivity as the STA moves across
different AP coverage areas.

The device support **Legacy Roaming**, which comprises the following procedures:

   1. **Background (BG) Scan**:

      - A standard scanning procedure executed while the STA remains connected
        to the network.
      - Triggered when the Received Signal Strength Indicator (RSSI) falls
        below a predefined threshold.

   2. **Roaming**:

      - Initiated when the RSSI crosses the threshold and a roaming hysteresis
        condition is met.
      - The roaming process can be carried out using either an
        **NDP (Null Data Packet)** or a **Deauthentication frame**. The default
        configuration is to use NDP. The configuration can be changed using
        Kconfig ``CONFIG_WIFI_SILABS_SIWX91X_ROAMING_USE_DEAUTH`` to use a
        deauthentication frame for Roaming.


**Challenges**:

   - The **Zephyr API** does not currently provide an API for Legacy Roaming.
     While APIs for **802.11r roaming** are defined, they do not apply to
     Legacy Roaming.

**Current Design**:

   - **BG Scan**: Performed using the Scan API.

   - **Roaming Configuration**: Configured during the first BG Scan. Roaming
     parameters are managed through **Kconfig** options.

Roaming is enabled by default, but BG Scan needs to be performed to initiate
and configure the Roaming parameters. Roaming can be disabled by disabling the
config flag ``WIFI_SILABS_SIWX91X_ENABLE_ROAMING``.

BG Scan stops when the STA gets disconnected. This BG Scan command again needs
to be given to enable BG Scan and Roaming.

The device support multi-probe in BG Scan where it can also scan APs with
different SSID. This configuration can be disabled using Kconfig.


Target Wake Time (TWT)
----------------------

The module supports **Individual Target Wake Time (TWT)** in compliance with
Wi-Fi 6 standards:

   - **Individual TWT**: Allows the module to negotiate wake times with the
     access point, optimizing power consumption for the device. It supports
     only as a requester.
   - Does not support broadcast TWT.

This feature is ideal for IoT devices requiring periodic connectivity with
minimal energy use.

The device does not support Explicit TWT. It does not support
TWT Information element.

The device does not support TWT quick setup. It does not support configuring
parameters like TWT interval, TWT wake ahead duration and dialog token.


Bugs and Limitations
--------------------

   - For WPA3 (SAE), our device has a limitation of 64 characters on the
     password.
   - EAP security modes are not supported.
   - The device supports bandwidth of 20 MHz only and 1 spatial stream.
   - 802.11r is not supported.
   - WMM power save mode is not supported.


Soft-AP Mode Features
=====================

In Soft-AP (Software Access Point) mode, the 917 Wi-Fi module can act as an
access point, allowing other devices to connect to it. Below are the key
features:


Security
--------

The module provides secure connections for devices connecting to the Soft-AP.
It supports the following security modes:

   - **Open**: No encryption, suitable for unsecured networks.
   - **WPA (Wi-Fi Protected Access)**: Provides improved security over WEP.
   - **WPA2 (Wi-Fi Protected Access II)**: Industry-standard encryption for
     most networks.
   - **Mixed Mode (WPA+WPA2)**: Ensures compatibility with networks that use
     both WPA and WPA2 security standards.

These modes allow flexible configurations for devices connecting to the Soft-AP.


Link Modes
----------

Soft-AP mode supports the following link modes:

   - **802.11b (Wi-Fi 0 and Wi-Fi 1)**: Legacy mode for basic connectivity.
   - **802.11g (Wi-Fi 3)**: Enhanced speeds for modern devices.
   - **802.11n (Wi-Fi 4)**: High-speed connectivity for demanding applications.


Hidden SSID
-----------

Allows the network name (SSID) to be concealed from broadcasting. This enhances
privacy by preventing the SSID from being visible to unauthorized users.


Bugs and Limitations
--------------------

   - The device supports bandwidth of 20 MHz only and 1 spatial stream.
   - Although STA and BLE can coexist simultaneously, AP and BLE cannot operate
     together.

Common Features
===============

Regulatory Domain
-----------------

The SiWx91x driver supports regulatory domain configuration to enforce
region-specific channel and transmit power limits in both STA and Soft-AP modes.

   - Country code values follow ISO/IEC 3166-1 alpha-2 (for example, ``US``, ``IN``, ``FR``).
   - The driver maps supported country codes to SiWx91x firmware region identifiers.
   - Supported regions include: ``EU``, ``CN``, ``KR``, ``JP``, and ``US``.
   - By default, the driver applies the ``US`` regulatory domain.

.. note::
   The regulatory domain must be configured **before** activating the device
   (scan, connect, or Soft-AP start). This ensures only allowed channels and
   transmit power levels are used.

   If the requested country code is not supported, the driver falls back to
   the default domain (``US``).

Maximum Transmit Power
----------------------

The driver supports configuration of the maximum transmit (TX) power in both
STA and Soft-AP modes. This setting defines the upper limit of RF output power,
subject to hardware and regulatory constraints.

   - Configuration is provided through Device Tree properties.
   - If not specified, the default maximum TX power is ``31 dBm``.

Device Tree Properties
~~~~~~~~~~~~~~~~~~~~~~

   - ``wifi-max-tx-pwr-scan``
     Maximum TX power (in dBm) used during Wi-Fi scanning. Applicable only in STA mode.

   - ``wifi-max-tx-pwr-join``
     Maximum TX power (in dBm) used when joining a Wi-Fi network.

.. note::
   The SiWx91x firmware does not support per-rate TX power control.

RTS Threshold
-------------

The driver supports configuration of the RTS (Request to Send) threshold,
which determines the frame size above which RTS/CTS handshaking is enabled.

   - Valid range: ``0`` to ``2347`` bytes.
   - A value of ``0`` means every transmitted frame is preceded by an RTS frame.
   - The default RTS threshold is ``2346`` bytes.
