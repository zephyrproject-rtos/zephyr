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

The stack can also operate as a statically configured time receiver on
networks that follow the IEEE 802.1AS automotive profile, where no Announce
messages are exchanged. See `Static timeReceiver operation`_ below.

Supported hardware
******************

Although the stack itself is hardware independent, Ethernet frame timestamping
support must be enabled in ethernet drivers.

Boards supported:

- :zephyr:board:`frdm_k64f`
- :zephyr:board:`nucleo_h743zi`
- :zephyr:board:`nucleo_h745zi_q`
- :zephyr:board:`nucleo_f767zi`
- :zephyr:board:`sam_e70_xplained`
- :zephyr:board:`native_sim` (only usable for simple testing, limited capabilities
  due to lack of hardware clock)
- :zephyr:board:`qemu_x86` (emulated, limited capabilities due to lack of hardware clock)

Enabling the stack
******************

The following configuration option must me enabled in :file:`prj.conf` file.

- :kconfig:option:`CONFIG_NET_GPTP`

Static timeReceiver operation
*****************************

Networks built after the IEEE 802.1AS automotive profile (AVnu "Automotive
Ethernet AVB Functional and Interoperability Specification") use static port
roles instead of the Best Master Clock Algorithm. A bridge on such a network
transmits Sync and Follow_Up messages but no Announce messages, and is not
required to answer Pdelay requests on its timeTransmitter ports. The default
stack cannot synchronize to such a bridge: without a received Announce a port
never reaches the time receiver ("slave") role.

Enabling :kconfig:option:`CONFIG_NET_GPTP_STATIC_TIME_RECEIVER` configures the
node as a statically configured time receiver: BMCA and all Announce handling
are bypassed, every port is pinned to the time receiver role, asCapable is forced so
synchronization does not depend on the Pdelay measurement, and the local clock
is disciplined from the received Sync and Follow_Up messages alone. The node
never becomes grandmaster and never transmits Sync or Announce messages, even
if :kconfig:option:`CONFIG_NET_GPTP_GM_CAPABLE` is set. This mirrors the
linuxptp ptp4l automotive time receiver configuration (BMCA "noop", clientOnly,
inhibit_announce, asCapable "true", ignore_source_id). The standards analog of
disabling the BMCA this way is the external port configuration of IEEE
802.1AS-2020 (clause 10.3.14, derived from IEEE 1588-2019 clause 17.6.2).

Application interfaces
**********************

The following Application Interfaces as defined in section 9 of the standard
are available:

- ``ClockSourceTime`` interface (:c:func:`gptp_clk_src_time_invoke`)
- ``ClockTargetPhaseDiscontinuity`` interface (:c:func:`gptp_register_phase_dis_cb`)
- ``ClockTargetEventCapture`` interface (:c:func:`gptp_event_capture`)

Testing
*******

The stack has been informally tested using the
`OpenAVnu gPTP <https://github.com/AVnu/gptp>`_ and
`Linux ptp4l <https://linuxptp.sourceforge.net/>`_ daemons.
The :zephyr:code-sample:`gPTP sample application <gptp>` from the Zephyr
source distribution can be used for testing.

.. _IEEE 802.1AS-2011 standard:
   https://standards.ieee.org/findstds/standard/802.1AS-2011.html

API Reference
*************

.. doxygengroup:: gptp
