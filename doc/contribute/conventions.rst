.. _naming_conventions:

Coding Style and Conventions
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

Generally, any subsystem can define its own naming conventions for symbols.
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

   #include <subdirectory/header.h>

For example, if you have two files, :file:`include/pci.h` and
:file:`include/drivers/pci.h`, and have set both ``-Iinclude/drivers``
and ``-Iinclude`` for your compile, then any code using

.. code-block:: c

   #include <pci.h>

becomes ambiguous, while

.. code-block:: c

   #include <drivers/pci.h>

is not. Not having ``-Iinclude/drivers`` forces users to use the second
form which is more explicit.

Return Codes
************

Zephyr uses the standard codes in :file:`errno.h` for all APIs.

As a general rule, ``0`` indicates success; a negative errno.h code indicates
an error condition. The table below shows the error code conventions based on
device driver use cases, but they can also be applied to other kernel
components.

+-----------------+------------------------------------------------+
| Code            | Meaning                                        |
+=================+================================================+
| 0               | Success.                                       |
+-----------------+------------------------------------------------+
| -EIO            | General failure.                               |
+-----------------+------------------------------------------------+
| -ENOTSUP        | Operation is not supported or operation is     |
|                 | invalid.                                       |
+-----------------+------------------------------------------------+
| -EINVAL         | Device configuration is not valid or function  |
|                 | argument is not valid.                         |
+-----------------+------------------------------------------------+
| -EBUSY          | Device controller is busy.                     |
+-----------------+------------------------------------------------+
| -EACCES         | Device controller is not accessible.           |
+-----------------+------------------------------------------------+
| -ENODEV         | Device type is not supported.                  |
+-----------------+------------------------------------------------+
| -EPERM          | Device is not configured or operation is not   |
|                 | permitted.                                     |
+-----------------+------------------------------------------------+
| -ENOSYS         | Function is not implemented.                   |
+-----------------+------------------------------------------------+

.. _coding_style:

Coding Style
************

Use this coding guideline to ensure that your development complies with
the project's style and naming conventions.

In general, follow the `Linux kernel coding style`_, with the following
exceptions:

* Add braces to every ``if`` and ``else`` body, even for single-line code
  blocks. Use the ``--ignore BRACES`` flag to make :program:`checkpatch`
  stop complaining.
* Use hard tab stops. Set the tab width 8 spaces. Break lines at 80 characters.
  If you are trying to align comments after declarations, use spaces instead of
  tabs to align them.
* Use C89-style single line comments, :literal:`/* */`. The C99-style
  single line comment, //, is not allowed.
* Use :literal:`/**  */` for any comments that need to appear in the
  documentation.

Checking for Conformity Using Checkpatch
========================================

The Linux kernel GPL-licensed tool :program:`checkpatch` is used to
check coding style conformity. :program:`Checkpatch` is available in the
scripts directory. To invoke it when committing code, edit your
:file:`.git/hooks/pre-commit` file to contain:

.. code-block:: bash

   #!/bin/sh

   set -e exec

   exec git diff --cached | ${ZEPHYR_BASE}/scripts/checkpatch.pl - || true

.. _Linux kernel coding style: https://www.kernel.org/doc/Documentation/CodingStyle
