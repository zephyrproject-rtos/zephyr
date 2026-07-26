// SPDX-License-Identifier: Apache-2.0
//
// Migrate RTIO cancellation handles from `struct rtio_sqe *` to the opaque,
// generation-checked `rtio_sqe_handle_t` (Zephyr 4.5).
//
// A cancellation handle is now the validatable identity returned by
// rtio_sqe_copy_in_get_handles() / sensor_stream() / adc_stream(), and
// rtio_sqe_cancel() takes the owning RTIO context plus the handle:
//
//     -  struct rtio_sqe *h;
//     +  rtio_sqe_handle_t h;
//        ...
//        rtio_sqe_copy_in_get_handles(r, sqes, &h, n);
//        ...
//     -  rtio_sqe_cancel(h);
//     +  rtio_sqe_cancel(r, h);
//
// This patch handles the common in-application pattern: a handle variable filled
// by one of the producer calls and later passed to rtio_sqe_cancel(). The
// producer's RTIO context is bound and threaded into the cancel call.
//
// Also rewrites reads of the deprecated RTIO_SQE_CANCELED sqe.flags bit into
// rtio_iodev_sqe_is_canceled() calls, since cancellation state moved into the
// atomic status word of the owning rtio_iodev_sqe.
//
// Out of scope (update by hand): handles stored in a struct field or otherwise
// cancelled in a different function than they were produced (e.g. Zephyr's
// sensing subsystem), and handles kept in arrays. These do not bind to a simple
// identifier / single producer call and are called out in the migration guide.
// Writes of RTIO_SQE_CANCELED also cannot be rewritten mechanically; request
// cancellation with rtio_sqe_cancel() instead (report mode flags them).
//
// Usage:
//     spatch --sp-file scripts/coccinelle/rtio_sqe_handle.cocci \
//            --macro-file scripts/coccinelle/macros.h --dir . --in-place
// or via the coccicheck wrapper (which passes the macro file itself):
//     MODE=patch scripts/coccicheck

virtual patch
virtual report

// ---------------------------------------------------------------------------
// rtio_sqe_copy_in_get_handles(r, sqes, &h, n)
// ---------------------------------------------------------------------------

@copyin_producer@
identifier h;
expression r, sqes, n;
@@
rtio_sqe_copy_in_get_handles(r, sqes, &h, n)

@depends on patch && copyin_producer@
identifier copyin_producer.h;
@@
- struct rtio_sqe *h;
+ rtio_sqe_handle_t h;

@depends on patch && copyin_producer@
identifier copyin_producer.h;
expression copyin_producer.r;
@@
- rtio_sqe_cancel(h)
+ rtio_sqe_cancel(r, h)

// ---------------------------------------------------------------------------
// sensor_stream(iodev, ctx, userdata, &h)
// ---------------------------------------------------------------------------

@sensor_producer@
identifier h;
expression iodev, ctx, ud;
@@
sensor_stream(iodev, ctx, ud, &h)

@depends on patch && sensor_producer@
identifier sensor_producer.h;
@@
- struct rtio_sqe *h;
+ rtio_sqe_handle_t h;

@depends on patch && sensor_producer@
identifier sensor_producer.h;
expression sensor_producer.ctx;
@@
- rtio_sqe_cancel(h)
+ rtio_sqe_cancel(ctx, h)

// ---------------------------------------------------------------------------
// adc_stream(iodev, ctx, userdata, &h)
// ---------------------------------------------------------------------------

@adc_producer@
identifier h;
expression iodev, ctx, ud;
@@
adc_stream(iodev, ctx, ud, &h)

@depends on patch && adc_producer@
identifier adc_producer.h;
@@
- struct rtio_sqe *h;
+ rtio_sqe_handle_t h;

@depends on patch && adc_producer@
identifier adc_producer.h;
expression adc_producer.ctx;
@@
- rtio_sqe_cancel(h)
+ rtio_sqe_cancel(ctx, h)

// ---------------------------------------------------------------------------
// RTIO_SQE_CANCELED flag checks -> rtio_iodev_sqe_is_canceled()
//
// Cancellation state moved from the (deprecated) RTIO_SQE_CANCELED bit in
// sqe.flags into the atomic status word of the owning rtio_iodev_sqe. Reads
// of the flag become rtio_iodev_sqe_is_canceled() calls; writes cannot be
// rewritten mechanically (request cancellation with rtio_sqe_cancel()) and
// are flagged in report mode below.
// ---------------------------------------------------------------------------

@depends on patch@
expression E;
@@
- FIELD_GET(RTIO_SQE_CANCELED, E->sqe.flags) == 1
+ rtio_iodev_sqe_is_canceled(E)

@depends on patch@
expression E;
@@
- FIELD_GET(RTIO_SQE_CANCELED, E->sqe.flags) == 0
+ !rtio_iodev_sqe_is_canceled(E)

@depends on patch@
expression E;
@@
- FIELD_GET(RTIO_SQE_CANCELED, E->sqe.flags)
+ rtio_iodev_sqe_is_canceled(E)

@depends on patch@
expression E;
@@
- FIELD_GET(RTIO_SQE_CANCELED, E.sqe.flags)
+ rtio_iodev_sqe_is_canceled(&E)

@depends on patch@
expression E;
@@
- (E->sqe.flags & RTIO_SQE_CANCELED) != 0
+ rtio_iodev_sqe_is_canceled(E)

@depends on patch@
expression E;
@@
- (E->sqe.flags & RTIO_SQE_CANCELED) == 0
+ !rtio_iodev_sqe_is_canceled(E)

@depends on patch@
expression E;
@@
- E->sqe.flags & RTIO_SQE_CANCELED
+ rtio_iodev_sqe_is_canceled(E)

@depends on patch@
expression E;
@@
- E.sqe.flags & RTIO_SQE_CANCELED
+ rtio_iodev_sqe_is_canceled(&E)

// ---------------------------------------------------------------------------
// report mode: flag cancel calls that still use the old single-argument form
// (e.g. struct-field / cross-function handles the patch cannot rewrite).
// ---------------------------------------------------------------------------

@leftover depends on report@
expression h;
position p;
@@
rtio_sqe_cancel@p(h)

@script:python depends on report@
p << leftover.p;
@@
coccilib.report.print_report(p[0],
	"rtio_sqe_cancel() now takes (struct rtio *r, rtio_sqe_handle_t handle); "
	"update this call and the handle's type by hand.")

@leftover_canceled depends on report@
position p;
@@
RTIO_SQE_CANCELED@p

@script:python depends on report@
p << leftover_canceled.p;
@@
coccilib.report.print_report(p[0],
	"RTIO_SQE_CANCELED is deprecated; cancellation state moved to the "
	"rtio_iodev_sqe status word. Read it with rtio_iodev_sqe_is_canceled() "
	"and request cancellation with rtio_sqe_cancel().")
