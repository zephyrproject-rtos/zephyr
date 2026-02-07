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

Zephyr supports two LoRaWAN stack implementations:

* **LoRaMac-node** (default): Semtech's `LoRaMac-node library`_, included as a
  Zephyr module. This backend supports all LoRa radio chipsets.

* **LoRa Basics Modem**: Semtech's newer `LoRa Basics Modem`_ library. This
  backend currently supports sx126x radio chipsets (SX1261, SX1262, SX1268,
  LLCC68) only.

The backend can be selected using Kconfig:

* :kconfig:option:`CONFIG_LORA_MODULE_BACKEND_LORAMAC_NODE` - LoRaMac-node (default)
* :kconfig:option:`CONFIG_LORA_MODULE_BACKEND_LORA_BASICS_MODEM` - LoRa Basics Modem

.. note::

        ``LoRaMac-node`` has been deprecated by Semtech in favor of
        ``LoRa Basics Modem``. New applications targeting sx126x radios should
        consider using the LoRa Basics Modem backend.


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

* :kconfig:option:`CONFIG_LORA_MODULE_BACKEND_LORAMAC_NODE`

* :kconfig:option:`CONFIG_LORA_MODULE_BACKEND_LORA_BASICS_MODEM`

* :kconfig:option:`CONFIG_LORAWAN_SYSTEM_MAX_RX_ERROR`

* :kconfig:option:`CONFIG_LORAWAN_REGION_AS923`

* :kconfig:option:`CONFIG_LORAWAN_REGION_AU915`

* :kconfig:option:`CONFIG_LORAWAN_REGION_CN470`

* :kconfig:option:`CONFIG_LORAWAN_REGION_CN779` (LoRaMac-node only)

* :kconfig:option:`CONFIG_LORAWAN_REGION_EU433` (LoRaMac-node only)

* :kconfig:option:`CONFIG_LORAWAN_REGION_EU868`

* :kconfig:option:`CONFIG_LORAWAN_REGION_KR920`

* :kconfig:option:`CONFIG_LORAWAN_REGION_IN865`

* :kconfig:option:`CONFIG_LORAWAN_REGION_US915`

* :kconfig:option:`CONFIG_LORAWAN_REGION_RU864`

API Reference
*************

LoRa PHY
========

.. doxygengroup:: lora_interface

LoRaWAN
=======

.. doxygengroup:: lorawan_api
