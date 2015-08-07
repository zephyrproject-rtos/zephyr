.. _nanokernel:

Nanokernel Services
###################

Section Scope
*************

This section provides an overview of the most important nanokernel
services. The information contained here is an aid to better understand
how the kernel operates at the nanokernel level.

Document Format
***************

Each service is broken off to its own section, containing a definition, a
functional description, the service initialization syntax, and a table
of APIs with the context which may call them. Please refer to the API documentation for further
details regarding the functionality of each service.

.. toctree::
   :maxdepth: 1

   nanokernel_fibers
   nanokernel_timers
   nanokernel_signaling
   nanokernel_data
   nanokernel_interrupts