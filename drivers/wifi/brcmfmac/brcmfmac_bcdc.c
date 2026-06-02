/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * BCDC (Broadcom Comm Driver Codec) protocol over SDPCM/SDIO F2.
 *
 * dedicated RX kthread drains F2 and dispatches frames
 * by SDPCM channel:
 *   chan=0 (control) -> match by reqid, copy payload into the
 *                       waiting query_dcmd caller's buffer, signal
 *                       its completion semaphore.
 *   chan=1 (event)   -> raw frame body handed to the registered
 *                       event callback (if any).
 *   chan=2 (data)    -> logged + discarded (net_if arrives in 4.5).
 *
 * query_dcmd is now async at the protocol level: TX, then block on
 * a per-request semaphore until the RX thread signals or timeout.
 * F2's CIS rdy_timeout is overridden to 0 so sdio_enable_func polls
 * IOR directly instead of sleeping ~2 s per chip CIS request.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/sd/sdio.h>

#include "brcmfmac_priv.h"

LOG_MODULE_DECLARE(brcmfmac, CONFIG_WIFI_LOG_LEVEL);

/* RX buffer for the two-phase CMD53 read in `brcmfmac_rx_thread_fn`.
 * Must hold the largest SDPCM frame the chip will deliver in a single
 * F2 FIFO read. Empirically we've seen ~4.4 KB aggregated
 * frames during a TCP-fed bulk upload; SDPCM headers, BDC headers,
 * Ethernet header, and any A-MSDU framing push the worst case above
 * what we initially predicted. 8192 covers everything observed in
 * practice with comfortable slack. If `fh->len > sizeof(rx_buf)` ever
 * trips again, the right move is dynamic net_pkt allocation rather
 * than another static bump.
 *
 * History: was 512 -> truncated frames >~470 B, surfaced as TCP
 * checksum drops upstream. Bumped to 2048 -> exposed CMD53 overread
 * DATA_CRC () which forced the two-phase split.
 * Then sized to 8192 here to absorb chip-side aggregation.
 */
#define BRCMFMAC_BCDC_RX_BUF       8192
#define BRCMFMAC_BCDC_TIMEOUT_MS  2000
#define BRCMFMAC_RX_IDLE_SLEEP_MS    1

/* Single-instance stack + thread storage. DT only describes one
 * brcmfmac node today; if multi-instance is wanted later, move
 * these into a per-instance macro expansion in core.c.
 */
K_KERNEL_STACK_DEFINE(brcmfmac_rx_stack, CONFIG_WIFI_BRCMFMAC_RX_THREAD_STACK_SIZE);
static struct k_thread brcmfmac_rx_thread;

K_KERNEL_STACK_DEFINE(brcmfmac_tx_stack, 4096);
static struct k_thread brcmfmac_tx_thread;

static void brcmfmac_tx_thread_fn(void *p1, void *p2, void *p3);

/* Counter for periodic ISR-rate logging from the RX thread. */
static atomic_t brcmf_isr_fires;

/* SDPCM per-frame flow-control state.
 *
 * The chip puts a flow-control byte at sw->flow in every SDPCM header it
 * sends. Each bit corresponds to an access category (BE/BK/VO/VI); a set
 * bit means "host should stop sending traffic on this AC." Mirrors Linux
 * brcmf_sdio_hdparse + TX gate at sdio.c:2911.
 *
 * Simplification vs Linux: we collapse all ACs to a single xoff/xon
 * boolean. Any non-zero sw->flow stops all host TX; zero re-enables.
 * Fine-grained per-AC was overkill for the current single-iface workload.
 */
static atomic_t brcmf_fcstate;		/* 0 = ok to send, 1 = chip xoff'd */
static uint8_t  brcmf_flowcontrol;	/* last sw->flow byte seen */

static const char *const brcmfmac_bcme[] = {
	"BCME_OK",
	"BCME_ERROR",
	"BCME_BADARG",
	"BCME_BADOPTION",
	"BCME_NOTUP",
	"BCME_NOTDOWN",
	"BCME_NOTAP",
	"BCME_NOTSTA",
	"BCME_BADKEYIDX",
	"BCME_RADIOOFF",
	"BCME_NOTBANDLOCKED",
	"BCME_NOCLK",
	"BCME_BADRATESET",
	"BCME_BADBAND",
	"BCME_BUFTOOSHORT",
	"BCME_BUFTOOLONG",
	"BCME_BUSY",
	"BCME_NOTASSOCIATED",
	"BCME_BADSSIDLEN",
	"BCME_OUTOFRANGECHAN",
	"BCME_BADCHAN",
	"BCME_BADADDR",
	"BCME_NORESOURCE",
	"BCME_UNSUPPORTED",
	"BCME_BADLEN",
	"BCME_NOTREADY",
	"BCME_EPERM",
	"BCME_NOMEM",
	"BCME_ASSOCIATED",
	"BCME_RANGE",
	"BCME_NOTFOUND",
	"BCME_WME_NOT_ENABLED",
	"BCME_TSPEC_NOTFOUND",
	"BCME_ACM_NOTSUPPORTED",
	"BCME_NOT_WME_ASSOCIATION",
	"BCME_SDIO_ERROR",
	"BCME_DONGLE_DOWN",
	"BCME_VERSION",
	"BCME_TXFAIL",
	"BCME_RXFAIL",
	"BCME_NODEVICE",
	"BCME_NMODE_DISABLED",
	"BCME_NONRESIDENT",
	"BCME_SCANREJECT",
	"BCME_USAGE_ERROR",
	"BCME_IOCTL_ERROR",
	"BCME_SERIAL_PORT_ERR",
	"BCME_DISABLED",
	"BCME_DECERR",
	"BCME_ENCERR",
	"BCME_MICERR",
	"BCME_REPLAY",
	"BCME_IE_NOTFOUND",
};

static const char *brcmfmac_bcme_str(int error)
{
	error = -error;
	if (error < 0 || error >= ARRAY_SIZE(brcmfmac_bcme)) {
		return "unknown";
	}

	return brcmfmac_bcme[error];
}

static void brcmfmac_handle_ctrl(struct brcmfmac_data *data,
				 struct cdc_hdr *rcdc, uint16_t outlen)
{
	uint16_t reqid = (uint16_t)(rcdc->flags >> BCDC_REQ_ID_SHIFT);

	if (!data->pending.active || reqid != data->pending.reqid) {
		LOG_DBG("rx ctrl: unmatched reqid=%u (pending.active=%d pending.reqid=%u)",
			reqid, data->pending.active, data->pending.reqid);
		return;
	}

	if (rcdc->flags & BCDC_FLAG_ERROR) {
		LOG_WRN("rx ctrl: chip BCDC error (status=%s reqid=%u)",
			brcmfmac_bcme_str(rcdc->status), reqid);
		data->pending.status = -EIO;
		data->pending.out_copied = 0;
	} else {
		uint16_t want = outlen;

		if (want > data->pending.out_capacity) {
			want = data->pending.out_capacity;
		}
		if (want > 0 && data->pending.out_buf != NULL) {
			memcpy(data->pending.out_buf,
			       (uint8_t *)rcdc + sizeof(*rcdc), want);
		}
		data->pending.out_copied = want;
		data->pending.status = 0;
	}

	k_sem_give(&data->pending.done);
}

/* Strip SDPCM headers, hand the body (BDC + L2 frame, or BDC + event frame)
 * to the net layer. Both chan=1 (event) and chan=2 (data) use this shape.
 */
static const uint8_t *brcmfmac_rx_body(struct sdpcm_sw_hdr *sw,
				       uint16_t total_len, uint16_t *body_len_out)
{
	uint16_t hdr_len = sw->hdrlen;

	if (hdr_len > total_len) {
		LOG_WRN("rx: hdrlen=%u > total=%u", hdr_len, total_len);
		return NULL;
	}
	*body_len_out = (uint16_t)(total_len - hdr_len);
	return (const uint8_t *)sw + (hdr_len - sizeof(struct sdpcm_frame_hdr));
}

/* SDHC SDIO_INT callback: chip asserted DAT1 (CARD_INT). Wake the RX
 * thread. Runs in ISR context; keep it minimal. The SDHC driver auto-
 * masks SIGNAL_ENABLE[CARD_INT] before invoking the callback, so we
 * re-arm via sdhc_enable_interrupt() in brcmfmac_rx_wait() below.
 */
static void brcmfmac_card_int_cb(const struct device *sdhc_dev, int reason,
				 const void *user_data)
{
	ARG_UNUSED(sdhc_dev);
	struct brcmfmac_data *data = (struct brcmfmac_data *)user_data;

	if (reason == SDHC_INT_SDIO) {
		atomic_inc(&brcmf_isr_fires);
		k_sem_give(&data->rx_irq_sem);
	}
}

/* Block on the SDIO_INT semaphore with a short safety-net timeout.
 * The k_sem_take timeout caps the worst-case latency when an SDIO
 * IRQ is missed; 1 ms keeps DHCP-class round-trips healthy.
 */
static void brcmfmac_rx_wait(struct brcmfmac_data *data)
{
	(void)sdhc_enable_interrupt(data->card.sdhc, brcmfmac_card_int_cb,
				    SDHC_INT_SDIO, data);
	(void)k_sem_take(&data->rx_irq_sem, K_MSEC(1));
}

static void brcmfmac_rx_thread_fn(void *p1, void *p2, void *p3)
{
	struct brcmfmac_data *data = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	static uint8_t rx_buf[BRCMFMAC_BCDC_RX_BUF] __aligned(4);
	static uint32_t rx_iters;
	static uint32_t rx_valid;
	static uint32_t rx_ist_acc;  /* OR of masked SDPCMD_INTSTATUS this window */

	LOG_DBG("rx thread started");

	while (1) {
		rx_iters++;
		if ((rx_iters & 0x3FF) == 0) {
			LOG_DBG("rx: t=%lldms iters=%u isr=%ld valid=%u",
				k_uptime_get(), rx_iters,
				(long)atomic_get(&brcmf_isr_fires), rx_valid);
			LOG_DBG("rx: ist=0x%x txseq=%u tx_max=%u",
				rx_ist_acc, data->sdpcm_txseq,
				data->sdpcm_tx_max);
			rx_ist_acc = 0;
		}

		/* Ack the chip-side SDPCMD intstatus BEFORE the speculative
		 * F2 read. Matches Linux's sdio_irq_thread /
		 * brcmf_sdio_intr_rstatus ordering: clear the latched HMB_SW
		 * bits as soon as the host has been notified, so DAT1 can
		 * deassert and the SDHC card-int line doesn't re-fire on an
		 * empty F2 (which would otherwise storm the ISR every time
		 * brcmfmac_rx_wait re-arms SIGNAL_ENABLE[CARD_INT]).
		 */
		{
			const struct bcm_core *sdio_core =
				brcmfmac_chip_core_find(data, BCMA_CORE_SDIO_DEV);
			if (sdio_core != NULL) {
				uint32_t ist = 0;

				if (brcmfmac_sdio_backplane_read32(
					    data,
					    sdio_core->base + SDPCMD_INTSTATUS,
					    &ist) == 0) {
					uint32_t masked =
						ist & BRCMFMAC_HOSTINTMASK;

					rx_ist_acc |= masked;
					if (masked != 0) {
						(void)brcmfmac_sdio_backplane_write32(
							data,
							sdio_core->base +
								SDPCMD_INTSTATUS,
							masked);
					}
				}
			}
		}

		/* Read one block first to learn the frame size.
		 *
		 * The CYW43439 F2 FIFO only yields the bytes it actually has
		 * queued. A multi-block CMD53 that requests more blocks than
		 * the chip has buffered comes back with a DATA_CRC error from
		 * the SDHCI controller -- the chip stops driving the bus before
		 * the requested block count is satisfied. So we can't issue a
		 * speculative max-MTU read up front; we have to size each
		 * transfer to what's actually available.
		 *
		 * The first block always contains the SDPCM frame header at
		 * offset 0 (fh->len + fh->notlen XOR check). Once we have
		 * that, fh->len tells us the total frame size -- including
		 * the bytes we already read in this first block. If more
		 * blocks are needed, phase 2 reads them in a single follow-up
		 * CMD53 sized to the exact remaining block count.
		 *
		 * Linux brcmfmac avoids the second CMD53 by carrying
		 * sw->nextlen forward as a hint for the *next* frame's first
		 * read. That optimisation is parked (see project memory
		 * `project_rpi_zero_2w_parking_lot.md` item 9).
		 */
		int ret = sdio_read_addr(&data->radio, BRCMFMAC_F2_FIFO_ADDR,
					 rx_buf, BRCMFMAC_F2_BLOCK_SIZE);
		if (ret != 0) {
			LOG_DBG("rx thread: F2 read failed: %d", ret);
			brcmfmac_rx_wait(data);
			continue;
		}

		struct sdpcm_frame_hdr *fh = (void *)rx_buf;
		struct sdpcm_sw_hdr *sw = (void *)(rx_buf + sizeof(*fh));

		uint16_t xor_check = (uint16_t)(fh->len ^ fh->notlen);

		if (xor_check != 0xFFFFu) {
			/* No valid frame queued -- chip's F2 returned junk. */
			brcmfmac_rx_wait(data);
			continue;
		}
		if (fh->len < sizeof(*fh) + sizeof(*sw)) {
			/* Header-only frame -- chip flow control / credit
			 * signaling. Linux's brcmfmac handles these silently.
			 */
			LOG_DBG("rx thread: short frame (len=%u)", fh->len);
			brcmfmac_rx_wait(data);
			continue;
		}

		/* Update tx-window from the chip. Mirrors Linux's
		 * brcmf_sdio_hdparse: every valid SDPCM header carries the
		 * chip's current tx_seq_max in sw->credit. Clamp to seq+2 if
		 * the window is implausibly far ahead (>0x40 of our txseq) --
		 * Linux does the same to defend against firmware bugs. Then
		 * wake any TX path waiting on credits.
		 */
		{
			uint8_t new_max = sw->credit;

			if ((uint8_t)(new_max - data->sdpcm_txseq) > 0x40) {
				new_max = data->sdpcm_txseq + 2;
			}
			data->sdpcm_tx_max = new_max;
			k_sem_give(&data->tx_credit_sem);
			rx_valid++;

			/* SDPCM per-frame flow-control byte. Mirrors Linux
			 * brcmf_sdio_hdparse: when the chip's host-RX queue
			 * fills it sets bits in sw->flow telling host to stop
			 * sending. Without this we overrun the chip's WLAN TX
			 * queue and trip a multi-second internal recovery.
			 */
			uint8_t new_fc = sw->flow;

			if (brcmf_flowcontrol != new_fc) {
				bool now_xoff = new_fc != 0;

				brcmf_flowcontrol = new_fc;
				atomic_set(&brcmf_fcstate, now_xoff ? 1 : 0);
				LOG_DBG("fc: %s flow=0x%02x",
					now_xoff ? "xoff" : "xon", new_fc);
			}
		}

		/* If the frame spans more than one block, read the
		 * remaining blocks in a single follow-up CMD53. fh->len is
		 * total frame length; we already have the first
		 * BRCMFMAC_F2_BLOCK_SIZE bytes. Round up to the next block.
		 */
		if (fh->len > BRCMFMAC_F2_BLOCK_SIZE) {
			uint16_t remaining = fh->len - BRCMFMAC_F2_BLOCK_SIZE;
			uint16_t extra_blocks =
				(remaining + BRCMFMAC_F2_BLOCK_SIZE - 1) /
				BRCMFMAC_F2_BLOCK_SIZE;
			size_t extra_bytes = (size_t)extra_blocks *
					     BRCMFMAC_F2_BLOCK_SIZE;
			if (BRCMFMAC_F2_BLOCK_SIZE + extra_bytes >
			    sizeof(rx_buf)) {
				/* Frame too big for rx_buf. Drain the rest of
				 * the frame from the FIFO in rx_buf-sized
				 * chunks so the next read starts on a clean
				 * frame boundary -- otherwise we'd interpret
				 * mid-frame bytes as a new SDPCM header and
				 * either crash or feed garbage upstream.
				 */
				LOG_WRN("rx: frame too big (len=%u, max=%zu) -- draining",
					fh->len, sizeof(rx_buf));
				size_t to_drain = extra_bytes;

				while (to_drain > 0) {
					size_t chunk = to_drain < sizeof(rx_buf)
						       ? to_drain
						       : sizeof(rx_buf);
					int dret = sdio_read_addr(
						&data->radio,
						BRCMFMAC_F2_FIFO_ADDR,
						rx_buf, chunk);
					if (dret != 0) {
						LOG_WRN("rx: drain failed (%d), FIFO desynced",
							dret);
						break;
					}
					to_drain -= chunk;
				}
				continue;
			}
			ret = sdio_read_addr(&data->radio,
					     BRCMFMAC_F2_FIFO_ADDR,
					     rx_buf + BRCMFMAC_F2_BLOCK_SIZE,
					     extra_bytes);
			if (ret != 0) {
				LOG_WRN("rx: phase-2 read failed (len=%u, ret=%d)",
					fh->len, ret);
				brcmfmac_rx_wait(data);
				continue;
			}
		}

		switch (sw->chan) {
		case SDPCM_CHAN_CTRL: {
			if (fh->len < sizeof(*fh) + sizeof(*sw) + sizeof(struct cdc_hdr)) {
				LOG_WRN("rx ctrl: undersized (len=%u)", fh->len);
				break;
			}
			struct cdc_hdr *rcdc = (void *)((uint8_t *)sw + sizeof(*sw));
			uint16_t payload_avail = (uint16_t)(fh->len - sizeof(*fh)
							     - sizeof(*sw)
							     - sizeof(*rcdc));
			uint16_t outlen = rcdc->outlen;

			if (outlen > payload_avail) {
				outlen = payload_avail;
			}
			brcmfmac_handle_ctrl(data, rcdc, outlen);
			break;
		}
		case SDPCM_CHAN_EVENT: {
			uint16_t body_len;
			const uint8_t *body = brcmfmac_rx_body(sw, fh->len, &body_len);

			if (body != NULL) {
				brcmfmac_net_rx_event(data, body, body_len);
			}
			break;
		}
		case SDPCM_CHAN_DATA: {
			uint16_t body_len;
			const uint8_t *body = brcmfmac_rx_body(sw, fh->len, &body_len);

			if (body != NULL) {
				brcmfmac_net_rx_data(data, body, body_len);
			}
			break;
		}
		default:
			LOG_DBG("rx: unknown chan=%u len=%u", sw->chan, fh->len);
			break;
		}
	}
}

/* Enable F2 + wait for IOR ourselves. Zephyr's sdio_enable_func sleeps
 * for the full CIS-advertised rdy_timeout (~2 s on this chip) before
 * its first IOR poll; setting rdy_timeout=0 just makes it poll too
 * fast (CONFIG_SD_RETRY_COUNT x ~0 ms = ~3 ms total, chip is not
 * ready that quickly). Linux brcmfmac polls IOR with ~10 ms gaps up
 * to ~500 ms; we mirror that.
 */
static int brcmfmac_bcdc_enable_f2(struct brcmfmac_data *data)
{
	uint8_t reg;
	int ret = sdio_read_byte(&data->card.func0, SDIO_CCCR_IO_EN, &reg);

	if (ret != 0) {
		return ret;
	}
	reg |= BIT(SDIO_FUNC_NUM_2);
	ret = sdio_write_byte(&data->card.func0, SDIO_CCCR_IO_EN, reg);
	if (ret != 0) {
		return ret;
	}

	int64_t t0 = k_uptime_get();

	for (int i = 0; i < 50; i++) {
		ret = sdio_read_byte(&data->card.func0, SDIO_CCCR_IO_RD, &reg);
		if (ret != 0) {
			return ret;
		}
		if (reg & BIT(SDIO_FUNC_NUM_2)) {
			LOG_DBG("F2 IOR up after %lld ms",
				(long long)(k_uptime_get() - t0));
			return 0;
		}
		k_msleep(10);
	}
	LOG_ERR("F2 IOR timeout after %lld ms (IOR=0x%02x)",
		(long long)(k_uptime_get() - t0), reg);
	return -ETIMEDOUT;
}

int brcmfmac_bcdc_init(struct brcmfmac_data *data)
{
	int ret = sdio_init_func(&data->card, &data->radio, SDIO_FUNC_NUM_2);

	if (ret != 0) {
		LOG_ERR("sdio_init_func(F2) failed: %d", ret);
		return ret;
	}

	ret = brcmfmac_bcdc_enable_f2(data);
	if (ret != 0) {
		LOG_ERR("F2 enable failed: %d", ret);
		return ret;
	}

	ret = sdio_set_block_size(&data->radio, BRCMFMAC_F2_BLOCK_SIZE);
	if (ret != 0) {
		LOG_ERR("sdio_set_block_size(F2, %u) failed: %d",
			BRCMFMAC_F2_BLOCK_SIZE, ret);
		return ret;
	}

	k_mutex_init(&data->bcdc_mutex);
	k_sem_init(&data->pending.done, 0, 1);
	data->pending.active = false;

	/* Tell the chip which intstatus bits should assert DAT1. Without this
	 * the chip's SDIO core stays silent on the bus even when it has frames
	 * queued -- our SDHC CARD_INT path would never see an assert, and the
	 * RX thread would sleep at its safety-net timeout forever. Linux does
	 * the same write in brcmf_sdio_init right after F2 is up.
	 */
	{
		const struct bcm_core *sdio_core =
			brcmfmac_chip_core_find(data, BCMA_CORE_SDIO_DEV);

		if (sdio_core != NULL) {
			int rc2 = brcmfmac_sdio_backplane_write32(
				data, sdio_core->base + SDPCMD_HOSTINTMASK,
				BRCMFMAC_HOSTINTMASK);
			if (rc2 != 0) {
				LOG_WRN("hostintmask write failed: %d (CARD_INT path will be slow)",
					rc2);
			} else {
				LOG_DBG("chip hostintmask = 0x%08x",
					BRCMFMAC_HOSTINTMASK);
			}
		}
	}

	/* Enable the SDIO card's per-function interrupt output via CCCR IENx
	 * (F0 register 0x04). Bit 0 = master enable (IENM); bit N = function N
	 * IRQ enable. Without this the chip's SDIO core can have HMB bits set
	 * but won't actually pull DAT1 low to interrupt the host. Mirrors
	 * Linux's brcmf_sdiod_intr_register (brcmfmac/bcmsdh.c:144-147), which
	 * enables FUNC0 | FUNC1 | FUNC2: the chip's SDPCMD core sits on F1, so
	 * HMB frame-ready signals are F1-level IRQs -- enabling F2 alone
	 * leaves DAT1 silent even with HOSTINTMASK + intstatus set.
	 */
	{
		uint8_t ienx = 0;
		int rc2 = sdio_read_byte(&data->card.func0,
					 SDIO_CCCR_INT_EN, &ienx);
		if (rc2 == 0) {
			ienx |= 0x01;                       /* IENM master */
			ienx |= 1u << SDIO_FUNC_NUM_1;      /* F1 (SDPCMD/HMB) */
			ienx |= 1u << SDIO_FUNC_NUM_2;      /* F2 (data path) */
			rc2 = sdio_write_byte(&data->card.func0,
					      SDIO_CCCR_INT_EN, ienx);
		}
		if (rc2 != 0) {
			LOG_WRN("CCCR IENx setup failed: %d (CARD_INT will not fire)",
				rc2);
		} else {
			LOG_DBG("CCCR IENx = 0x%02x (master + F1 SDPCMD + F2 data)",
				ienx);
		}
	}

	/* SDPCM tx-credit accounting (mirrors Linux brcmfmac/sdio.c).
	 * Initial window of 4 lets the first 4 outgoing frames fly before
	 * we need the chip to report its tx_max in an RX SDPCM header.
	 */
	data->sdpcm_txseq = 0;
	data->sdpcm_tx_max = 4;
	k_sem_init(&data->tx_credit_sem, 0, 1);

	/* SDIO in-band IRQ -- replaces the old 1 ms RX poll. The SDHC
	 * driver auto-masks SIGNAL_ENABLE[CARD_INT] each time the ISR
	 * fires; brcmfmac_rx_wait() re-arms via sdhc_enable_interrupt()
	 * (idempotent). First call from the rx thread also performs the
	 * initial enable.
	 */
	k_sem_init(&data->rx_irq_sem, 0, 1);

	k_thread_create(&brcmfmac_rx_thread, brcmfmac_rx_stack,
			K_KERNEL_STACK_SIZEOF(brcmfmac_rx_stack),
			brcmfmac_rx_thread_fn, data, NULL, NULL,
			CONFIG_WIFI_BRCMFMAC_RX_THREAD_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(&brcmfmac_rx_thread, "brcmfmac_rx");

	/* TX glom ring + worker. iface_send copies each net frame into a
	 * pre-allocated slot and signals tx_pending_sem; the tx-thread
	 * drains the ring, packs up to N frames into tx_glom_buf, and
	 * issues one CMD53 per burst.
	 */
	atomic_set(&data->tx_ring_head, 0);
	atomic_set(&data->tx_ring_tail, 0);
	k_sem_init(&data->tx_pending_sem, 0, 1);
	for (int i = 0; i < BRCMFMAC_TX_RING_SLOTS; i++) {
		data->tx_ring[i].len = 0;
	}

	/* TX thread at one priority LOWER than RX (CONFIG_WIFI_BRCMFMAC_RX_THREAD_PRIO + 1).
	 * RX must preempt TX so that credit-grant SDPCM headers are parsed
	 * promptly; with both at the same priority and timeslicing off (Zephyr
	 * default), a back-to-back-CMD53 TX loop starves RX and credits stall.
	 */
	k_thread_create(&brcmfmac_tx_thread, brcmfmac_tx_stack,
			K_KERNEL_STACK_SIZEOF(brcmfmac_tx_stack),
			brcmfmac_tx_thread_fn, data, NULL, NULL,
			CONFIG_WIFI_BRCMFMAC_RX_THREAD_PRIO + 1, 0, K_NO_WAIT);
	k_thread_name_set(&brcmfmac_tx_thread, "brcmfmac_tx");

	data->f2_ready = true;
	LOG_DBG("F2 claimed (block_size=%u), rx+tx threads up",
		BRCMFMAC_F2_BLOCK_SIZE);
	return 0;
}

/* Build SDPCM headers in-place at the start of `frame` and TX `total`
 * bytes (padded to 4) via incrementing CMD53 on F2. Caller must hold
 * bcdc_mutex (serializes both txseq counter and F2 access).
 */
int brcmfmac_bcdc_tx_frame(struct brcmfmac_data *data, uint8_t chan,
			   uint8_t *frame, uint16_t total)
{
	struct sdpcm_frame_hdr *fh = (void *)frame;
	struct sdpcm_sw_hdr *sw = (void *)(frame + sizeof(*fh));

	/* Wait for SDPCM tx-credit AND chip-side flow control (mirrors
	 * Linux brcmfmac/sdio.c::data_ok + brcmu_pktq_mlen(..., ~flowcontrol)).
	 *
	 * Two independent gates:
	 *   (1) credit window: chip's tx_max - txseq must be > 0 (8-bit wrap
	 *       trick: if txseq has overtaken tx_max, (tx_max - txseq) is
	 *       >= 0x80, so a 0x80 mask catches both "no credits" and "neg
	 *       delta" in one check).
	 *   (2) flow control: chip's last sw->flow byte must be zero. When
	 *       chip's host-RX queue fills it sets bits here telling host to
	 *       hold. The rx-thread updates brcmf_fcstate from every RX
	 *       SDPCM header.
	 *
	 * Up to 100 ms total wait (5 x 20 ms). If we still can't send after
	 * that, return -EAGAIN so the caller can drop or requeue.
	 */
	for (int retry = 0; retry < 5; retry++) {
		uint8_t delta = (uint8_t)(data->sdpcm_tx_max - data->sdpcm_txseq);
		bool have_credit = (delta != 0 && (delta & 0x80) == 0);
		bool xoff = atomic_get(&brcmf_fcstate) != 0;

		if (have_credit && !xoff) {
			break;
		}
		if (k_sem_take(&data->tx_credit_sem, K_MSEC(20)) != 0) {
			LOG_WRN("tx_frame: credit/fc timeout (txseq=%u tx_max=%u fcstate=%ld)",
				data->sdpcm_txseq, data->sdpcm_tx_max,
				(long)atomic_get(&brcmf_fcstate));
			return -EAGAIN;
		}
	}

	fh->len = total;
	fh->notlen = (uint16_t)~total;

	sw->seq = data->sdpcm_txseq++;
	sw->chan = chan;
	sw->nextlen = 0;
	sw->hdrlen = (uint8_t)(sizeof(*fh) + sizeof(*sw));
	sw->flow = 0;
	sw->credit = 0;
	sw->reserved[0] = 0;
	sw->reserved[1] = 0;

	uint16_t padded = (total + 3) & ~3u;

	return sdio_write_addr(&data->radio, BRCMFMAC_F2_FIFO_ADDR, frame, padded);
}

/* TX glom worker. iface_send pushes pre-built SDPCM frames into the ring
 * (slot->data already has sw_hdr + bdc_hdr + payload at the right offsets;
 * only fh->len/notlen and sw->seq are filled in here under bcdc_mutex).
 *
 * Pack up to BRCMFMAC_TX_GLOM_MAX_FRAMES frames into tx_glom_buf, gated by:
 *   - chip's SDPCM tx-credit window (same delta check as bcdc_tx_frame)
 *   - chip's per-frame xoff (brcmf_fcstate)
 *   - glom buffer size (12 KB)
 *
 * Then one F2 CMD53 for the entire batch. If no credit/xoff at all, wait on
 * tx_credit_sem and retry (the rx thread signals it on every chip credit
 * update). If credit allows only a partial batch, send what we have now.
 */
static void brcmfmac_tx_thread_fn(void *p1, void *p2, void *p3)
{
	struct brcmfmac_data *data = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (;;) {
		(void)k_sem_take(&data->tx_pending_sem, K_FOREVER);

		while (atomic_get(&data->tx_ring_tail) !=
		       atomic_get(&data->tx_ring_head)) {

			k_mutex_lock(&data->bcdc_mutex, K_FOREVER);

			uint16_t glom_len = 0;
			uint8_t frame_count = 0;

			while (frame_count < BRCMFMAC_TX_GLOM_MAX_FRAMES) {
				int tail = atomic_get(&data->tx_ring_tail);
				int head = atomic_get(&data->tx_ring_head);

				if (tail == head) {
					break;
				}

				struct brcmfmac_tx_slot *slot = &data->tx_ring[tail];
				uint16_t padded = (slot->len + 3) & ~3u;

				if (glom_len + padded > BRCMFMAC_TX_GLOM_BUF_SIZE) {
					break;
				}

				uint8_t delta = (uint8_t)(data->sdpcm_tx_max -
							  data->sdpcm_txseq);
				bool have_credit = (delta != 0 &&
						    (delta & 0x80) == 0);
				bool xoff = atomic_get(&brcmf_fcstate) != 0;

				if (!have_credit || xoff) {
					break;
				}

				struct sdpcm_frame_hdr *fh =
					(struct sdpcm_frame_hdr *)slot->data;
				struct sdpcm_sw_hdr *sw =
					(struct sdpcm_sw_hdr *)(slot->data + sizeof(*fh));

				fh->len = slot->len;
				fh->notlen = (uint16_t)~slot->len;
				sw->seq = data->sdpcm_txseq++;

				memcpy(data->tx_glom_buf + glom_len, slot->data, slot->len);
				if (padded > slot->len) {
					memset(data->tx_glom_buf + glom_len + slot->len,
					       0, padded - slot->len);
				}
				glom_len += padded;
				frame_count++;

				atomic_set(&data->tx_ring_tail,
					   (tail + 1) % BRCMFMAC_TX_RING_SLOTS);
			}

			if (frame_count > 0) {
				int rc = sdio_write_addr(&data->radio,
							 BRCMFMAC_F2_FIFO_ADDR,
							 data->tx_glom_buf,
							 glom_len);
				if (rc != 0) {
					LOG_ERR("tx_glom: F2 TX failed: %d (count=%u len=%u)",
						rc, frame_count, glom_len);
				}
			}

			k_mutex_unlock(&data->bcdc_mutex);

			if (frame_count == 0) {
				/* Ring non-empty but no credit / xoff. Wait. */
				if (k_sem_take(&data->tx_credit_sem,
					       K_MSEC(100)) != 0) {
					LOG_WRN("tx_glom: credit timeout txseq=%u tx_max=%u fc=%ld",
						data->sdpcm_txseq,
						data->sdpcm_tx_max,
						(long)atomic_get(&brcmf_fcstate));
				}
			}
		}
	}
}

int brcmfmac_bcdc_query_dcmd(struct brcmfmac_data *data, uint32_t cmd,
			     const uint8_t *tx_payload, uint16_t tx_len,
			     uint8_t *rx_buf, uint16_t rx_capacity)
{
	if (!data->f2_ready) {
		return -EAGAIN;
	}

	int rc = k_mutex_lock(&data->bcdc_mutex, K_FOREVER);

	if (rc != 0) {
		return rc;
	}

	/* Sized to hold the largest GET response payload we care about
	 * inline: chip's "counters" struct (~840 B on this fw rev) and
	 * "fwcap" (~150 B). cdc.len doubles as the chip-side TX-room
	 * indicator, so the wire frame must be sized to rx_capacity, not
	 * tx_len -- see comment below.
	 */
	static uint8_t tx_buf[2048] __aligned(4);

	const size_t hdr_len = sizeof(struct sdpcm_frame_hdr)
			     + sizeof(struct sdpcm_sw_hdr)
			     + sizeof(struct cdc_hdr);

	if (hdr_len + tx_len > sizeof(tx_buf)) {
		rc = -EMSGSIZE;
		goto out;
	}

	memset(tx_buf, 0, sizeof(tx_buf));

	struct cdc_hdr *cdc = (void *)(tx_buf + sizeof(struct sdpcm_frame_hdr)
					      + sizeof(struct sdpcm_sw_hdr));

	/* cdc.len is one u32 -- the shared buffer size for both request
	 * parsing (chip reads name etc.) and response output (chip writes
	 * up to len bytes back). For a GET we need len >= rx_capacity or
	 * the chip returns BUFTOOSHORT (-14).
	 */
	uint16_t cdc_len = (tx_len > rx_capacity) ? tx_len : rx_capacity;

	cdc->cmd    = cmd;
	cdc->outlen = cdc_len;
	cdc->inlen  = 0;
	data->bcdc_reqid++;
	cdc->flags  = ((uint32_t)data->bcdc_reqid << BCDC_REQ_ID_SHIFT);
	cdc->status = 0;

	if (tx_payload != NULL && tx_len > 0) {
		memcpy((uint8_t *)cdc + sizeof(*cdc), tx_payload, tx_len);
	}

	/* Pad the on-wire frame to cdc_len so the chip sees room for its
	 * full response; any bytes past tx_len are already zeroed by the
	 * memset above (well, just the header was; pad explicitly).
	 */
	uint16_t payload_len = cdc_len;
	uint16_t total = (uint16_t)(hdr_len + payload_len);

	if (hdr_len + payload_len > sizeof(tx_buf)) {
		rc = -EMSGSIZE;
		goto out;
	}
	if (payload_len > tx_len) {
		memset((uint8_t *)cdc + sizeof(*cdc) + tx_len, 0,
		       payload_len - tx_len);
	}

	/* Publish the waiter context BEFORE TX -- the RX thread may race
	 * us to the response (chip can be fast).
	 */
	data->pending.reqid = data->bcdc_reqid;
	data->pending.out_buf = rx_buf;
	data->pending.out_capacity = rx_capacity;
	data->pending.out_copied = 0;
	data->pending.status = 0;
	k_sem_reset(&data->pending.done);
	data->pending.active = true;

	rc = brcmfmac_bcdc_tx_frame(data, SDPCM_CHAN_CTRL, tx_buf, total);
	if (rc != 0) {
		LOG_ERR("bcdc_query: TX failed: %d", rc);
		data->pending.active = false;
		goto out;
	}

	rc = k_sem_take(&data->pending.done, K_MSEC(BRCMFMAC_BCDC_TIMEOUT_MS));
	data->pending.active = false;

	if (rc == -EAGAIN) {
		LOG_ERR("bcdc_query: timeout waiting on reqid=%u",
			data->pending.reqid);
		rc = -ETIMEDOUT;
		goto out;
	}
	if (rc != 0) {
		goto out;
	}

	rc = (data->pending.status != 0)
		? data->pending.status
		: (int)data->pending.out_copied;

out:
	k_mutex_unlock(&data->bcdc_mutex);
	return rc;
}

int brcmfmac_bcdc_iovar_get(struct brcmfmac_data *data, const char *name,
			    uint8_t *buf, uint16_t len)
{
	size_t name_len = strlen(name) + 1;
	static uint8_t scratch[64] __aligned(4);

	if (name_len > sizeof(scratch)) {
		return -EMSGSIZE;
	}

	memset(scratch, 0, sizeof(scratch));
	memcpy(scratch, name, name_len);

	return brcmfmac_bcdc_query_dcmd(data, BRCMFMAC_WLC_GET_VAR,
					scratch, (uint16_t)name_len,
					buf, len);
}

/* SET dcmd: same TX path as query_dcmd, BCDC_FLAG_SET in flags, no
 * response payload (chip echoes status). We still wait for the chip's
 * matching reqid ack so the call is synchronous.
 */
int brcmfmac_bcdc_set_dcmd(struct brcmfmac_data *data, uint32_t cmd,
			   const uint8_t *tx_payload, uint16_t tx_len)
{
	if (!data->f2_ready) {
		return -EAGAIN;
	}

	int rc = k_mutex_lock(&data->bcdc_mutex, K_FOREVER);

	if (rc != 0) {
		return rc;
	}

	static uint8_t tx_buf[1024] __aligned(4);

	const size_t hdr_len = sizeof(struct sdpcm_frame_hdr)
			     + sizeof(struct sdpcm_sw_hdr)
			     + sizeof(struct cdc_hdr);

	if (hdr_len + tx_len > sizeof(tx_buf)) {
		rc = -EMSGSIZE;
		goto out;
	}

	memset(tx_buf, 0, hdr_len);

	struct cdc_hdr *cdc = (void *)(tx_buf + sizeof(struct sdpcm_frame_hdr)
					      + sizeof(struct sdpcm_sw_hdr));

	cdc->cmd    = cmd;
	cdc->outlen = tx_len;
	cdc->inlen  = 0;
	data->bcdc_reqid++;
	cdc->flags  = ((uint32_t)data->bcdc_reqid << BCDC_REQ_ID_SHIFT)
		    | BCDC_FLAG_SET;
	cdc->status = 0;

	if (tx_payload != NULL && tx_len > 0) {
		memcpy((uint8_t *)cdc + sizeof(*cdc), tx_payload, tx_len);
	}

	uint16_t total = (uint16_t)(hdr_len + tx_len);

	data->pending.reqid = data->bcdc_reqid;
	data->pending.out_buf = NULL;
	data->pending.out_capacity = 0;
	data->pending.out_copied = 0;
	data->pending.status = 0;
	k_sem_reset(&data->pending.done);
	data->pending.active = true;

	rc = brcmfmac_bcdc_tx_frame(data, SDPCM_CHAN_CTRL, tx_buf, total);
	if (rc != 0) {
		LOG_ERR("bcdc_set: TX failed: %d", rc);
		data->pending.active = false;
		goto out;
	}

	rc = k_sem_take(&data->pending.done, K_MSEC(BRCMFMAC_BCDC_TIMEOUT_MS));
	data->pending.active = false;

	if (rc == -EAGAIN) {
		LOG_ERR("bcdc_set: timeout on reqid=%u cmd=%u",
			data->pending.reqid, cmd);
		rc = -ETIMEDOUT;
		goto out;
	}
	if (rc == 0) {
		rc = data->pending.status;
	}

out:
	k_mutex_unlock(&data->bcdc_mutex);
	return rc;
}

int brcmfmac_bcdc_iovar_set(struct brcmfmac_data *data, const char *name,
			    const uint8_t *value, uint16_t value_len)
{
	size_t name_len = strlen(name) + 1;
	static uint8_t scratch[1024] __aligned(4);

	if (name_len + value_len > sizeof(scratch)) {
		return -EMSGSIZE;
	}

	memset(scratch, 0, name_len);
	memcpy(scratch, name, name_len);
	if (value != NULL && value_len > 0) {
		memcpy(scratch + name_len, value, value_len);
	}

	return brcmfmac_bcdc_set_dcmd(data, BRCMFMAC_WLC_SET_VAR,
				      scratch, (uint16_t)(name_len + value_len));
}

int brcmfmac_bcdc_bsscfg_iovar_set_int(struct brcmfmac_data *data,
				       const char *name, uint32_t bsscfgidx,
				       int32_t value)
{
	static uint8_t scratch[64] __aligned(4);
	const char *prefix = "bsscfg:";
	size_t prefix_len = strlen(prefix);
	size_t name_len = strlen(name) + 1;     /* include NUL */

	if (prefix_len + name_len + 8 > sizeof(scratch)) {
		return -EMSGSIZE;
	}

	uint8_t *p = scratch;

	memcpy(p, prefix, prefix_len);
	p += prefix_len;
	memcpy(p, name, name_len);
	p += name_len;
	/* bsscfgidx LE32 */
	p[0] = (uint8_t)(bsscfgidx & 0xFF);
	p[1] = (uint8_t)((bsscfgidx >> 8) & 0xFF);
	p[2] = (uint8_t)((bsscfgidx >> 16) & 0xFF);
	p[3] = (uint8_t)((bsscfgidx >> 24) & 0xFF);
	p += 4;
	/* value LE32 (signed) */
	uint32_t v = (uint32_t)value;

	p[0] = (uint8_t)(v & 0xFF);
	p[1] = (uint8_t)((v >> 8) & 0xFF);
	p[2] = (uint8_t)((v >> 16) & 0xFF);
	p[3] = (uint8_t)((v >> 24) & 0xFF);

	uint16_t total = (uint16_t)(prefix_len + name_len + 8);

	return brcmfmac_bcdc_set_dcmd(data, BRCMFMAC_WLC_SET_VAR, scratch, total);
}
