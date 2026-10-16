.. _lockdep:

Runtime Lock Dependency Validator
#################################

The lock dependency validator (lockdep) detects locking patterns that can
deadlock. It observes successful and potentially blocking lock acquisitions,
builds a directed graph of lock ordering, and reports an error when a new edge
would close a cycle. For example, after observing ``A -> B``, acquiring ``A``
while holding ``B`` reports a possible deadlock even when the two sequences run
at different times or in different threads.

Unlike the Linux kernel's `runtime locking correctness validator
<https://docs.kernel.org/locking/lockdep-design.html>`_, which groups equivalent
locks into classes, this validator uses individual lock instances as graph
vertices. It retains per-context held-lock tracking and whole-graph cycle
detection.

Enabling lockdep
****************

Enable :kconfig:option:`CONFIG_LOCKDEP`. The validator instruments both
:c:struct:`k_mutex` and :c:struct:`k_spinlock`. It is intended for test and
debug builds because every lock operation performs additional bookkeeping.
The validator does not allocate memory: all storage limits are configured by
the following options:

* :kconfig:option:`CONFIG_LOCKDEP_MAX_LOCKS` controls tracked lock instances and
  graph vertices.
* :kconfig:option:`CONFIG_LOCKDEP_MAX_DEPENDENCIES` controls graph edges.
* :kconfig:option:`CONFIG_LOCKDEP_MAX_HELD_LOCKS` controls nesting depth.
* :kconfig:option:`CONFIG_LOCKDEP_REPORT_BUFFER_SIZE` controls the per-CPU
  diagnostic buffer.

Exhausting a table produces a warning and makes subsequent validation
incomplete. :kconfig:option:`CONFIG_LOCKDEP_PANIC_ON_ERROR` changes circular
dependency and bookkeeping reports into kernel panics. Recursive acquisition
of a spinlock always panics, since continuing would otherwise hang the CPU.

Stack usage
===========

Validation runs on the stack of whichever thread or interrupt acquires the
lock. The dependency walk and the report formatter together add roughly 350 to
500 bytes to the deepest call path through :c:func:`k_spin_lock` or
:c:func:`k_mutex_lock` on 64-bit targets, and somewhat less on 32-bit targets.
Threads whose stacks are sized tightly for a release build can overflow once
the validator is enabled. This is most visible on targets with a hardware
stack guard that reserves part of the stack object, such as RISC-V with
:kconfig:option:`CONFIG_PMP_STACK_GUARD`, where an overflow is reported as an
access fault inside the validator rather than as a lockdep diagnostic.

Budget at least 512 bytes of additional stack for every thread that takes
locks, including the interrupt stack. When :kconfig:option:`CONFIG_TEST` is
set, :kconfig:option:`CONFIG_TEST_EXTRA_STACK_SIZE` is raised automatically so
that the kernel test suites can run with the validator enabled.

Instance identity
*****************

Each mutex or spinlock address is tracked independently. Two locks initialized
from the same call site are not assumed to have the same ordering rules.
Initializing a mutex does not consume a graph entry until that mutex is first
used. Reinitializing a previously tracked mutex removes its old dependencies;
the next use starts a new lock lifetime.

Cycles are reported only when all edges involve the same concrete instances.
For example, observing ``A1 -> B1`` and ``B2 -> A2`` does not report a cycle.
The validator does not infer that these pairs could be interchangeable.
Spinlocks have no initialization hook: reusing a spinlock's address for a new
lock can retain dependencies learned from the previous occupant.
Since edges are also per instance, several lock pairs with the same ordering
each consume a dependency entry.

Diagnostics
***********

The validator has no application-facing API. Detection is automatic for all
:c:struct:`k_mutex` and :c:struct:`k_spinlock` users once
:kconfig:option:`CONFIG_LOCKDEP` is enabled. The internal ``z_lockdep_*``
interface in :file:`include/zephyr/debug/lockdep.h` exposes held-lock queries,
counters for registered instances, dependency edges, errors and capacity
overflows, and a state reset. It exists to support the kernel test suite and
is not part of the stable API.

Implementation scope
********************

The validator covers exclusive mutex and spinlock ordering, arbitrary-length
cycles, mutex recursion, failed non-blocking acquisition, mixed mutex/spinlock
chains, and per-CPU spinlock nesting. Unlike Linux lockdep, it does not group
instances into lock classes, model reader/writer locking or interrupt-safe
versus interrupt-unsafe state, or cache previously validated complete chains.
