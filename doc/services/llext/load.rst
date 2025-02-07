Loading extensions
##################

Once an extension is built and the ELF file is available, it can be loaded into
the Zephyr application using the LLEXT API, which provides a way to load the
extension into memory, access its symbols and call its functions.

Loading an extension
====================

An extension may be loaded using any implementation of a :c:struct:`llext_loader`
which has a set of function pointers that provide the necessary functionality
to read the ELF data. A loader also provides some minimal context (memory)
needed by the :c:func:`llext_load` function. An implementation over a buffer
containing an ELF in addressable memory in memory is available as
:c:struct:`llext_buf_loader`.

The extensions are loaded with a call to the :c:func:`llext_load` function,
passing in the extension name and the configured loader. Once that completes
successfully, the extension is loaded into memory and is ready to be used.

.. note::
   When :ref:`User Mode <usermode_api>` is enabled, the extension will not be
   included in any user memory domain. To allow access from user mode, the
   :c:func:`llext_add_domain` function must be called.

Initializing and cleaning up the extension
==========================================

The extension may define a number of initialization functions that must be
called after loading but before any function in it can be used; this is typical
in languages such as C++ that provide the concept of object constructors. The
same is true for cleanup functions that must be called before unloading the
extension.

LLEXT supports calling the functions listed in the ``.preinit_array`` and
``.init_array`` sections of the ELF file with the :c:func:`llext_bringup`
function, and the functions listed in the ``.fini_array`` section with the
:c:func:`llext_teardown` function. These APIs are compatible with
:ref:`User Mode <usermode_api>`, and thus can be called from either kernel or
user context.

.. important::
   The code run by these functions is fully determined by the contents of the
   ELF file. This may have security implications if its origin is untrusted.

If the extension requires a dedicated thread, the :c:func:`llext_bootstrap`
function can be used to minimize boilerplate code. This function has a
signature that is compatible with the :c:func:`k_thread_create` API, and will
call :c:func:`llext_bringup`, then a user-specified function in the same
context, and finally :c:func:`llext_teardown` before returning.

Accessing code and data
=======================

To interact with the newly loaded extension, the host application must use the
:c:func:`llext_find_sym` function to get the address of the exported symbol.
The returned ``void *`` can then be cast to the appropriate type and used.

A wrapper for calling a function with no arguments is provided in
:c:func:`llext_call_fn`.

Advanced users that need direct access to areas of the newly loaded extension
may want to refer to :c:func:`llext_get_section_info` and other LLEXT
inspection APIs.

Cleaning up after use
=====================

The :c:func:`llext_unload` function must be called to free the memory used by
the extension once it is no longer required. After this call completes, all
pointers to symbols in the extension that were obtained will be invalid.

Troubleshooting
###############

This feature is being actively developed and as such it is possible that some
issues may arise. Since linking does modify the binary code, in case of errors
the results are difficult to predict. Some common issues may be:

* Results from :c:func:`llext_find_sym` point to an invalid address;

* Constants and variables defined in the extension do not have the expected
  values;

* Calling a function defined in an extension results in a hard fault, or memory
  in the main application is corrupted after returning from it.

If any of this happens, the following tips may help understand the issue:

* Make sure :kconfig:option:`CONFIG_LLEXT_LOG_LEVEL` is set to ``DEBUG``, then
  obtain a log of the :c:func:`llext_load` invocation.

* If possible, disable memory protection (MMU/MPU) and see if this results in
  different behavior.

* Try to simplify the extension to the minimum possible code that reproduces
  the issue.

* Use a debugger to inspect the memory and registers to try to understand what
  is happening. See :ref:`Debugging extensions <llext_debug>` for more details.

If the issue persists, please open an issue in the GitHub repository, including
all the above information.
