.. _cbor_api:

CBOR
####

`CBOR <https://cbor.io/>`_ (Concise Binary Object Representation) is a data format whose design
goals include the possibility of extremely small code size, fairly small message size, and
extensibility without the need for version negotiation.

Zephyr provides support for CBOR via the `zcbor`_ library, which is pulled in as a West module.

Configuration
*************

To enable CBOR support, enable the :kconfig:option:`CONFIG_ZCBOR` Kconfig option.

API Reference
*************

The zcbor library provides its own API documentation, please refer to it for more information.

.. _`zcbor`: https://github.com/zephyrproject-rtos/zcbor
