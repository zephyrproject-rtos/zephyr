Title: Intel S1000 CRB tests

Description:

This test illustrates the various features enabled on Intel S1000 CRB.

Features exhibited in this test set
============================

GPIO toggling
  - GPIO_23 configured as input
  - GPIO_24 configured as output and interrupt capable
  - GPIO_23 and GPIO_24 are shorted
  - Upon toggling GPIO_23, GPIO_24 also changes state appropriately and
    also calls its callback function if interrupt is configured

I2C slave communication
  - Intel S1000 CRB I2C configured as master, 7 bit mode, standard speed
  - 2 LED matrices are configured as slaves
  - The LED matrices are written over I2C to emit blue light and red
    light alternately
  - Read functionality verified by reading LED0 after every write and
    dumping the result on to the console

Interrupt handling
  - All peripheral interrupts are enabled by default
  - Each peripheral interrupt can be disabled by calling irq_disable().
    For e.g. GPIO IRQ can be disabled by calling "irq_disable(GPIO_DW_0_IRQ);"

UART prints
  - Displays the various prints dumped to the console by the above modules

---------------------------------------------------------------------------

Building and Running Project:

This project outputs to the console.  It can be built and executed
on Intel S1000 CRB using the flyswatter2 as follows:

    make flash

---------------------------------------------------------------------------

Troubleshooting:

Problems caused by out-dated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    make clean          # discard results of previous builds
                        # but keep existing configuration info
or
    make pristine       # discard results of previous builds
                        # and restore pre-defined configuration info

---------------------------------------------------------------------------

Sample Output:

***** BOOTING ZEPHYR OS v1.9.99-intel_internal - BUILD: Oct 31 2017 14:48:57 *****
Sample app running on: xtensa Intel S1000 CRB
Reading GPIO_24 = 0
LED0 = 10
GPIO_24 triggered
Reading GPIO_24 = 1
LED0 = 41
