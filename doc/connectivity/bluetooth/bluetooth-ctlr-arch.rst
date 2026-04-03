.. _bluetooth-ctlr-arch:

LE Controller
#############

Overview
********

.. image:: img/ctlr_overview.png

#. HCI

   * Host Controller Interface, Bluetooth standard
   * Provides Zephyr Bluetooth HCI Driver

#. HAL

   * Hardware Abstraction Layer
   * Vendor Specific, and Zephyr Driver usage

#. Ticker

   * Soft real time radio/resource scheduling

#. LL_SW

   * Software-based Link Layer implementation
   * States and roles, control procedures, packet controller

#. Util

   * Bare metal memory pool management
   * Queues of variable count, lockless usage
   * FIFO of fixed count, lockless usage
   * Mayfly concept based deferred ISR executions


Architecture
************

Execution Overview
==================

.. image:: img/ctlr_exec_overview.png


Architecture Overview
=====================

.. image:: img/ctlr_arch_overview.png


Scheduling
**********

.. image:: img/ctlr_sched.png


Ticker
======

.. image:: img/ctlr_sched_ticker.png


Upper Link Layer and Lower Link Layer
=====================================

.. image:: img/ctlr_sched_ull_lll.png


Scheduling Variants
===================

.. image:: img/ctlr_sched_variant.png


ULL and LLL Timing
==================

.. image:: img/ctlr_sched_ull_lll_timing.png


Event Handling
**************

.. image:: img/ctlr_sched_event_handling.png


Scheduling Closely Spaced Events
================================

.. image:: img/ctlr_sched_msc_close_events.png


Aborting Active Event
=====================

.. image:: img/ctlr_sched_msc_event_abort.png


Cancelling Pending Event
========================

.. image:: img/ctlr_sched_msc_event_cancel.png


Pre-emption of Active Event
===========================

.. image:: img/ctlr_sched_msc_event_preempt.png


Data Flow
*********

Transmit Data Flow
==================

.. image:: img/ctlr_dataflow_tx.png


Receive Data Flow
=================

.. image:: img/ctlr_dataflow_rx.png


Execution Priorities
********************

.. image:: img/ctlr_exec_prio.png

- Event handle (0, 1) < Event preparation (2, 3) < Event/Rx done (4) < Tx
  request (5) < Role management (6) < Host (7).

- LLL is vendor ISR, ULL is Mayfly ISR concept, Host is kernel thread.

Link Layer Control Procedures
*****************************

Following is a brief fly in on the main concepts in the implementation of
control procedures handling in ULL.

Three main execution contexts
=============================

- HCI/LLCP API
   * Miscellaneous procedure initiation API
   * From ull_llcp.c::ull_cp_<proc>() for initiating local procedures
   * Interface with running procedures, local and remote

- lll_prepare context to drive ull_cp_run()
   * LLCP main state machine entry/tick'er
   * From ull_peripheral.c/ull_central.c::ticker_cb via ull_conn_llcp()

- rx_demux context to drive ull_cp_tx_ack() and ull_cp_rx()
   * LLCP tx ack handling and PDU reception
   * From ull_conn.c::ull_conn_rx()
   * Handles passing PDUs into running procedures as well as possibly initiating remote procedures

Data structures and PDU helpers
===============================

- struct llcp_struct
   * Main LLCP data store
   * Defined in ull_conn_types.h and declared as part of struct ll_conn
   * Holds local and remote procedure request queues as well as conn specific LLCP data
   * Basic conn-level abstraction

- struct proc_ctx
   * General procedure context data, contains miscellaneous procedure data and state as well as sys_snode_t for queueing
   * Defined in ull_llcp_internal.h, declared/instantiated through ull_llcp.c::create_procedure()
   * Also holds node references used in tx_ack as well as rx_node retention mechanisms

- struct llcp_mem_pool
   * Mem pool used to implement procedure contexts resource - instantiated in both a local and a remote version
   * Used through ull_llcp.c::create_procedure()

- Miscellaneous pdu gymnastics
   * Encoding and decoding of control pdus done by ull_llcp_pdu.c::llcp_pdu_encode/decode_<PDU>()
   * Miscellaneous pdu validation handled by ull_llcp.c::pdu_validate_<PDU>() via ull_llcp.c::pdu_is_valid()

LLCP local and remote request/procedure state machines
======================================================

- ull_llcp_local.c
   * State machine handling local initiated procedures
   * Naming concept: lr _<...> => local request machine
   * Local procedure queue handling
   * Local run/rx/tx_ack switch

- ull_llcp_remote.c
   * Remote versions of the above
   * Naming concept: rr_<...> => remote request machine
   * Also handling of remote procedure initiation by llcp_rx_new()
   * Miscellaneous procedure collision handling (in rr_st_idle())

- ull_llcp_common/conn_upd/phy/enc/cc/chmu.c
   * Individual procedure implementations (ull_llcp_common.c collects the simpler ones)
   * Naming concept: lp_<...> => local initiated procedure, rp_<...> => remote initiated procedure
   * Handling of procedure flow from init (possibly through instant) to completion and host notification if applicable

Miscellaneous concepts
======================

- Procedure collision handling
   * See BT spec for explanation
   * Basically some procedures can exist in parallel but some can't - for instance only one instant based at a time
   * Spec states rules for how to handle/resolve collisions when they happen

- Termination handling
   * Specific rules apply re. how termination is handled.
   * Since we have resource handling re. procedure contexts and terminate must always be available this is handled as a special case
   * Note also - there are miscellaneous cases where connection termination is triggered on invalid peer behavior

- New remote procedure handling
   * Table new_proc_lut[] maps LLCP PDUs to procedures/roles used in llcp_rr_new()
   * Note - for any given connection, there can only ever be ONE remote procedure in the remote procedure queue

- Miscellaneous minors
   * pause/resume concepts - there are two (see spec for details)
   * procedure execution can be paused by the encryption procedure
   * data TX can be paused by PHY, DLE and ENC procedure
   * RX node retention - ensures no waiting for allocation of RX node when needed for notification


Miscellaneous unit test concepts
================================

- Individual ZTEST unit test for each procedure
   * zephyr/tests/bluetooth/controller/ctrl_<proc>

- Rx node handling is mocked
   * Different configs are handled by separate conf files (see ctrl_conn_update for example)
   * ZTEST(periph_rem_no_param_req, test_conn_update_periph_rem_accept_no_param_req)

- Emulated versions of rx_demux/prepare context used in unit tests - testing ONLY procedure PDU flow
   * event_prepare()/event_done() helpers emulating LLL prepare/done flow
   * lt_rx()/lt_tx() 'lower tester' emulation of rx/tx
   * ut_rx_node() 'upper tester' emulation of notification flow handling
   * Bunch of helpers to generate and parse PDUs, as well as miscellaneous mocked ull_stuff()




Lower Link Layer
****************

LLL Execution
=============

.. image:: img/ctlr_exec_lll.png


LLL Resume
----------

.. image:: img/ctlr_exec_lll_resume_top.png

.. image:: img/ctlr_exec_lll_resume_bottom.png


Bare metal utilities
********************

Memory FIFO and Memory Queue
============================

.. image:: img/ctlr_mfifo_memq.png

Mayfly
======

.. image:: img/ctlr_mayfly.png


* Mayfly are multi-instance scalable ISR execution contexts
* What a Work is to a Thread, Mayfly is to an ISR
* List of functions executing in ISRs
* Execution priorities map to IRQ priorities
* Facilitate cross execution context scheduling
* Race-to-idle execution
* Lock-less, bare metal

Legacy Controller
*****************

.. image:: img/ctlr_legacy.png

Bluetooth Low Energy Controller - Vendor Specific Details
*********************************************************

Hardware Requirements
=====================

Nordic Semiconductor
--------------------

The Nordic Semiconductor Bluetooth Low Energy Controller implementation
requires the following hardware peripherals.

.. list-table:: SoC Peripheral Use
   :header-rows: 1
   :widths: 15 15 15 10 50

   * - Resource
     - nRF Peripheral
     - # instances
     - Zephyr Driver Accessible
     - Description
   * - Clock
     - NRF_CLOCK
     - 1
     - Yes
     - * A Low Frequency Clock (LFCLOCK) or sleep clock, for low power
         consumption between Bluetooth radio events
       * A High Frequency Clock (HFCLOCK) or active clock, for high precision
         packet timing and software based transceiver state switching with
         inter-frame space (tIFS) timing inside Bluetooth radio events
   * - RTC [a]_
     - NRF_RTC0
     - 1
     - **No**
     - * Uses 2 capture/compare registers
   * - Timer
     - NRF_TIMER0 or NRF_TIMER4 [1]_, and NRF_TIMER1 [0]_
     - 2 or 1 [1]_
     - **No**
     - * 2 instances, one each for packet timing and tIFS software switching,
         respectively
       * 7 capture/compare registers (3 mandatory, 1 optional for ISR profiling,
         4 for single timer tIFS switching) on first instance
       * 4 capture/compare registers for second instance, if single tIFS timer
         is not used.
   * - PPI [b]_
     - NRF_PPI
     - 21 channels (20 [2]_), and 2 channel groups [3]_
     - Yes [4]_
     - * Used for radio mode switching to achieve tIFS timings, for PA/LNA
         control
   * - DPPI [c]_
     - NRF_DPPI
     -  20 channels, and 2 channel groups [3]_
     - Yes [4]_
     - * Used for radio mode switching to achieve tIFS timings, for PA/LNA
         control
   * - SWI [d]_
     - NRF_SWI4 and NRF_SWI5, or NRF_SWI2 and NRF_SWI3 [5]_
     - 2
     - **No**
     - * 2 instances, for Lower Link Layer and Upper Link Layer Low priority
         execution context
   * - Radio
     - NRF_RADIO
     - 1
     - **No**
     - * 2.4 GHz radio transceiver with multiple radio standards such as 1 Mbps,
         2 Mbps and Coded PHY S2/S8 Long Range Bluetooth Low Energy technology
   * - RNG [e]_
     - NRF_RNG
     - 1
     - Yes
     -
   * - ECB [f]_
     - NRF_ECB
     - 1
     - **No**
     -
   * - CBC-CCM [g]_
     - NRF_CCM
     - 1
     - **No**
     -
   * - AAR [h]_
     - NRF_AAR
     - 1
     - **No**
     -
   * - GPIO [i]_
     - NRF_GPIO
     - 2 GPIO pins for PA and LNA, 1 each
     - Yes
     - * Additionally, 10 Debug GPIO pins (optional)
   * - GPIOTE [j]_
     - NRF_GPIOTE
     - 1
     - Yes
     - * Used for PA/LNA
   * - TEMP [k]_
     - NRF_TEMP
     - 1
     - Yes
     - * For RC sourced LFCLOCK calibration
   * - UART [l]_
     - NRF_UART0
     - 1
     - Yes
     - * For HCI interface in Controller only builds
   * - IPC [m]_
     - NRF_IPC [5]_
     - 1
     - Yes
     - * For HCI interface in Controller only builds


.. [a] Real Time Counter (RTC)
.. [b] Programmable Peripheral Interconnect (PPI)
.. [c] Distributed Programmable Peripheral Interconnect (DPPI)
.. [d] Software Interrupt (SWI)
.. [e] Random Number Generator (RNG)
.. [f] AES Electronic Codebook Mode Encryption (ECB)
.. [g] Cipher Block Chaining (CBC) - Message Authentication Code with Counter
       Mode encryption (CCM)
.. [h] Accelerated Address Resolver (AAR)
.. [i] General Purpose Input Output (GPIO)
.. [j] GPIO tasks and events (GPIOTE)
.. [k] Temperature sensor (TEMP)
.. [l] Universal Asynchronous Receiver Transmitter (UART)
.. [m] Interprocess Communication peripheral (IPC)


.. [0] :kconfig:option:`CONFIG_BT_CTLR_TIFS_HW` ``=n``
.. [1] :kconfig:option:`CONFIG_BT_CTLR_SW_SWITCH_SINGLE_TIMER` ``=y``
.. [2] When not using pre-defined PPI channels
.. [3] For software-based tIFS switching
.. [4] Drivers that use nRFx interfaces
.. [5] For nRF53x Series
