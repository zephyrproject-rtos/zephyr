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

* A **verification function**, which wraps the implementation function
  and does validation of all the arguments passed in.

* An **unmarshalling function**, which is an automatically generated
  handler that must be included by user source code.

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

* An entry for the system call is created in the dispatch table
  ``_k_sycall_table``, expressed in ``include/generated/syscall_dispatch.c``

* A weak handler function is declared, which is just an alias of the
  'unimplemented system call' handler. This is necessary since the real
  handler function may or may not be built depending on the kernel
  configuration. For example, if a user thread makes a sensor subsystem
  API call, but the sensor subsystem is not enabled, the weak handler
  will be invoked instead.

* An unmarshalling function is defined in ``include/generated/<name>_mrsh.c``

The body of the API is created in the generated system header. Using the
example of :c:func:`k_sem_init()`, this API is declared in
``include/kernel.h``. At the bottom of ``include/kernel.h`` is::

    #include <syscalls/kernel.h>

Inside this header is the body of :c:func:`k_sem_init()`::

    static inline void k_sem_init(struct k_sem * sem, unsigned int initial_count, unsigned int limit)
    {
    #ifdef CONFIG_USERSPACE
            if (z_syscall_trap()) {
                    z_arch_syscall_invoke3(*(u32_t *)&sem, *(u32_t *)&initial_count, *(u32_t *)&limit, K_SYSCALL_K_SEM_INIT);
                    return;
            }
            compiler_barrier();
    #endif
            z_impl_k_sem_init(sem, initial_count, limit);
    }

This generates an inline function that takes three arguments with void
return value. Depending on context it will either directly call the
implementation function or go through a system call elevation. A
prototype for the implementation function is also automatically generated.

The final layer is the invocation of the system call itself. All architectures
implementing system calls must implement the seven inline functions
:c:func:`_arch_syscall_invoke0` through :c:func:`_arch_syscall_invoke6`.  These
functions marshal arguments into designated CPU registers and perform the
necessary privilege elevation. In this layer, all arguments are treated as an
unsigned 32-bit type. There is always a 32-bit unsigned return value, which
may or may not be used.

Some system calls may have more than six arguments. The number of
arguments passed via registers is limited to six for all
architectures. Additional arguments will need to be passed in an array
in the source memory space, which needs to be treated as untrusted
memory in the handler function. This code (packing, unpacking and
validation) is generated automatically as needed in the stub above and
in the unmarshalling function.

Some system calls may return a value that will not fit in a 32-bit
register, such as APIs that return a 64-bit value. In this scenario,
the return value is populated in a **untrusted** memory buffer that is
passed in as a final argument.  Likewise, this code is generated
automatically.

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

* :c:macro:`Z_SYSCALL_SPECIFIC_DRIVER()` is a runtime check to verify that
  a provided pointer is a valid instance of a specific device driver, that
  the calling thread has permissions on it, and that the driver has been
  initialized. It does this by checking the init function pointer that
  is stored within the driver instance and ensuring that it matches the
  provided value, which should be the address of the specific driver's
  init function.

If any check fails, the macros will return a nonzero value. The macro
:c:macro:`Z_OOPS()` can be used to induce a kernel oops which will kill the
calling thread. This is done instead of returning some error condition to
keep the APIs the same when calling from supervisor mode.

Verifier Definition
===================

All system calls are dispatched to a verifier function with a prefixed
``z_vrfy_`` name based on the system call.  They have exactly the same
return type and argument types as the wrapped system call.  Their job
is to execute the system call (generally by calling the implementation
function) after having validated all arguments.

The verifier is itself invoked by an automatically generated
unmarshaller function which takes care of unpacking the register
arguments from the architecture layer and casting them to the correct
type.  This is defined in a header file that must be included from
user code, generally somewhere after the definition of the verifier in
a translation unit (so that it can be inlined).

For example:

.. code-block:: c

    static int z_vrfy_k_sem_take(struct k_sem *sem, s32_t timeout)
    {
        Z_OOPS(Z_SYSCALL_OBJ(sem, K_OBJ_SEM));
        return z_impl_k_sem_take(sem, timeout);
    }
    #include <syscalls/k_sem_take_mrsh.c>

Configuration Options
*********************

Related configuration options:

* :option:`CONFIG_USERSPACE`

APIs
****

Helper macros for creating system call handlers are provided in
:zephyr_file:`kernel/include/syscall_handler.h`:

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
:zephyr_file:`include/syscall.h`:

* :c:func:`_arch_syscall_invoke0`
* :c:func:`_arch_syscall_invoke1`
* :c:func:`_arch_syscall_invoke2`
* :c:func:`_arch_syscall_invoke3`
* :c:func:`_arch_syscall_invoke4`
* :c:func:`_arch_syscall_invoke5`
* :c:func:`_arch_syscall_invoke6`

