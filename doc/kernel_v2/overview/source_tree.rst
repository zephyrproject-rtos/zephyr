.. _source_tree_v2:

Source Tree Structure
#####################

Understanding the Zephyr source tree can be helpful in locating the code
associated with a particular Zephyr feature.

The Zephyr source tree provides the following top-level directories,
each of which may have one or more additional levels of subdirectories
which are not described here.

:file:`arch`
    Architecture-specific kernel and system-on-chip (SoC) code.
    Each supported architecture (for example, x86 and ARM)
    has its own subdirectory,
    which contains additional subdirectories for the following areas:

    * architecture-specific kernel source files
    * architecture-specific kernel include files for private APIs
    * SoC-specific code

:file:`boards`
    Board related code and configuration files.

:file:`doc`
    Zephyr documentation source files and tools.

:file:`drivers`
    Device driver code.

:file:`ext`
    Externally created code that has been integrated into Zephyr
    from other sources, such as hardware interface code supplied by
    manufacturers and cryptographic library code.

:file:`fs`
    File system code.

:file:`include`
    Include files for all public APIs, except those defined under :file:`lib`.

:file:`kernel`
    Architecture-independent kernel code.

:file:`lib`
    Library code, including the minimal standard C library.

:file:`misc`
    Miscellaneous code that doesn't belong to any of the other top-level
    directories.

:file:`net`
    Networking code, including the Bluetooth stack and networking stacks.

:file:`samples`
    Sample applications that demonstrate the use of Zephyr features.

:file:`scripts`
    Various programs and other files used to build and test Zephyr
    applications.

:file:`tests`
    Test code and benchmarks for Zephyr features.

:file:`usb`
    USB device stack code.
