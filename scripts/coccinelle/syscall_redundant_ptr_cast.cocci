///
/// In syscall verifiers (z_vrfy_*) a pointer parameter forwarded to the
/// corresponding z_impl_* with an identical cast — (T *)ptr where ptr is
/// already declared as T * — is redundant.  The cast silences compiler
/// type-checking without adding any value; drop it.
///
// Confidence: High
// Copyright (c) 2026 Hubble Network
// SPDX-License-Identifier: Apache-2.0
// Comments:
//   Run with:
//     spatch --sp-file scripts/coccinelle/misc/syscall_redundant_ptr_cast.cocci \
//            --dir drivers/ --no-includes --include-headers -D report
//   For automatic patching:
//     spatch --sp-file scripts/coccinelle/misc/syscall_redundant_ptr_cast.cocci \
//            --dir drivers/ --no-includes --include-headers -D patch \
//            --in-place
// Options: --no-includes --include-headers

virtual context
virtual org
virtual report
virtual patch

@find depends on !(file in "ext")@
identifier vrfy =~ "^z_vrfy_";
identifier impl =~ "^z_impl_";
type T;
identifier ptr;
position p;
@@

static inline int vrfy(..., T *ptr, ...) {
  ...
  impl(..., (T *)ptr@p, ...)
  ...
}

// ======================================================================
// Context mode
// ======================================================================

@depends on context && find@
identifier find.vrfy;
identifier find.impl;
type find.T;
identifier find.ptr;
@@

static inline int vrfy(..., T *ptr, ...) {
  ...
* impl(..., (T *)ptr, ...)
  ...
}

// ======================================================================
// Report mode
// ======================================================================

@script:python depends on report && find@
vrfy << find.vrfy;
ptr  << find.ptr;
p    << find.p;
@@

msg = "redundant cast for pointer '%s' in %s: already the correct type, drop the cast" \
      % (ptr, vrfy)
coccilib.report.print_report(p[0], msg)

// ======================================================================
// Org mode
// ======================================================================

@script:python depends on org && find@
vrfy << find.vrfy;
ptr  << find.ptr;
p    << find.p;
@@

msg = "redundant cast for pointer '%s' in %s: already the correct type, drop the cast" \
      % (ptr, vrfy)
msg_safe = msg.replace("[", "@(").replace("]", ")")
cocci.print_main(msg_safe, p)

// ======================================================================
// Patch mode — remove the redundant cast
// ======================================================================

@fix depends on patch && !(file in "ext")@
identifier vrfy =~ "^z_vrfy_";
type T;
identifier ptr;
@@

static inline int vrfy(..., T *ptr, ...) {
<...
- (T *)ptr
+ ptr
...>
}
