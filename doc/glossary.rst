:orphan:

.. _glossary:

Glossary of Terms
#################

.. glossary::
   :sorted:

   API
      (Application Program Interface) A defined set of routines and protocols for
      building application software.

   application
      The set of user-supplied files that the Zephyr build system uses
      to build an application image for a specified board configuration.
      It can contain application-specific code, kernel configuration settings,
      and at least one CMakeLists.txt file.
      The application's kernel configuration settings direct the build system
      to create a custom kernel that makes efficient use of the board's
      resources.
      An application can sometimes be built for more than one type of board
      configuration (including boards with different CPU architectures),
      if it does not require any board-specific capabilities.

   application image
      A binary file that is loaded and executed by the board for which
      it was built.
      Each application image contains both the application's code and the
      Zephyr kernel code needed to support it. They are compiled as a single,
      fully-linked binary.
      Once an application image is loaded onto a board, the image takes control
      of the system, initializes it, and runs as the system's sole application.
      Both application code and kernel code execute as privileged code
      within a single shared address space.

   architecture
      An instruction set architecture (ISA) along with a programming model.

   board
      A target system with a defined set of devices and capabilities,
      which can load and execute an application image. It may be an actual
      hardware system or a simulated system running under QEMU. A board can
      contain one or more :term:`SoCs <SoC>`.
      The Zephyr kernel supports a :ref:`variety of boards <boards>`.

   board configuration
      A set of kernel configuration options that specify how the devices
      present on a board are used by the kernel.
      The Zephyr build system defines one or more board configurations
      for each board it supports. The kernel configuration settings that are
      specified by the build system can be over-ridden by the application,
      if desired.

   board name
      The human-readable name of a :term:`board`. Uniquely and descriptively
      identifies a particular system, but does not include additional
      information that may be required to actually build a Zephyr image for it.
      See :ref:`board_terminology` for additional details.

   board qualifiers
      The set of additional tokens, separated by a forward slash (`/`) that
      follow the :term:`board name` (and optionally :term:`board revision`) to
      form the :term:`board target`. The currently accepted qualifiers are
      :term:`SoC`, :term:`CPU cluster` and :term:`variant`.
      See :ref:`board_terminology` for additional details.

   board revision
      An optional version string that identifies a particular revision of a
      hardware system. This is useful to avoid duplication of board files
      whenever small changes are introduced to a hardware system.
      See :ref:`porting_board_revisions` and :ref:`application_board_version`
      for more information.

   board target
     The full string that can be provided to any of the Zephyr build tools to
     compile and link an image for a particular hardware system. This string
     uniquely identifies the combination of :term:`board name`, :term:`board
     revision` and :term:`board qualifiers`.
     See :ref:`board_terminology` for additional details.

   CPU cluster
     A group of one or more :term:`CPU cores <CPU core>`, all executing the same image
     within the same address space and in a symmetrical (SMP) configuration.
     Only :term:`CPU cores <CPU core>` of the same :term:`architecture` can be in a single
     cluster. Multiple CPU clusters (each of one or more cores) can coexist in
     the same :term:`SoC`.

   CPU core
     A single processing unit, with its own Program Counter, executing program
     instructions sequentially. CPU cores are part of a :term:`CPU cluster`,
     which can contain one or more cores.

   device runtime power management
      Device Runtime Power Management (PM) refers the capability of devices to
      save energy independently of the system power state. Devices will keep
      reference of their usage and will automatically be suspended or resumed.
      This feature is enabled via the :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME`
      Kconfig option.

   idle thread
      A system thread that runs when there are no other threads ready to run.

   IDT
      (Interrupt Descriptor Table) a data structure used by the x86
      architecture to implement an interrupt vector table. The IDT is used
      to determine the correct response to interrupts and exceptions.

   ISR
      (Interrupt Service Routine) Also known as an interrupt handler, an ISR
      is a callback function whose execution is triggered by a hardware
      interrupt (or software interrupt instructions) and is used to handle
      high-priority conditions that require interrupting the current code
      executing on the processor.

   kernel
      The set of Zephyr-supplied files that implement the Zephyr kernel,
      including its core services, device drivers, network stack, and so on.

   power domain
      A power domain is a collection of devices for which power is
      applied and removed collectively in a single action. Power
      domains are represented by :c:struct:`device`.

   power gating
      Power gating reduces power consumption by shutting off areas of an
      integrated circuit that are not in use.

   SoC
      A `System on a chip`_, that is, an integrated circuit that contains at
      least one :term:`CPU cluster` (in turn with at least one :term:`CPU core`),
      as well as peripherals and memory.

   SoC family
      One or more :term:`SoCs <SoC>` or :term:`SoC series` that share enough
      in common to consider them related and under a single family denomination.

   SoC series
      A number of different :term:`SoCs <SoC>` that share similar characteristics and
      features, and that the vendor typically names and markets together.

   system power state
      System power states describe the power consumption of the system as a
      whole. System power states are represented by :c:enum:`pm_state`.

   variant
      In the context of :term:`board qualifiers`, a variant designates a
      particular type or configuration of a build for a combination of :term:`SoC`
      and :term:`CPU cluster`. Common uses of the variant concept include
      introducing both secure and non-secure builds for platforms with Trusted
      Execution Environment support, or selecting the type of RAM used in a
      build.

   west
      A multi-repo meta-tool developed for the Zephyr project. See :ref:`west`.

   west installation
      An obsolete term for a :term:`west workspace` used prior to west 0.7.

   west manifest
      A YAML file, usually named :file:`west.yml`, which describes projects, or
      the Git repositories which make up a :term:`west workspace`, along with
      additional metadata. See :ref:`west-basics` for general information
      and :ref:`west-manifests` for details.

   west manifest repository
      The Git repository in a :term:`west workspace` which contains the
      :term:`west manifest`. Its location is given by the :ref:`manifest.path
      configuration option <west-config-index>`. See :ref:`west-basics`.

   west workspace
      A directory on your system with a :file:`.west` subdirectory and
      a :term:`west manifest repository`. You clone the Zephyr source
      code onto your system by creating a west workspace using the
      ``west init`` command. See :ref:`west-basics`.

   XIP
      (eXecute In Place) a method of executing programs directly from long
      term storage rather than copying it into RAM, saving writable memory for
      dynamic data and not the static program code.

.. _System on a chip: https://en.wikipedia.org/wiki/System_on_a_chip
