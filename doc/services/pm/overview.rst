Overview
########

The interfaces and APIs provided by the power management subsystem
are designed to be architecture and SoC independent. This enables power
management implementations to be easily adapted to different SoCs and
architectures.

The architecture and SoC independence is achieved by separating the core PM
infrastructure from implementations of the SoC specific components.
Thus a coherent abstraction is presented to the rest of the OS and the application layer.

The power management features are classified into the following categories:

* :ref:`System Power Management <pm-system>`: manages the power state of the
  CPU or SoC as a whole, entering a low-power state when the system is idle.
* :doc:`Device Power Management <device>`: manages the power state of
  individual devices, either at runtime based on device usage or together
  with the system power state.
