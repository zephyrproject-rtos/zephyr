.. _call_offloading:

Call Offloading
###############

.. contents::
    :local:
    :depth: 1

Some APIs are, in relative terms, stack-hungry. Without mitigation, every thread that calls such an
API must size its own stack for the deepest call it makes.

A :dfn:`call offloader` is a pool of worker threads fed by a priority-sorted queue of pending
callers. A caller hands a function and an argument blob to :c:func:`offloader_dispatch` and blocks
until a worker has run it, so the deep stack is paid once per worker instead of once per calling
thread.

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

Enable :kconfig:option:`CONFIG_OFFLOADER`, then declare an engine together with its worker pool
using :c:macro:`OFFLOADER_ENGINE_DEFINE`.

.. code-block:: c

   #include <zephyr/offloader/engine.h>

   OFFLOADER_ENGINE_DEFINE(my_engine, 1, 3072);

Hand a function and an argument blob to :c:func:`offloader_dispatch`. The call blocks until a
worker has run the function and returns.

.. code-block:: c

   offloader_dispatch(&my_engine, my_work_fn, &my_args);

Keep the following constraints in mind:

* :c:func:`offloader_dispatch` must not be called from an ISR. The assertion guarding this is
  build-configurable, but the underlying pend refuses ISR context with ``k_panic()`` in every
  build, so a violation is a fatal error rather than a silently ignored one.

* :c:macro:`OFFLOADER_ENGINE_DEFINE` starts the workers from a ``SYS_INIT()`` entry at
  ``POST_KERNEL`` priority 1. A call dispatched before that entry runs finds no worker, queues
  itself and blocks forever. There is no assertion for this case: it presents as a silent hang.

* The argument blob must stay valid for the duration of the call.

* The caller pends with ``K_FOREVER``. There is no timeout, no cancellation and no bound on the
  pending queue.

* Each worker stack must accommodate the deepest offloaded call, where "deepest" includes any
  same-engine dispatches nested inside it, which run inline on the worker's own stack.

* The API is kernel-mode only. :c:func:`offloader_dispatch` is not a system call; it dereferences
  internal thread state and takes a kernel spinlock, and the engine object the macro defines is
  static kernel memory. A user thread cannot reach the engine, and calling the function from user
  mode faults. The workers themselves are created with no options on stacks from
  ``K_KERNEL_STACK_ARRAY_DEFINE()``, so they are plain kernel threads.

* Every queued call pays a dispatch and two context switches, plus spinlock round trips on the
  engine and scheduler locks. When no worker is idle it also pays the promotion pass: a walk of
  the pending queue down to the caller's priority and a scan of the worker array. A dispatch that
  takes the inline escape described below pays none of this.

Sizing the pool
***************

``_num_threads`` sets how many offloaded calls can run concurrently; ``_stack_size`` must cover
the deepest call made through the engine. The pool costs ``_num_threads * _stack_size`` bytes
plus the thread objects, against the stack each caller no longer has to reserve. A single worker
is the cheapest configuration and serializes all callers. Adding workers buys concurrency at the
cost of more RAM.

Design
******

An engine owns a priority-sorted ``pending`` wait queue of callers and an ``idle`` wait queue of
workers. A dispatch wakes one idle worker if there is one and otherwise runs the promotion pass
below. The caller then pends on ``pending`` either way: waking an idle worker is not a direct
hand-off, and the woken worker unpends the caller from ``pending`` itself and parks it on a third
``running`` wait queue for the duration of the call.

Unlike a :ref:`workqueue <workqueues_v2>`, offloading is synchronous, and the worker runs the
function at the caller's priority or higher.

Priority handling:

* A worker idles at the highest cooperative priority and adopts its client's priority before
  running the function, so it is briefly cooperative and high-priority between being woken and
  picking up its client.

* When no worker is idle, the promotion pass raises the *lowest*-priority busy worker to the new
  caller's priority. It does so only when the pending callers at or above that priority, counting
  the new caller, outnumber the busy workers currently at or above it, and only if some busy
  worker is actually running below the caller's priority.

* At most one promotion is issued per dispatch, however large the shortfall, and the count of
  workers already serving uses each worker's current priority, which may itself be boosted. A
  worker promoted earlier therefore makes the pool look adequate to a later high-priority caller.

* Promotion raises work already in flight, which then temporarily runs at the higher priority. It
  is priority inheritance applied to running work, not a hand-off of the pending request.

Ordering and concurrency:

* Callers are served by priority. Within a priority they are served in submission order. Under
  :kconfig:option:`CONFIG_SCHED_DEADLINE` the wait queue compares deadlines after priority, so
  submission order holds only among callers that have not set one.

* With more than one worker, calls run concurrently. The offloaded API must be reentrant, and
  concurrent dispatches have no ordering guarantee relative to each other.

* Dispatching from a thread that is itself a worker of the same engine calls the function inline
  on that worker's stack rather than queueing it. Queueing would deadlock outright on a
  single-worker engine, or whenever every other worker is also blocked; with spare workers it
  would merely occupy one.

* That escape covers the same engine only. A worker that dispatches to a second engine blocks and
  keeps occupying a worker of the first, so a dispatch cycle between two single-worker engines
  deadlocks.

Offloading an API
*****************

To offload an API, define an engine, pack each call's arguments and its return value into a
struct, unpack them in a work function, and wrap it in a thin function that dispatches and
returns the result (if any).

.. code-block:: c

   #include <zephyr/offloader/engine.h>

   OFFLOADER_ENGINE_DEFINE(my_engine, 1, 3072);

   struct do_thing_args {
           const uint8_t *input;
           size_t input_len;
           int ret;
   };

   static void do_thing(void *args)
   {
           struct do_thing_args *a = args;

           a->ret = thing(a->input, a->input_len);
   }

   int offloaded_thing(const uint8_t *input, size_t input_len)
   {
           struct do_thing_args a = {
                   .input = input,
                   .input_len = input_len,
           };

           offloader_dispatch(&my_engine, do_thing, &a);

           return a.ret;
   }

:zephyr_file:`tests/lib/offloader` exercises this pattern in tree.

Redirecting call sites
======================

Writing the wrapper is only half the job: every caller still has to be changed to call
``offloaded_thing()`` instead of ``thing()``. Giving the wrapper the *same signature* as the
original avoids that.

.. code-block:: c

   #include <thing.h>

   __typeof__(thing) offloaded_thing;

   #ifndef THING_NO_REDIRECT
   #define thing offloaded_thing
   #endif

Every ``thing(...)`` the compiler sees after that line resolves to the wrapper, with no edit to
the call site. Two rules make this work:

* Include the real API header *before* the redirect. The macro replaces the identifier
  everywhere, including in the API's own declaration, so a redirect that is in scope when
  ``thing.h`` is parsed corrupts the prototype it is meant to shadow.

* Build the file that *implements* the wrappers with the redirect suppressed, as the guard above
  allows. Without it, the body of ``offloaded_thing()`` calls itself rather than the real
  ``thing()``.

Choosing the scope
==================

The redirect can be applied in three different scopes:

* a set of translation units
* a set of libraries
* globally

Including the redirect header file in a single source file, or in a small set of them, is the
simplest and most obvious way to offload a single call site. Include it after the real API header,
as described above, and the redirect is local to that translation unit.

When applying the redirect to a library it is easiest to pass the redirect as a compiler flag in
the library's build system. The redirect is then in scope for every translation unit of that
library, and every call the library makes to the API is offloaded. The rest of the build is
unaffected.

Using the same trick as for a library, the redirect can be applied to the entire build from the
top-level build system. Every translation unit sees the redirect, so every call to the API in the
build is offloaded with no source change at all. This scope comes with two extra requirements:

* The redirect **must** be suppressed in the translation unit that implements the wrapper, and in
  any that implement the original API. Without that, those definitions are rewritten into
  definitions of the wrapper name and the wrapper body calls itself, overflowing the worker stack.
  This is why a global redirect only fits an API whose implementation is a prebuilt or
  out-of-tree binary, or one whose every implementing library can be compiled with the guard
  macro defined.

* The redirect header must include the real API header itself. A compiler flag places it
  textually first in every translation unit, so no source file can satisfy the ordering rule
  above on its behalf. This also pulls the whole API header into every translation unit in the
  build.

Keep the following in mind:

* The redirect is textual and compiler-local. Calls made from a translation unit that does not
  see the ``#define``, including calls from inside a prebuilt library, keep reaching the original
  symbol.

* An object-like ``#define`` replaces *every* occurrence of the token, not just calls: a struct
  member, a local variable or a parameter that happens to share the name is rewritten too. A
  prefixed API makes this unlikely, but at whole-build scope the question covers the entire tree.
  A function-like ``#define thing(...) offloaded_thing(__VA_ARGS__)`` replaces only calls, at the
  cost of no longer redirecting the address-of form below.

* Taking the address of the function yields the wrapper: ``&thing`` becomes
  ``&offloaded_thing``. This is harmless while the signatures match, but it is visible if the
  address is stored in a table or compared against the original, and a redirected address
  registered as a callback will panic if that callback is ever invoked from an ISR.

* A worker that calls the redirected name re-enters the same engine and takes the inline escape
  described above rather than deadlocking.

API Reference
*************

.. doxygengroup:: offloader
