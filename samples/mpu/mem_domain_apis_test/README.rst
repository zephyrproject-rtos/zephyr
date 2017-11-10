.. _mem_domain_apis_test:

Memory Domain APIs Test
#######################

Overview
********

This is a simple application that demonstrates the usage of Memory Domain APIs.
Two memory domains (app0_domain and app1_domain) and three application threads
(app thread 0, 1, 2) are created by the main thread.
These threads will yield their execution alternately and we can observe the
changes of domain partition regions during context switch via the serial console.

Building and Running
********************

This project can be built and executed as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/mpu/mem_domain_apis_test
   :board: frdm_k64f
   :goals: build flash
   :compact:

Reset the board and you will see messages on the corresponding
Serial Port.

Sample Output
=============

In the beginning, main thread adds app threads into specific memory domains.

.. code-block:: console

   ***** BOOTING ZEPHYR OS v1.9.0 - BUILD: Sep 20 2017 11:07:53 *****
   initialize memory domains
   add app thread 0 (0x20000280) into app0_domain.
   add app thread 1 (0x200002f8) into app1_domain.
   add app thread 2 (0x20000370) into app0_domain.

After main thread yields its execution, we can see app thread 0 starts and the
domain partition regions are set according to the memory partitions of app0_domain.

.. code-block:: console

   main thread (0x200000c0) yields.
   [general] [DBG] arm_core_mpu_configure: Region info: 0x20000600 0x400
   [general] [DBG] _region_init: [4] 0x20000000 0x200005ff 0x0079e79e 0x00000001
   [general] [DBG] _region_init: [7] 0x20000600 0x2000061f 0x0071c71c 0x00000001
   [general] [DBG] _region_init: [11] 0x20000620 0x2002ffff 0x0079e79e 0x00000001
   [general] [DBG] configure_mpu_mem_domain: configure thread 0x20000280's domain
   [general] [DBG] arm_core_mpu_configure_mem_domain: configure domain: 0x20000440
   [general] [DBG] arm_core_mpu_configure_mem_domain: set region 0x8 0x20000420 0x20
   [general] [DBG] _region_init: [8] 0x20000420 0x2000043f 0x0079e79e 0x00000001
   [general] [DBG] arm_core_mpu_configure_mem_domain: set region 0x9 0x20000400 0x20
   [general] [DBG] _region_init: [9] 0x20000400 0x2000041f 0x00514514 0x00000001
   [general] [DBG] arm_core_mpu_configure_mem_domain: disable region 0xa
   app thread 0 (0x20000280) starts.

Then, app thread 0 yields its execution and app thread 1 is going to be switched in.
We can see the domain partition regions are set according to the memory partitions of
app1_domain.

.. code-block:: console

   app thread 0 (0x20000280) yields.
   [general] [DBG] arm_core_mpu_configure: Region info: 0x20000a20 0x400
   [general] [DBG] _region_init: [4] 0x20000000 0x20000a1f 0x0079e79e 0x00000001
   [general] [DBG] _region_init: [7] 0x20000a20 0x20000a3f 0x0071c71c 0x00000001
   [general] [DBG] _region_init: [11] 0x20000a40 0x2002ffff 0x0079e79e 0x00000001
   [general] [DBG] configure_mpu_mem_domain: configure thread 0x200002f8's domain
   [general] [DBG] arm_core_mpu_configure_mem_domain: configure domain: 0x2000050c
   [general] [DBG] arm_core_mpu_configure_mem_domain: set region 0x8 0x20000400 0x20
   [general] [DBG] _region_init: [8] 0x20000400 0x2000041f 0x0079e79e 0x00000001
   [general] [DBG] arm_core_mpu_configure_mem_domain: set region 0x9 0x20000420 0x20
   [general] [DBG] _region_init: [9] 0x20000420 0x2000043f 0x00514514 0x00000001
   [general] [DBG] arm_core_mpu_configure_mem_domain: disable region 0xa
   app thread 1 (0x200002f8) starts.
