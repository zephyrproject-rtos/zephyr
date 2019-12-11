.. _build_overview:

Build Overview
##############

The Zephyr build process can be divided into two main phases: a
configuration phase (driven by *CMake*) and a build phase (driven by
*Make* or *Ninja*). We will describe the build phase using *Make* as
example.


Configuration Phase
*******************

The configuration phase begins when the user invokes *CMake*,
specifying a source application directory and a board target.

.. figure:: build-config-phase.svg
    :align: center
    :alt: Zephyr's build configuration phase
    :figclass: align-center
    :width: 80%

*CMake* begins by processing the *CMakeLists.txt* file in the application
directory, which refers to the *CMakeLists.txt* file in the Zephyr
top-level directory, which in turn refers to *CMakeLists.txt* files
throughout the build tree (directly and indirectly). Its primary
output is a set of Makefiles  to drive the build process, but *CMake*
scripts do some build processing of their own:

Device tree
   Using *cpp*, device-tree specifications (*.dts/.dtsi* files) are
   collected from the target’s architecture, SoC, board, and
   application directories and compiled with *dtc*. Then the build
   tool (scripts/dts) convert this into *.h* files for later
   consumption.

Device tree fixup
   Files named *dts_fixup.h* from the target’s architecture, SoC,
   board, and application directories are concatenated into a single
   *dts_fixup.h*. Its purpose is to normalize constants output in the
   previous step so they have the names expected by the source files
   in the build phase.

Kconfig
   The build tool reads the *Kconfig* files for the target
   architecture, the target SoC, the target board, the target
   application, as well as *Kconfig* files associated with subsystems
   throughout the source tree. It incorporates the device tree outputs
   to allow configurations to make use of that data. It ensures the
   desired configuration is consistent, outputs *autoconf.h* for the
   build phase.

Build Phase
***********

The build phase begins when the user invokes *make*. Its ultimate
output is a complete Zephyr application in a format suitable for
loading/flashing on the desired target board (*zephyr.elf*,
*zephyr.hex*, etc.) The build phase can be broken down, conceptually,
into four stages: the pre-build, first-pass binary, final binary, and
post-processing.

Pre-build occurs before any source files are compiled, because during
this phase header files used by the source files are generated.

Pre-build
=========

Offset generation
   Access to high-level data structures and members is sometimes
   required when the definitions of those structures is not
   immediately accessible (e.g., assembly language). The generation of
   *offsets.h* (by *gen_offset_header.py*) facilitates this.

System call boilerplate
   The *gen_syscall_header.py*, *parse_syscalls.py* and
   *gen_syscall_header.py* scripts work together to bind potential
   system call functions with their implementations.

.. figure:: build-build-phase-1.svg
    :align: center
    :alt: Zephyr's build stage I
    :figclass: align-center
    :width: 80%

First-pass binary
=================

Compilation proper begins with the first-pass binary. Source files (C
and assembly) are collected from various subsystems (which ones is
decided during the configuration phase), and compiled into archives
(with reference to header files in the tree, as well as those
generated during the configuration phase and the pre-build stage).

If memory protection is enabled, then:

Partition grouping
   The gen_app_partitions.py script scans all the
   generated archives and outputs linker scripts to ensure that
   application partitions are properly grouped and aligned for the
   target’s memory protection hardware.

Then *cpp* is used to combine linker script fragments from the target’s
architecture/SoC, the kernel tree, optionally the partition output if
memory protection is enabled, and any other fragments selected during
the configuration process, into a *linker.cmd* file. The compiled
archives are then linked with *ld* as specified in the
*linker.cmd*.

In some configurations, this is the final binary, and the next stage
is skipped.

.. figure:: build-build-phase-2.svg
    :align: center
    :alt: Zephyr's build stage II
    :figclass: align-center
    :width: 80%

Final binary
============

In some configurations, the binary from the previous stage is
incomplete, with empty and/or placeholder sections that must be filled
in by, essentially, reflection. When :ref:`usermode` is enabled:

Kernel object hashing
   The *gen_kobject_list.py* scans the *ELF DWARF*
   debug data to find the address of the all kernel objects. This
   list is passed to *gperf*, which generates a perfect hash function and
   table of those addresses, then that output is optimized by
   *process_gperf.py*, using known properties of our special case.

Then, the link from the previous stage is repeated, this time with the
missing pieces populated.

.. figure:: build-build-phase-3.svg
    :align: center
    :alt: Zephyr's build stage III
    :figclass: align-center
    :width: 80%


Post processing
===============

Finally, if necessary, the completed kernel is converted from *ELF* to
the format expected by the loader and/or flash tool required by the
target. This is accomplished in a straightforward manner with *objdump*.

.. figure:: build-build-phase-4.svg
    :align: center
    :alt: Zephyr's build final stage
    :figclass: align-center
    :width: 80%
