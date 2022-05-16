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

   board
      A target system with a defined set of devices and capabilities,
      which can load and execute an application image. It may be an actual
      hardware system or a simulated system running under QEMU.
      The Zephyr kernel supports a :ref:`variety of boards <boards>`.

   board configuration
      A set of kernel configuration options that specify how the devices
      present on a board are used by the kernel.
      The Zephyr build system defines one or more board configurations
      for each board it supports. The kernel configuration settings that are
      specified by the build system can be over-ridden by the application,
      if desired.

   device runtime power management
      Device Runtime Power Management (PM) refers the capability of devices to
      save energy independently of the the system power state. Devices will keep
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
      `System on a chip`_

   system power state
      System power states describe the power consumption of the system as a
      whole. System power states are are represented by :c:enum:`pm_state`.

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
