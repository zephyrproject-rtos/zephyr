.. _modem:

Modem modules
#############

This service provides modules necessary to communicate with modems.

Modems are self-contained devices that implement the hardware and
software necessary to perform RF (Radio-Frequency) communication,
including GNSS, Cellular, WiFi etc.

The modem modules are inter-connected dynamically using
data-in/data-out pipes making them independently testable and
highly flexible, ensuring stability and scalability.

Modem pipe
**********

This module is used to abstract data-in/data-out communication over
a variety of mechanisms, like UART and CMUX DLCI channels, in a
thread-safe manner.

A modem backend will internally contain an instance of a modem_pipe
structure, alongside any buffers and additional structures required
to abstract away its underlying mechanism.

The modem backend will return a pointer to its internal modem_pipe
structure when initialized, which will be used to interact with the
backend through the modem pipe API.

.. doxygengroup:: modem_pipe

Modem PPP
*********

This module defines and binds a L2 PPP network interface, described in
:ref:`net_l2_interface`, to a modem backend. The L2 PPP interface sends
and receives network packets. These network packets have to be wrapped
in PPP frames before being transported via a modem backend. This module
performs said wrapping.

.. doxygengroup:: modem_ppp

Modem CMUX
**********

This module is an implementation of CMUX following the 3GPP 27.010
specification. CMUX is a multiplexing protocol, allowing for multiple
bi-directional streams of data, called DLCI channels. The module
attaches to a single modem backend, exposing multiple modem backends,
each representing a DLCI channel.

.. doxygengroup:: modem_cmux
