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

Memfault and nRF Cloud powered by Memfault
******************************************

`Memfault`_ is an IoT observability platform that includes OTA management.
Devices check in with Memfault's service periodically for an OTA update, and
when an update is available, download and install the binary.

Zephyr projects that use MCUboot and have a direct Internet connection can
leverage the `Memfault Firmware SDK's <Memfault Firmware SDK_>`_ OTA client to
download a payload from Memfault's OTA service, load it into the secondary
partition, and then reboot into the new image.
See `Memfault OTA for Zephyr documentation`_ for more details on this support
for Zephyr projects.

For Nordic Semiconductor cellular chip users, the Memfault
Firmware SDK includes support for downloading the payload over a cellular
connection using nRF Connect SDK's FOTA and downloader libraries.
See the `Memfault Quickstart for the nRF91 Series`_ for more
information. Zephyr projects with a Bluetooth Low Energy connection can
leverage the `Memfault Diagnostic Service`_ (MDS) with the `Memfault iOS SDK`_
and `Memfault Android SDK`_ to deliver the update payload to the device. For
Nordic Semiconductor's Bluetooth Low Energy chip users, the Zephyr-based
nRF Connect SDK provides an implementation of MDS. See the
`nRF Connect SDK documentation on MDS`_ for more information as well as
`nRF Cloud powered by Memfault`_ for detail on using the Memfault platform with
Nordic Semiconductor Bluetooth Low Energy chips.
See :ref:`external_module_memfault_firmware_sdk` for overall integration
details and examples.

.. _Memfault: https://memfault.com/
.. _Memfault Firmware SDK: https://github.com/memfault/memfault-firmware-sdk
.. _Memfault OTA for Zephyr documentation: https://docs.memfault.com/docs/mcu/zephyr-guide#ota
.. _Memfault Quickstart for the nRF91 Series: https://docs.memfault.com/docs/mcu/quickstart-nrf9160
.. _Memfault Diagnostic Service: https://docs.memfault.com/docs/mcu/mds
.. _Memfault iOS SDK: https://github.com/memfault/memfault-cloud-ios
.. _Memfault Android SDK: https://github.com/memfault/memfault-cloud-android
.. _nRF Connect SDK documentation on MDS: https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/libraries/bluetooth/services/mds.html
.. _nRF Cloud powered by Memfault: https://nrfcloud.com/#/
