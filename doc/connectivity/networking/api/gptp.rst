.. _gptp_interface:

generic Precision Time Protocol (gPTP)
######################################

.. contents::
    :local:
    :depth: 2

Overview
********

This gPTP stack supports the protocol and procedures as defined in
the `IEEE 802.1AS-2011 standard`_ (Timing and Synchronization for
Time-Sensitive Applications in Bridged Local Area Networks).

Supported features
*******************

The stack handles communications and state machines defined in the
`IEEE 802.1AS-2011 standard`_. Mandatory requirements for a full-duplex
point-to-point link endpoint, as defined in Annex A of the standard,
are supported.

The stack is in principle capable of handling communications on multiple network
interfaces (also defined as "ports" in the standard) and thus act as
a 802.1AS bridge. However, this mode of operation has not been validated on
the Zephyr OS.

Supported hardware
******************

Although the stack itself is hardware independent, Ethernet frame timestamping
support must be enabled in ethernet drivers.

Boards supported:

- :ref:`frdm_k64f`
- :ref:`nucleo_h743zi_board`
- :ref:`nucleo_h745zi_q_board`
- :ref:`nucleo_f767zi_board`
- :ref:`sam_e70_xplained`
- :ref:`native_posix` (only usable for simple testing, limited capabilities
  due to lack of hardware clock)
- :ref:`qemu_x86` (emulated, limited capabilities due to lack of hardware clock)

Enabling the stack
******************

The following configuration option must me enabled in :file:`prj.conf` file.

- :kconfig:option:`CONFIG_NET_GPTP`

Application interfaces
**********************

Only two Application Interfaces as defined in section 9 of the standard
are available:

- ``ClockTargetPhaseDiscontinuity`` interface (:c:func:`gptp_register_phase_dis_cb`)
- ``ClockTargetEventCapture`` interface  (:c:func:`gptp_event_capture`)

Testing
*******

The stack has been informally tested using the
`OpenAVnu gPTP <https://github.com/AVnu/gptp>`_ and
`Linux ptp4l <http://linuxptp.sourceforge.net/>`_ daemons.
The :ref:`gPTP sample application <gptp-sample>` from the Zephyr
source distribution can be used for testing.

.. _IEEE 802.1AS-2011 standard:
   https://standards.ieee.org/findstds/standard/802.1AS-2011.html

API Reference
*************

.. doxygengroup:: gptp
