.. _microkernel:

Microkernel Services
####################

Section Scope
*************

This section provides an overview of the most important microkernel
services, and their operation.

Each service contains a definition, a function description, and a table
of :abbr:`APIs (Application Program Interfaces)` including the context that may
call them. This section does not replace the Application Program Interface
documentation but rather complements it. The examples should provide
you with enough insight to understand the functionality but are not
meant to replace the detailed in-code documentation. Please refer to the API documentation for
further details regarding each object's functionality.

Services Documentation
*********************

.. toctree::
   :maxdepth: 1

   microkernel_tasks
   microkernel_timers
   microkernel_memory
   microkernel_signaling
   microkernel_data
   microkernel_interrupts