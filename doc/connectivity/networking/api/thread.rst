.. _thread_protocol_interface:

Thread protocol
###############

.. contents::
    :local:
    :depth: 2

Overview
********
Thread is a low-power mesh networking technology, designed specifically for home
automation applications. It is an IPv6-based standard, which uses 6LoWPAN
technology over IEEE 802.15.4 protocol. IP connectivity lets you easily connect
a Thread mesh network to the internet with a Thread Border Router.

The Thread specification provides a high level of network security. Mesh networks
built with Thread are secure - only authenticated devices can join the network
and all communications within the mesh are encrypted. More information about
Thread protocol can be found at
`Thread Group website <https://www.threadgroup.org>`_.

Zephyr integrates an open source Thread protocol implementation called OpenThread,
documented on the `OpenThread website <https://openthread.io/>`_.

Internet connectivity
*********************

A Thread Border Router is required to connect mesh network to the internet.
An open source implementation of Thread Border Router is provided by the OpenThread
community. See
`OpenThread Border Router guide <https://openthread.io/guides/border-router>`_
for instructions on how to set up a Border Router.

Sample usage
************

You can try using OpenThread with the Zephyr Echo server and Echo client samples,
which provide out-of-the-box configuration for OpenThread. To enable OpenThread
support in these samples, build them with ``overlay-ot.conf`` overlay config file.
See :zephyr:code-sample:`sockets-echo-server` and :zephyr:code-sample:`sockets-echo-client`
samples for details.

Zephyr also provides an :zephyr:code-sample:`openthread-shell`, which is useful for
testing and debugging Thread and its underlying IEEE 802.15.4 drivers.

Thread related APIs
*******************

OpenThread Driver API
========================

OpenThread L2 uses Zephyr's protocol agnostic IEEE 802.15.4 driver API
internally. This API is of interest to **driver developers** that want to
support OpenThread.

The driver API is part of the :ref:`ieee802154_driver_api` subsystem and
documented there.

OpenThread L2 Adaptation Layer API
==================================

Zephyr's OpenThread L2 platform adaptation layer glues the external OpenThread
stack together with Zephyr's IEEE 802.15.4 protocol agnostic driver API. This
API is of interest to OpenThread L2 **subsystem contributors** only.

OpenThread Platform API
=======================

The OpenThread platform API is defined by the OpenThread stack and implemented in Zephyr as an
OpenThread module. Applications can use this implementation directly, or access it through the
OpenThread L2 adaptation layer.

Using the OpenThread L2 Adaptation Layer API
--------------------------------------------

To use the OpenThread platform API via the OpenThread L2 adaptation layer, enable both the
:kconfig:option:`CONFIG_NET_L2_OPENTHREAD` and :kconfig:option:`CONFIG_NETWORKING` Kconfig options
by setting them to ``y``. The adaptation layer will use the OpenThread radio API implementation
found in :file:`modules/openthread/platform/radio.c`. In this setup, the OpenThread stack is
initialized and managed by the adaptation layer.

Using the OpenThread Platform API Directly
------------------------------------------

You can also use the OpenThread platform API directly, bypassing the OpenThread L2 adaptation
layer. However, this approach requires you to provide your own implementation of the OpenThread
radio API that is compatible with your specific radio driver.

To use the OpenThread platform API directly, set the :kconfig:option:`CONFIG_OPENTHREAD` Kconfig
option to ``y``, and do **not** set :kconfig:option:`CONFIG_NET_L2_OPENTHREAD`. In this case, you
must implement the following functions from the `OpenThread radio API
<https://openthread.io/reference/group/radio-config>`_ using your own radio driver:

* ``otPlatRadioGetPromiscuous``
* ``otPlatRadioGetCcaEnergyDetectThreshold``
* ``otPlatRadioGetTransmitPower``
* ``otPlatRadioGetIeeeEui64``
* ``otPlatRadioSetPromiscuous``
* ``otPlatRadioGetCaps``
* ``otPlatRadioGetTransmitBuffer``
* ``otPlatRadioSetPanId``
* ``otPlatRadioEnable``
* ``otPlatRadioDisable``
* ``otPlatRadioReceive``
* ``otPlatRadioGetRssi``
* ``otPlatRadioGetReceiveSensitivity``
* ``otPlatRadioEnergyScan``
* ``otPlatRadioSetExtendedAddress``
* ``otPlatRadioSetShortAddress``
* ``otPlatRadioAddSrcMatchExtEntry``
* ``otPlatRadioTransmit``
* ``otPlatRadioClearSrcMatchShortEntries``
* ``otPlatRadioClearSrcMatchExtEntries``
* ``otPlatRadioEnableSrcMatch``
* ``otPlatRadioAddSrcMatchShortEntry``
* ``otPlatRadioClearSrcMatchShortEntry``
* ``otPlatRadioClearSrcMatchExtEntry``

Additionally, you must implement the following functions from the OpenThread radio API (see
:zephyr_file:`include/zephyr/net/openthread.h`) to handle radio initialization and event processing:

* :c:func:`platformRadioInit`
* :c:func:`platformRadioProcess`

To initialize the OpenThread stack in this approach, either call the :c:func:`ot_platform_init`
function in your application, or enable the :kconfig:option:`CONFIG_OPENTHREAD_SYS_INIT` Kconfig
option to automatically initialize OpenThread during system startup. You can set the
initialization priority using the :kconfig:option:`CONFIG_OPENTHREAD_SYS_INIT_PRIORITY` Kconfig
option.

.. doxygengroup:: openthread
