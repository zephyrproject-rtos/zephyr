.. _nrf54lm20bsim:

NRF54LM20 simulated board (BabbleSim)
#####################################

.. contents::
   :depth: 1
   :backlinks: entry
   :local:


Overview
********

To allow simulating nRF54LM20 SOCs, a Zephyr target board is provided: the
``nrf54lm20bsim/nrf54lm20/cpuapp``.

This uses `BabbleSim`_ to simulate the radio activity, and the
:ref:`POSIX architecture<Posix arch>` and the `native simulator`_ to
run applications natively on the development system. This has the benefit of
providing native code execution performance and easy debugging using
native tools, but inherits :ref:`its limitations <posix_arch_limitations>`.

Just like for the nrf54lm20dk target, the nrf54lm20bsim/nrf54lm20/cpuapp build target provides
support for the application core, on the simulated nRF54LM20 SOC.

.. note::

   Unlike real nRF54LM20 devices, the nrf54lm20bsim target has unlimited RAM, and code does not
   occupy its RRAM.

.. note::

   This simulated target does **not** yet support targeting the cpuflpr core.

.. warning::

   This target is experimental.

This board includes models of some of the nRF54LM20 SOC peripherals:

* AAR (Accelerated Address Resolver)
* CCM (AES CCM mode encryption)
* CLOCK (Clock control)
* CRACEN (Crypto Accelerator Engine)
* DPPI (Distributed Programmable Peripheral Interconnect)
* ECB (AES electronic codebook mode encryption)
* EGU (Event Generator Unit)
* FICR (Factory Information Configuration Registers)
* GPIO & GPIOTE
* GRTC (Global Real-time Counter)
* PPIB (PPI Bridge)
* RADIO
* RRAMC (Resistive RAM Controller)
* TEMP (Temperature sensor)
* TIMER
* UARTE (UART with Easy DMA)
* UICR (User Information Configuration Registers)

and will use the same drivers as the nrf54lm20dk targets for these.
For more information on what is modeled to which level of detail,
check the `HW models implementation status`_.

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

The same considerations as for the :ref:`nrf54l15bsim<nrf54l15bsim_tz>` target apply to this.
