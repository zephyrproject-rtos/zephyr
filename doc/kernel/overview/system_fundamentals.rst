.. _system_fundamentals:

System Fundamentals
###################

An :dfn:`application image` is a binary that controls the operation
of a hardware system, or of a simulated system running under QEMU.
Each application image contains both the application's code and the Zephyr
kernel code needed to support it. They are compiled as a single,
fully-linked binary.

Once an application image has been loaded onto a target system, the image takes control
of the system, initializes it, and runs forever as the system's sole application.
Both application code and kernel code execute as privileged code within a single
shared address space.

A :dfn:`application` is a set of user-supplied files that the Zephyr build
system processes to generate an application image. The application consists of
application-specific code, a collection of kernel configuration settings, and at
least one Makefile. The application's kernel configuration settings enable the build
system to create a kernel tailor-made to meet the needs of the application
and to make the best use of the system's resources.

The |codename| supports a variety of target systems, known as *platforms*;
each :dfn:`platform` has its own set of hardware devices and capabilities. One or more
*platform configurations* are defined for a given platform; each
:dfn:`platform configuration` indicates how the devices that may be present on the
platform are to be used by the kernel. The platform and platform configuration concepts
make it possible to develop a single application that can be used by a set of related
target systems, or even target systems based on different CPU architectures.