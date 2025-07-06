:orphan:

.. rt1xxx-lpclink2-probes

A debug probe is used for both flashing and debugging the board. This board has
an :ref:`lpc-link2-onboard-debug-probe`. The default firmware present on this
probe is the :ref:`lpclink2-daplink-onboard-debug-probe`.

Based on the host tool installed, please use the following instructions
to setup your debug probe:

* :ref:`jlink-debug-host-tools`:
  `Using J-Link with LPC-Link2 Probe`_
* :ref:`linkserver-debug-host-tools`:
  `Using CMSIS-DAP with LPC-Link2 Probe`_
* :ref:`pyocd-debug-host-tools`:
  `Using CMSIS-DAP with LPC-Link2 Probe`_

Using CMSIS-DAP with LPC-Link2 Probe
------------------------------------

1. Follow the instructions provided at
   :ref:`lpclink2-cmsis-onboard-debug-probe` to reprogram the default debug
   probe firmware on this board.
#. Ensure the SWD isolation jumpers are populated

Using J-Link with LPC-Link2 Probe
---------------------------------

There are two options: the onboard debug circuit can be updated with Segger
J-Link firmware, or a :ref:`jlink-external-debug-probe` can be attached to the
EVK.

To update the onboard debug circuit, please do the following:

1. Switch the power source for the EVK to a different source than the
   debug USB, as the J-Link firmware does not power the EVK via the
   debug USB.
#. Follow the instructions provided at
   :ref:`lpclink2-jlink-onboard-debug-probe` to reprogram the default debug
   probe firmware on this board.
#. Ensure the SWD isolation jumpers are populated.

To attach an external J-Link probe, ensure the SWD isolation jumpers are
removed, then connect the probe to the external JTAG/SWD header
