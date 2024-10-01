.. _ota:

Over-the-Air Update
###################

Overview
********

Over-the-Air (OTA) Update is a method for delivering firmware updates to remote
devices using a network connection. Although the name implies a wireless
connection, updates received over a wired connection (such as Ethernet)
are still commonly referred to as OTA updates. This approach requires server
infrastructure to host the firmware binary and implement a method of signaling
when an update is available. Security is a concern with OTA updates; firmware
binaries should be cryptographically signed and verified before upgrading.

The :ref:`dfu` section discusses upgrading Zephyr firmware using MCUboot. The
same method can be used as part of OTA. The binary is first downloaded
into an unoccupied code partition, usually named ``slot1_partition``, then
upgraded using the :ref:`mcuboot` process.

Examples of OTA
***************

Golioth
=======

`Golioth`_ is an IoT management platform that includes OTA updates. Devices are
configured to observe your available firmware revisions on the Golioth Cloud.
When a new version is available, the device downloads and flashes the binary. In
this implementation, the connection between cloud and device is secured using
TLS/DTLS, and the signed firmware binary is confirmed by MCUboot before the
upgrade occurs.

1. A working sample can be found on the `Golioth Firmware SDK repository`_
2. The `Golioth OTA documentation`_ includes complete information about the
   versioning process

Eclipse hawkBit |trade|
=======================

`Eclipse hawkBit`_ |trade| is an update server framework that uses polling on a
REST api to detect firmware updates. When a new update is detected, the binary
is downloaded and installed. MCUboot can be used to verify the signature before
upgrading the firmware.

There is a :zephyr:code-sample:`hawkbit-api` sample included in the
Zephyr :zephyr:code-sample-category:`mgmt` section.

UpdateHub
=========

`UpdateHub`_ is a platform for remotely updating embedded devices. Updates can
be manually triggered or monitored via polling. When a new update is detected,
the binary is downloaded and installed. MCUboot can be used to verify the
signature before upgrading the firmware.

There is an :zephyr:code-sample:`updatehub-fota` sample included in the Zephyr
:zephyr:code-sample-category:`mgmt` section.

SMP Server
==========

A Simple Management Protocol (SMP) server can be used to update firmware via
Bluetooth Low Energy (BLE) or UDP. :ref:`mcu_mgr` is used to send a signed
firmware binary to the remote device where it is verified by MCUboot before the
upgrade occurs.

There is an :zephyr:code-sample:`smp-svr` sample included in the Zephyr
:zephyr:code-sample-category:`mgmt` section.

Lightweight M2M (LWM2M)
=======================

The :ref:`lwm2m_interface` protocol includes support for firmware update via
:kconfig:option:`CONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT`. Devices securely
connect to an LwM2M server using DTLS. A :zephyr:code-sample:`lwm2m-client` sample is
available but it does not demonstrate the firmware update feature.

.. _MCUboot bootloader: https://mcuboot.com/
.. _Golioth: https://golioth.io/
.. _Golioth Firmware SDK repository: https://github.com/golioth/golioth-firmware-sdk/tree/main/examples/zephyr/fw_update
.. _Golioth OTA documentation: https://docs.golioth.io/device-management/ota
.. _Eclipse hawkBit: https://www.eclipse.org/hawkbit/
.. _UpdateHub: https://updatehub.io/
