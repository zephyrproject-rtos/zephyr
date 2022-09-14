.. _intel_adsp_code_relocation:

Intel ADSP CAVS Code relocation
###############################

Overview
********

A simple sample that shows code relocation working for Intel ADSP CAVS
boards (v18 and v25). The interesting bit is the custom linker file.
As rimage (the tool used to sign the image) mandates that elf files
sections TEXT, DATA and BSS be contiguous, some work is done in the
linker script to ensure that.


Building and Running
********************

This application can be built and executed as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: intel_adsp/cavs25
   :goals: build
   :compact:


Sample Output
=============

.. code-block:: console

    main location: 0xbe0105e4
    Calling relocated code
    Relocated code! reloc location 0xbe008010
    maybe_bss location: 0x9e004218 maybe_bss value: 0
