Overview
########

The interfaces and APIs provided by the power management subsystem
are designed to be architecture and SOC independent. This enables power
management implementations to be easily adapted to different SOCs and
architectures.

The architecture and SOC independence is achieved by separating the core
infrastructure and the SOC specific implementations. The SOC specific
implementations are abstracted to the application and the OS using hardware
abstraction layers.

The power management features are classified into the following categories.

* System Power Management
* Device Power Management
