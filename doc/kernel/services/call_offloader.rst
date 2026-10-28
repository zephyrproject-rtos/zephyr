.. _call_offloading:

Call Offloading
###############

.. contents::
    :local:
    :depth: 1

Some APIs are, in relative terms, stack-hungry. Without mitigation, every thread that calls such an
API must size its own stack for the deepest call it makes.

A :dfn:`call offloader` is a pool of worker threads fed by a priority-sorted queue of pending
callers. A caller hands a function and an argument blob to :c:func:`call_offloader_dispatch` and
blocks until a worker has run it, so the deep stack is paid once per worker instead of once per
calling thread.

Unlike a :ref:`workqueue <workqueues_v2>` offloading is synchronous and the worker runs the
function at the caller's priority or higher.

A :c:func:`call_offloader_dispatch` either wakes an idle worker or promotes an active worker to the
callers priority, and then pends the caller until its request has been handled. The call_offloader
API handles function reentry, priority inheritance, the caller resources (memory pool and custom data).
It is meant to mimic the behaviour of a direct call to the offloaded function as closely as
possible, while still providing the stack savings of offloading.

Rationale
*********

Stack sizing is per-thread and worst-case. Every thread that might call a stack-hungry API has to
reserve the deepest call it can reach, even though only a few of those calls are ever in flight at
once. A system with many such callers therefore reserves the deep stack once per thread and leaves
nearly all of it idle. On a constrained device that reservation, rather than the code itself, is
what runs out.

The obvious alternatives each give something up:

* Enlarging every caller's stack is exactly the per-thread reservation described above, repeated
  for each caller.

* Moving the work to a :ref:`workqueue <workqueues_v2>` concentrates the deep stack onto one
  thread, but changes how the work is called: it runs asynchronously, at the queue's fixed
  priority, and cannot return a value inline. Synchronous call sites have to be restructured into
  submit-and-wait form, and the work no longer tracks the caller's priority.

An offloader concentrates the stack the same way while keeping the call synchronous, returning its
result inline, and running it at the caller's priority. The API behind it has to be wrapped, as
shown below, but its call sites do not have to change.

The saving is the difference between reserving the deep stack for every caller and reserving it
for every worker, so offloading pays off when callers substantially outnumber workers. That is the
usual shape for a shared, stack-hungry service. What it costs is the per-call overhead and the
constraints below.

Usage
*****

Enable :kconfig:option:`CONFIG_CALL_OFFLOADER`, then declare an engine together with its worker
pool using :c:macro:`CALL_OFFLOADER_ENGINE_DEFINE`.

.. code-block:: c

   #include <zephyr/call_offloader/call_offloader.h>

   CALL_OFFLOADER_ENGINE_DEFINE(my_engine, 1, 3072);

Hand a function and an argument blob to :c:func:`call_offloader_dispatch`. The call blocks until a
worker has run the function and returns.

.. code-block:: c

   call_offloader_dispatch(&my_engine, my_work_fn, &my_args);

Keep the following constraints in mind:

* :c:func:`call_offloader_dispatch` must not be called from an ISR.

* :c:func:`call_offloader_dispatch` must not be called before the kernel has started.

* The argument blob must stay valid for the duration of the call.

* The caller pends with ``K_FOREVER``. There is no timeout, no cancellation and no bound on the
  pending queue.

* A fatal error inside an offloaded call kills the worker running it, not the caller, so the
  application's :c:func:`k_sys_fatal_error_handler` sees the engine's worker as the faulting
  thread. State the offloaded call left behind, such as a library mutex it held, is not recovered. A
  direct call that faults the same way leaves the same wreckage.

* A thread blocked in :c:func:`call_offloader_dispatch` must not be aborted. The kernel neither
  frees nor overwrites its stack on abort, but the worker is still reading the argument blob from
  it risking a use-after-free.

* Each worker stack must accommodate the deepest offloaded call, where "deepest" includes any
  same-engine dispatches nested inside it, which run inline on the worker's own stack.

* The API is kernel-mode only. :c:func:`call_offloader_dispatch` is not a system call, it
  dereferences internal thread state and takes a kernel spinlock.

* Every dispatch pays a small ammount of overhead in comparison to a direct call, roughly two
  context switches and a few spinlock acquisitions.

Sizing the pool
***************

``_num_threads`` sets how many offloaded calls can run concurrently; ``_stack_size`` must cover
the deepest call made through the engine. The pool costs ``_num_threads * _stack_size`` bytes
plus the thread objects, against the stack each caller no longer has to reserve. A single worker
is the cheapest configuration and serializes all callers. Adding workers buys concurrency at the
cost of more RAM.

Userspace
*********

It is not possible to offload an API called from user mode.

The primary reason is thread stack isolation. Under MPU or MMU protection, a user thread only has
access to its own stack object, even when it shares a memory domain and kernel object permissions
with another user thread. Because both the argument struct and, in most APIs, caller-supplied
buffers live on the calling thread's stack, a ``K_USER`` worker thread would trigger a memory
access fault as soon as it dereferenced them.

Running the worker as a privileged kernel thread avoids the memory fault, but would execute the
offloaded function at supervisor privilege on unvalidated pointers supplied by user mode, allowing
a user thread to elevate its privilege level.

While the privilege boundary could be policed per API with custom verification and buffer copying
(at which point it is equivalent to a system call), the cross-stack access restriction is a
fundamental limitation of user mode isolation in the Zephyr kernel.

Offloading an API
*****************

To offload an API, define an engine and wrap each call with :c:macro:`CALL_OFFLOADER_WRAP`. The
macro takes the engine, the return type, the name of the wrapper and the parameters, each written
as the type in parentheses followed by the name. The body that follows is the code the engine runs
on a worker thread.

.. code-block:: c

   #include <zephyr/call_offloader/call_offloader.h>

   CALL_OFFLOADER_ENGINE_DEFINE(my_engine, 1, 3072);

   CALL_OFFLOADER_WRAP(my_engine, int, thing, (const uint8_t *) input, (size_t) input_len)
   {
           return thing(input, input_len);
   }

This defines ``call_offloader_thing()`` with the same signature as ``thing()``. Calling it packs
the arguments, blocks until a worker has run the body, and returns its result. The arguments live
on the caller's stack for the duration of the call, so a pointer parameter still refers to memory
the caller owns.

Parameter types whose declarator encloses the name, such as function pointer and array types,
cannot be written as ``(type) name`` and need a typedef.

:zephyr_file:`tests/kernel/call_offloader` exercises this pattern in tree.

Redirecting call sites
======================

Writing the wrapper is only half the job: every caller still has to call
``call_offloader_thing()`` instead of ``thing()``. Because :c:macro:`CALL_OFFLOADER_WRAP` gives
the wrapper the exact signature of the original function, a macro redirect header can rewrite the
call sites without touching them:

.. code-block:: c

   #ifndef CALL_OFFLOADER_THING_NO_REDIRECT
   #define thing call_offloader_thing
   #endif

When that macro is in scope *before* ``<thing.h>`` is parsed, the preprocessor rewrites both the
prototype in ``<thing.h>`` and every subsequent call to ``thing(...)`` into
``call_offloader_thing(...)``. Two rules make this work:

* The redirect must be parsed before the real API header so that the header's own prototype
  declares ``call_offloader_thing()`` with the original signature. This only works for names the
  header declares as plain functions, not as macros or ``static inline`` functions.

* The redirect header must contain only preprocessor directives and must not include any other
  header, so that it can be injected ahead of the first line of a translation unit without
  disturbing feature test macros such as ``_POSIX_C_SOURCE``.

Choosing the scope
==================

In CMake, the redirect header is injected with the compiler's ``-imacros`` flag, while the
translation units that implement the wrappers (and the library implementing the real API) define
the guard macro so they still see the original symbol names:

.. code-block:: cmake

   set(thing_header ${ZEPHYR_BASE}/include/zephyr/call_offloader/services/thing.h)

   zephyr_library()
   zephyr_library_sources(thing.c)
   zephyr_library_compile_definitions(CALL_OFFLOADER_THING_NO_REDIRECT)

   zephyr_compile_options(
     "SHELL:$<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,imacros> ${thing_header}>"
   )

And on the target that implements ``thing()`` itself:

.. code-block:: cmake

   target_compile_definitions(thing_lib PRIVATE CALL_OFFLOADER_THING_NO_REDIRECT)

:zephyr_file:`include/zephyr/call_offloader/services/psa.h` and
:zephyr_file:`lib/call_offloader/services/CMakeLists.txt` use this exact setup for PSA Crypto.

The same redirect header can be applied at three scopes:

* **Single translation unit**: include the redirect header before ``<thing.h>`` in that source
  file.
* **Library**: add the ``$<TARGET_PROPERTY:compiler,imacros>`` option with
  ``target_compile_options()`` on the consumer library target.
* **Global**: add it with ``zephyr_compile_options()``, as shown above, and define the
  ``_NO_REDIRECT`` guard on both the wrapper library and the library implementing the original API.
  Without the guard on the implementing library, its function definitions are renamed to the
  wrapper and the wrapper body calls itself recursively.

Keep the following in mind:

* The redirect is textual and compiler-local. Calls made from a translation unit that does not
  see the ``#define``, including calls from inside a prebuilt library, keep reaching the original
  symbol.

* An object-like ``#define`` replaces *every* occurrence of the token, not just calls: a struct
  member, a local variable or a parameter that happens to share the name is rewritten too. A
  prefixed API makes this unlikely, but at whole-build scope the question covers the entire tree.
  A function-like ``#define thing(...) call_offloader_thing(__VA_ARGS__)`` replaces only calls,
  at the cost of no longer renaming the prototype in ``<thing.h>`` or redirecting the address-of
  form below.

* Taking the address of the function yields the wrapper: ``&thing`` becomes
  ``&call_offloader_thing``. This is harmless while the signatures match, but it is visible if the
  address is stored in a table or compared against the original, and a redirected address
  registered as a callback will panic if that callback is ever invoked from an ISR.

* A worker that calls the redirected name re-enters the same engine and takes the inline escape
  described above rather than deadlocking.

API Reference
*************

.. doxygengroup:: call_offloader
