.. _ppp:

Point-to-Point Protocol (PPP) Support
#####################################

.. contents::
    :local:
    :depth: 2

Overview
********

`Point-to-Point Protocol
<https://en.wikipedia.org/wiki/Point-to-Point_Protocol>`_ (PPP) is a data link
layer (layer 2) communications protocol used to establish a direct connection
between two nodes. PPP is used over many types of serial links since IP packets
cannot be transmitted over a modem line on their own, without some data link
protocol.

In Zephyr, each individual PPP link is modelled as a network interface. This
is similar to how Linux implements PPP.

PPP support must be enabled at compile time by setting option
:option:`CONFIG_NET_PPP` and :option:`CONFIG_NET_L2_PPP`.

See also the :zephyr_file:`samples/net/sockets/echo_server/overlay-ppp.conf`
file for configuration option examples.
