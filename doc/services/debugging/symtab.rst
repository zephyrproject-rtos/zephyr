.. _symtab:

Symbol Table (Symtab)
#####################

The Symtab module, when enabled, will generate full symbol table during the Zephyr linking
stage that keep tracks of the information about the functions' name and address, for advanced application
with a lot of functions, this is expected to consume a sizable amount of ROM.

Currently, this is being used to look up the function names during a stack trace in supported architectures.


Usage
*****

Application can gain access to the symbol table data structure by including the :file:`symtab.h` header
file and call :c:func:`symtab_get`. For now, we only provide :c:func:`symtab_find_symbol_name`
function to look-up the symbol name and offset of an address. More advanced functionalities and be
achieved by directly accessing the members of the data structure.

Configuration
*************

Configure this module using the following options.

* :kconfig:option:`CONFIG_SYMTAB`: enable the generation of the symbol table.

API documentation
*****************

.. doxygengroup:: symtab_apis
