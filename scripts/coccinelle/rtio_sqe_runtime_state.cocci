// SPDX-License-Identifier: Apache-2.0
//
// Migrate drivers off the runtime state that moved out of `struct rtio_sqe`
// into the owning `struct rtio_iodev_sqe` (Zephyr 4.5), so that the submission
// description stays immutable once submitted.
//
// Three members moved into the wrapper's `rt` scratch union:
//
//     -  iodev_sqe->sqe.await.ok / .callback / .userdata
//     +  iodev_sqe->rt.await.ok / .callback / .userdata
//
//     -  iodev_sqe->sqe.delay.expiry
//     +  iodev_sqe->rt.delay_expiry
//
// Both are plain field moves on an rtio_iodev_sqe and are rewritten in patch
// mode. Reaching them through a bare `struct rtio_sqe *` no longer works at
// all: get to the wrapper first, with CONTAINER_OF() or the rtio_iodev_sqe the
// op was handed. Those sites fail to compile, so there is nothing to flag.
//
// The mempool RX buffer moved the same way, but its migration is not a field
// move. rtio_sqe_rx_buf() used to write the block it allocated back into
// sqe.rx.buf / sqe.rx.buf_len, so a driver could read the buffer back out of
// the sqe in a later step. It now binds into iodev_sqe->rt.rx_bind, which is
// internal, and sqe.rx stays as the caller submitted it: NULL for a mempool
// submission, and the requested maximum rather than the allocated length. The
// compiler says nothing. A driver has to call rtio_sqe_rx_buf() again where it
// uses the buffer; the call is idempotent and returns the already bound block.
//
// Rewriting such a read needs the buffer length the driver wants and an error
// path for a failed allocation, neither of which a semantic patch can supply,
// so report mode flags them instead. It flags sqe.rx reads only in files that
// call rtio_sqe_rx_buf() themselves, since those are the ones that bind a
// mempool buffer and can therefore read back the wrong thing. A driver that
// never binds one, such as the I2C and I3C fallback handlers, reads exactly
// what the caller submitted both before and after this change.
//
// Usage:
//     spatch --sp-file scripts/coccinelle/rtio_sqe_runtime_state.cocci \
//            --macro-file scripts/coccinelle/macros.h --dir . --in-place
// or via the coccicheck wrapper (which passes the macro file itself):
//     MODE=patch scripts/coccicheck

virtual patch
virtual report

// ---------------------------------------------------------------------------
// OP_AWAIT signaling state -> iodev_sqe->rt.await
// ---------------------------------------------------------------------------

@depends on patch@
expression E;
identifier f = {ok, callback, userdata};
@@
- E->sqe.await.f
+ E->rt.await.f

@depends on patch@
expression E;
identifier f = {ok, callback, userdata};
@@
- E.sqe.await.f
+ E.rt.await.f

// ---------------------------------------------------------------------------
// OP_DELAY absolute expiration -> iodev_sqe->rt.delay_expiry
//
// Only the expiry moved. sqe.delay.timeout is the caller's input and stays.
// ---------------------------------------------------------------------------

@depends on patch@
expression E;
@@
- E->sqe.delay.expiry
+ E->rt.delay_expiry

@depends on patch@
expression E;
@@
- E.sqe.delay.expiry
+ E.rt.delay_expiry

// ---------------------------------------------------------------------------
// report mode: flag sqe.rx reads in files that bind a mempool buffer.
// ---------------------------------------------------------------------------

@binds_rx_buf@
@@
rtio_sqe_rx_buf(...)

@leftover_rx depends on report && binds_rx_buf@
expression E;
position p;
@@
(
E->sqe.rx.buf@p
|
E->sqe.rx.buf_len@p
|
E.sqe.rx.buf@p
|
E.sqe.rx.buf_len@p
)

@script:python depends on report@
p << leftover_rx.p;
@@
coccilib.report.print_report(p[0],
	"the RX buffer of a mempool submission is bound in the rtio_iodev_sqe, "
	"not in the immutable sqe. Call rtio_sqe_rx_buf() where the buffer is "
	"used; it returns the block already bound to the submission.")
