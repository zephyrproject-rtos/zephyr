.. _nrf54l15bsim:

NRF54L15 simulated boards (BabbleSim)
#####################################

.. contents::
   :depth: 1
   :backlinks: entry
   :local:


Overview
********

To allow simulating nRF54L15 SOCs a Zephyr target boards is provided: the
``nrf54l15bsim/nrf54l15/cpuapp``.

This uses `BabbleSim`_ to simulate the radio activity, and the
:ref:`POSIX architecture<Posix arch>` and the `native simulator`_ to
run applications natively on the development system. This has the benefit of
providing native code execution performance and easy debugging using
native tools, but inherits :ref:`its limitations <posix_arch_limitations>`.

Just like for the nrf54l15dk target,
the nrf54l15bsim/nrf54l15/cpuapp build target provides support for the application core,
on the simulated nRF54L15 SOC.

.. note::

   This simulated target does **not** yet support targeting the cpuflpr core.

.. warning::

   This target is experimental.

This boards include models of some of the nRF54L15 SOC peripherals:

* AAR (Accelerated Address Resolver)
* CCM (AES CCM mode encryption)
* CLOCK (Clock control)
* DPPI (Distributed Programmable Peripheral Interconnect)
* ECB (AES electronic codebook mode encryption)
* EGU (Event Generator Unit)
* FICR (Factory Information Configuration Registers)
* GPIO & GPIOTE
* GRTC (Global Real-time Counter)
* PPIB (PPI Bridge)
* RADIO
* RRAMC (Resistive RAM Controller)
* RTC (Real Time Counter)
* TEMP (Temperature sensor)
* TIMER
* UICR (User Information Configuration Registers)

and will use the same drivers as the nrf54l15dk targets for these.
For more information on what is modeled to which level of detail,
check the `HW models implementation status`_.

Note that unlike a real nrf54l15 device, the nrf54l15bsim boards have unlimited RAM, and code does
not occupy their RRAM.

.. _BabbleSim:
   https://BabbleSim.github.io

.. _native simulator:
   https://github.com/BabbleSim/native_simulator/blob/main/docs/README.md

.. _HW models implementation status:
   https://github.com/BabbleSim/ext_nRF_hw_models/blob/main/docs/README_impl_status.md


Building for, and using this board
**********************************

You can follow the instructions from the :ref:`nrf52_bsim board <nrf52bsim_build_and_run>`.
Simply change the board/target appropriately when building.


TrustZone, TF-M and other security considerations
*************************************************

ARM's TrustZone is not modeled in this board. This means that:

* There is no differentiation between secure and non secure execution states or bus accesses.
* All RAM, flash and peripherals are in principle accessible from all SW. Peripherals with their
  own interconnect master ports can, in principle, access any other peripheral or RAM area.
* There is no nrf54l15bsim/nrf54l15/cpuapp/ns board/build target, or possibility of mixing secure
  and non-secure images.
* Currently there is no model of the SPU, and therefore neither RRAM, RAM areas or peripherals
  can be labeled as restricted for secure or non secure access.
* TF-M cannot be used.

Note that the CRACEN peripheral is not modeled.
As crypto library, Mbed TLS can be used with its SW crypto backend.
As entropy driver, the :dtcompatible:`zephyr,native-posix-rng` is enabled by default.
