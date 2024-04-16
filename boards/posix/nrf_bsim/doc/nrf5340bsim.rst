.. _nrf5340bsim:

NRF5340 simulated boards (BabbleSim)
####################################

.. contents::
   :depth: 1
   :backlinks: entry
   :local:


Overview
********

To allow simulating nrf5340 SOCs two Zephyr target boards are provided: the
``nrf5340bsim_nrf5340_cpuapp`` and ``nrf5340bsim_nrf5340_cpunet``.

These use `BabbleSim`_ to simulate the radio activity, and the
:ref:`POSIX architecture<Posix arch>` and the `native simulator`_ to
run applications natively on the development system. This has the benefit of
providing native code execution performance and easy debugging using
native tools, but inherits :ref:`its limitations <posix_arch_limitations>`.

Just like for the nrf5340dk targets,
the nrf5340bsim_nrf5340_cpuapp build target provides support for the application core,
and the nrf5340bsim_nrf5340_cpunet build target provides support for the network
core on the simulated nRF5340 SOC.

These boards include models of some of the nRF5340 SOC peripherals:

* Radio
* Timers
* AAR (Accelerated Address Resolver)
* AES CCM & AES ECB encryption HW
* CLOCK (Clock control)
* DPPI (Distributed Programmable Peripheral Interconnect)
* EGU (Event Generator Unit)
* FICR (Factory Information Configuration Registers)
* NVMC (Non-Volatile Memory Controller / Flash)
* RNG (Random Number Generator)
* RTC (Real Time Counter)
* TEMP (Temperature sensor)
* UICR (User Information Configuration Registers)

and will use the same drivers as the nrf5340dk targets for these.
For more information on what is modelled to which level of detail,
check the `HW models implementation status`_.

.. note::

   The IPC and MUTEX peripherals are not yet present in these models. Therefore communication
   between the cores using Zephyr's IPC driver is not yet possible.

Note that unlike a real nrf5340 device, the nrf5340bsim boards have unlimited RAM and flash for
code.

.. _BabbleSim:
   https://BabbleSim.github.io

.. _native simulator:
   https://github.com/BabbleSim/native_simulator/blob/main/docs/README.md

.. _HW models implementation status:
   https://github.com/BabbleSim/ext_nRF_hw_models/blob/main/docs/README_impl_status.md


Building for, and using these boards
************************************

If you are interested in developing on only one of the MCUs in this SOC, you
can use the corresponding simulated target, nrf5340bsim_nrf5340_cpuapp or nrf5340bsim_nrf5340_cpunet
following the instructions from the :ref:`nrf52_bsim board <nrf52bsim_build_and_run>`.
Simply change the board/target appropriately when building.


.. note::

   Unlike in real HW, the net core MCU is set-up to automatically boot at start, to facilitate
   developing without an image in the application core. You can control
   this with either :kconfig:option:`CONFIG_NATIVE_SIMULATOR_AUTOSTART_MCU`, or the command line
   option ``--cpu1_autostart``.

   If an MCU is booted without any image, it will automatically set itself to sleep.


Assembling both MCUs images into a single executable
****************************************************

By default, when you build targeting either nrf5340bsim_nrf5340_cpuapp or
nrf5340bsim_nrf5340_cpunet you will end up with a library (``zephyr/zephyr.elf``) that corresponds
to that MCU code image, and an executable (``zephyr/zephyr.exe``) that includes the native simulator
runner, SOC HW models, that image, and an empty image for the other MCU.

If you want to assemble an executable including a previously built image for the other MCU,
built with either Zephyr's build system or another native simulator compatible build system,
you can provide that image to the Zephyr build of the second image using
:kconfig:option:`CONFIG_NATIVE_SIMULATOR_EXTRA_IMAGE_PATHS`.


.. note::

   These libraries/images are **not** embedded images. You cannot use them for embedded devices,
   and cannot use an embedded image to assemble a native executable.

.. note::

   OpenAMP is not yet supported in these boards.

TrustZone, TF-M and other security considerations
*************************************************

ARM's TrustZone is not modelled in these boards. This means that:

* There is no differentiation between secure and non secure execution states or bus accesses.
* All RAM, flash and peripherals are in principle accessible from all SW. Peripherals with their
  own interconnect master ports can, in principle, access any other peripheral or RAM area.
* There is no nrf5340bsim_nrf5340_cpuapp_ns board/build target, or posibility of mixing secure
  and non-secure images.
* Currently there is no model of the SPU, and therefore neither flash, RAM areas or peripherals
  can be labelled as restricted for secure or non secure access.
* TF-M cannot be used.

Note that the ARM cryptocell-312 peripheral is not modelled. The mbedTLS library can still be used
but with a SW crypto backend instead of the cryptocell HW acceleration.
