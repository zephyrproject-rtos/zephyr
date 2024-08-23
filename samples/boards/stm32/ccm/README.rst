.. _samples_boards_stm32_ccm:

STM32 CCM example
#################

Overview
********

Show usage of the Core Coupled Memory (CCM) that is available
on several STM32 devices. The very important difference with
normal RAM is that CCM can not be used for DMA.

By prefixing a variable with __ccm_data_section, __ccm_bss_section,
or __ccm_noinit_section those variables are placed in the CCM.

The __ccm_data_section prefix should be used for variables that
are initialized. Like the normal data section the initial
values take up space in the FLASH image.

The __ccm_bss_section prefix should be used for variables that
should be initialized to 0. Like the normal bss section they
do not take up FLASH space.

The __ccm_noinit_section prefix should be used for variables
that don't need to have a defined initial value (for example
buffers that will receive data). Compared to bss or data the
kernel does not need to initialize the noinit section making
the startup slightly faster.

To add CCM support to a board, add the following line to the
board's DTS file ``chosen`` section:

.. code-block:: console

   zephyr,ccm = &ccm0;

For example the olimex STM32 E407 DTS file looks like this:

.. literalinclude:: ../../../../boards/olimex/stm32_e407/olimex_stm32_e407.dts
   :linenos:

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/stm32/ccm
   :goals: build flash

The first time the example is run after power on, the output will
look like this:

.. code-block:: console

   ***** BOOTING ZEPHYR OS v1.10.99 - BUILD: Jan 14 2018 09:32:46 *****

   CCM (Core Coupled Memory) usage example

   The total used CCM area   : [0x10000000, 0x10000021)
   Zero initialized BSS area : [0x10000000, 0x10000007)
   Uninitialized NOINIT area : [0x10000008, 0x10000013)
   Initialised DATA area     : [0x10000014, 0x10000021)
   Start of DATA in FLASH    : 0x08003940

   Checking initial variable values: ... PASSED

   Initial variable values:
   ccm_data_var_8  addr: 0x10000014 value: 0x12
   ccm_data_var_16 addr: 0x10000016 value: 0x3456
   ccm_data_var_32 addr: 0x10000018 value: 0x789abcde
   ccm_data_array  addr: 0x1000001c size: 5 value:
           0x11 0x22 0x33 0x44 0x55
   ccm_bss_array addr: 0x10000000 size: 7 value:
           0x00 0x00 0x00 0x00 0x00 0x00 0x00
   ccm_noinit_array addr: 0x10000008 size: 11 value:
           0xa9 0x99 0xba 0x90 0xe1 0x2a 0xba 0x93 0x4c 0xfe 0x4b

   Variable values after writing:
   ccm_data_var_8  addr: 0x10000014 value: 0xed
   ccm_data_var_16 addr: 0x10000016 value: 0xcba9
   ccm_data_var_32 addr: 0x10000018 value: 0x87654321
   ccm_data_array  addr: 0x1000001c size: 5 value:
           0xaa 0xaa 0xaa 0xaa 0xaa
   ccm_bss_array addr: 0x10000000 size: 7 value:
           0xbb 0xbb 0xbb 0xbb 0xbb 0xbb 0xbb
   ccm_noinit_array addr: 0x10000008 size: 11 value:
           0xcc 0xcc 0xcc 0xcc 0xcc 0xcc 0xcc 0xcc 0xcc 0xcc 0xcc

   Example end


First, each CCM section is listed with its address and size. Next, some usage
examples are shown. Note that the ``noinit`` section holds variables with
uninitialized data. After writing to the variables, they all should hold the
values shown above.

When the board is reset (without power-cycling), the output looks like this:

.. code-block:: console

   ***** BOOTING ZEPHYR OS v1.10.99 - BUILD: Jan 14 2018 09:32:46 *****

   CCM (Core Coupled Memory) usage example

   The total used CCM area   : [0x10000000, 0x10000021)
   Zero initialized BSS area : [0x10000000, 0x10000007)
   Uninitialized NOINIT area : [0x10000008, 0x10000013)
   Initialised DATA area     : [0x10000014, 0x10000021)
   Start of DATA in FLASH    : 0x08003940

   Checking initial variable values: ... PASSED

   Initial variable values:
   ccm_data_var_8  addr: 0x10000014 value: 0x12
   ccm_data_var_16 addr: 0x10000016 value: 0x3456
   ccm_data_var_32 addr: 0x10000018 value: 0x789abcde
   ccm_data_array  addr: 0x1000001c size: 5 value:
           0x11 0x22 0x33 0x44 0x55
   ccm_bss_array addr: 0x10000000 size: 7 value:
           0x00 0x00 0x00 0x00 0x00 0x00 0x00
   ccm_noinit_array addr: 0x10000008 size: 11 value:
           0xcc 0xcc 0xcc 0xcc 0xcc 0xcc 0xcc 0xcc 0xcc 0xcc 0xcc

   Variable values after writing:
   ccm_data_var_8  addr: 0x10000014 value: 0xed
   ccm_data_var_16 addr: 0x10000016 value: 0xcba9
   ccm_data_var_32 addr: 0x10000018 value: 0x87654321
   ccm_data_array  addr: 0x1000001c size: 5 value:
           0xaa 0xaa 0xaa 0xaa 0xaa
   ccm_bss_array addr: 0x10000000 size: 7 value:
           0xbb 0xbb 0xbb 0xbb 0xbb 0xbb 0xbb
   ccm_noinit_array addr: 0x10000008 size: 11 value:
           0xcc 0xcc 0xcc 0xcc 0xcc 0xcc 0xcc 0xcc 0xcc 0xcc 0xcc

   Example end


The difference with the first run is that the ccm_noinit section still has the
values from the last write. It is important to notice that this is not guaranteed,
it still should be considered uninitialized leftover data.
