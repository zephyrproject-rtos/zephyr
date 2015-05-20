Object Documentation
####################

Use this information to understand how the different kernel objects of
Tiny Mountain function. The purpose of this section is to help you
understand the most important object of the operating system. In order
to help you navigate through the content, we have divided the objects
in :ref:`basicObjects`, :ref:`nanokernelObjects` and
:ref:`microkernelObjects` objects.

We strongly recommend that you start with the :ref:`basicObjects` before
moving on to the :ref:`nanokernelObjects` or the
:ref:`microkernelObjects`. Additionally, we have included some
:ref:`driverExamples` for better comprehension of the objects' function.

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

.. toctree:: Table of Contents
   :maxdepth: 2

   object_basic.rst
   object_microkernel.rst
   object_nanokernel.rst