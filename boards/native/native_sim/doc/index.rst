.. zephyr:board:: native_sim

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

   | ``native_sim`` is an evolution of its predecessor ``native_posix``.
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

The 64 bit version, ``native_sim/native/64``, compiles your code targeting the
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
but, accessing the host for test purposes from your embedded code will be more difficult.

.. _native_sim_peripherals_c_compat:

Peripherals and backends C library compatibility
================================================

While most native_sim drivers and backends support any C library, the drivers listed below have
limited compatibility:

.. csv-table:: Drivers/backends limited to some libCs
   :header: "Driver class", "Driver name", "Driver Kconfig", "libC choices"

   "Bluetooth", ":ref:`Userchan <nsim_bt_host_cont>`", ":kconfig:option:`CONFIG_BT_USERCHAN`", "Host and pico libC"
   "USB", ":ref:`USB native posix <nsim_per_usb>`", ":kconfig:option:`CONFIG_USB_NATIVE_POSIX`", "Host libC"

Cross-compiling native_sim
**************************

It is possible to :ref:`cross compile native_sim<posix_arch_cross_compile>`.

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

.. _native_sim_about_time:

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

.. zephyr:board-supported-hw::

Here are more details on the peripherals that are currently provided with this board:

.. grid:: 1
   :class-container: sd-index-grid
   :gutter: 0

   .. grid-item-card:: Audio and video
      :class-card: sd-index-card

      .. rst-class:: sd-index-watermark

      :material-twotone:`videocam;7em`

      * :ref:`Digital microphone (DMIC) <nsim_per_dmic>`
      * :ref:`Display driver <nsim_per_disp_sdl>`
      * :ref:`Inter-IC sound (I2S) <nsim_per_i2s>`
      * :ref:`Video capture driver <nsim_per_video_fifo>`

   .. grid-item-card:: Buses
      :class-card: sd-index-card

      .. rst-class:: sd-index-watermark

      :material-twotone:`cable;7em`

      * :ref:`CAN controller <nsim_per_can>`
      * :ref:`I2C controller <nsim_per_i2c>`
      * :ref:`UART/Serial <nsim_per_uart>`

        * :ref:`PTY driver <native_ptty_uart>`
        * :ref:`TTY driver <native_tty_uart>`

      * :ref:`USB controller <nsim_per_usb>`

   .. grid-item-card:: Core
      :class-card: sd-index-card

      .. rst-class:: sd-index-watermark

      :material-twotone:`memory;7em`

      * :ref:`Clock, timer and system tick model <nsim_per_timer>`
      * :ref:`Entropy device <nsim_per_entr>`
      * :ref:`Interrupt controller <nsim_per_intc>`
      * :ref:`Real time clock <nsim_per_rtc>`

   .. grid-item-card:: Input, sensors and indicators
      :class-card: sd-index-card

      .. rst-class:: sd-index-watermark

      :material-twotone:`sensors;7em`

      * :ref:`Fuel gauge <nsim_per_fuel_gauge>`
      * :ref:`Input events <nsim_per_input>`

        * :ref:`evdev driver <nsim_per_input_evdev>`
        * :ref:`Input SDL touch <nsim_per_input_sdl_touch>`

      * :ref:`LED device <nsim_per_led>`
      * :ref:`Temperature sensor <native_linux_temp_sensor>`

   .. grid-item-card:: Networking and wireless
      :class-card: sd-index-card

      .. rst-class:: sd-index-watermark

      :material-twotone:`lan;7em`

      * :ref:`Bluetooth controller <nsim_bt_host_cont>`
      * :ref:`Ethernet driver <nsim_per_ethe>`
      * :ref:`Offloaded sockets driver <nsim_per_offloaded_sockets>`
      * :ref:`Wi-Fi driver <nsim_per_wifi>`

   .. grid-item-card:: Storage
      :class-card: sd-index-card

      .. rst-class:: sd-index-watermark

      :material-twotone:`sd_card;7em`

      * :ref:`EEPROM simulator <nsim_per_eeprom>`
      * :ref:`Flash simulator <nsim_per_flash_simu>`

.. toctree::
   :hidden:

   peripherals

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

  This driver is selected by default if no UART driver is compiled in.
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

Host (FUSE) filesystem access
*****************************

When building Zephyr with a filesystem, the device partitions can be exposed through the host file
system by enabling :kconfig:option:`CONFIG_FUSE_FS_ACCESS`. This option enables a FUSE
(File system in User space) layer that mounts the simulated embedded filesystem in the host
filesystem, maps the Zephyr file system calls to the required UNIX file system calls, and provides
access to its partitions with normal operating system commands such as ``cd``, ``ls`` and ``mkdir``.

By default the partitions are exposed through the directory :file:`flash/` in the
current working directory. This directory can be changed via the command line
option ``--flash-mount``.

On exit, the native_sim board application will take care of unmounting the
directory. In the unfortunate case that the native_sim board application
crashes, you can cleanup the stale mount point by using the program
``fusermount``:

.. code-block:: console

   $ fusermount -u flash

You can chose to use the v2 FUSE host library or the v3 with
:kconfig:option:`CONFIG_FUSE_LIBRARY_VERSION`. By default v3 is selected.
When using the v2, a minimal version of 2.6 is necessary. For v3, 3.0 should suffice.
You will also need ``pkg-config`` setup to correctly pickup the FUSE install path and compiler flags.
Note that using this feature with the 32-bit native_sim variant requires the 32-bit version of the
corresponding FUSE library.

For example, to use the v3 of the library, on a Ubuntu 24.04 host system, install the ``pkg-config``
and ``libfuse3-dev:i386`` for 32-bit builds, and ``libfuse3-dev`` for 64-bit builds:

.. code-block:: console

   $ sudo dpkg --add-architecture i386
   $ sudo apt update
   $ sudo apt-get install pkg-config libfuse3-dev:i386 libfuse3-dev
   $ export PKG_CONFIG_PATH=/usr/lib/i386-linux-gnu/pkgconfig

Similarly ``libfuse-dev:i386`` and ``libfuse-dev`` provide the 32 and 64-bit FUSE v2 library
and headers.

.. _native_mount_fs:

Host filesystem mount
*********************

Enabling :kconfig:option:`CONFIG_FILE_SYSTEM_NATIVE_MOUNT` allows mounting a path from the host
filesystem into Zephyr's filesystem hierarchy. This allows Zephyr to access, and modify, host files
and folders like any other embedded mount point.

To mount a host directory, pass the ``-volume`` option to the executable on the command line:

.. code-block:: console

   $ zephyr.exe -volume=/host/dir:/zephyr/dir

The option follows Docker volume mount syntax: ``HOST-DIR:ZEPHYR-DIR[:ro]``. An optional ``:ro``
suffix mounts the volume as read-only. The option can be provided multiple times to mount several
host directories.
