.. zephyr:board:: kv260_a53

Overview
********
This board configuration targets the Application Processing Unit (APU) on the AMD/Xilinx KV260 development board. The APU consists of ARM Cortex‑A53 cores and provides a general‑purpose application execution environment.

This processing unit is based on an ARM Cortex-A53 CPU, it also enables the following devices:

* ARM Generic Timer (system timer)
* Xilinx Zynq UART
* SMP: disabled by default (enable as needed)

Devices
========
System Timer
------------

* Zephyr tick rate: CONFIG_SYS_CLOCK_TICKS_PER_SEC.
* Hardware timer frequency: set via CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC in the defconfig.
* The ARM Generic Timer is used.

Serial Port
-----------

* This board configuration uses a single serial communication channel with the on-chip UART1.
* baud rate: 115200 (ensure DTS chosen/aliases align with the UART chosen for console).

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

To build a Zephyr application, such as the hello world sample shown below.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: kv260_a53
   :goals: build

To confirm a Zephyr application, it is able to use u-boot command.

.. code-block:: console

        ZynqMP>
        ZynqMP> dcache off
        ZynqMP> icache off
        ZynqMP> setenv loadaddr 0x8000000
        ZynqMP> fatload mmc 1:1 ${loadaddr} zephyr.elf
        1212792 bytes read in 330 ms (3.5 MiB/s)
        ZynqMP> bootelf ${loadaddr}
        ## Star*** Booting Zephyr OS build v4.2.0-5452-g169cf86969d7 ***

Notes
*************************

* The mmc device/partition (e.g., 1:1) and loadaddr may vary depending on the platform setup; adjust as needed.

