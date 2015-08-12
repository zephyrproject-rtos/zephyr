.. _system_fundamentals:

System Fundamentals
###################

A Zephyr *application image* is an executable file that controls the operation
of a hardware system, or a simulated system running under QEMU.
Each application image contains both application code and a complete Zephyr
kernel, which are compiled into a single, fully linked executable.
Once an application image has been loaded onto a target system, it takes
control of the system, initializes it, and runs forever as the system's sole
application. Both application code and kernel code execute as privileged code
within a single shared address space.

A Zephyr *application* is a set of user-supplied files that the Zephyr
build system processes to generate an application image. The application
consists of application-specific code, a collection of kernel configuration
settings, and a Makefile. The application's kernel configuration settings
enable the build system to create a Zephyr kernel tailor-made to meet the
needs of the application and to make the best use of the system's resources.

The Zephyr kernel supports a variety of target systems, known as *platforms*,
each of which has its own set of hardware devices and capabilities.
One or more *platform configurations* are defined for a given platform,
each of which indicates how the devices that may be present on the platform
are to be used by the kernel. The platform and platform configuration concepts
make it possible to develop a single Zephyr application that can be used by
a set of related target systems, or even target systems based on different CPU
architectures.
