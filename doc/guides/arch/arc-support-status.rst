Zephyr support status on ARC processors
#######################################

Overview
********

This page describes current state of Zephyr for ARC processors and some future
plans. Please note that

 * plans are given without exact deadlines
 * software features require corresponding hardware to be present and
   configured the proper way
 * not all the features can be enabled at the same time

Support status
**************

Legend:
**Y** - yes, supported; **N** - no, not supported; **WIP** - Work In Progress;
**TBD** - to be decided


+---------------------------------------------------------------------+------------+-------------+--------+----------+
|                                                                     | **Processor families**                       |
+---------------------------------------------------------------------+------------+-------------+--------+----------+
|                                                                     | **EM**     | **HS3x/4x** | **EV** | **HS6x** |
+---------------------------------------------------------------------+------------+-------------+--------+----------+
| Port status                                                         | upstreamed | upstreamed  | WIP    | WIP      |
+---------------------------------------------------------------------+------------+-------------+--------+----------+
| **Features**                                                                                                       |
+---------------------------------------------------------------------+------------+-------------+--------+----------+
| Closely coupled memories (ICCM, DCCM) [#f1]_                        | Y          | Y           | TBD    | TBD      |
+---------------------------------------------------------------------+------------+-------------+--------+----------+
| Execution with caches - Instruction/Data, L1/L2 caches              | Y          | Y           | Y      | Y        |
+---------------------------------------------------------------------+------------+-------------+--------+----------+
| Hardware-assisted unaligned memory access                           | Y [#f2]_   | Y           | TBD    | Y        |
+---------------------------------------------------------------------+------------+-------------+--------+----------+
| Regular interrupts with multiple priority levels, direct interrupts | Y          | Y           | TBD    | Y        |
+---------------------------------------------------------------------+------------+-------------+--------+----------+
| Fast interrupts, separate register banks for fast interrupts        | Y          | Y           | TBD    | N        |
+---------------------------------------------------------------------+------------+-------------+--------+----------+
| Hardware floating point unit (FPU)                                  | Y          | Y           | N      | N        |
+---------------------------------------------------------------------+------------+-------------+--------+----------+
| Symmetric multiprocessing (SMP) support, switch-based               | N/A        | Y           | TBD    | WIP      |
+---------------------------------------------------------------------+------------+-------------+--------+----------+
| Hardware-assisted stack checking                                    | Y          | Y           | TBD    | N        |
+---------------------------------------------------------------------+------------+-------------+--------+----------+
| Hardware-assisted atomic operations                                 | N/A        | Y           | TBD    | Y        |
+---------------------------------------------------------------------+------------+-------------+--------+----------+
| DSP ISA                                                             | Y          | N [#f3]_    | TBD    | TBD      |
+---------------------------------------------------------------------+------------+-------------+--------+----------+
| DSP AGU/XY extensions                                               | N [#f3]_   | N [#f3]_    | TBD    | TBD      |
+---------------------------------------------------------------------+------------+-------------+--------+----------+
| Userspace                                                           | Y          | N           | N      | N        |
+---------------------------------------------------------------------+------------+-------------+--------+----------+
| Memory protection unit (MPU)                                        | Y          | Y           | TBD    | N        |
+---------------------------------------------------------------------+------------+-------------+--------+----------+
| Memory management unit (MMU)                                        | N/A        | N           | N/A    | N        |
+---------------------------------------------------------------------+------------+-------------+--------+----------+
| SecureShield                                                        | Y          | N/A         | N/A    | N/A      |
+---------------------------------------------------------------------+------------+-------------+--------+----------+
| **Toolchains**                                                                                                     |
+---------------------------------------------------------------------+------------+-------------+--------+----------+
| GNU (open source GCC-based)                                         | Y          | Y           | N      | WIP      |
+---------------------------------------------------------------------+------------+-------------+--------+----------+
| MetaWare (proprietary Clang-based)                                  | Y          | Y           | Y      | Y        |
+---------------------------------------------------------------------+------------+-------------+--------+----------+
| **Simulators**                                                                                                     |
+---------------------------------------------------------------------+------------+-------------+--------+----------+
| QEMU (open source) [#f4]_                                           | Y          | Y           | N      | WIP      |
+---------------------------------------------------------------------+------------+-------------+--------+----------+
| nSIM (proprietary, provided by MetaWare Development Tools)          | Y          | Y           | Y      | Y        |
+---------------------------------------------------------------------+------------+-------------+--------+----------+

Notes
*****

.. [#f1] usage of CCMs is limited on SMP systems
.. [#f2] except the systems with secure features (SecureShield) due to HW
         limitation
.. [#f3] We only support save/restore ACCL/ACCH registers in task's context.
         Rest of DSP/AGU registers save/restore isn't implemented but kernel
         itself does not use these registers. This allows single task per
         core to use DSP/AGU safely.
.. [#f4] QEMU doesn't support all the ARC processor's HW features. For the
         detailed info please check the ARC QEMU documentation
