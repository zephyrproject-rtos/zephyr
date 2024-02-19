.. _native_sim:

Native simulator - native_sim
#############################

.. contents::
   :depth: 1
   :backlinks: entry
   :local:

Overview
********

The ``native_sim`` board is a :ref:`POSIX architecture<Posix arch>` based board.
With it, a Zephyr application can be compiled together with
the Zephyr kernel, and libraries, creating a normal Linux executable.

``native_sim`` is based on the
`native simulator <https://github.com/BabbleSim/native_simulator/>`_
and the :ref:`POSIX architecture<Posix arch>`.

This board does not intend to simulate any particular HW, but it provides
a few peripherals such as an Ethernet driver, display, UART, etc., to enable
developing and testing application code which would require them.
See `Peripherals`_ for more information.

.. note::

   | ``native_sim`` is an evolution of the older :ref:`native_posix<native_posix>`.
   | Some components, code, options names, and documentation will still use the old native_posix
     names. But all components which worked with native_posix will work with native_sim.

Host system dependencies
************************

Please check the
:ref:`Posix Arch Dependencies<posix_arch_deps>`

.. _nativesim_important_limitations:

Important limitations and unsupported features
**********************************************

``native_sim`` is based on the :ref:`POSIX architecture<Posix arch>`, and therefore
:ref:`its limitations <posix_arch_limitations>` and considerations apply to it.

Similarly, it inherits the POSIX architecture
:ref:`unsupported features set <posix_arch_unsupported>`.

Note that some drivers may have limitations, or may not support their whole driver API optional
functionality.

.. _native_sim_how_to_use:

How to use it
*************

Compiling
=========

To build, simply specify the ``native_sim`` board as target:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: native_sim
   :goals: build
   :compact:

Running
=======

The result of the compilation is an executable (``zephyr.exe``) placed in the
``zephyr/`` subdirectory of the ``build`` folder.
Run the ``zephyr.exe`` executable as you would any other Linux console application.

.. code-block:: console

   $ ./build/zephyr/zephyr.exe
   # Press Ctrl+C to exit

This executable accepts several command line options depending on the
compilation configuration.
You can run it with the ``--help`` command line switch to get a list of
available options.

.. code-block:: console

   $ ./build/zephyr/zephyr.exe --help

Note that the Zephyr kernel does not actually exit once the application is
finished. It simply goes into the idle loop forever.
Therefore you must stop the application manually (Ctrl+C in Linux).

Application tests using the :ref:`ztest framework<test-framework>` will exit after all
tests have completed.

If you want your application to gracefully finish when it reaches some point,
you may add a conditionally compiled (:kconfig:option:`CONFIG_ARCH_POSIX`) call to
``nsi_exit(int status)`` at that point.

.. _native_sim_debug:

Debugging
=========

Since the Zephyr executable is a native application, it can be debugged and
instrumented as any other native program. The program is compiled with debug
information, so it can be run directly in, for example, ``gdb`` or instrumented
with ``valgrind``.

Because the execution of your Zephyr application is normally deterministic
(there are no asynchronous or random components), you can execute the
code multiple times and get the exact same result. Instrumenting the
code does not affect its execution.

To ease debugging you may want to compile your code without optimizations
(e.g., ``-O0``) by setting :kconfig:option:`CONFIG_NO_OPTIMIZATIONS`.

For ease of debugging consider using an IDE as GUI for your debugger.

.. _native_sim_asan:

Address Sanitizer (ASan)
========================

You can also build Zephyr with the `Address Sanitizer`_. To do this, set
:kconfig:option:`CONFIG_ASAN`, for example, in the application project file, or in the
``west build`` or ``cmake`` command line invocation.

Note that you will need the ASan library installed in your system.
In Debian/Ubuntu this is ``libasan1``.

.. _Address Sanitizer:
   https://github.com/google/sanitizers/wiki/AddressSanitizer

Undefined Behavior Sanitizer (UBSan)
====================================

You can also build Zephyr with the `Undefined Behavior Sanitizer`_. To do this, set
:kconfig:option:`CONFIG_UBSAN`, for example, in the application project file, or in the
``west build`` or ``cmake`` command line invocation.

.. _Undefined Behavior Sanitizer:
   https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html

Coverage reports
================

See
:ref:`coverage reports using the POSIX architecture<coverage_posix>`.


.. _native_sim32_64:

32 and 64bit versions
=====================

native_sim comes with two targets: A 32 bit and 64 bit version.
The 32 bit version, ``native_sim``, is the default target, which will compile
your code for the ILP32 ABI (i386 in a x86 or x86_64 system) where pointers
and longs are 32 bits.
This mimics the ABI of most embedded systems Zephyr targets,
and is therefore normally best to test and debug your code, as some bugs are
dependent on the size of pointers and longs.
This target requires either a 64 bit system with multilib support installed or
one with a 32bit userspace.

The 64 bit version, ``native_sim_64``, compiles your code targeting the
LP64 ABI (x86-64 in x86 systems), where pointers and longs are 64 bits.
You can use this target if you cannot compile or run 32 bit binaries.

.. _native_sim_Clib_choice:

C library choice
****************

native_sim may be compiled with a choice of C libraries.
By default it will be compiled with the host C library (:kconfig:option:`CONFIG_EXTERNAL_LIBC`),
but you can also select to build it with :kconfig:option:`CONFIG_MINIMAL_LIBC` or with
:kconfig:option:`CONFIG_PICOLIBC`.
If you select some feature which are not compatible with the host C library,
:ref:`Picolibc <c_library_picolibc>` will be selected by default instead.

When building with either :ref:`minimal <c_library_minimal>` or :ref:`Picolibc<c_library_picolibc>`
you will build your code in a more similar way as when building for the embedded target,
you will be able to test your code interacting with that C library,
and there will be no conflicts with the :ref:`POSIX OS abstraction<posix_support>` shim,
but, accessing the host for test purposes from your embedded code will be more
difficult, and you will have a limited choice of
:ref:`drivers and backends to chose from<native_sim_peripherals_c_compat>`.

Rationale for this port and comparison with other options
*********************************************************

The native_sim board shares the overall
:ref:`intent of the POSIX architecture<posix_arch_rationale>`,
while being a HW agnostic test platform which in some cases utilizes the host
OS peripherals.
It does not intend to model any particular HW, and as such can only be used
to develop and test application code which is far decoupled from the HW.

For developing and testing SW which requires specific HW, while retaining the
benefits of the POSIX architecture other solutions like the
:ref:`bsim boards<bsim boards>`
should be considered.

Check the :ref:`POSIX architecture comparison <posix_arch_compare>`
with other development and test options for more insights.

.. _native_sim_architecture:

Architecture
************

This board is based on the POSIX architecture port of Zephyr and shares
:ref:`its basic architecture<posix_arch_architecture>` regarding threading
and CPU/HW scheduling.

If you are interested on the inner workings of the native simulator itself, you can check
`its documentation <https://github.com/BabbleSim/native_simulator/blob/main/docs/README.md>`_.

This board does not try to emulate any particular embedded CPU or SOC.
The code is compiled natively for the host system (typically x86).

About time in native_sim
========================

Normally simulated time runs fully decoupled from the real host time
and as fast as the host compute power would allow.
This is desirable when running in a debugger or testing in batch, but not if
interacting with external interfaces based on the real host time.

The Zephyr kernel is only aware of the simulated time as provided by the
HW models. Therefore any normal Zephyr thread will also know only about
simulated time.

The only link between the simulated time and the real/host time, if any,
is created by the clock and timer model.

This model can be configured to slow down the execution of native_sim to
real time.
You can do this with the ``--rt`` and ``--no-rt`` options from the command line.
The default behavior is set with
:kconfig:option:`CONFIG_NATIVE_SIM_SLOWDOWN_TO_REAL_TIME`.

Note that all this model does is wait before raising the
next system tick interrupt until the corresponding real/host time.
If, for some reason, native_sim runs slower than real time, all this
model can do is "catch up" as soon as possible by not delaying the
following ticks.
So if the host load is too high, or you are running in a debugger, you will
see simulated time lagging behind the real host time.
This solution ensures that normal runs are still deterministic while
providing an illusion of real timeness to the observer.

When locked to real time, simulated time can also be set to run faster or
slower than real time.
This can be controlled with the ``--rt-ratio=<ratio>`` and ``-rt-drift=<drift>``
command line options. Note that both of these options control the same
underlying mechanism, and that ``drift`` is by definition equal to
``ratio - 1``.
It is also possible to adjust this clock speed on the fly with
:c:func:`native_rtc_adjust_clock()`.

In this way if, for example, ``--rt-ratio=2`` is given, the simulated time
will advance at twice the real time speed.
Similarly if ``--rt-drift=-100e-6`` is given, the simulated time will progress
100ppm slower than real time.
Note that these 2 options have no meaning when running in non real-time
mode.

How simulated time and real time relate to each other
-----------------------------------------------------

Simulated time (``st``) can be calculated from real time (``rt``) as

.. math::
  st = (rt - last\_rt) \times ratio + last\_st

And vice-versa:

.. math::
  rt = (st - last\_st) / ratio + last\_rt

Where ``last_rt`` and ``last_st`` are respectively the real time and the
simulated time when the last clock ratio adjustment took place.

All times are kept in microseconds.

.. _native_sim_peripherals:

Peripherals
***********

The following peripherals are currently provided with this board:

**Interrupt controller**
  A simple yet generic interrupt controller is provided. It can nest interrupts
  and provides interrupt priorities. Interrupts can be individually masked or
  unmasked. SW interrupts are also supported.

**Clock, timer and system tick model**
  This model provides the system tick timer. By default
  :kconfig:option:`CONFIG_SYS_CLOCK_TICKS_PER_SEC` configures it to tick every 10ms.

  Please refer to the section `About time in native_sim`_ for more
  information.

**UART/Serial**
   Two optional native UART drivers are available:

   **PTTY driver (UART_NATIVE_POSIX)**
      With this driver, one or two Zephyr UART devices can be created. These
      can be connected to the Linux process stdin/stdout or a newly created
      pseudo-tty. For more information refer to the section `PTTY UART`_.

   **TTY driver (UART_NATIVE_TTY)**
      An UART driver for interacting with host-attached serial port devices
      (eg. USB to UART dongles). For more information refer to the section
      `TTY UART`_.

**Real time clock**
  The real time clock model provides a model of a constantly powered clock.
  By default this is initialized to the host time at boot.

  This RTC can also be set to start from time 0 with the ``--rtc-reset`` command
  line option.

  It is possible to offset the RTC clock value at boot with the
  ``--rtc-offset=<offset>`` option,
  or to adjust it dynamically with the function :c:func:`native_rtc_offset`.

  After start, this RTC advances with the simulated time, and is therefore
  affected by the simulated time speed ratio.
  See `About time in native_sim`_ for more information.

  The time can be queried with the functions :c:func:`native_rtc_gettime_us`
  and :c:func:`native_rtc_gettime`. Both accept as parameter the clock source:

  - ``RTC_CLOCK_BOOT``: It counts the simulated time passed since boot.
    It is not subject to offset adjustments
  - ``RTC_CLOCK_REALTIME``: RTC persistent time. It is affected by
    offset adjustments.
  - ``RTC_CLOCK_PSEUDOHOSTREALTIME``: A version of the real host time,
    as if the host was also affected by the clock speed ratio and offset
    adjustments performed to the simulated clock and this RTC. Normally
    this value will be a couple of hundredths of microseconds ahead of the
    simulated time, depending on the host execution speed.
    This clock source should be used with care, as depending on the actual
    execution speed of native_sim and the host load,
    it may return a value considerably ahead of the simulated time.

  Note this device does not yet have an :ref:`RTC API compatible driver <rtc_api>`.

.. _nsim_per_entr:

**Entropy device**
  An entropy device based on the host :c:func:`random` API.
  This device will generate the same sequence of random numbers if initialized
  with the same random seed.
  You can change this random seed value by using the command line option:
  :samp:`--seed={<random_seed>}` where the value specified is a 32-bit integer
  such as 97229 (decimal),  0x17BCD (hex), or 0275715 (octal).

.. _nsim_per_ethe:

**Ethernet driver**
  A simple TAP based ethernet driver is provided. The driver expects that the
  **zeth** network interface already exists in the host system. The **zeth**
  network interface can be created by the ``net-setup.sh`` script found in
  the `net-tools`_ zephyr project repository. User can communicate with the
  Zephyr instance via the **zeth** network interface. Multiple TAP based
  network interfaces can be created if needed. The IP address configuration
  can be specified for each network interface instance.

  Note that this device can only be used with Linux hosts.

.. _net-tools:
   https://github.com/zephyrproject-rtos/net-tools

.. _nsim_bt_host_cont:

**Bluetooth controller**
  It's possible to use the host's Bluetooth adapter as a Bluetooth
  controller for Zephyr. To do this the HCI device needs to be passed as
  a command line option to ``zephyr.exe``. For example, to use ``hci0``,
  use ``sudo zephyr.exe --bt-dev=hci0``. Using the device requires root
  privileges (or the CAP_NET_ADMIN POSIX capability, to be exact) so
  ``zephyr.exe`` needs to be run through ``sudo``. The chosen HCI device
  must be powered down and support Bluetooth Low Energy (i.e. support the
  Bluetooth specification version 4.0 or greater).

  Another possibility is to use a HCI TCP server which acts as a
  :ref:`virtual Bluetooth controller<bluetooth_virtual_posix>` over TCP.
  To connect to a HCI TCP server its IP address and port number must
  be specified. For example, to connect to a HCI TCP server with IP
  address 127.0.0.0 and port number 1020 use ``zephyr.exe --bt-dev=127.0.0.1:1020``.
  This alternative option is mainly aimed for testing Bluetooth connectivity over
  a virtual Bluetooth controller that does not depend on the Linux Bluetooth
  stack and its HCI interface.

.. _nsim_per_usb:

**USB controller**
  It's possible to use the Virtual USB controller working over USB/IP
  protocol. More information can be found in
  :ref:`Testing USB over USP/IP in native_sim <testing_USB_native_sim>`.

.. _nsim_per_disp_sdl:

**Display driver**
  A display driver is provided that creates a window on the host machine to
  render display content.

  This driver requires a 32-bit version of the `SDL2`_ library on the host
  machine and ``pkg-config`` settings to correctly pickup the SDL2 install path
  and compiler flags.

  On a Ubuntu 22.04 host system, for example, install the ``pkg-config`` and
  ``libsdl2-dev:i386`` packages, and configure the pkg-config search path with
  these commands:

  .. code-block:: console

     $ sudo dpkg --add-architecture i386
     $ sudo apt update
     $ sudo apt-get install pkg-config libsdl2-dev:i386
     $ export PKG_CONFIG_PATH=/usr/lib/i386-linux-gnu/pkgconfig

.. _SDL2:
   https://www.libsdl.org/download-2.0.php

.. _nsim_per_flash_simu:

**EEPROM simulator**
  The EEPROM simulator can also be used in the native targets. In these, you have the added feature
  of keeping the EEPROM content on a file on the host filesystem.
  By default this is kept in the file :file:`eeprom.bin` in the current working directory, but you
  can select the location of this file and its name with the command line parameter ``--eeprom``.
  Some more information can be found in :ref:`the emulators page <emul_eeprom_simu_brief>`.

**Flash simulator**
  The flash simulator can also be used in the native targets. In this you have the option to keep
  the flash content in a binary file on the host file system or in RAM. The behavior of the flash
  device can be configured through the native_sim board devicetree or Kconfig settings under
  :kconfig:option:`CONFIG_FLASH_SIMULATOR`.

  By default the binary data is located in the file :file:`flash.bin` in the current
  working directory. The location of this file can be changed through the
  command line parameter ``--flash``. The flash data will be stored in raw format
  and the file will be truncated to match the size specified in the devicetree
  configuration. In case the file does not exists the driver will take care of
  creating the file, else the existing file is used.

  Some more information can be found in :ref:`the emulators page <emul_flash_simu_brief>`.

  The flash content can be accessed from the host system, as explained in the
  `Host based flash access`_ section.

**Input events**
  Two optional native input drivers are available:

  **evdev driver**
    A driver is provided to read input events from a Linux evdev input device and
    inject them back into the Zephyr input subsystem.

    The driver is automatically enabled when :kconfig:option:`CONFIG_INPUT` is
    enabled and the devicetree contains a node such as:

    .. code-block:: dts

       evdev {
         compatible = "zephyr,native-linux-evdev";
       };

    The application then has to be run with a command line option to specify
    which evdev device node has to be used, for example
    ``zephyr.exe --evdev=/dev/input/event0``.

  **Input SDL touch**
    This driver emulates a touch panel input using the SDL library. It can be enabled with
    :kconfig:option:`CONFIG_INPUT_SDL_TOUCH` and configured with the device tree binding
    :dtcompatible:`zephyr,input-sdl-touch`.

    More information on using SDL and the Display driver can be found in
    :ref:`its section <nsim_per_disp_sdl>`.

**CAN controller**
  It is possible to use a host CAN controller with the native SockerCAN Linux driver. It can be
  enabled with :kconfig:option:`CONFIG_CAN_NATIVE_LINUX` and configured with the device tree binding
  :dtcompatible:`zephyr,native-linux-can`.

.. _native_ptty_uart:

PTTY UART
=========

This driver can be configured with :kconfig:option:`CONFIG_UART_NATIVE_POSIX`
to instantiate up to two UARTs. By default only one UART is enabled.
With :kconfig:option:`CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE`
you can enable the second one.

For the first UART, it can link it to a new
pseudoterminal (i.e. :file:`/dev/pts{<nbr>}`), or map the UART input and
output to the executable's ``stdin`` and ``stdout``.
This is chosen by selecting either
:kconfig:option:`CONFIG_NATIVE_UART_0_ON_OWN_PTY` or
:kconfig:option:`CONFIG_NATIVE_UART_0_ON_STDINOUT`
For interactive use with the :ref:`shell_api`, choose the first (OWN_PTY) option.
The second (STDINOUT) option can be used with the shell for automated
testing, such as when piping other processes' output to control it.
This is because the shell subsystem expects access to a raw terminal,
which (by default) a normal Linux terminal is not.

When :kconfig:option:`CONFIG_NATIVE_UART_0_ON_OWN_PTY` is chosen, the name of the
newly created UART pseudo-terminal will be displayed in the console.
If you want to interact with it manually, you should attach a terminal emulator
to it. This can be done, for example with the command:

.. code-block:: console

   $ xterm -e screen /dev/<ttyn> &

where :file:`/dev/tty{<n>}` should be replaced with the actual TTY device.

You may also chose to automatically attach a terminal emulator to the first UART
by passing the command line option ``-attach_uart`` to the executable.
The command used for attaching to the new shell can be set with the command line
option ``-attach_uart_cmd=<"cmd">``. Where the default command is given by
:kconfig:option:`CONFIG_NATIVE_UART_AUTOATTACH_DEFAULT_CMD`.
Note that the default command assumes both ``xterm`` and ``screen`` are
installed in the system.

This driver only supports poll mode. Interrupt and async mode are not supported.
Neither runtime configuration or line control are supported.

.. _native_tty_uart:

TTY UART
========

With this driver an application can use the polling UART API (``uart_poll_out``,
``uart_poll_in``) to write and read characters to and from a connected serial
port device.

This driver is automatically enabled when a devicetree contains a node
with ``"zephyr,native-tty-uart"`` compatible property and ``okay`` status, such
as one below.

.. code-block:: dts

   uart {
     status = "okay";
     compatible = "zephyr,native-tty-uart";
     serial-port = "/dev/ttyUSB0";
     current-speed = <115200>;
   };

Interaction with serial ports can be configured in several different ways:

* The default serial port and baud rate can be set via the device tree
  properties ``serial-port`` and ``current-speed`` respectively.  The
  ``serial-port`` property is optional.
* Serial port and baud rate can also be set via command line options ``X_port``
  and ``X_baud`` respectively, where ``X`` is a name of a node. Command line
  options override values from the devicetree.
* The rest of the configuration options such as number of data and stop bits,
  parity, as well as baud rate can be set at runtime with ``uart_configure``.

Multiple instances of such uart drivers are supported.

The :zephyr:code-sample:`uart-native-tty` sample app provides a working example of the
driver.

This driver only supports poll mode. Interrupt and async mode are not supported.
It has runtime configuration support, but no line control support.

.. _native_sim_backends:

Subsystems backends
*******************

Apart from its own peripherals, the native_sim board also has some dedicated
backends for some of Zephyr's subsystems. These backends are designed to ease
development by integrating more seamlessly with the host operating system:

.. _nsim_back_console:

**Console backend**:
  A console backend which by default is configured to
  redirect any :c:func:`printk` write to the native host application's
  ``stdout``.

  This driver is selected by default if the `PTTY UART`_ is not compiled in.
  Otherwise :kconfig:option:`CONFIG_UART_CONSOLE` will be set to select the UART as
  console backend.

.. _nsim_back_logger:

**Logger backend**:
  A backend which prints all logger output to the process ``stdout``.
  It supports timestamping, which can be enabled with
  :kconfig:option:`CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP`; and colored output which can
  be enabled with :kconfig:option:`CONFIG_LOG_BACKEND_SHOW_COLOR` and controlled
  with the command line options ``--color``, ``--no-color`` and
  ``--force-color``.

  In native_sim, by default, the logger is configured with
  :kconfig:option:`CONFIG_LOG_MODE_IMMEDIATE`.

  This backend can be selected with :kconfig:option:`CONFIG_LOG_BACKEND_NATIVE_POSIX`
  and is enabled by default.

.. _nsim_back_trace:

**Tracing**:
  A backend/"bottom" for Zephyr's CTF tracing subsystem which writes the tracing
  data to a file in the host filesystem.
  More information can be found in :ref:`Common Tracing Format <ctf>`

Emulators
*********

All :ref:`available HW emulators <emulators>` can be used with native_sim.

.. _native_fuse_flash:

Host based flash access
***********************

If a flash device is present, the file system partitions on the flash
device can be exposed through the host file system by enabling
:kconfig:option:`CONFIG_FUSE_FS_ACCESS`. This option enables a FUSE
(File system in User space) layer that maps the Zephyr file system calls to
the required UNIX file system calls, and provides access to the flash file
system partitions with normal operating system commands such as ``cd``,
``ls`` and ``mkdir``.

By default the partitions are exposed through the directory :file:`flash/` in the
current working directory. This directory can be changed via the command line
option ``--flash-mount``. As this directory operates as a mount point for FUSE
you have to ensure that it exists before starting the native_sim board.

On exit, the native_sim board application will take care of unmounting the
directory. In the unfortunate case that the native_sim board application
crashes, you can cleanup the stale mount point by using the program
``fusermount``:

.. code-block:: console

   $ fusermount -u flash

Note that this feature requires a 32-bit version of the FUSE library, with a
minimal version of 2.6, on the host system and ``pkg-config`` settings to
correctly pickup the FUSE install path and compiler flags.

On a Ubuntu 22.04 host system, for example, install the ``pkg-config`` and
``libfuse-dev:i386`` packages, and configure the pkg-config search path with
these commands:

.. code-block:: console

   $ sudo dpkg --add-architecture i386
   $ sudo apt update
   $ sudo apt-get install pkg-config libfuse-dev:i386
   $ export PKG_CONFIG_PATH=/usr/lib/i386-linux-gnu/pkgconfig

.. _native_sim_peripherals_c_compat:

Peripherals and backends C library compatibility
************************************************

Today, some native_sim peripherals and backends are, so far, only available when compiling with the
host libC (:kconfig:option:`CONFIG_EXTERNAL_LIBC`):

.. csv-table:: Drivers/backends vs libC choice
   :header: Driver class, driver name, driver kconfig, libC choices

     ADC, ADC emul, :kconfig:option:`CONFIG_ADC_EMUL`, All
     Bluetooth, :ref:`Userchan <nsim_bt_host_cont>`, :kconfig:option:`CONFIG_BT_USERCHAN`, Host libC
     CAN, CAN native Linux, :kconfig:option:`CONFIG_CAN_NATIVE_LINUX`, All
     Console backend, :ref:`POSIX arch console <nsim_back_console>`, :kconfig:option:`CONFIG_POSIX_ARCH_CONSOLE`, All
     Display, :ref:`Display SDL <nsim_per_disp_sdl>`, :kconfig:option:`CONFIG_SDL_DISPLAY`, All
     Entropy, :ref:`Native posix entropy <nsim_per_entr>`, :kconfig:option:`CONFIG_FAKE_ENTROPY_NATIVE_POSIX`, All
     EEPROM, EEPROM simulator, :kconfig:option:`CONFIG_EEPROM_SIMULATOR`, Host libC
     EEPROM, EEPROM emulator, :kconfig:option:`CONFIG_EEPROM_EMULATOR`, All
     Ethernet, :ref:`Eth native_posix <nsim_per_ethe>`, :kconfig:option:`CONFIG_ETH_NATIVE_POSIX`, All
     Flash, :ref:`Flash simulator <nsim_per_flash_simu>`, :kconfig:option:`CONFIG_FLASH_SIMULATOR`, All
     Flash, :ref:`Host based flash access <native_fuse_flash>`, :kconfig:option:`CONFIG_FUSE_FS_ACCESS`, Host libC
     GPIO, GPIO emulator, :kconfig:option:`CONFIG_GPIO_EMUL`, All
     GPIO, SDL GPIO emulator, :kconfig:option:`CONFIG_GPIO_EMUL_SDL`, All
     I2C, I2C emulator, :kconfig:option:`CONFIG_I2C_EMUL`, All
     Input, Input SDL touch, :kconfig:option:`CONFIG_INPUT_SDL_TOUCH`, All
     Input, Linux evdev, :kconfig:option:`CONFIG_NATIVE_LINUX_EVDEV`, All
     Logger backend, :ref:`Native backend <nsim_back_logger>`, :kconfig:option:`CONFIG_LOG_BACKEND_NATIVE_POSIX`, All
     RTC, RTC emul, :kconfig:option:`CONFIG_RTC_EMUL`, All
     Serial, :ref:`UART native posix/PTTY <native_ptty_uart>`, :kconfig:option:`CONFIG_UART_NATIVE_POSIX`, All
     Serial, :ref:`UART native TTY <native_tty_uart>`, :kconfig:option:`CONFIG_UART_NATIVE_TTY`, All
     SPI, SPI emul, :kconfig:option:`CONFIG_SPI_EMUL`, All
     System tick, Native_posix timer, :kconfig:option:`CONFIG_NATIVE_POSIX_TIMER`, All
     Tracing, :ref:`Posix tracing backend <nsim_back_trace>`, :kconfig:option:`CONFIG_TRACING_BACKEND_POSIX`, All
     USB, :ref:`USB native posix <nsim_per_usb>`, :kconfig:option:`CONFIG_USB_NATIVE_POSIX`, Host libC
