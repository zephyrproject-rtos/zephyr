.. _lora_api:
.. _lorawan_api:

LoRa and LoRaWAN
################

Overview
********

LoRa (abbrev. for Long Range) is a proprietary low-power wireless
communication protocol developed by the `Semtech Corporation`_.

LoRa acts as the physical layer (PHY) based on the chirp spread spectrum
(CSS) modulation technique.

LoRaWAN (for Long Range Wide Area Network) defines a networking layer
on top of the LoRa PHY.

Zephyr provides APIs for LoRa to send raw data packets directly over the
wireless interface as well as APIs for LoRaWAN to connect the end device
to the internet through a gateway.

The Zephyr implementation is based on Semtech's `LoRaMac-node library`_, which
is included as a Zephyr module.

.. note::

        ``LoRaMac-node`` has been deprecated by Semtech in favor of
        `LoRa Basics Modem`_. Porting the Zephyr API's to use
        ``LoRa Basics Modem`` as the backend is in progress.

        Currently, only the base LoRa API is supported for the SX1261, SX1262,
        SX1272 and SX1276 chipsets through
        :kconfig:option:`CONFIG_LORA_MODULE_BACKEND_LORA_BASICS_MODEM`.


The LoRaWAN specification is published by the `LoRa Alliance`_.

.. _`Semtech Corporation`: https://www.semtech.com/

.. _`LoRaMac-node library`: https://github.com/Lora-net/LoRaMac-node

.. _`LoRa Basics Modem`: https://github.com/Lora-net/SWL2001

.. _`LoRa Alliance`: https://lora-alliance.org/

Configuration Options
*********************

LoRa PHY
========

Related configuration options can be found under
:zephyr_file:`drivers/lora/Kconfig`.

* :kconfig:option:`CONFIG_LORA`

* :kconfig:option:`CONFIG_LORA_SHELL`

* :kconfig:option:`CONFIG_LORA_INIT_PRIORITY`

LoRaWAN
=======

Related configuration options can be found under
:zephyr_file:`subsys/lorawan/Kconfig`.

* :kconfig:option:`CONFIG_LORAWAN`

* :kconfig:option:`CONFIG_LORAWAN_SYSTEM_MAX_RX_ERROR`

* :kconfig:option:`CONFIG_LORAMAC_REGION_AS923`

* :kconfig:option:`CONFIG_LORAMAC_REGION_AU915`

* :kconfig:option:`CONFIG_LORAMAC_REGION_CN470`

* :kconfig:option:`CONFIG_LORAMAC_REGION_CN779`

* :kconfig:option:`CONFIG_LORAMAC_REGION_EU433`

* :kconfig:option:`CONFIG_LORAMAC_REGION_EU868`

* :kconfig:option:`CONFIG_LORAMAC_REGION_KR920`

* :kconfig:option:`CONFIG_LORAMAC_REGION_IN865`

* :kconfig:option:`CONFIG_LORAMAC_REGION_US915`

* :kconfig:option:`CONFIG_LORAMAC_REGION_RU864`

API Reference
*************

LoRa PHY
========

.. doxygengroup:: lora_interface

LoRaWAN
=======

.. doxygengroup:: lorawan_api
