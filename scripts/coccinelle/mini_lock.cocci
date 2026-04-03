/// Find missing unlocks.  This semantic match considers the specific case
/// where the unlock is missing from an if branch, and there is a lock
/// before the if and an unlock after the if.  False positives are due to
/// cases where the if branch represents a case where the function is
/// supposed to exit with the lock held, or where there is some preceding
/// function call that releases the lock.
///
// Confidence: Moderate
// Copyright: (C) 2010-2012 Nicolas Palix.  GPLv2.
// Copyright: (C) 2010-2012 Julia Lawall, INRIA/LIP6.  GPLv2.
// Copyright: (C) 2010-2012 Gilles Muller, INRIA/LiP6.  GPLv2.
// URL: http://coccinelle.lip6.fr/

virtual context
virtual org
virtual report

@prelocked depends on !(file in "ext")@
position p1,p;
expression E1;
@@

(
irq_lock@p1
|
k_mutex_lock@p1
|
k_sem_take@p1
|
k_spin_lock@p1
) (E1@p,...);

@looped depends on !(file in "ext")@
position r;
@@

for(...;...;...) { <+... return@r ...; ...+> }

@balanced exists@
position p1 != prelocked.p1;
position prelocked.p;
position pif;
identifier lock,unlock;
expression x <= prelocked.E1;
expression E,prelocked.E1;
expression E2;
@@

if (E) {
 ... when != E1
 lock(E1@p,...)
 ... when any
}
... when != E1
    when != \(x = E2\|&x\)
if@pif (E) {
 ... when != E1
 unlock@p1(E1,...)
 ... when any
}

@err depends on !(file in "ext") exists@
expression E1;
position prelocked.p,balanced.pif;
position up != prelocked.p1;
position r!=looped.r;
identifier lock,unlock;
statement S1,S2;
@@

*lock(E1@p,...);
... when != E1
    when any
    when != if@pif (...) S1
if (...) {
  ... when != E1
      when != if@pif (...) S2
*  return@r ...;
}
... when != E1
    when any
*unlock@up(E1,...);

@script:python depends on org@
p << prelocked.p1;
lock << err.lock;
unlock << err.unlock;
p2 << err.r;
@@

cocci.print_main(lock,p)
cocci.print_secs(unlock,p2)

@script:python depends on report@
p << prelocked.p1;
lock << err.lock;
unlock << err.unlock;
p2 << err.r;
@@

msg = "preceding lock on line %s" % (p[0].line)
coccilib.report.print_report(p2[0],msg)
