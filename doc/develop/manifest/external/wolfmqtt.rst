.. _external_module_wolfmqtt:

wolfMQTT
########

Introduction
************

wolfMQTT is a lightweight, portable MQTT client library optimized for
embedded systems, RTOS environments, and resource-constrained devices.
It provides a client implementation of the MQTT protocol with support
for MQTT v3.1.1 and v5.0. The library offers features such as QoS
levels 0-2, Last Will and Testament (LWT), and compatibility with
various MQTT brokers. Its support for multiple build configurations
makes it suitable for a wide range of IoT applications and hardware
platforms that are utilizing the Zephyr RTOS.

wolfMQTT has support for the Zephyr networking stack so applications
can use the wolfMQTT API to establish MQTT connections with brokers and
other devices or services over the network.

wolfMQTT is dual licensed under GPLv3 and commercial licenses.

GitHub Repository: `wolfMQTT Repository`_

Requirements
************

* :ref:`external_module_wolfssl` for secure communication (TLS support)

Usage with Zephyr
*****************

Add wolfMQTT as a project to your west.yml:

.. code-block:: yaml

  manifest:
    remotes:
    # <your other remotes>
    - name: wolfmqtt
      url-base: https://github.com/wolfssl
  projects:
    # <your other projects>
    - name: wolfmqtt
      path: modules/lib/wolfmqtt
      revision: v1.21.0
      remote: wolfmqtt

.. note::

   The revision shown above is an example. Check the `wolfMQTT Repository`_
   releases page for the latest release tag to ensure you are using the desired
   version.

Update west's modules:

.. code-block:: bash

   west update

Now west recognizes ``wolfmqtt`` as a module, and will include its
Kconfig and CMakeLists.txt in the build system.

For more regarding the usage of wolfMQTT with Zephyr, please refer to
the `wolfMQTT Zephyr Example Usage`_.

For application code examples in Zephyr, please refer to the
`wolfSSL NXP AppCodeHub`_.

For wolfMQTT API documentation, please refer to the `wolfMQTT Documentation`_.

Reference
*********

.. target-notes::

.. _wolfMQTT Repository:
    https://github.com/wolfSSL/wolfMQTT

.. _wolfMQTT Zephyr Example Usage:
    https://github.com/wolfSSL/wolfMQTT/tree/master/zephyr

.. _wolfSSL NXP AppCodeHub:
    https://github.com/wolfSSL/nxp-appcodehub

.. _wolfMQTT Documentation:
    https://www.wolfssl.com/documentation/manuals/wolfmqtt/
