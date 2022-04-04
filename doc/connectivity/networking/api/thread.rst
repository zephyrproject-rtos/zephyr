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
See :ref:`sockets-echo-server-sample` and :ref:`sockets-echo-client-sample` for
details.
