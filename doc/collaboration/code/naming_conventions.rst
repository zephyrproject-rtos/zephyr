.. _naming_conventions:

Code Style and Naming Conventions
#################################

Naming Conventions
******************

Unlike desktop operating systems, where applications are written in user-space
and drivers are used to cross the boundary between kernel and user space, *all*
applications in the Zephyr Kernel are written in kernel space. Applications are
linked with the kernel, creating a shared and common namespace.

To ensure proper execution of both kernel and applications, it makes sense to
divide the namespace into kernel and application subspaces. This is achieved
by restricting the kernel’s global symbols and macros to a well-defined set of
name prefixes. These prefixes apply both to public symbols, which applications
can reference, and to private symbols, which only the kernel itself is
permitted to reference. Symbols that do not begin with a kernel namespace
prefix are available to applications with a few exceptions. See `Exceptions
to the Namespace`_ for details.

+----------+--------------------------------------+------------------------+
| Prefix   | Description                          | Example                |
+==========+======================================+========================+
| \_       | Denotes a private symbol.            | ``_k_signal_event``    |
+----------+--------------------------------------+------------------------+
| atomic\_ | Denotes an atomic operation.         | ``atomic_inc``         |
+----------+--------------------------------------+------------------------+
| device\_ | Denotes an API relating to devices   | ``device_get_binding`` |
|          | and their initialization.            |                        |
+----------+--------------------------------------+------------------------+
| fiber\_  | Denotes an operation invoked by a    | ``fiber_event_send``   |
|          | fiber; typically a microkernel       |                        |
|          | operation.                           |                        |
+----------+--------------------------------------+------------------------+
| irq\_    | Denotes an IRQ management operation. | ``irq_disable``        |
+----------+--------------------------------------+------------------------+
| isr\_    | Denotes an operation called by an    | ``isr_event_send``     |
|          | Interrupt Service Routine; typically |                        |
|          | a microkernel operation.             |                        |
+----------+--------------------------------------+------------------------+
| k\_      | Microkernel-specific function.       | ``k_memcpy``           |
+----------+--------------------------------------+------------------------+
| k_do\_   | Microkernel-specific functions       | ``k_do_event_signal``  |
|          | indicating essential operation       |                        |
|          | within the kernel space. Do not use  |                        |
|          | these functions unless absolutely    |                        |
|          | necessary.                           |                        |
+----------+--------------------------------------+------------------------+
| nano\_   | Denotes an operation provided by the | ``nano_fifo_put``      |
|          | nanokernel; typically used in a      |                        |
|          | microkernel system, not just a       |                        |
|          | nanokernel system.                   |                        |
+----------+--------------------------------------+------------------------+
| sys\_    | Catch-all for APIs that do not fit   | ``sys_write32``        |
|          | into the other namespaces.           |                        |
+----------+--------------------------------------+------------------------+
| task\_   | Denotes an operation invoked by a    | ``task_send_event``    |
|          | task; typically a microkernel        |                        |
|          | operation.                           |                        |
+----------+--------------------------------------+------------------------+


If your additional symbol does not fall into the above classification, consider
renaming it.

Exceptions to the Namespace
===========================

Some kernel APIs use well-known names that lack prefixes. A few examples are:

* :code:`ntohl`

* :code:`open`

* :code:`close`

* :code:`read`

* :code:`write`

* :code:`ioctl`

In rare cases a few global symbols do not use the normal kernel prefixes;
 :cpp:func:`kernel_version_get()` is one such example.

Subsystem Naming Conventions
============================

Generally, any sub-system can define its own naming conventions for symbols.
However, these should be implemented with their own namespace prefix (for
example, ``bt\_`` for BlueTooth, or ``net\_`` for IP). This limits possible
clashes with applications. Following this prefix convention with subsystems
keeps a consistent interface for all users.

Minimize Include Paths
======================

The current build system uses a series of :file:`defs.objs` files to define the
common pieces for a given subsystem. For example, common defines for x86
architecture are located under :file:`$ROOT/arch/x86`, with platform-specific
defines underneath it, like :file:`$ROOT/arch/x86/platform/ia32`.
Be careful to not add all possible :literal:`include` paths to the
:file:`defs.obj` files. Too many default paths can cause problems when more than
one file has the same name. The only :literal:`include paths` into
:file:`${vBASE}/include` should be :file:`${vBASE}/include` itself, and the header
files should be included with:

.. code-block:: c

   #include <subdirectory/header.h>.

For example, if you have two files, :file:`include/pci.h` and
:file:`include/drvers/pci.h`, and have set both :option:`-Iinclude/drivers`
and :option:`-Iinclude` for your compile, then any code using

.. code-block:: c

   #include <pci.h> becomes ambiguous, while
   #include <drivers/pci.h>

is not. Not having :option:`-Iinclude/drivers` forces users to use the second
form which is more explicit.

.. include:: coding_style.rst
