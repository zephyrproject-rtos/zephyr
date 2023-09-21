.. _modules:

Modules
#######

The modules subsystem provides a toolbox for extending the functionality of an
application with loadable code.

The toolbox consists of an elf parsing library, a modules management
library, and a shell interface to manipulate modules.

A module may be loaded from any seekable byte stream that implements the
:ref:`struct module_stream` set of callbacks. An implementation over a buffer
in memory is available as :ref:`struct module_buf_stream`


API Reference
*************

.. doxygengroup:: modules

.. doxygengroup:: module_buf_stream

.. doxygengroup:: elf
