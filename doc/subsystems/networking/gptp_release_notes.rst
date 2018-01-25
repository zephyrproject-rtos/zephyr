gPTP stack for Zephyr
#####################

Overview
********

This gPTP stack supports the protocol and procedures as defined in
the IEEE 802.1AS-2011 standard (Timing and Syncronization for
Time-Sensitive Applications in Bridged Local Area Networks).

Supported features
*******************

The stack handles communications and state machines defined in the standard
IEEE802.1AS-2011. Mandatory requirements, for a full-duplex point-to-point link
endpoint, as defined in Annex A of the standard are supported.

The stack is in principle capable of handling communications on multiple network
interfaces (also defined as "ports" in the standard) and thus act as
a 802.1AS bridge. However, this mode of operation has not been validated.

Supported hardware
******************

Although the stack itself is hardware independent, ethernet frame timestamping
support must be enabled in ethernet drivers.

Boards supported:

- NXP FRDM-K64F
- QEMU (emulated, limited capabilities due to lack of hardware clock)

Enabling the stack
******************

In menuconfig, the following configuration must me enabled:

- CONFIG_NET_GPTP (Networking -> Link layer and IP networking support -> IP stack -> Link layer options -> Enable IEEE802.1AS support)

Application interfaces
**********************

Only two Application Interfaces as defined in section 9 of the standard
are available:

- ClockTargetPhaseDiscontinuity interface (gptp_event_capture)
- ClockTargetEventCapture interface (gptp_register_phase_dis_cb)

Function prototypes can be found in "include/net/gptp.h".

Testing
*******

The stack has been informally tested using the OpenAVB gPTP and
Linux ptp4l daemons.
