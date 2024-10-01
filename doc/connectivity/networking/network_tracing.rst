.. _network_tracing:

Network Tracing
###############

.. contents::
    :local:
    :depth: 2

User can enable network core stack and socket API calls tracing.

The :kconfig:option:`CONFIG_TRACING_NET_CORE` option contols the core network
stack tracing. This option is enabled by default if tracing and networking
are enabled. The system will start to collect the receiving and sending call
verdicts i.e., whether the network packet was successfully sent or received.
It will also collect packet sending or receiving timings i.e., how long
it took to deliver the network packet, and the network interface, priority
and traffic class used.

The :kconfig:option:`CONFIG_TRACING_NET_SOCKETS` option can be used to track
BSD socket call usage in the system. It is enabled if tracing and BSD socket
API support are enabled. The system will start to collect what BSD socket
API calls are made and what parameters the API calls are using and returning.

See the :ref:`tracing documentation <tracing>` for how to use the tracing
service.
