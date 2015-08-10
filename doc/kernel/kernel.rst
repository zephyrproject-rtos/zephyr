.. _zephyr_kernel:

The Zephyr Kernel
#################

The purpose of this section is to help you understand the operation of the kernel and its features.
This section also provides detailed information regarding the microkernel and the nanokernel
services. Finally, you can find the information regarding the kernel's drivers and networking
capabilities.

This section does not replace the Application Program Interface
documentation, rather, it offers additional information. The examples should provide
you with enough insight to understand the functionality of the services but are not
meant to replace the detailed in-code documentation.

.. toctree::
   :maxdepth: 2

   overview/overview.rst
   common/common.rst
   microkernel/microkernel.rst
   nanokernel/nanokernel.rst
   drivers/drivers.rst
   networking/networking.rst

.. rubric:: Abbreviations

+---------------+-------------------------------------------------------------------+
| Abbreviations | Definition                                                        |
+===============+===================================================================+
| API           | Application Program Interface: typically a defined set            |
|               | of routines and protocols for building software inputs and output |
|               | mechanisms.                                                       |
+---------------+-------------------------------------------------------------------+
| ISR           | Interrupt Service Routine                                         |
+---------------+-------------------------------------------------------------------+
| IDT           | Interrupt Descriptor Table                                        |
+---------------+-------------------------------------------------------------------+
| XIP           | eXecute In Place                                                  |
+---------------+-------------------------------------------------------------------+
