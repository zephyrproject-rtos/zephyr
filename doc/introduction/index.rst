.. _introducing_zephyr:

Introduction
############

Zephyr is an open source real-time operating system (RTOS) for resource-constrained and embedded
devices, from simple sensors and wearables to smart watches and wireless gateways.

The diagram below shows its main components. It is a simplified view for illustration purposes and
does not reflect the exact architecture of the system. Each block links to the corresponding
section of the documentation.

.. The diagram below is plain reStructuredText: edit the labels and the link targets here. Each
   block starts with its title, a link that covers the whole block, followed by the items it
   contains, which can be links too. The layout is defined in doc/_static/css/stack-diagram.css,
   where every "zstack-<name>" class is assigned an area of the grid.

.. container:: zstack

   .. container:: zstack-layers

      .. container:: zstack-block zstack-application

         :ref:`Application <application>`

         :ref:`C <language_c>` · :ref:`C++ <language_cpp>`

      .. container:: zstack-block zstack-networking

         :ref:`Networking <networking>`

         | :ref:`IPv4/IPv6 <ip_stack_overview>` · :ref:`TLS <secure_sockets_interface>`
         | :ref:`MQTT <mqtt_socket_interface>` · :ref:`CoAP <coap_sock_interface>` · …

      .. container:: zstack-block zstack-storage

         :ref:`Storage <storage_services>`

         | :ref:`File systems <file_system_api>`
         | :ref:`Settings <settings_api>` · …

      .. container:: zstack-block zstack-device-mgmt

         :ref:`Device Mgmt <device_mgmt>`

         | :ref:`MCUboot <mcuboot>`
         | :ref:`MCUmgr <mcu_mgr>` · :ref:`OTA <ota>`

      .. container:: zstack-block zstack-frameworks

         :ref:`Frameworks <frameworks>`

         | :ref:`Sensing <sensing>` · :ref:`LLEXT <llext>`
         | :ref:`mpipe <mpipe>` · …

      .. container:: zstack-block zstack-osal

         :ref:`OS Abstraction <osal>`

         | :ref:`POSIX <posix_support>`
         | :ref:`CMSIS-RTOS <cmsis_rtos_v2>`

      .. container:: zstack-block zstack-connectivity

         :ref:`Connectivity <connectivity>`

         | :ref:`Bluetooth <bluetooth>` · :ref:`Wi-Fi <wifi_mgmt>`
         | :ref:`USB <usb>` · :ref:`CAN <canbus>` · …

      .. container:: zstack-block zstack-power

         :ref:`Power <power_management_services>`

         | :ref:`System <pm-system>` · :doc:`Device </services/pm/device>`
         | :ref:`CPU frequency <cpu_freq>`

      .. container:: zstack-block zstack-diagnostics

         :ref:`Diagnostics <observability_services>`

         | :ref:`Logging <logging_api>` · :ref:`Shell <shell_api>`
         | :ref:`Tracing <tracing>` · …

      .. container:: zstack-block zstack-ipc

         :ref:`IPC <ipc_services>`

         | :ref:`zbus <zbus>`
         | :ref:`Multicore IPC <ipc_service>` · …

      .. container:: zstack-block zstack-security

         :ref:`Security <security_services>`

         | :ref:`PSA Crypto <psa_crypto>` · :ref:`TF-M <tfm>`
         | :ref:`Secure storage <secure_storage>` · …

      .. container:: zstack-split zstack-split-os

         .. container:: zstack-block zstack-kernel

            :ref:`Kernel <kernel>`

            | :ref:`Multithreading <threads_v2>` · :ref:`Preemptive scheduling <scheduling_v2>`
            | :ref:`SMP <smp_arch>` · :ref:`Tickless <kernel_timing>` ·
              :ref:`Memory protection <memory_domain>`
            | :ref:`User mode <usermode_api>` · :ref:`Message queues <message_queues_v2>` · …

         .. container:: zstack-block zstack-drivers

            :ref:`Device Drivers <api_peripherals>`

            | :ref:`Analog <analog_peripherals>` · :ref:`Audio <audio_reference>` ·
              :ref:`Communication <communication_peripherals>` ·
              :ref:`Display <display_peripherals>`
            | :ref:`GPIO & Input <gpio_input_peripherals>` ·
              :ref:`Memory & Storage <memory_storage_peripherals>` ·
              :ref:`Power <power_peripherals>` · :ref:`Sensors <sensor_peripherals>`
            | :ref:`Motion & Actuation <motion_actuation_peripherals>` ·
              :ref:`System & Diagnostics <system_diagnostics_peripherals>` ·
              :ref:`Timing <timing_peripherals>`

         .. raw:: html

            <svg class="zstack-cut" viewBox="0 0 100 100" preserveAspectRatio="none"
                 aria-hidden="true" focusable="false">
              <polygon points="0,0 46.2,0 51.1,100 0,100"/>
              <polygon points="47.5,0 100,0 100,100 52.4,100"/>
            </svg>

      .. container:: zstack-split zstack-split-hardware

         .. container:: zstack-block zstack-architectures

            :ref:`Architectures <arch>`

            Arm · :doc:`RISC-V </hardware/arch/risc-v>` ·
            :ref:`TriCore <tricore_developer_guide>` · :ref:`x86 <x86_developer_guide>` ·
            :ref:`Xtensa <xtensa_developer_guide>` · …

         .. container:: zstack-block zstack-boards

            :zephyr:board-count:`100` :ref:`Boards <boards>`

            MCUs · SoCs · FPGAs

         .. raw:: html

            <svg class="zstack-cut" viewBox="0 0 100 100" preserveAspectRatio="none"
                 aria-hidden="true" focusable="false">
              <polygon points="0,0 46.2,0 48.9,100 0,100"/>
              <polygon points="47.5,0 100,0 100,100 50.2,100"/>
            </svg>

      .. container:: zstack-block zstack-modules

         .. container:: zstack-modules-libraries

            :ref:`Modules <modules>`

            | Graphics
            | Radio stacks
            | Crypto
            | File systems
            | Bootloader
            | …

         .. container:: zstack-modules-hals

            | SoC HALs
            | Sensor HALs
            | …

   .. container:: zstack-tooling

      .. container:: zstack-block zstack-tools

         :ref:`Tools <developing_with_zephyr>`

         :ref:`west <west>` · :ref:`CMake <cmake-details>` ·
         :ref:`Zephyr SDK <toolchain_zephyr_sdk>` · :ref:`SBOM <west-spdx>` ·
         :ref:`Hardening tool <hardening>`

      .. container:: zstack-block zstack-configuration

         :ref:`Configuration <build_overview>`

         :ref:`Devicetree <dt-guide>` · :ref:`Kconfig <kconfig>`

      .. container:: zstack-block zstack-testing

         :ref:`Testing <testing>`

         :ref:`Twister <twister_script>` · :zephyr:board:`native_sim <native_sim>` ·
         :ref:`QEMU <application_run_qemu>`

To start using Zephyr, follow the :ref:`getting_started`, then browse the
:ref:`supported boards <boards>` and the :zephyr:code-sample-category:`samples <samples>`.

Components
**********

Kernel
======

The :ref:`Zephyr kernel <kernel>` provides :ref:`threads <threads_v2>`,
:ref:`interrupt handling <interrupts_v2>`, synchronization primitives such as semaphores and
mutexes, data passing primitives such as message queues and pipes, timers, and
:ref:`memory allocation <memory_management_api>` from heaps and fixed-size slabs.

:ref:`Scheduling <scheduling_v2>` is priority-based, with cooperative and preemptible threads,
optional time slicing, earliest-deadline-first (EDF) scheduling and meta-IRQ priorities. The kernel
supports :ref:`symmetric multiprocessing <smp_arch>`.

On hardware that provides memory protection, stack overflows are detected and threads can run in
:ref:`user mode <usermode_api>` at a reduced privilege level, with
:ref:`permissions on kernel objects <kernelobjects>` and :ref:`memory domains <memory_domain>` to
isolate them.

Device Drivers
==============

Each type of device (UART, SPI, I2C, sensor, display, ...) has a generic API, listed by category in
:ref:`api_peripherals`, so that application code does not depend on a specific device. The
:ref:`device model <device_model_api>` initializes the drivers that are part of a system. The
devices themselves are described in :ref:`devicetree <dt-guide>`.

Services
========

Services are provided by :term:`subsystems <subsystem>` that an application enables as needed:

* :ref:`Networking <networking>`: IPv4 and IPv6 stack with a BSD sockets API, TLS, application
  protocols such as MQTT, CoAP, LwM2M and HTTP, and Thread networking with OpenThread.
* :ref:`Storage <storage_services>`: file systems (ext2, FAT, LittleFS), key-value stores (NVS,
  ZMS), flash circular buffer (FCB), settings, secure storage.
* :ref:`Device management <device_mgmt>`: device firmware upgrade with MCUboot, remote management
  with MCUmgr, over-the-air updates.
* :ref:`Frameworks <frameworks>`: sensing subsystem, loadable extensions (LLEXT), multimedia
  pipelines, state machines.
* :ref:`OS abstraction <osal>`: POSIX and CMSIS-RTOS APIs, to port existing code to Zephyr.
* :ref:`Connectivity <connectivity>`: Bluetooth host and LE controller, Bluetooth Mesh, USB, CAN,
  LoRa and LoRaWAN, Modbus, modems.
* :ref:`Power management <power_management_services>`: system and device power management, CPU
  frequency scaling.
* :ref:`Diagnostics <observability_services>`: logging, shell, tracing, debugging and profiling.
* :ref:`Inter-process communication <ipc_services>`: zbus message bus between threads, IPC
  service between cores.
* :ref:`Security <security_services>`: PSA Crypto API, integration of Trusted Firmware-M,
  authentication.

The complete list is in :ref:`os_services`.

Modules
=======

Zephyr reuses externally maintained projects, called :ref:`modules <modules>`: hardware abstraction
layers (HALs) from silicon vendors, and libraries such as cryptography, file systems, graphics or
radio stacks. Each module has its own repository and is fetched by west. The modules of the default
manifest are listed in :ref:`west_projects_index`.

Supported Hardware
******************

Zephyr supports :zephyr:board-count:`100` boards, which can be searched in the
:ref:`board catalog <boards>`, on the following architectures (see :ref:`arch`):

* ARCv2 (EM and HS) and ARCv3 (HS6X)
* ARMv6-M, ARMv7-M, and ARMv8-M (Cortex-M)
* ARMv7-A and ARMv8-A (Cortex-A, 32- and 64-bit)
* ARMv7-R, ARMv8-R (Cortex-R, 32- and 64-bit)
* Qualcomm Hexagon
* Intel x86 (32- and 64-bit)
* MIPS (MIPS32 Release 1 specification)
* OpenRISC (32-bit)
* Renesas RX
* RISC-V (32- and 64-bit)
* SPARC V8
* Tensilica Xtensa
* TriCore (TC1.6.2 and TC1.8)

The :ref:`porting guides <porting>` describe how to add support for an architecture, an SoC or a
board.

Application Development
***********************

An application is written in C or C++ (see :ref:`language_support`) and uses the APIs of the
kernel, device drivers and services described above. The build system compiles the application and
Zephyr into a single binary. See :ref:`application`.

.. _zephyr_intro_configurability:

Configuration
=============

Hardware is described with :ref:`devicetree <dt-guide>` and software features are selected with
:ref:`Kconfig <kconfig>`. Both are processed at build time, so that an image only contains what
the application has enabled.

Tools
=====

:ref:`West <west>` manages the workspace and its repositories, and builds, flashes and debugs
applications. The :ref:`build system <build_overview>` is based on CMake. The
:ref:`Zephyr SDK <toolchain_zephyr_sdk>` contains toolchains for the supported architectures.
Development is supported on Linux, macOS and Windows.

West can also generate a :ref:`software bill of materials <west-spdx>` for a build, and the
:ref:`hardening tool <hardening>` checks the configuration of an application against security
recommendations.

Testing
=======

Tests are written with the :ref:`Ztest framework <test-framework>`. :ref:`Twister <twister_script>`
builds and runs them, as well as the samples, on boards and on emulators such as QEMU. The
:zephyr:board:`native_sim` board runs Zephyr as a Linux executable, to develop and test without
hardware. See :ref:`testing`.

Licensing
*********

Zephyr is licensed under the Apache 2.0 license, as found in the :zephyr_file:`LICENSE` file.
Imported components that use another license are listed in :ref:`zephyr_licensing`.

.. _project-resources:

Resources
*********

Getting started
   :ref:`getting_started`, :zephyr:code-sample-category:`samples` and :ref:`glossary`.

Getting help
   :ref:`help` describes how to reach the community on the `Discord server`_ and the
   `mailing lists`_.

Code and releases
   `Source code repository`_, :ref:`releases <zephyr_release_notes>` and
   :ref:`contribution guide <contribute_to_zephyr>`.

Issues and security
   `GitHub issues`_, :ref:`security documentation <security_section>` and
   :ref:`vulnerability reporting <reporting>`.

.. _Discord server: https://chat.zephyrproject.org
.. _mailing lists: https://lists.zephyrproject.org/g/main/subgroups
.. _Source code repository: https://github.com/zephyrproject-rtos/zephyr
.. _GitHub issues: https://github.com/zephyrproject-rtos/zephyr/issues
