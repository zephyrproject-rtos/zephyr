:orphan:

.. segger-ecc-systemview

Using Segger SystemView and RTT
-------------------------------

Note that when using SEGGER SystemView or RTT with this SOC, the RTT control
block address must be set manually within SystemView or the RTT Viewer. The
address provided to the tool should be the location of the ``_SEGGER_RTT``
symbol, which can be found using a debugger or by examining the ``zephyr.map``
file output by the linker.

The RTT control block address must be provided manually because this SOC
supports ECC RAM. If the SEGGER tooling searches the ECC RAM space for the
control block a fault will occur, provided that ECC is enabled and the RAM
segment being searched has not been initialized to a known value.
