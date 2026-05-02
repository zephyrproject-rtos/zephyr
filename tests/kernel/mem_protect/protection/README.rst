.. _protection_tests:

Protection tests
#################################

Overview
********
This test case verifies that protection is provided
against the following security issues:

* Write to read-only data.
* Write to text.
* Execute from data.
* Execute from stack.
* Execute from heap.

Building and Running
********************

This project can be built and executed as follows:


.. zephyr-app-commands::
   :zephyr-app: tests/kernel/mem_protect/protection
   :board: <board name>
   :goals: run
   :compact:

Connect the board to your host computer using the USB port.
Flash the generated zephyr.bin on the board.
Reset the board.

Sample Output
=============

.. code-block:: console

   ***** BOOTING ZEPHYR OS v1.8.99 - BUILD: Jun 19 2017 12:44:27 *****
   Running test suite test_protection
   tc_start() - write_ro
   trying to write to rodata at 0x00003124
   ***** BUS FAULT *****
     Executing thread ID (thread): 0x200001bc
     Faulting instruction address:  0x88c
     Imprecise data bus error
   Caught system error -- reason 0
   ===================================================================
   PASS - write_ro.
   tc_start() - write_text
   trying to write to text at 0x000006c0
   ***** BUS FAULT *****
     Executing thread ID (thread): 0x200001bc
     Faulting instruction address:  0xd60
     Imprecise data bus error
   Caught system error -- reason 0
   ===================================================================
   PASS - write_text.
   tc_start() - exec_data
   trying to call code written to 0x2000041d
   ***** BUS FAULT *****
     Executing thread ID (thread): 0x200001bc
     Faulting instruction address:  0x2000041c
     Imprecise data bus error
   Caught system error -- reason 0
   ===================================================================
   PASS - exec_data.
   tc_start() - exec_stack
   trying to call code written to 0x20000929
   ***** BUS FAULT *****
     Executing thread ID (thread): 0x200001bc
     Faulting instruction address:  0x20000928
     Imprecise data bus error
   Caught system error -- reason 0
   ===================================================================
   PASS - exec_stack.
   tc_start() - exec_heap
   trying to call code written to 0x20000455
   ***** BUS FAULT *****
     Executing thread ID (thread): 0x200001bc
     Faulting instruction address:  0x20000454
     Imprecise data bus error
   Caught system error -- reason 0
   ===================================================================
   PASS - exec_heap.
   ===================================================================
   PROJECT EXECUTION SUCCESSFUL
