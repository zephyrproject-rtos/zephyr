.. _secure_call:

Secure Calls
############

Overview
********

``__secure_call`` is a build-time code-generation pattern for safely crossing
the Arm TrustZone-M Non-Secure/Secure boundary, mirroring Zephyr's
:ref:`syscalls` pattern. A function is decorated in a header:

.. code-block:: c

   __secure_call int foo(const uint8_t *buf, size_t len);

and the build generates the Non-Secure inline wrapper and the Secure-side
``cmse_nonsecure_entry`` veneer that crosses the boundary. The developer
provides two Secure-side functions:

* ``z_secure_vrfy_foo()`` — validates that pointers supplied by Non-Secure code
  actually point into Non-Secure-accessible memory (using the
  ``Z_SECURE_MEMORY_READ`` / ``Z_SECURE_MEMORY_WRITE`` helpers) before
  dereferencing them, then calls the implementation.
* ``z_secure_impl_foo()`` — the actual trusted implementation.

The Non-Secure wrapper serialises the crossing with ``z_secure_call_lock()`` /
``z_secure_call_unlock()`` so a context switch cannot occur while execution is
in the Secure world.

The address-attribution and bus-level pieces this builds on — the CPU security
partition (SAU), the secure-domain hand-off, and the MPC/PPC bus filters — are
described in :ref:`mpc_api` and :ref:`ppc_api`.

Intent
******

``__secure_call`` exists to make crossing the Secure boundary look and feel like
the :ref:`syscalls` mechanism that Zephyr already uses to cross the user/kernel
boundary. In both cases untrusted code needs to invoke a trusted service, and in
both cases the trusted side cannot assume anything about the arguments it is
handed.

The goal is not to invent a new security primitive but to make an existing one
hard to misuse:

- Every crossing is expressed the same way — a decorated declaration — so the
  set of Secure entry points is explicit, greppable, and generated rather than
  hand-written.

- Every crossing has a single, obvious place to validate untrusted input: the
  hand-written ``z_secure_vrfy_*`` function. This is the direct analogue of a
  syscall's ``z_vrfy_*`` handler.

- The unsafe mechanics — the veneer, the ``cmse_nonsecure_entry`` attribute, the
  argument marshalling, and the serialisation lock — are generated so that
  individual call sites cannot get them subtly wrong.

Threat Model
************

The Non-Secure (NS) world is untrusted. The Secure (S) world — the Trusted
Execution Environment — is trusted. A flawed or malicious Non-Secure image must
not be able to read or corrupt Secure memory, hijack Secure execution, or leave
the Secure world in an inconsistent state. This mirrors the syscall threat model,
where the roles of "untrusted caller" and "trusted callee" are played by user
mode and supervisor mode instead of NS and S.

Concretely, ``__secure_call`` is designed to protect against the following, for
a Secure image accepting calls from an untrusted Non-Secure image:

- **Direct access to Secure memory from Non-Secure code.** This is enforced in
  hardware, not by this layer: the SAU security partition and the MPC/PPC bus
  filters (see :ref:`mpc_api` and :ref:`ppc_api`) mark Secure RAM, flash, and
  peripherals inaccessible to NS bus masters. ``__secure_call`` relies on this
  partitioning being correct.

- **Entry into Secure code at an arbitrary address.** Non-Secure code can only
  branch into the Secure world through a Non-Secure-Callable (NSC) veneer that
  begins with an ``SG`` instruction. The generated ``cmse_nonsecure_entry``
  veneers are the only legal entry points; a branch to any other Secure address
  faults. The set of veneers is exactly the set of decorated declarations.

- **The confused-deputy attack.** Non-Secure code can pass any pointer value it
  likes as an argument, including one that aliases Secure memory, hoping the
  trusted Secure implementation will dereference it on the caller's behalf. Each
  ``z_secure_vrfy_*`` function must validate every caller-supplied pointer with
  :c:macro:`Z_SECURE_MEMORY_READ` / :c:macro:`Z_SECURE_MEMORY_WRITE` before it is
  touched. These use the CMSE ``TT`` instruction to confirm the *entire* buffer
  is Non-Secure and accessible at the current privilege level; a check failure
  aborts the call rather than dereferencing the pointer.

- **Preemption corrupting the Secure stack.** A context switch while execution
  is in the Secure world could re-enter the Secure world on a second stack frame
  or leave the first in an inconsistent state. The generated NS wrapper brackets
  the crossing with ``z_secure_call_lock()`` / ``z_secure_call_unlock()`` so this
  cannot happen. The default implementation locks the scheduler; platforms that
  admit concurrent Secure entry (e.g. multi-core) must override these with a
  mutex pair.

Security Objectives
===================

For a call crossing from Non-Secure into Secure we aim to guarantee that:

- The Secure implementation is only ever reached through a generated veneer and
  its matching ``z_secure_vrfy_*`` validator; there is no path that reaches
  ``z_secure_impl_*`` while skipping validation.

- No pointer supplied by Non-Secure code is dereferenced by Secure code until it
  has been proven to refer wholly to Non-Secure-accessible memory of the
  required access type.

- A crossing runs to completion, from the Non-Secure side's point of view,
  without an intervening context switch into other Non-Secure work.

Non-Goals
=========

The following are explicitly **not** addressed by this mechanism:

- **The Secure image itself is trusted.** Bugs in ``z_secure_impl_*`` functions,
  or in any other Secure-side code, are outside this threat model in the same
  way that supervisor-mode bugs are outside the syscall threat model. The
  ``z_secure_vrfy_*`` boundary only protects against untrusted *input*, not
  against a mistake in the trusted implementation.

- **The build and toolchain are trusted.** The code generator, the compiler, the
  CMSE support, and the linker that places the NSC veneer region are all part of
  the trusted build, exactly as the syscall generation step is.

- **The hardware partitioning is assumed correct.** ``__secure_call`` does not
  configure the SAU, MPC, or PPC; it depends on them being set up so that Secure
  memory is genuinely unreachable from Non-Secure masters. A misconfigured
  partition undermines every guarantee above.

- **Denial of service is not prevented.** Non-Secure code is free to never make
  a call, to spin, or (with the default scheduler-lock serialisation) to hold
  the lock for the duration of a legitimately long Secure call. As with syscalls
  and user mode, availability is not a goal here.

- **Side channels are out of scope.** Timing, power, and microarchitectural
  leakage from Secure execution are not addressed by this pattern.

API Reference
*************

.. doxygengroup:: secure_call
