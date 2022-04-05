.. _tcpc_api:

TCPC
####

Overview
********

`TCPC <tcpc-specification>`_ (USB Type-C Port Controller)
The TCPC is a device used to simplify the implementation of a USB-C system
by providing the following three function:

* VBUS and VCONN control `USB Type-C <usb-type-c-specification>`_:
  The TCPC may provide a Source device, the mechanism to control VBUS sourcing,
  and a Sink device, the mechanism to control VBUS sinking. A similar mechanism
  is provided for the control of VCONN.

* CC control and sensing:
  The TCPC implements logic for controlling the CC pin pull-up and pull-down
  resistors. It also provides a way to sense and report what resistors are
  present on the CC pin.

* Power Delivery message reception and transmission `USB Power Delivery <usb-pd-specification>`_:
  The TCPC sends and receives messages constructed in the TCPM and places them
  on the CC lines.

.. _tcpc-api:

TCPC API
========

The TCPC device driver functions as the liaison between the TCPC device and the
application software; this is accomplished by the Zephyr's API provided by the
device driver that's used to  communicate with and control the TCPC device.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_USBC_TCPC_DRIVER`

API Reference
*************

.. doxygengroup:: usb_type_c
.. doxygengroup:: usb_type_c_port_controller_api
.. doxygengroup:: usb_power_delivery

.. _tcpc-specification:
   https://www.usb.org/document-library/usb-type-cr-port-controller-interface-specification

.. _usb-type-c-specification:
   https://www.usb.org/document-library/usb-type-cr-cable-and-connector-specification-revision-21

.. _usb-pd-specification:
   https://www.usb.org/document-library/usb-power-delivery
