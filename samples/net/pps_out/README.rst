.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

.. zephyr:code-sample:: pps-out
   :name: PTP PPS output
   :relevant-api: ptp precision_clock precision_pps_output

   Generate a PPS signal from a PTP-synchronized hardware clock.

Overview
********

This sample runs a time-receiver-only PTP ordinary clock using IEEE 802.3
Layer-2 transport and the peer-to-peer (P2P) delay mechanism. It adapts the
Ethernet PTP hardware clock (PHC) to the protocol-neutral precision timing API
and drives a one-pulse-per-second (PPS) hardware output aligned to PHC second
boundaries. This differs from the :zephyr:code-sample:`ptp` sample, whose
default configuration uses UDP/IPv4 transport and the end-to-end (E2E) delay
mechanism.

The sample selects the first Ethernet interface, matching the PTP clock
instance's PHC selection, rather than the default network interface. A loopback
or dummy default interface therefore does not prevent PHC discovery.

The PPS output is produced by the autonomous
:kconfig:option:`CONFIG_PRECISION_PPS_OUTPUT` service, which is built on the
generic scheduled clock output provider. That provider is protocol-neutral: it
exposes hardware-timed outputs in the owning clock's own time domain and knows
nothing about PTP. This sample supplies a PTP-backed precision clock, but the
PPS service would work the same way on any precision clock whose provider
advertises scheduled output; PTP merely adapts the Ethernet PHC to it.

A scheduled output drives a periodic *waveform*, which repeats a high pulse
every period starting at an absolute first rising edge. A PPS signal is a
waveform with a fixed one-second period. The high-pulse width follows one of two policies: the
provider's native default width, or an exact width supplied by the caller. This
sample uses the subsystem Kconfig defaults via
``PRECISION_PPS_OUTPUT_CONFIG_DEFAULTS``. The default is an exact 200-millisecond
active-high pulse. Unsupported exact requests fail without changing policy.

The service is instance-based. The sample owns a single
:c:struct:`precision_pps_output` object and a matching
:c:struct:`precision_pps_output_config`, both defined statically in
:zephyr_file:`samples/net/pps_out/src/main.c`. It initializes the instance with
:c:func:`precision_pps_output_init`, starts it with
:c:func:`precision_pps_output_start`, and observes it through the registered
callback and :c:func:`precision_pps_output_state_get`. The compiled-in
configuration selects channel 0, the preferred 200-millisecond pulse width, a
10-millisecond start guard beyond the provider's minimum lead time, a
100-millisecond hard-step threshold, and a 250-millisecond poll interval.
Additional instances could run independently on other clocks or channels; this
sample uses one.

The service polls the PHC and the output status on its own, on a workqueue
shared by all PPS instances and independent of the sample's application code.
It arms the output as soon as the PHC is available and does not gate the output
on PTP port state. Before PTP synchronizes, the output follows the PHC's
unsynchronized timescale. If PTP steps the PHC beyond the configured threshold,
the output becomes unconfigured or inactive, or its configuration changes, the
service stops and rearms the channel from a fresh PHC reading; a successful
rearm advances the reported ``rearm_count``.

The sample reports transitions through the callback and periodic state
snapshots. The logged ``service_active`` flag and the "service armed" messages
describe the *service's* view of the channel, which is distinct from the
physical signal. Output status separately reports whether the channel is
*configured* and, only when the provider can observe it, whether the output is
physically active. Successfully arming the channel confirms only that the
provider accepted the configuration; it does not prove PTP synchronization,
physical waveform presence, or edge accuracy. Confirming the pulse train
requires external measurement.

Requirements
************

The sample supports :zephyr:board:`nucleo_h563zi`. It routes the Ethernet MAC
FlexPPS output to ``PG8`` on ST Morpho connector pin 66. Connect the measurement
instrument and board with a common ground. The PTP time-transmitter and all
PTP-aware network devices must use Layer-2 transport and the P2P delay mechanism.

.. note::

   On STM32H562/H563/H573, the FlexPPS provider avoids the ES0565 pulse-train
   erratum by issuing single-pulse commands and rearming each second. It
   accepts only a one-second period, matching the PPS service, and reports a
   pulse-width limit of at most 500 milliseconds based on the configured rate
   envelope. This differs from ST's documented coarse-correction workaround
   and has been measured on STM32H563.

   Scheduled output is disabled on STM32H742/H743/H750/H753 because of ES0392
   until a workaround is validated on those devices. PTP fine timestamp
   correction remains enabled.

PTP Time-Transmitter Configuration
**********************************

For example, configure a Linux ``ptp4l`` time-transmitter with hardware
timestamping, Layer-2 transport, and P2P delay measurement:

.. code-block:: ini

   [global]
   time_stamping       hardware
   network_transport   L2
   delay_mechanism     P2P
   domainNumber        0

Start ``ptp4l`` on the PTP-capable Ethernet interface:

.. code-block:: console

   sudo ptp4l -i <interface> -f p2p-l2.cfg -m

Building and Running
********************

Build and flash the sample as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/net/pps_out
   :board: nucleo_h563zi/stm32h563xx
   :goals: build flash
   :compact:

After startup, the sample logs the service-armed configuration and periodic
service-state snapshots, and reports clock steps, inactive output, rearms, and
transient provider errors. These logs reflect the service's view of the
channel, not a measured signal. Use the network shell commands to inspect the
PTP instance and its ports:

.. code-block:: console

   net ptp
   net ptp 1

Use the PTP clock shell command to read the PHC that drives the output:

.. code-block:: console

   ptp_clock get <device>
