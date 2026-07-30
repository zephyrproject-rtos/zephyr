.. _introducing_zephyr:

Introduction
############

Zephyr is a small, scalable, open source real-time operating system (RTOS)
designed for resource-constrained devices, from simple environmental sensors
and wearables to sophisticated industrial controllers, medical devices, and
smart home products. Alongside its kernel, Zephyr provides all the OS services,
protocol stacks, device drivers, and development tools needed to take an
embedded product from prototype to production.

Zephyr is a `Linux Foundation`_ project, developed in the open by a worldwide
community of individual contributors and member companies. It is permissively
licensed and vendor neutral: applications are portable across the wide range of
:ref:`supported boards <boards>`, which span many CPU
:ref:`architectures <arch>`.

Zephyr at a Glance
******************

Zephyr is much more than a kernel: applications build on top of a rich set of
OS services and a consistent device driver model, and are configured, built,
and tested using a complete, cross-platform development environment. Click any
block to learn more.

.. only:: html

   .. grid::
      :gutter: 3
      :margin: 0
      :class-container: arch-diagram

      .. grid-item::
         :columns: 12 12 8 8

         .. grid:: 1
            :gutter: 2
            :margin: 0

            .. grid-item-card:: Application
               :class-card: arch-block arch-layer-app
               :link: application
               :link-type: ref
               :link-alt: Application Development
               :text-align: center
               :shadow: none

               Your code and configuration, portable across all supported
               hardware

            .. grid-item-card:: OS Services & Subsystems
               :class-card: arch-block arch-layer-services
               :link: os_services
               :link-type: ref
               :link-alt: OS Services
               :text-align: center
               :shadow: none

               Networking, Bluetooth, file systems, power management, logging,
               and much more

            .. grid-item-card:: Kernel
               :class-card: arch-block arch-layer-kernel
               :link: kernel
               :link-type: ref
               :link-alt: Kernel Services
               :text-align: center
               :shadow: none

               Threads, scheduling, interrupts, synchronization, memory
               management

            .. grid-item-card:: Device Drivers & Hardware Abstraction
               :class-card: arch-block arch-layer-drivers
               :link: device_model_api
               :link-type: ref
               :link-alt: Device Driver Model
               :text-align: center
               :shadow: none

               Consistent driver APIs and device model across vendors and SoCs

            .. grid-item-card:: Hardware
               :class-card: arch-block arch-layer-hw
               :link: boards
               :link-type: ref
               :link-alt: Supported Boards
               :text-align: center
               :shadow: none

               Boards and SoCs from many vendors, across all supported CPU
               architectures

      .. grid-item::
         :columns: 12 12 4 4

         .. rst-class:: arch-tools-title

         Developer Tools

         .. grid:: 1
            :gutter: 2
            :margin: 0

            .. grid-item-card:: West
               :class-card: arch-block arch-tool
               :link: west
               :link-type: ref
               :link-alt: West meta-tool
               :text-align: center
               :shadow: none

               One command-line tool to manage, build, flash, and debug

            .. grid-item-card:: Build & Configuration
               :class-card: arch-block arch-tool
               :link: build_overview
               :link-type: ref
               :link-alt: Build and Configuration Systems
               :text-align: center
               :shadow: none

               CMake, Kconfig, and devicetree

            .. grid-item-card:: Zephyr SDK
               :class-card: arch-block arch-tool
               :link: toolchain_zephyr_sdk
               :link-type: ref
               :link-alt: Zephyr SDK
               :text-align: center
               :shadow: none

               Toolchains for every supported architecture

            .. grid-item-card:: Twister
               :class-card: arch-block arch-tool
               :link: twister_script
               :link-type: ref
               :link-alt: Twister test runner
               :text-align: center
               :shadow: none

               Run test suites on real or simulated hardware

            .. grid-item-card:: Simulation & Emulation
               :class-card: arch-block arch-tool
               :link: /boards/native/native_sim/doc/index
               :link-type: doc
               :link-alt: Native simulator board
               :text-align: center
               :shadow: none

               Develop and test without hardware using native_sim, QEMU, and
               more

.. only:: latex

   Applications build on top of a rich set of :ref:`OS services and subsystems
   <os_services>`, the :ref:`kernel <kernel>`, and a consistent :ref:`device
   driver model <device_model_api>` that abstracts the underlying hardware. A
   complete development environment surrounds this stack: the :ref:`west
   <west>` meta-tool, the CMake/Kconfig/devicetree-based :ref:`build and
   configuration systems <build_overview>`, the :ref:`Zephyr SDK
   <toolchain_zephyr_sdk>` toolchains, and the :ref:`Twister <twister_script>`
   test runner.

Distinguishing Features
***********************

Zephyr offers a large and ever growing number of features, including:

**A small, fast, real-time kernel**
   * Cooperative and preemptive :ref:`threading <threads_v2>`, with a choice
     of :ref:`scheduling algorithms <scheduling_v2>` including earliest
     deadline first (EDF)
   * A complete set of :ref:`synchronization and data passing <kernel_api>`
     primitives
   * :ref:`Interrupt handling <interrupts_v2>`, :ref:`memory management
     <memory_management_api>`, and :ref:`SMP <smp_arch>` support
   * Optional :ref:`user mode <usermode_api>` with thread isolation and
     memory protection

.. _zephyr_intro_configurability:

**Highly configurable and modular**
   * Applications incorporate *only* the capabilities they need, with system
     resources defined at build time for a minimal footprint
   * Fine-grained build-time configuration with :ref:`Kconfig <kconfig>`
   * Additional libraries, HALs, and entire projects integrate as
     :ref:`modules <modules>`; code can even be loaded at runtime using
     :ref:`linkable extensions <llext>`

**Broad hardware support**
   * Multiple CPU :ref:`architectures <arch>`, including Arm Cortex-M/R/A,
     RISC-V, x86, and Xtensa
   * Hardware described declaratively with :ref:`devicetree <dt-guide>`
   * A consistent :ref:`device driver model <device_model_api>` and generic
     :ref:`peripheral APIs <api_peripherals>` that keep applications portable
     across the wide range of :ref:`supported boards <boards>`

**Connectivity and rich OS services**
   * A native :ref:`networking stack <networking>` with BSD sockets support
     and a broad catalog of protocols
   * A qualification-ready :ref:`Bluetooth LE <bluetooth>` host and
     controller, including Bluetooth Mesh
   * :ref:`USB <usb>` device and host stacks, and more :ref:`connectivity
     options <connectivity>`
   * Dozens of other :term:`subsystems <subsystem>`: :ref:`storage and file
     systems <storage_services>`, persistent :ref:`settings <settings_api>`,
     multi-backend :ref:`logging <logging_api>`, an interactive :ref:`shell
     <shell_api>`, :ref:`power management <pm-guide>`, :ref:`device management
     and firmware updates <device_mgmt>`, and the rest of the :ref:`OS
     services <os_services>`
   * Standard interfaces available through :ref:`POSIX support
     <posix_support>` and other :ref:`portability layers <osal>`

**Develop and test from anywhere**
   * First-class development experience on Linux, macOS, and Windows
   * Run and debug applications as native processes using
     :zephyr:board:`native_sim <native_sim>` or emulators, before hardware is
     even available
   * Automated :ref:`testing <testing>` at scale with the :ref:`Twister
     <twister_script>` test runner
   * Extensive :ref:`debugging <debugging>` and :ref:`tracing <tracing>`
     options

**Ready for production**
   * Dedicated :ref:`security processes <security-overview>` and vulnerability
     management
   * An ongoing functional :ref:`safety effort <safety_overview>`
   * :ref:`Long-term support (LTS) releases <release_process_lts>` and a
     well-defined :ref:`API lifecycle <api_lifecycle>`
   * An open :ref:`governance and development model <development_model>`

Licensing
*********

Zephyr is permissively licensed using the `Apache 2.0 license`_ (as found in
the ``LICENSE`` file in the project's `GitHub repo`_). There are some imported
or reused components of the Zephyr project that use other licensing, as
described in :ref:`Zephyr_Licensing`.

Where to Go Next
****************

Ready to dive deeper? Pick your path below. If you come across unfamiliar
terms along the way, the :ref:`glossary` has you covered.

.. only:: html

   .. grid:: 1 2 3 3
      :gutter: 3
      :margin: 0
      :class-container: intro-next-grid

      .. grid-item-card:: Getting Started
         :class-card: sd-index-card intro-next-card
         :link: getting_started
         :link-type: ref
         :link-alt: Getting Started Guide

         .. rst-class:: sd-index-watermark

         :material-twotone:`rocket_launch;5em`

         Set up your development environment and build your first application.

      .. grid-item-card:: Samples & Demos
         :class-card: sd-index-card intro-next-card
         :link: /samples/index
         :link-type: doc
         :link-alt: Samples and Demos

         .. rst-class:: sd-index-watermark

         :material-twotone:`play_circle;5em`

         Explore ready-to-run samples, from basic I/O to complete connected
         applications.

      .. grid-item-card:: Supported Boards
         :class-card: sd-index-card intro-next-card
         :link: boards
         :link-type: ref
         :link-alt: Supported Boards

         .. rst-class:: sd-index-watermark

         :material-twotone:`developer_board;5em`

         Find your board, or the right one for your next project, in the
         board catalog.

      .. grid-item-card:: Application Development
         :class-card: sd-index-card intro-next-card
         :link: application
         :link-type: ref
         :link-alt: Application Development

         .. rst-class:: sd-index-watermark

         :material-twotone:`code;5em`

         Learn how Zephyr applications are structured, configured, and built.

      .. grid-item-card:: Contributing
         :class-card: sd-index-card intro-next-card
         :link: contribute_to_zephyr
         :link-type: ref
         :link-alt: Contributing to Zephyr

         .. rst-class:: sd-index-watermark

         :material-twotone:`volunteer_activism;5em`

         Zephyr is built by its community: report issues, contribute code,
         improve documentation.

      .. grid-item-card:: Releases & LTS
         :class-card: sd-index-card intro-next-card
         :link: release_process
         :link-type: ref
         :link-alt: Release Process

         .. rst-class:: sd-index-watermark

         :material-twotone:`fact_check;5em`

         Learn about Zephyr's release cadence and long-term support (LTS)
         releases.

.. only:: latex

   * :ref:`Getting Started Guide <getting_started>`
   * :zephyr:code-sample-category:`samples`
   * :ref:`Supported Boards <boards>`
   * :ref:`Application Development <application>`
   * :ref:`Contributing to Zephyr <contribute_to_zephyr>`
   * :ref:`Release Process <release_process>`

.. _project-resources:

Community & Resources
*********************

Zephyr is developed in the open by a worldwide community. These are the best
places to get help, follow the project, and get involved:

**Ask questions and get help**
   Join the `Discord server`_ for real-time discussions with the community, or
   post to the `user mailing list`_. Before you ask, have a look at the
   :ref:`tips on asking for help <help>`.

**Follow and join development**
   Development happens in the open on `GitHub`_, where you can also report
   bugs and request features using `GitHub Issues`_. Longer-form design
   discussions happen on the `developer mailing list`_ (see the `full list of
   mailing lists`_), and additional resources are available on the
   `project wiki`_.

**Security**
   Learn about Zephyr's :ref:`approach to security <security-overview>`,
   browse published `security advisories`_, and report vulnerabilities to
   vulnerabilities@zephyrproject.org.

**Around the project**
   Visit the `Zephyr Project website`_ to learn more about the project, its
   members, and products built with Zephyr, and watch `Zephyr Tech Talks`_ for
   technical deep dives presented by maintainers and community members.

.. _Linux Foundation: https://www.linuxfoundation.org
.. _Apache 2.0 license:
   https://github.com/zephyrproject-rtos/zephyr/blob/main/LICENSE
.. _GitHub repo: https://github.com/zephyrproject-rtos/zephyr
.. _Discord server: https://chat.zephyrproject.org
.. _user mailing list: https://lists.zephyrproject.org/g/users
.. _developer mailing list: https://lists.zephyrproject.org/g/devel
.. _full list of mailing lists: https://lists.zephyrproject.org/g/main/subgroups
.. _GitHub: https://github.com/zephyrproject-rtos/zephyr
.. _GitHub Issues: https://github.com/zephyrproject-rtos/zephyr/issues
.. _project wiki: https://github.com/zephyrproject-rtos/zephyr/wiki
.. _security advisories: https://github.com/zephyrproject-rtos/zephyr/security
.. _Zephyr Project website: https://www.zephyrproject.org
.. _Zephyr Tech Talks: https://www.zephyrproject.org/tech-talks
