Code_Naming_Conventions
#######################


The Purpose of Naming Conventions
*********************************

Unlike desktop operating systems, where applications are written in user-space
and drivers are used to cross the boundary between kernel and user space, all
applications on Tiny Mountain are written in kernel space.  These are then
linked against the kernel creating a shared and common namespace.

To ensure proper execution of both kernel and applications, it makes sense to
divide the namespace into kernel and application subspaces.  This is
achieved by restricting the kernel’s global symbols and macros to a well-defined
set of name prefixes.  These prefixes apply both to public symbols, which
applications can reference, and private symbols, which only the kernel itself
is permitted to reference.  Symbols that do not begin with a kernel namespace
prefix are available to applications with a few exceptions.  See `Exceptions to
the Namespace`_ for details.

+-------------------+---------------------------------------------------------+
| Prefix            | Description                                             |
+===================+=========================================================+
| \_                | Denotes a private kernel symbol (e.g. _k_signal_event). |
+-------------------+---------------------------------------------------------+
| atomic\_          | Denotes an atomic operation (e.g. atomic_inc).          |
+-------------------+---------------------------------------------------------+
| context\          | Denotes an operation invoked by a fiber or a task (e.g  |
|                   | context_type_get).                                      |
+-------------------+---------------------------------------------------------+
| fiber\_           | Denotes an operation invoked by a fiber; typically a    |
|                   | microkernel operation (e.g. fiber_event_send).          |
+-------------------+---------------------------------------------------------+
| irq\_             | Denotes an IRQ management operation (e.g. ireq_disable).|
+-------------------+---------------------------------------------------------+
| isr\_             | Denotes an operation called by an Interrupt Service     |
|                   | Routine; typically a microkernel operation (e.g.        |
|                   | isr_event_send).                                        |
+-------------------+---------------------------------------------------------+
| k\_               | Microkernel specific function (e.g. k_memcpy)           |
+-------------------+---------------------------------------------------------+
| k_do\_            | Microkernel specific functions that indicate essential  |
|                   | operation within the kernel space.  Do not use these    |
|                   | functions unless absolutely necessary.                  |
+-------------------+---------------------------------------------------------+
| nano\_            | Denotes an operation provided by the nanokernel;        |
|                   | typically may be used in a microkernel system, not just |
|                   | a nanokernel system (e.g. nano_fifo_put).               |
+-------------------+---------------------------------------------------------+
| sys\_             | Catch-all for APIs that do not fit into the other       |
|                   | namespaces.                                             |
+-------------------+---------------------------------------------------------+
| task\_            | Denotes an operation invoked by a task; typically a     |
|                   | microkernel operation (e.g. task_send_event).           |
+-------------------+---------------------------------------------------------+

If your additional symbol does not fall into the above classification, consider
renaming it.

Exceptions to the Namespace
***************************

Some kernel APIs use well known names that do not use the prefixes, for example:
ntohl, open, close, read, write, ioctl.

There are also a very limited and rare number of global symbols that do not use
the normal kernel prefixes; for example: kernel_version_get().

Subsystem Naming Conventions
****************************

In general any sub-system can define its own naming conventions for symbols.
However, they should be implemented using their own namespace prefix (for
example, bt\_ for BlueTooth, or net\_ for IP).  This limits possible clashes
with applications.  It is highly encouraged that any sub-system continue to
follow the above prefix conventions to keep a consistent interface for all
users.

Minimize Include Paths
**********************

The current build system uses a series of :file:`defs.objs` files to define
the common pieces for a given sub-system.  For example, there are common defines for
all architectures under :file:`$ROOT/arch/x86`, and then more specific
defines for each supported board underneath it, like
:file:`$ROOT/arch/x86/generic_pc`.

Do not add all possible include paths in the :file:`defs.obj` files.
Too many default paths cause problems when more than one file have the same
name.  The only include path into :file:`${vBASE}/include` should be
:file:`${vBASE}/include` itself, and the files should
code-block::#include <subdirectory/header.h>.

For example, if you have two files, :file:`include/pci.h` and
:file:`include/drvers/pci.h`, and have set both :option:`-Iinclude/drivers`
and :option:`-Iinclude` for your compile, then any code using
code-block::#include <pci.h> becomes ambiguous, while
code-block::#include <drivers/pci.h> is not.  Not having
:option:`-Iinclude/drivers` forces users to use the second form which is more
explicit.
