/// Syscall verifiers (z_vrfy_*) must never forward a callback struct pointer
/// to the corresponding z_impl_*.  Callback structs contain kernel-mode
/// function pointers; forwarding a userspace-controlled pointer gives the
/// caller the ability to redirect kernel execution to an arbitrary address
/// (CWE-822 / privilege escalation).
///
/// The correct fix is to remove the __syscall annotation from
/// callback-manipulation functions entirely, making them kernel-only inline
/// wrappers — exactly like the corresponding set_cb counterparts in Zephyr.
/// If the function must remain a syscall for other reasons, the verifier must
/// unconditionally reject the call (K_OOPS(K_SYSCALL_VERIFY_MSG(false, ...))).
///
// Confidence: High
// Copyright (c) 2026 Hubble Network
// SPDX-License-Identifier: Apache-2.0
// Comments:
//   Run with:
//     spatch --sp-file scripts/coccinelle/syscall_callback_ptr.cocci \
//            --dir drivers/ --no-includes --include-headers -D report
// Options: --no-includes --include-headers

virtual context
virtual org
virtual report

// ======================================================================
// Detect: callback struct pointer forwarded directly (no cast)
// ======================================================================
//
// Three rules cover three cases:
//   find       - struct <name containing "callback" or ending in "_cb"> *cb
//   find_cast  - same, but forwarded with a redundant (struct ...) cast
//   find_fptr  - function pointer typedef named 'cb' (e.g. bc12_callback_t cb)
//
// Verifiers are matched regardless of return type and storage class.
// Verifiers that check the callback against NULL before forwarding it
// (e.g. K_SYSCALL_VERIFY_MSG(cb == NULL, ...)) are not reported.
//
// ======================================================================

@find depends on !(file in "ext")@
identifier vrfy =~ "^z_vrfy_";
identifier impl =~ "^z_impl_";
identifier cb_type =~ "callback\|_cb$";
identifier cb;
type R;
position p;
@@

R vrfy(..., struct cb_type *cb, ...) {
  ... when != \(cb == NULL\|NULL == cb\|!cb\)
  impl(..., cb@p, ...)
  ...
}

// ======================================================================
// Detect: callback struct pointer forwarded with redundant cast
// ======================================================================

@find_cast depends on !(file in "ext")@
identifier vrfy =~ "^z_vrfy_";
identifier impl =~ "^z_impl_";
identifier cb_type =~ "callback\|_cb$";
identifier cb;
type R;
position p;
@@

R vrfy(..., struct cb_type *cb, ...) {
  ... when != \(cb == NULL\|NULL == cb\|!cb\)
  impl(..., (struct cb_type *)cb@p, ...)
  ...
}

// ======================================================================
// Detect: function pointer typedef forwarded directly (matched by parameter name)
// ======================================================================

@find_fptr depends on !(file in "ext") && !find && !find_cast@
identifier vrfy =~ "^z_vrfy_";
identifier impl =~ "^z_impl_";
type T;
identifier cb =~ "^cb$";
type R;
position p;
@@

R vrfy(..., T cb, ...) {
  ... when != \(cb == NULL\|NULL == cb\|!cb\)
  impl(..., cb@p, ...)
  ...
}

// ======================================================================
// Context mode
// ======================================================================

@depends on context && find@
identifier find.vrfy;
type find.R;
identifier find.impl;
identifier find.cb_type;
identifier find.cb;
@@

R vrfy(..., struct cb_type *cb, ...) {
  ...
* impl(..., cb, ...)
  ...
}

@depends on context && find_cast@
identifier find_cast.vrfy;
type find_cast.R;
identifier find_cast.impl;
identifier find_cast.cb_type;
identifier find_cast.cb;
@@

R vrfy(..., struct cb_type *cb, ...) {
  ...
* impl(..., (struct cb_type *)cb, ...)
  ...
}

@depends on context && find_fptr@
identifier find_fptr.vrfy;
type find_fptr.R;
identifier find_fptr.impl;
type find_fptr.T;
identifier find_fptr.cb;
@@

R vrfy(..., T cb, ...) {
  ...
* impl(..., cb, ...)
  ...
}

// ======================================================================
// Report mode
// ======================================================================

@script:python depends on report && find@
vrfy << find.vrfy;
cb_type << find.cb_type;
cb   << find.cb;
p    << find.p;
@@

msg = "callback pointer 'struct %s *%s' forwarded to z_impl_* in %s: " \
      "contains a kernel-mode function pointer controllable by userspace; " \
      "remove __syscall from this function (CWE-822)" % (cb_type, cb, vrfy)
coccilib.report.print_report(p[0], msg)

@script:python depends on report && find_cast@
vrfy << find_cast.vrfy;
cb_type << find_cast.cb_type;
cb   << find_cast.cb;
p    << find_cast.p;
@@

msg = "callback pointer 'struct %s *%s' forwarded to z_impl_* in %s: " \
      "contains a kernel-mode function pointer controllable by userspace; " \
      "remove __syscall from this function (CWE-822)" % (cb_type, cb, vrfy)
coccilib.report.print_report(p[0], msg)

@script:python depends on report && find_fptr@
vrfy << find_fptr.vrfy;
T    << find_fptr.T;
cb   << find_fptr.cb;
p    << find_fptr.p;
@@

msg = "callback typedef '%s %s' forwarded to z_impl_* in %s: " \
      "likely a function pointer controllable by userspace; " \
      "remove __syscall from this function (CWE-822)" % (T, cb, vrfy)
coccilib.report.print_report(p[0], msg)

// ======================================================================
// Org mode
// ======================================================================

@script:python depends on org && find@
vrfy << find.vrfy;
cb_type << find.cb_type;
cb   << find.cb;
p    << find.p;
@@

msg = "'struct %s *%s' forwarded in %s: kernel function pointer reachable from userspace (CWE-822)" \
      % (cb_type, cb, vrfy)
msg_safe = msg.replace("[", "@(").replace("]", ")")
cocci.print_main(msg_safe, p)

@script:python depends on org && find_cast@
vrfy << find_cast.vrfy;
cb_type << find_cast.cb_type;
cb   << find_cast.cb;
p    << find_cast.p;
@@

msg = "'struct %s *%s' forwarded in %s: kernel function pointer reachable from userspace (CWE-822)" \
      % (cb_type, cb, vrfy)
msg_safe = msg.replace("[", "@(").replace("]", ")")
cocci.print_main(msg_safe, p)

@script:python depends on org && find_fptr@
vrfy << find_fptr.vrfy;
T    << find_fptr.T;
cb   << find_fptr.cb;
p    << find_fptr.p;
@@

msg = "'%s %s' forwarded in %s: likely a function pointer reachable from userspace (CWE-822)" \
      % (T, cb, vrfy)
msg_safe = msg.replace("[", "@(").replace("]", ")")
cocci.print_main(msg_safe, p)
