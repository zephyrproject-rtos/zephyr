.. _syscalls:

System Calls
############
User threads run with a reduced set of privileges than supervisor threads:
certain CPU instructions may not be used, and they have access to only a
limited part of the memory map. System calls (may) allow user threads to
perform operations not directly available to them.

This section describes how to declare new system calls and discusses a few
implementation details relevant to them.

Components
==========

All system calls have the following components:

* A **C prototype** for the API, declared in some header under ``include/`` and
  prefixed with :c:macro:`__syscall`.  This prototype is never implemented
  manually, instead it gets created by the ``scripts/gen_syscalls.py`` script.
  What gets generated is an inline function which either calls the
  implementation function directly (if called from supervisor mode) or goes
  through privilege elevation and validation steps (if called from user
  mode).

* An **implementation function**, which is the real implementation of the
  system call. The implementation function may assume that all parameters
  passed in have been validated if it was invoked from user mode.

* A **handler function**, which wraps the implementation function and does
  validation of all the arguments passed in.

C Prototype
===========

The C prototype represents how the API is invoked from either user or
supervisor mode. For example, to initialize a semaphore:

.. code-block:: c

    __syscall void k_sem_init(struct k_sem *sem, unsigned int initial_count,
                              unsigned int limit);

The :c:macro:`__syscall` attribute is very special. To the C compiler, it
simply expands to 'static inline'. However to the post-build
``gen_syscalls.py`` script, it indicates that this API is a system call and
generates the body of the function.  The ``gen_syscalls.py`` script does some
parsing of the function prototype, to determine the data types of its return
value and arguments, and has some limitations:

* Array arguments must be passed in as pointers, not arrays. For example,
  ``int foo[]`` or ``int foo[12]`` is not allowed, but should instead be
  expressed as ``int *foo``.

* Function pointers horribly confuse the limited parser. The workaround is
  to typedef them first, and then express in the argument list in terms
  of that typedef.

* :c:macro:`__syscall` must be the first thing in the prototype.

Any header file that declares system calls must include a special generated
header at the very bottom of the header file. This header follows the
naming convention ``syscalls/<name of header file>``. For example, at the
bottom of ``include/sensor.h``:

.. code-block:: c

    #include <syscalls/sensor.h>

Implementation Details
----------------------

Declaring an API with :c:macro:`__syscall` causes some code to be generated in
C and header files, all of which can be found in the project out directory
under ``include/generated/``:

* The system call is added to the enumerated type of system call IDs,
  which is expressed in ``include/generated/syscall_list.h``. It is the name
  of the API in uppercase, prefixed with ``K_SYSCALL_``.

* A prototype for the handler function is also created in
  ``include/generated/syscall_list.h``

* An entry for the system call is created in the dispatch table
  ``_k_sycall_table``, expressed in ``include/generated/syscall_dispatch.c``

* A weak handler function is declared, which is just an alias of the
  'unimplemented system call' handler. This is necessary since the real
  handler function may or may not be built depending on the kernel
  configuration. For example, if a user thread makes a sensor subsystem
  API call, but the sensor subsystem is not enabled, the weak handler
  will be invoked instead.

The body of the API is created in the generated system header. Using the
example of :c:func:`k_sem_init()`, this API is declared in
``include/kernel.h``. At the bottom of ``include/kernel.h`` is::

    #include <syscalls/kernel.h>

Inside this header is the body of :c:func:`k_sem_init()`::

    K_SYSCALL_DECLARE3_VOID(K_SYSCALL_K_SEM_INIT, k_sem_init, struct k_sem *,
                            sem, unsigned int, initial_count,
                            unsigned int, limit);

This generates an inline function that takes three arguments with void
return value. Depending on context it will either directly call the
implementation function or go through a system call elevation. A
prototype for the implementation function is also automatically generated.

The header containing :c:macro:`K_SYSCALL_DECLARE3_VOID()` is itself
generated due to its repetitive nature and can be found in
``include/generated/syscall_macros.h``.

Implementation Function
=======================

The implementation function is what actually does the work for the API.
Zephyr normally does little to no error checking of arguments, or does this
kind of checking with assertions. When writing the implementation function,
validation of any parameters is optional and should be done with assertions.

All implementation functions must follow the naming convention, which is the
name of the API prefixed with ``_impl_``. Implementation functions may be
declared in the same header as the API as a static inline function or
declared in some C file. There is no prototype needed for implementation
functions, these are automatically generated.

Handler Function
================

The handler function runs on the kernel side when a user thread makes
a system call. When the user thread makes a software interrupt to elevate to
supervisor mode, the common system call entry point uses the system call ID
provided by the user to look up the appropriate handler function for that
system call and jump into it.

Handler functions only run when system call APIs are invoked from user mode.
If an API is invoked from supervisor mode, the implementation is simply called.

The purpose of the handler function is to validate all the arguments passed in.
This includes:

* Any kernel object pointers provided. For example, the semaphore APIs must
  ensure that the semaphore object passed in is a valid semaphore and that
  the calling thread has permission on it.

* Any memory buffers passed in from user mode. Checks must be made that the
  calling thread has read or write permissions on the provided buffer.

* Any other arguments that have a limited range of valid values.

Handler functions involve a great deal of boilerplate code which has been
made simpler by some macros in ``kernel/include/syscall_handlers.h``.
Handler functions should be declared using these macros.

Argument Validation
-------------------

Several macros exist to validate arguments:

* :c:macro:`_SYSCALL_OBJ()` Checks a memory address to assert that it is
  a valid kernel object of the expected type, that the calling thread
  has permissions on it, and that the object is initialized.

* :c:macro:`_SYSCALL_OBJ_INIT()` is the same as
  :c:macro:`_SYSCALL_OBJ()`, except that the provided object may be
  uninitialized. This is useful for handlers of object init functions.

* :c:macro:`_SYSCALL_OBJ_NEVER_INIT()` is the same as
  :c:macro:`_SYSCALL_OBJ()`, except that the provided object must be
  uninitialized. This is not used very often, currently only for
  :c:func:`k_thread_create()`.

* :c:macro:`_SYSCALL_MEMORY_READ()` validates a memory buffer of a particular
  size. The calling thread must have read permissions on the entire buffer.

* :c:macro:`_SYSCALL_MEMORY_WRITE()` is the same as
  :c:macro:`_SYSCALL_MEMORY_READ()` but the calling thread must additionally
  have write permissions.

* :c:macro:`_SYSCALL_MEMORY_ARRAY_READ()` validates an array whose total size
  is expressed as separate arguments for the number of elements and the
  element size. This macro correctly accounts for multiplication overflow
  when computing the total size. The calling thread must have read permissions
  on the total size.

* :c:macro:`_SYSCALL_MEMORY_ARRAY_WRITE()` is the same as
  :c:macro:`_SYSCALL_MEMORY_ARRAY_READ()` but the calling thread must
  additionally have write permissions.

* :c:macro:`_SYSCALL_VERIFY_MSG()` does a runtime check of some boolean
  expression which must evaluate to true otherwise the check will fail.
  A variant :c:macro:`_SYSCALL_VERIFY` exists which does not take
  a message parameter, instead printing the expression tested if it
  fails. The latter should only be used for the most obvious of tests.

If any check fails, a kernel oops will be triggered which will kill the
calling thread. This is done instead of returning some error condition to
keep the APIs the same when calling from supervisor mode.

Handler Declaration
-------------------

All handler functions have the same prototype:

.. code-block:: c

    u32_t _handler_<API name>(u32_t arg1, u32_t arg2, u32_t arg3,
                              u32_t arg4, u32_t arg5, u32_t arg6, void *ssf)

All handlers return a value. Handlers are passed exactly six arguments, which
were sent from user mode to the kernel via registers in the
architecture-specific system call implementation, plus an opaque context
pointer which indicates the system state when the system call was invoked from
user code.

To simplify the prototype, the variadic :c:macro:`_SYSCALL_HANDLER()` macro
should be used to declare the handler name and names of each argument. Type
information is not necessary since all arguments and the return value are
:c:type:`u32_t`. Using :c:func:`k_sem_init()` as an example:

.. code-block:: c

    _SYSCALL_HANDLER(k_sem_init, sem, initial_count, limit)
    {
        ...
    }

Note that system calls may have more than six arguments. In this case,
the sixth and subsequent arguments to the system call are placed into a struct,
and a pointer to that struct is passed to the handler as its sixth argument.
See ``include/syscall.h`` to see how this is done; the struct passed in must be
validated like any other memory buffer.

After validating all the arguments, the handler function needs to then call
the implementation function. If the implementation function returns a value,
this needs to be returned by the handler, otherwise the handler should return
0.

Using :c:func:`k_sem_init()` as an example again, we need to enforce that the
semaphore object passed in is a valid semaphore object (but not necessarily
initialized), and that the limit parameter is nonzero:

.. code-block:: c

    _SYSCALL_HANDLER(k_sem_init, sem, initial_count, limit)
    {
        _SYSCALL_OBJ_INIT(sem, K_OBJ_SEM);
        _SYSCALL_VERIFY(limit != 0);
        _impl_k_sem_init((struct k_sem *)sem, initial_count, limit);
        return 0;
    }
