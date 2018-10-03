.. _syscalls:

System Calls
############
User threads run with a reduced set of privileges than supervisor threads:
certain CPU instructions may not be used, and they have access to only a
limited part of the memory map. System calls (may) allow user threads to
perform operations not directly available to them.

When defining system calls, it is very important to ensure that access to the
API's private data is done exclusively through system call interfaces.
Private kernel data should never be made available to user mode threads
directly. For example, the ``k_queue`` APIs were intentionally not made
available as they store bookkeeping information about the queue directly
in the queue buffers which are visible from user mode.

APIs that allow the user to register callback functions that run in
supervisor mode should never be exposed as system calls. Reserve these
for supervisor-mode access only.

This section describes how to declare new system calls and discusses a few
implementation details relevant to them.

Components
**********

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
***********

The C prototype represents how the API is invoked from either user or
supervisor mode. For example, to initialize a semaphore:

.. code-block:: c

    __syscall void k_sem_init(struct k_sem *sem, unsigned int initial_count,
                              unsigned int limit);

The :c:macro:`__syscall` attribute is very special. To the C compiler, it
simply expands to 'static inline'. However to the post-build
``parse_syscalls.py`` script, it indicates that this API is a system call.
The ``parse_syscalls.py`` script does some parsing of the function prototype,
to determine the data types of its return value and arguments, and has some
limitations:

* Array arguments must be passed in as pointers, not arrays. For example,
  ``int foo[]`` or ``int foo[12]`` is not allowed, but should instead be
  expressed as ``int *foo``.

* Function pointers horribly confuse the limited parser. The workaround is
  to typedef them first, and then express in the argument list in terms
  of that typedef.

* :c:macro:`__syscall` must be the first thing in the prototype.

The preprocessor is intentionally not used when determining the set of
system calls to generate. However, any generated system calls that don't
actually have a handler function defined (because the related feature is not
enabled in the kernel configuration) will instead point to a special handler
for unimplemented system calls. Data type definitions for APIs should not
have conditional visibility to the compiler.

Any header file that declares system calls must include a special generated
header at the very bottom of the header file. This header follows the
naming convention ``syscalls/<name of header file>``. For example, at the
bottom of ``include/sensor.h``:

.. code-block:: c

    #include <syscalls/sensor.h>

Invocation Context
==================

Source code that uses system call APIs can be made more efficient if it is
known that all the code inside a particular C file runs exclusively in
user mode, or exclusively in supervisor mode. The system will look for
the definition of macros :c:macro:`__ZEPHYR_SUPERVISOR__` or
:c:macro:`__ZEPHYR_USER__`, typically these will be added to the compiler
flags in the build system for the related files.

* If :option:`CONFIG_USERSPACE` is not enabled, all APIs just directly call
  the implementation function.

* Otherwise, the default case is to make a runtime check to see if the
  processor is currently running in user mode, and either make the system call
  or directly call the implementation function as appropriate.

* If :c:macro:`__ZEPHYR_SUPERVISOR__` is defined, then it is assumed that
  all the code runs in supervisor mode and all APIs just directly call the
  implementation function. If the code was actually running in user mode,
  there will be a CPU exception as soon as it tries to do something it isn't
  allowed to do.

* If :c:macro:`__ZEPHYR_USER__` is defined, then it is assumed that all the
  code runs in user mode and system calls are unconditionally made.

Implementation Details
======================

Declaring an API with :c:macro:`__syscall` causes some code to be generated in
C and header files by ``scripts/gen_syscalls.py``, all of which can be found in
the project out directory under ``include/generated/``:

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
In this example, the implementation of the :c:macro:`K_SYSCALL_DECLARE3_VOID()`
macro will be::

    #if !defined(CONFIG_USERSPACE) || defined(__ZEPHYR_SUPERVISOR__)

    #define K_SYSCALL_DECLARE3_VOID(id, name, t0, p0, t1, p1, t2, p2) \
            extern void _impl_##name(t0 p0, t1 p1, t2 p2); \
            static inline void name(t0 p0, t1 p1, t2 p2) \
            { \
                    _impl_##name(p0, p1, p2); \
            }

    #elif defined(__ZEPHYR_USER__)
    #define K_SYSCALL_DECLARE3_VOID(id, name, t0, p0, t1, p1, t2, p2) \
            static inline void name(t0 p0, t1 p1, t2 p2) \
            { \
                    _arch_syscall_invoke3((u32_t)p0, (u32_t)p1, (u32_t)p2, id); \
            }

    #else /* mixed kernel/user macros */
    #define K_SYSCALL_DECLARE3_VOID(id, name, t0, p0, t1, p1, t2, p2) \
            extern void _impl_##name(t0 p0, t1 p1, t2 p2); \
            static inline void name(t0 p0, t1 p1, t2 p2) \
            { \
                    if (_is_user_context()) { \
                            _arch_syscall_invoke3((u32_t)p0, (u32_t)p1, (u32_t)p2, id); \
                    } else { \
                            compiler_barrier(); \
                            _impl_##name(p0, p1, p2); \
                    } \
            }
    #endif

The header containing :c:macro:`K_SYSCALL_DECLARE3_VOID()` is itself
generated due to its repetitive nature and can be found in
``include/generated/syscall_macros.h``. It is created by
``scripts/gen_syscall_header.py``.

The final layer is the invocation of the system call itself. All architectures
implementing system calls must implement the seven inline functions
:c:func:`_arch_syscall_invoke0` through :c:func:`_arch_syscall_invoke6`.  These
functions marshal arguments into designated CPU registers and perform the
necessary privilege elevation. In this layer, all arguments are treated as an
unsigned 32-bit type. There is always a 32-bit unsigned return value, which
may or may not be used.

Some system calls may have more than six arguments. The number of arguments
passed via registers is fixed at six for all architectures. Additional
arguments will need to be passed in a struct, which needs to be treated as
untrusted memory in the handler function. This is done by the derived
functions :c:func:`_syscall_invoke7` through :c:func:`_syscall_invoke10`.

Some system calls may return a value that will not fit in a 32-bit register,
such as APIs that return a 64-bit value. In this scenario, the return value is
populated in a memory buffer that is passed in as an argument. For example,
see the implementation of :c:func:`_syscall_ret64_invoke0` and
:c:func:`_syscall_ret64_invoke1`.

Implementation Function
***********************

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
****************

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
===================

Several macros exist to validate arguments:

* :c:macro:`Z_SYSCALL_OBJ()` Checks a memory address to assert that it is
  a valid kernel object of the expected type, that the calling thread
  has permissions on it, and that the object is initialized.

* :c:macro:`Z_SYSCALL_OBJ_INIT()` is the same as
  :c:macro:`Z_SYSCALL_OBJ()`, except that the provided object may be
  uninitialized. This is useful for handlers of object init functions.

* :c:macro:`Z_SYSCALL_OBJ_NEVER_INIT()` is the same as
  :c:macro:`Z_SYSCALL_OBJ()`, except that the provided object must be
  uninitialized. This is not used very often, currently only for
  :c:func:`k_thread_create()`.

* :c:macro:`Z_SYSCALL_MEMORY_READ()` validates a memory buffer of a particular
  size. The calling thread must have read permissions on the entire buffer.

* :c:macro:`Z_SYSCALL_MEMORY_WRITE()` is the same as
  :c:macro:`Z_SYSCALL_MEMORY_READ()` but the calling thread must additionally
  have write permissions.

* :c:macro:`Z_SYSCALL_MEMORY_ARRAY_READ()` validates an array whose total size
  is expressed as separate arguments for the number of elements and the
  element size. This macro correctly accounts for multiplication overflow
  when computing the total size. The calling thread must have read permissions
  on the total size.

* :c:macro:`Z_SYSCALL_MEMORY_ARRAY_WRITE()` is the same as
  :c:macro:`Z_SYSCALL_MEMORY_ARRAY_READ()` but the calling thread must
  additionally have write permissions.

* :c:macro:`Z_SYSCALL_VERIFY_MSG()` does a runtime check of some boolean
  expression which must evaluate to true otherwise the check will fail.
  A variant :c:macro:`Z_SYSCALL_VERIFY` exists which does not take
  a message parameter, instead printing the expression tested if it
  fails. The latter should only be used for the most obvious of tests.

* :c:macro:`Z_SYSCALL_DRIVER_OP()` checks at runtime if a driver
  instance is capable of performing a particular operation.  While this
  macro can be used by itself, it's mostly a building block for macros
  that are automatically generated for every driver subsystem.  For
  instance, to validate the GPIO driver, one could use the
  :c:macro:`Z_SYSCALL_DRIVER_GPIO()` macro.

If any check fails, the macros will return a nonzero value. The macro
:c:macro:`Z_OOPS()` can be used to induce a kernel oops which will kill the
calling thread. This is done instead of returning some error condition to
keep the APIs the same when calling from supervisor mode.

Handler Declaration
===================

All handler functions have the same prototype:

.. code-block:: c

    u32_t _handler_<API name>(u32_t arg1, u32_t arg2, u32_t arg3,
                              u32_t arg4, u32_t arg5, u32_t arg6, void *ssf)

All handlers return a value. Handlers are passed exactly six arguments, which
were sent from user mode to the kernel via registers in the
architecture-specific system call implementation, plus an opaque context
pointer which indicates the system state when the system call was invoked from
user code.

To simplify the prototype, the variadic :c:macro:`Z_SYSCALL_HANDLER()` macro
should be used to declare the handler name and names of each argument. Type
information is not necessary since all arguments and the return value are
:c:type:`u32_t`. Using :c:func:`k_sem_init()` as an example:

.. code-block:: c

    Z_SYSCALL_HANDLER(k_sem_init, sem, initial_count, limit)
    {
        ...
    }

After validating all the arguments, the handler function needs to then call
the implementation function. If the implementation function returns a value,
this needs to be returned by the handler, otherwise the handler should return
0.

.. note:: Do not forget that all the arguments to the handler are passed in as
    unsigned 32-bit values.  If checks are needed on parameters that are
    actually signed values, casts may be needed in order for these checks to
    be performed properly.

Using :c:func:`k_sem_init()` as an example again, we need to enforce that the
semaphore object passed in is a valid semaphore object (but not necessarily
initialized), and that the limit parameter is nonzero:

.. code-block:: c

    Z_SYSCALL_HANDLER(k_sem_init, sem, initial_count, limit)
    {
        Z_OOPS(Z_SYSCALL_OBJ_INIT(sem, K_OBJ_SEM));
        Z_OOPS(Z_SYSCALL_VERIFY(limit != 0));
        _impl_k_sem_init((struct k_sem *)sem, initial_count, limit);
        return 0;
    }

Simple Handler Declarations
---------------------------

Many kernel or driver APIs have very simple handler functions, where they
either accept no arguments, or take one object which is a kernel object
pointer of some specific type. Some special macros have been defined for
these simple cases, with variants depending on whether the API has a return
value:

* :c:macro:`Z_SYSCALL_HANDLER1_SIMPLE()` one kernel object argument, returns
  a value
* :c:macro:`Z_SYSCALL_HANDLER1_SIMPLE_VOID()` one kernel object argument,
  no return value
* :c:macro:`Z_SYSCALL_HANDLER0_SIMPLE()` no arguments, returns a value
* :c:macro:`Z_SYSCALL_HANDLER0_SIMPLE_VOID()` no arguments, no return value

For example, :c:func:`k_sem_count_get()` takes a semaphore object as its
only argument and returns a value, so its handler can be completely expressed
as:

.. code-block:: c

    Z_SYSCALL_HANDLER1_SIMPLE(k_sem_count_get, K_OBJ_SEM, struct k_sem *);

System Calls With 6 Or More Arguments
=====================================

System calls may have more than six arguments, however the number of arguments
passed in via registers when the privilege elevation is invoked is fixed at six
for all architectures. In this case, the sixth and subsequent arguments to the
system call are placed into a struct, and a pointer to that struct is passed to
the handler as its sixth argument.

See ``include/syscall.h`` to see how this is done; the struct passed in must be
validated like any other memory buffer. For example, for a system call
with nine arguments, arguments 6 through 9 will be passed in via struct, which
must be verified since memory pointers from user mode can be incorrect or
malicious:

.. code-block:: c

    Z_SYSCALL_HANDLER(k_foo, arg1, arg2, arg3, arg4, arg5, more_args_ptr)
    {
        struct _syscall_9_args *margs = (struct _syscall_9_args *)more_args_ptr;

        Z_OOPS(Z_SYSCALL_MEMORY_READ(margs, sizeof(*margs)));

        ...

     }

It is also very important to note that arguments passed in this way can change
at any time due to concurrent access to the argument struct. If any parameters
are subject to enforcement checks, they need to be copied out of the struct and
only then checked. One way to ensure this isn't optimized out is to declare the
argument struct as ``volatile``, and copy values out of it into local variables
before checking. Using the previous example:

.. code-block:: c

    Z_SYSCALL_HANDLER(k_foo, arg1, arg2, arg3, arg4, arg5, more_args_ptr)
    {
        volatile struct _syscall_9_args *margs =
                        (struct _syscall_9_args *)more_args_ptr;
        int arg8;

        Z_OOPS(Z_SYSCALL_MEMORY_READ(margs, sizeof(*margs)));
        arg8 = margs->arg8;
        Z_OOPS(Z_SYSCALL_VERIFY_MSG(arg8 < 12, "arg8 must be less than 12"));

        _impl_k_foo(arg1, arg2, arg3, arg3, arg4, arg5, margs->arg6,
                    margs->arg7, arg8, margs->arg9);
        return 0;
     }


System Calls With 64-bit Return Value
=====================================

If a system call has a return value larger than 32-bits, the handler will not
return anything. Instead, a pointer to a sufficient memory region for the
return value will be passed in as an additional argument. As an example, we
have the system call for getting the current system uptime:

.. code-block:: c

    __syscall s64_t k_uptime_get(void);

The handler function has the return area passed in as a pointer, which must
be validated as writable by the calling thread:

.. code-block:: c

    Z_SYSCALL_HANDLER(k_uptime_get, ret_p)
    {
        s64_t *ret = (s64_t *)ret_p;

        Z_OOPS(Z_SYSCALL_MEMORY_WRITE(ret, sizeof(*ret)));
        *ret = _impl_k_uptime_get();
        return 0;
    }

Configuration Options
*********************

Related configuration options:

* :option:`CONFIG_USERSPACE`

APIs
****

Helper macros for creating system call handlers are provided in
:file:`kernel/include/syscall_handler.h`:

* :c:macro:`Z_SYSCALL_HANDLER()`
* :c:macro:`Z_SYSCALL_HANDLER1_SIMPLE()`
* :c:macro:`Z_SYSCALL_HANDLER1_SIMPLE_VOID()`
* :c:macro:`Z_SYSCALL_HANDLER0_SIMPLE()`
* :c:macro:`Z_SYSCALL_HANDLER0_SIMPLE_VOID()`
* :c:macro:`Z_SYSCALL_OBJ()`
* :c:macro:`Z_SYSCALL_OBJ_INIT()`
* :c:macro:`Z_SYSCALL_OBJ_NEVER_INIT()`
* :c:macro:`Z_OOPS()`
* :c:macro:`Z_SYSCALL_MEMORY_READ()`
* :c:macro:`Z_SYSCALL_MEMORY_WRITE()`
* :c:macro:`Z_SYSCALL_MEMORY_ARRAY_READ()`
* :c:macro:`Z_SYSCALL_MEMORY_ARRAY_WRITE()`
* :c:macro:`Z_SYSCALL_VERIFY_MSG()`
* :c:macro:`Z_SYSCALL_VERIFY`

Functions for invoking system calls are defined in
:file:`include/syscall.h`:

* :c:func:`_arch_syscall_invoke0`
* :c:func:`_arch_syscall_invoke1`
* :c:func:`_arch_syscall_invoke2`
* :c:func:`_arch_syscall_invoke3`
* :c:func:`_arch_syscall_invoke4`
* :c:func:`_arch_syscall_invoke5`
* :c:func:`_arch_syscall_invoke6`
* :c:func:`_syscall_invoke7`
* :c:func:`_syscall_invoke8`
* :c:func:`_syscall_invoke9`
* :c:func:`_syscall_invoke10`
* :c:func:`_syscall_ret64_invoke0`
* :c:func:`_syscall_ret64_invoke1`

