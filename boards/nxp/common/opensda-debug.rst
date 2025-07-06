:orphan:

.. nxp-opensda-probes

A debug probe is used for both flashing and debugging the board. This board has
an :ref:`opensda-onboard-debug-probe`. The default firmware present on this
probe is the :ref:`opensda-daplink-onboard-debug-probe`.

Based on the host tool installed, please use the following instructions
to setup your debug probe:

* :ref:`jlink-debug-host-tools`: `Using J-Link on NXP OpenSDA Boards`_
* :ref:`linkserver-debug-host-tools`: `Using DAPLink on NXP OpenSDA Boards`_
* :ref:`pyocd-debug-host-tools`: `Using DAPLink on NXP OpenSDA Boards`_

Using DAPLink on NXP OpenSDA Boards
-----------------------------------

1. If the debug firmware has been modified, follow the instructions provided at
   :ref:`opensda-daplink-onboard-debug-probe` to reprogram the default debug
   probe firmware on this board.
#. Ensure the SWD isolation jumpers are populated

Using J-Link on NXP OpenSDA Boards
----------------------------------

There are two options: the onboard debug circuit can be updated with Segger
J-Link firmware, or a :ref:`jlink-external-debug-probe` can be attached to the
board.

To update the onboard debug circuit, please do the following:

1. If the debug firmware has been modified, follow the instructions provided at
   :ref:`opensda-jlink-onboard-debug-probe` to reprogram the default debug
   probe firmware on this board.
#. Ensure the SWD isolation jumpers are removed.

To attach an external J-Link probe, ensure the SWD isolation jumpers are
removed, and connect the external probe to the JTAG/SWD header.
