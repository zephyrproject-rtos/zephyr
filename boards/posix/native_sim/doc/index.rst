.. _native_sim:

Native simulator - native_sim
#############################

Overview
********

The native_sim board is an evolution of :ref:`native_posix<native_posix>`.
Just like with :ref:`native_posix<native_posix>` you can build your Zephyr application
with the Zephyr kernel, creating a normal Linux executable with your host tooling,
and can debug and instrument it like any other Linux program.

native_sim is based on the
`native simulator <https://github.com/BabbleSim/native_simulator/>`_
and the :ref:`POSIX architecture<Posix arch>`.

Host system dependencies
************************

Please check the
:ref:`Posix Arch Dependencies<posix_arch_deps>`

.. _nativesim_important_limitations:

Important limitations
*********************

Native_sim is based on the :ref:`POSIX architecture<Posix arch>`, and therefore
:ref:`its limitations <posix_arch_limitations>` and considerations apply to it.

How to use it
*************

To build, simply specify the native_sim board as target:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: native_sim
   :goals: build
   :compact:

Now you have a Linux executable, ``./build/zephyr/zephyr.exe``, you can use just like any
other Linux program.

You can run, debug, build it with sanitizers or with coverage just like with
:ref:`native_posix <native_posix>`.
Please check :ref:`native_posix's how to<native_posix_how_to_use>` for more info.

32 and 64bit versions
*********************

Just like native_posix, native_sim comes with two targets: A 32 bit and 64 bit version.
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

Unlike native_posix, native_sim may be compiled with a choice of C libraries.
By default it will be compiled with the host C library (:kconfig:option:`CONFIG_EXTERNAL_LIBC`),
but you can also select to build it with :kconfig:option:`CONFIG_MINIMAL_LIBC` or with
:kconfig:option:`CONFIG_PICOLIBC`.

When building with either :ref:`MINIMAL<c_library_minimal>` or :ref:`PICO<c_library_picolibc>` libC
you will build your code in a more similar way as when building for the embedded target,
you will be able to test your code interacting with that C library,
and there will be no conflicts with the :ref:`POSIX OS abstraction<posix_support>` shim,
but, accessing the host for test purposes from your embedded code will be more
difficult, and you will have a limited choice of
:ref:`drivers and backends to chose from<native_sim_peripherals>`.

Architecture
************

:ref:`native_posix's architecture description<native_posix_architecture>` as well as the
:ref:`POSIX architecture description<posix_arch_architecture>` are directly
applicable to native_sim.

If you are interested on the inner workigns of the native simulator itself, you can check
`its documentation <https://github.com/BabbleSim/native_simulator/blob/main/docs/README.md>`_.

.. _native_sim_peripherals:

Peripherals, subsystems backends and host based flash access
************************************************************

Today, native_sim supports the exact same
:ref:`peripherals and backends as native_posix<native_posix_peripherals>`,
with the only caveat that some of these are, so far, only available when compiling with the
host libC (:kconfig:option:`CONFIG_EXTERNAL_LIBC`).

.. csv-table:: Drivers/backends vs libC choice
   :header: Driver class, driver name, driver kconfig, libC choices

     adc, ADC emul, :kconfig:option:`CONFIG_ADC_EMUL`, all
     bluetooth, userchan, :kconfig:option:`CONFIG_BT_USERCHAN`, host libC
     can, can native posix, :kconfig:option:`CONFIG_CAN_NATIVE_POSIX_LINUX`, host libC
     console backend, POSIX arch console, :kconfig:option:`CONFIG_POSIX_ARCH_CONSOLE`, all
     display, display SDL, :kconfig:option:`CONFIG_SDL_DISPLAY`, all
     entropy, native posix entropy, :kconfig:option:`CONFIG_FAKE_ENTROPY_NATIVE_POSIX`, all
     eprom, eprom emulator, :kconfig:option:`CONFIG_EEPROM_EMULATOR`, host libC
     ethernet, eth native_posix, :kconfig:option:`CONFIG_ETH_NATIVE_POSIX`, host libC
     flash, flash simulator, :kconfig:option:`CONFIG_FLASH_SIMULATOR`, all
     flash, host based flash access, :kconfig:option:`CONFIG_FUSE_FS_ACCESS`, host libC
     gpio, GPIO emulator, :kconfig:option:`CONFIG_GPIO_EMUL`, all
     gpio, SDL GPIO emulator, :kconfig:option:`CONFIG_GPIO_EMUL_SDL`, all
     i2c, I2C emulator, :kconfig:option:`CONFIG_I2C_EMUL`, all
     input, input SDL touch, :kconfig:option:`CONFIG_INPUT_SDL_TOUCH`, all
     log backend, native backend, :kconfig:option:`CONFIG_LOG_BACKEND_NATIVE_POSIX`, all
     rtc, RTC emul, :kconfig:option:`CONFIG_RTC_EMUL`, all
     serial, uart native posix/PTTY, :kconfig:option:`CONFIG_UART_NATIVE_POSIX`, all
     serial, uart native TTY, :kconfig:option:`CONFIG_UART_NATIVE_TTY`, all
     spi, SPI emul, :kconfig:option:`CONFIG_SPI_EMUL`, all
     system tick, native_posix timer, :kconfig:option:`CONFIG_NATIVE_POSIX_TIMER`, all
     tracing, Posix tracing backend, :kconfig:option:`CONFIG_TRACING_BACKEND_POSIX`, all
     usb, USB native posix, :kconfig:option:`CONFIG_USB_NATIVE_POSIX`, host libC
