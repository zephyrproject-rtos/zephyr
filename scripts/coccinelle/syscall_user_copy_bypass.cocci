/// A syscall verifier (z_vrfy_*) that copies a user structure into kernel
/// memory with k_usermode_from_copy() must use only that copy. Reading
/// fields through the user pointer, or passing the user pointer to
/// z_impl_*, lets another user thread change the values between
/// validation and use (double fetch), so the checks done on the copy do
/// not cover what the implementation reads.
///
// Confidence: High
// Copyright The Zephyr Project Contributors
// SPDX-License-Identifier: Apache-2.0
// Comments:
//   Run with:
//     spatch --sp-file scripts/coccinelle/syscall_user_copy_bypass.cocci \
//            --dir drivers/ --no-includes --include-headers -D report
//   Patch mode rewrites the uses that follow the copy. Uses before the copy
//   are only reported and must be moved after it by hand.
// Options: --no-includes --include-headers

virtual context
virtual org
virtual report
virtual patch

@copied@
identifier vrfy =~ "^z_vrfy_";
identifier c, p;
@@

vrfy(...)
{
  <+...
  k_usermode_from_copy(&c, p, ...)
  ...+>
}

// ======================================================================
// Detect
// ======================================================================

// Taking the address or the size of a field, as in
// k_usermode_to_copy(&p->f, ..., sizeof(p->f)), does not read user memory.
@addr@
identifier copied.vrfy, copied.p;
identifier f;
position q;
@@

vrfy(...)
{
  <+...
(
  &p@q->f
|
  sizeof(p@q->f)
)
  ...+>
}

@find_deref depends on (report || org) && !(file in "ext")@
identifier copied.vrfy, copied.p;
identifier f;
position pos != addr.q;
@@

vrfy(...)
{
  <+...
  p@pos->f
  ...+>
}

@find_impl depends on (report || org) && !(file in "ext")@
identifier copied.vrfy, copied.p;
identifier impl =~ "^z_impl_";
position pos;
@@

vrfy(...)
{
  <+...
  impl(...,p@pos,...)
  ...+>
}

@depends on context && !(file in "ext")@
identifier copied.vrfy, copied.p;
identifier impl =~ "^z_impl_";
identifier f;
@@

vrfy(...)
{
  <+...
(
* p->f
|
* impl(...,p,...)
)
  ...+>
}

// ======================================================================
// Report and org modes
// ======================================================================

@script:python depends on report && find_deref@
vrfy << copied.vrfy;
p << copied.p;
c << copied.c;
pos << find_deref.pos;
@@

msg = "%s: '%s' is dereferenced in user memory although it is copied to '%s', use the copy" \
      % (vrfy, p, c)
coccilib.report.print_report(pos[0], msg)

@script:python depends on report && find_impl@
vrfy << copied.vrfy;
p << copied.p;
c << copied.c;
pos << find_impl.pos;
@@

msg = "%s: user pointer '%s' is passed to the implementation instead of its copy '%s'" \
      % (vrfy, p, c)
coccilib.report.print_report(pos[0], msg)

@script:python depends on org && find_deref@
vrfy << copied.vrfy;
p << copied.p;
c << copied.c;
pos << find_deref.pos;
@@

msg = "%s: '%s' is dereferenced in user memory although it is copied to '%s', use the copy" \
      % (vrfy, p, c)
cocci.print_main(msg, pos)

@script:python depends on org && find_impl@
vrfy << copied.vrfy;
p << copied.p;
c << copied.c;
pos << find_impl.pos;
@@

msg = "%s: user pointer '%s' is passed to the implementation instead of its copy '%s'" \
      % (vrfy, p, c)
cocci.print_main(msg, pos)

// ======================================================================
// Patch mode: use the kernel copy after k_usermode_from_copy()
// ======================================================================

@fix depends on patch && !(file in "ext")@
identifier copied.vrfy, copied.p, copied.c;
identifier impl =~ "^z_impl_";
identifier f;
expression list es;
@@

vrfy(...)
{
  ...
  k_usermode_from_copy(&c, p, ...)
  <...
(
- p->f
+ c.f
|
  impl(es,
- p
+ &c
  , ...)
)
  ...>
}
