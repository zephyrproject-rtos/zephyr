.. _llext:

Linkable Loadable Extensions (LLEXT)
####################################

The llext subsystem provides a toolbox for extending the functionality of an
application at runtime with linkable loadable code.

Extensions can be loaded from precompiled ELF formatted data which is
verified, loaded, and linked with other extensions. Extensions can be
manipulated and introspected to some degree, as well as unloaded when no longer
needed.

An extension may be loaded using any implementation of a :c:struct:`llext_loader`
which has a set of function pointers that provide the necessary functionality
to read the ELF data. A loader also provides some minimal context (memory)
needed by the :c:func:`llext_load` function. An implementation over a buffer
containing an ELF in addressable memory in memory is available as
:c:struct:`llext_buf_loader`.

API Reference
*************

.. doxygengroup:: llext

.. doxygengroup:: llext_symbols

.. doxygengroup:: llext_loader

.. doxygengroup:: llext_buf_loader
