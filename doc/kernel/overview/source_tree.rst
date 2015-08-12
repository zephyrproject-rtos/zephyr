.. _source_tree:

Source Tree Structure
#####################

The Zephyr source tree provides the following top-level directories,
each of which may have one or more additional levels of subdirectories
which are not described here.

:file:`arch`
    Architecture-specific nanokernel and platform code. Each supported
    architecture has its own subdirectory, which contains additional
    subdirectories for the following areas:

    * architecture-specific nanokernel source files
    * architecture-specific nanokernel include files for private APIs
    * platform-specific code
    * platform configuration files

:file:`doc`
    Zephyr documentation-related material and tools.

:file:`drivers`
    Device driver code.

:file:`include`
    Include files for all public APIs, except those defined under :file:`lib`.

:file:`kernel`
    Microkernel code, and architecture-independent nanokernel code.

:file:`lib`
    Library code, including the minimal standard C library.

:file:`misc`
    Miscellaneous code.

:file:`net`
    Networking code, including the Bluetooth stack and networking stacks.

:file:`samples`
    Sample applications for the microkernel, nanokernel, Bluetooth stack,
    and networking stacks.

:file:`scripts`
    Various programs and other files used to build and test Zephyr
    applications.
