/*
 * Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/virtio.h>
#include <zephyr/drivers/virtio/virtqueue.h>
#include <zephyr/irq.h>
#include <zephyr/irq_offload.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/internal/mm.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/ztest.h>
#include <errno.h>

#define CONSOLE_NODE DT_CHOSEN(zephyr_console)
#define CONSOLE_IRQN DT_IRQN(DT_PARENT(CONSOLE_NODE))

/* TX queue of console port 0, see PORT_TO_TX_VQ_IDX() in the driver. */
#define CONSOLE_TX_VQ 1

/* Time allowed for a submitted character to be completed by the device. */
#define TX_IDLE_TIMEOUT_MS 1000

BUILD_ASSERT(CONFIG_UART_VIRTIO_CONSOLE_TX_BUFSIZE > 1,
	     "the buffer reuse test expects more than one TX buffer");

/* The suite runs single-CPU (CONFIG_MP_MAX_NUM_CPUS=1). */
static const struct device *const console = DEVICE_DT_GET(CONSOLE_NODE);
static const struct device *const console_vdev = DEVICE_DT_GET(DT_PARENT(CONSOLE_NODE));

/* The test platform uses virtio-pci; service masked completions through its ISR. */
extern void virtio_pci_isr(const struct device *dev);

static struct virtq *tx_vq;
static atomic_t tx_in_flight;
static uint16_t tx_avail_after_first;
static uint16_t tx_avail_after_second;
static uint16_t held_tx_desc;
K_SEM_DEFINE(poll_thread_started, 0, 1);
K_SEM_DEFINE(poll_thread_done, 0, 1);
static struct k_thread poll_thread;
K_THREAD_STACK_DEFINE(poll_thread_stack, 1024);
static uint16_t poll_thread_avail_after;
static int fifo_fill_result;
static uint16_t fifo_fill_avail_after;
static atomic_t tx_callback_count;
static atomic_t tx_callback_outside_isr;
static atomic_t retry_pending;
static atomic_t retry_result;
static int irq_tx_ready_result;
static int irq_tx_complete_result;
static int irq_pending_result;

static void wait_tx_idle(void)
{
	for (uint32_t i = 0U; i < TX_IDLE_TIMEOUT_MS; i++) {
		if (virtq_get_free_desc_count(tx_vq) == tx_vq->num) {
			return;
		}
		k_sleep(K_MSEC(1));
	}

	zassert_equal(virtq_get_free_desc_count(tx_vq), tx_vq->num, "TX queue did not drain");
}

static bool wait_vq_used(struct virtq *vq, uint16_t used_before)
{
	for (uint32_t i = 0U; i < TX_IDLE_TIMEOUT_MS; i++) {
		if (sys_le16_to_cpu(vq->used->idx) != used_before) {
			return true;
		}
		k_sleep(K_MSEC(1));
	}

	return false;
}

static bool wait_tx_used(uint16_t used_before)
{
	return wait_vq_used(tx_vq, used_before);
}

static uint16_t tx_avail_idx(void)
{
	return sys_le16_to_cpu(tx_vq->avail->idx);
}

/* Must only be called after at least one character has been submitted. */
static uint64_t tx_last_desc_addr(void)
{
	uint16_t avail_idx = tx_avail_idx();
	uint16_t ring_idx = (avail_idx - 1) & (tx_vq->num - 1);
	uint16_t head = sys_le16_to_cpu(tx_vq->avail->ring[ring_idx]);

	return sys_le64_to_cpu(tx_vq->desc[head].addr);
}

/*
 * Submit two characters with interrupts locked and sample the ring after each
 * call to verify that a busy queue drops both characters without waiting.
 *
 * No assertion may run here: a failing zassert would longjmp and leave
 * interrupts disabled.
 */
static void send_two_chars(char first, char second)
{
	unsigned int key;

	key = irq_lock();
	uart_poll_out(console, first);
	tx_avail_after_first = tx_avail_idx();
	uart_poll_out(console, second);
	tx_avail_after_second = tx_avail_idx();
	atomic_set(&tx_in_flight, tx_vq->num - virtq_get_free_desc_count(tx_vq));
	irq_unlock(key);
}

static void hold_tx_queue(void)
{
	zassert_ok(virtq_get_free_desc(tx_vq, &held_tx_desc, K_NO_WAIT),
		   "could not reserve the TX descriptor");
}

static void release_tx_queue(void)
{
	virtq_add_free_desc(tx_vq, held_tx_desc);
}

static int hold_tx_buffer(uint64_t addr)
{
	struct virtq_buf vqbuf = {
		.addr = k_mem_virt_addr((uintptr_t)addr),
		.len = sizeof(char),
	};

	return virtq_add_buffer_chain(tx_vq, &vqbuf, 1, 1, NULL, NULL, K_NO_WAIT);
}

static void send_two_chars_from_isr(const void *arg)
{
	ARG_UNUSED(arg);

	send_two_chars('A', 'B');
}

static void drop_chars_from_isr(const void *arg)
{
	ARG_UNUSED(arg);

	for (size_t i = 0U; i < CONFIG_UART_VIRTIO_CONSOLE_TX_BUFSIZE + 1U; i++) {
		uart_poll_out(console, 'F');
	}
}

static void poll_second_char(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sem_give(&poll_thread_started);
	uart_poll_out(console, 'B');
	poll_thread_avail_after = tx_avail_idx();
	k_sem_give(&poll_thread_done);
}

static void call_virtio_pci_isr(const void *arg)
{
	virtio_pci_isr(arg);
}

static void process_tx_used_from_isr(const void *arg)
{
	ARG_UNUSED(arg);

	virtq_process_used(tx_vq);
}

static void read_irq_tx_status_from_isr(const void *arg)
{
	ARG_UNUSED(arg);

	uart_irq_update(console);
	irq_tx_ready_result = uart_irq_tx_ready(console);
	irq_tx_complete_result = uart_irq_tx_complete(console);
	irq_pending_result = uart_irq_is_pending(console);
}

static void enable_tx_irq_from_isr(const void *arg)
{
	ARG_UNUSED(arg);

	uart_irq_tx_enable(console);
}

static void fifo_fill_two_from_isr(const void *arg)
{
	static const uint8_t chars[] = {'A', 'B'};

	ARG_UNUSED(arg);
	fifo_fill_result = uart_fifo_fill(console, chars, ARRAY_SIZE(chars));
	fifo_fill_avail_after = tx_avail_idx();
}

static void count_tx_callback(const struct device *dev, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	atomic_inc(&tx_callback_count);
	if (!k_is_in_isr()) {
		atomic_set(&tx_callback_outside_isr, 1);
	}
}

static void retry_one_char_from_tx_callback(const struct device *dev, void *user_data)
{
	static const uint8_t retry_char = 'B';

	count_tx_callback(dev, user_data);
	uart_irq_update(dev);
	if ((uart_irq_is_pending(dev) != 0) && (uart_irq_tx_ready(dev) > 0) &&
	    atomic_cas(&retry_pending, 1, 0)) {
		atomic_set(&retry_result, uart_fifo_fill(dev, &retry_char, 1));
	}
}

static void fifo_fill_one_from_isr(const void *arg)
{
	static const uint8_t first_char = 'A';

	ARG_UNUSED(arg);
	fifo_fill_result = uart_fifo_fill(console, &first_char, 1);
}

/* Submit one character, wait for it to complete and return its buffer address. */
static uint64_t send_and_get_addr(char c)
{
	uart_poll_out(console, c);
	wait_tx_idle();

	return tx_last_desc_addr();
}

/* Submit one character and check that it was published to the available ring. */
static void send_and_check_accepted(char c)
{
	uint16_t avail_before = tx_avail_idx();

	uart_poll_out(console, c);
	zassert_equal(tx_avail_idx(), (uint16_t)(avail_before + 1), "the character was dropped");
}

ZTEST(uart_virtio_console, test_poll_out_with_irqs_locked)
{
	uint16_t avail_before;
	uint16_t used_after_first = 0U;
	uint16_t used_before;
	bool completed;
	bool second_completed = false;
	atomic_val_t callbacks_after_poll = 0;
	atomic_val_t callbacks_after_isr;
	atomic_val_t callbacks_outside_isr;
	unsigned int key;
	unsigned int irq = CONSOLE_IRQN;

	wait_tx_idle();
	uart_irq_tx_disable(console);
	atomic_set(&tx_callback_count, 0);
	atomic_set(&tx_callback_outside_isr, 0);
	zassert_ok(uart_irq_callback_user_data_set(console, count_tx_callback, NULL));
	irq_offload(enable_tx_irq_from_isr, NULL);
	avail_before = tx_avail_idx();
	used_before = sys_le16_to_cpu(tx_vq->used->idx);

	irq_disable(irq);
	uart_poll_out(console, 'C');
	tx_avail_after_first = tx_avail_idx();
	completed = wait_tx_used(used_before);
	if (completed) {
		used_after_first = sys_le16_to_cpu(tx_vq->used->idx);
		key = irq_lock();
		uart_poll_out(console, 'D');
		tx_avail_after_second = tx_avail_idx();
		atomic_set(&tx_in_flight, tx_vq->num - virtq_get_free_desc_count(tx_vq));
		callbacks_after_poll = atomic_get(&tx_callback_count);
		irq_unlock(key);
		irq_offload(call_virtio_pci_isr, (void *)console_vdev);
		second_completed = wait_tx_used(used_after_first);
		if (second_completed) {
			irq_offload(call_virtio_pci_isr, (void *)console_vdev);
		}
	}
	irq_enable(irq);
	callbacks_after_isr = atomic_get(&tx_callback_count);
	callbacks_outside_isr = atomic_get(&tx_callback_outside_isr);
	uart_irq_tx_disable(console);
	zassert_ok(uart_irq_callback_user_data_set(console, NULL, NULL));

	zassert_true(completed, "the device did not complete TX while its IRQ was masked");
	zassert_equal(tx_avail_after_first, (uint16_t)(avail_before + 1),
		      "the first character was not submitted");
	zassert_equal(tx_avail_after_second, (uint16_t)(tx_avail_after_first + 1U),
		      "poll_out dropped a character while interrupts were locked");
	zassert_equal(atomic_get(&tx_in_flight), 1, "the second character was not submitted");
	zassert_true(second_completed, "the second character did not complete");
	zassert_equal(callbacks_after_poll, 1, "poll_out invoked a completion callback");
	zassert_true(callbacks_after_isr >= 2, "the transport ISR did not notify TX completion");
	zassert_equal(callbacks_outside_isr, 0, "a TX completion callback ran outside an ISR");

	wait_tx_idle();

	send_and_check_accepted('.');
	wait_tx_idle();
}

ZTEST(uart_virtio_console, test_thread_reclaimed_tx_notifies_once_from_isr)
{
	uint16_t used_before;
	uint16_t used_after_reclaim;
	uint16_t last_used_after_reclaim;
	uint16_t used_after_isr;
	atomic_val_t callbacks_before;
	atomic_val_t callbacks_after_reclaim;
	atomic_val_t callbacks_after_isr;
	atomic_val_t callbacks_after_second_isr;
	atomic_val_t callbacks_outside_isr;
	unsigned int irq = CONSOLE_IRQN;
	bool completed;

	wait_tx_idle();
	uart_irq_tx_disable(console);
	atomic_set(&tx_callback_count, 0);
	atomic_set(&tx_callback_outside_isr, 0);
	zassert_ok(uart_irq_callback_user_data_set(console, count_tx_callback, NULL));
	irq_offload(enable_tx_irq_from_isr, NULL);
	callbacks_before = atomic_get(&tx_callback_count);
	used_before = sys_le16_to_cpu(tx_vq->used->idx);

	irq_disable(irq);
	uart_poll_out(console, 'C');
	completed = wait_tx_used(used_before);
	/* Leave only the coalesced notification for the transport ISR. */
	virtq_process_used(tx_vq);
	used_after_reclaim = sys_le16_to_cpu(tx_vq->used->idx);
	last_used_after_reclaim = tx_vq->last_used_idx;
	callbacks_after_reclaim = atomic_get(&tx_callback_count);
	irq_offload(call_virtio_pci_isr, (void *)console_vdev);
	used_after_isr = sys_le16_to_cpu(tx_vq->used->idx);
	callbacks_after_isr = atomic_get(&tx_callback_count);
	/*
	 * The real PCI ISR cleared the device status, so call the used-ring
	 * processing directly to make sure it does not repeat the notification.
	 */
	irq_offload(process_tx_used_from_isr, NULL);
	callbacks_after_second_isr = atomic_get(&tx_callback_count);
	callbacks_outside_isr = atomic_get(&tx_callback_outside_isr);
	uart_irq_tx_disable(console);
	irq_enable(irq);
	zassert_ok(uart_irq_callback_user_data_set(console, NULL, NULL));

	zassert_true(completed, "the device did not complete TX while its IRQ was masked");
	zassert_equal(last_used_after_reclaim, used_after_reclaim,
		      "thread reclamation left an unconsumed used entry");
	zassert_equal(callbacks_after_reclaim, callbacks_before,
		      "thread reclamation invoked a completion callback");
	zassert_equal(used_after_isr, used_after_reclaim, "the ISR received a new used entry");
	zassert_equal(callbacks_after_isr, callbacks_before + 1,
		      "the transport ISR did not deliver exactly one completion notification");
	zassert_equal(callbacks_after_second_isr, callbacks_after_isr,
		      "a later used-ring pass repeated the completion notification");
	zassert_equal(callbacks_outside_isr, 0, "a TX completion callback ran outside an ISR");
	wait_tx_idle();
}

ZTEST(uart_virtio_console, test_poll_out_from_isr)
{
	uint16_t avail_held;

	wait_tx_idle();
	hold_tx_queue();
	avail_held = tx_avail_idx();

	irq_offload(send_two_chars_from_isr, NULL);
	zassert_equal(tx_avail_after_first, avail_held,
		      "the ISR unexpectedly submitted a character");
	zassert_equal(tx_avail_after_second, avail_held,
		      "the ISR unexpectedly submitted a character");
	zassert_equal(atomic_get(&tx_in_flight), 1, "the reserved descriptor was lost");

	release_tx_queue();
	wait_tx_idle();

	send_and_check_accepted('.');
	wait_tx_idle();
}

ZTEST(uart_virtio_console, test_printk_transmits_all_characters)
{
	uint16_t avail_before;
	uint16_t avail_after;

	wait_tx_idle();
	avail_before = tx_avail_idx();
	printk("AB");
	avail_after = tx_avail_idx();
	wait_tx_idle();

	zassert_equal(avail_after, (uint16_t)(avail_before + 2U), "printk dropped a character");
}

ZTEST(uart_virtio_console, test_invalid_used_id_does_not_write_console)
{
	struct virtq vq;
	uint16_t desc;
	uint16_t avail_before;
	uint16_t avail_after;
	uint16_t last_used;
	uint32_t invalid_count;
	unsigned int key;

	zassert_ok(virtq_create(&vq, 1U), "could not create a test virtqueue");
	zassert_ok(virtq_get_free_desc(&vq, &desc, K_NO_WAIT), "could not reserve descriptor 0");
	vq.used->ring[0].id = sys_cpu_to_le32(0x10000U);
	vq.used->idx = sys_cpu_to_le16(1U);
	wait_tx_idle();
	avail_before = tx_avail_idx();
	key = irq_lock();
	virtq_process_used(&vq);
	irq_unlock(key);
	avail_after = tx_avail_idx();
	invalid_count = vq.invalid_used_count;
	last_used = vq.last_used_idx;
	virtq_free(&vq);

	zassert_equal(invalid_count, 1U, "the invalid descriptor ID was not counted");
	zassert_equal(last_used, 1U, "the invalid used entry was not consumed");
	zassert_equal(avail_after, avail_before, "used-ring processing wrote to the UART console");
}

ZTEST(uart_virtio_console, test_dropped_chars_do_not_consume_tx_buffers)
{
	uint64_t first_addr;
	uint64_t second_addr;
	uint8_t *held_buffer;
	uint8_t held_value;
	uint8_t held_value_after_drop;
	uint16_t avail_held;
	uint16_t avail_after_drop;
	uint16_t used_before;
	uint64_t desc_addr_held;
	uint64_t desc_addr_after_drop;
	unsigned int irq = CONSOLE_IRQN;
	bool completed;
	int ret;

	wait_tx_idle();

	/* Locate the slot the next accepted character uses and its successor. */
	first_addr = send_and_get_addr('W');
	second_addr = send_and_get_addr('X');
	/* Fill the rest of the ring, so the next character wraps to first_addr. */
	for (size_t i = 2U; i < CONFIG_UART_VIRTIO_CONSOLE_TX_BUFSIZE; i++) {
		(void)send_and_get_addr('W');
	}

	/* Keep the descriptor allocated even if the device completes before the drops. */
	used_before = sys_le16_to_cpu(tx_vq->used->idx);
	irq_disable(irq);
	held_buffer = k_mem_virt_addr((uintptr_t)first_addr);
	held_value = *held_buffer;
	ret = hold_tx_buffer(first_addr);
	if (ret != 0) {
		irq_enable(irq);
		zassert_ok(ret, "could not submit the TX buffer");
		return;
	}
	desc_addr_held = tx_last_desc_addr();
	avail_held = tx_avail_idx();
	virtio_notify_virtqueue(console_vdev, CONSOLE_TX_VQ);
	/*
	 * Drop one full ring of characters from an ISR while the queue is held.
	 * None of them may be published or consume a TX buffer.
	 */
	irq_offload(drop_chars_from_isr, NULL);
	avail_after_drop = tx_avail_idx();
	held_value_after_drop = *held_buffer;
	desc_addr_after_drop = tx_last_desc_addr();

	completed = wait_tx_used(used_before);
	if (completed) {
		irq_offload(call_virtio_pci_isr, (void *)console_vdev);
	}
	irq_enable(irq);
	zassert_true(completed, "the device did not complete the held TX buffer");
	wait_tx_idle();

	zassert_equal(desc_addr_held, first_addr,
		      "the outstanding descriptor does not reference the TX buffer");
	zassert_equal(held_value_after_drop, held_value,
		      "a dropped character overwrote a buffer held by TX");
	zassert_equal(avail_after_drop, avail_held, "a dropped character was published");
	zassert_equal(desc_addr_after_drop, first_addr,
		      "the outstanding descriptor no longer references the TX buffer");

	/* The next accepted character must reuse the slot that was not consumed. */
	send_and_check_accepted('I');
	zassert_equal(tx_last_desc_addr(), first_addr, "a dropped character consumed a TX buffer");
	zassert_not_equal(first_addr, second_addr, "the test did not locate distinct TX buffers");
	wait_tx_idle();
}

ZTEST(uart_virtio_console, test_poll_out_waits_for_tx_completion)
{
	uint16_t avail_after_first;
	uint16_t avail_while_irq_disabled;
	uint16_t used_before_first;
	uint16_t used_after_first;
	bool returned_before_irq;
	int wait_result;
	int join_result = -EIO;
	bool first_completed;
	bool second_completed = false;
	unsigned int irq = CONSOLE_IRQN;
	k_tid_t thread_id;

	wait_tx_idle();
	k_sem_reset(&poll_thread_started);
	k_sem_reset(&poll_thread_done);
	poll_thread_avail_after = 0U;
	used_before_first = sys_le16_to_cpu(tx_vq->used->idx);

	irq_disable(irq);
	uart_poll_out(console, 'A');
	avail_after_first = tx_avail_idx();
	thread_id = k_thread_create(&poll_thread, poll_thread_stack,
				    K_THREAD_STACK_SIZEOF(poll_thread_stack), poll_second_char,
				    NULL, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	int started = thread_id == NULL ? -EIO : k_sem_take(&poll_thread_started, K_MSEC(100));

	if (thread_id != NULL) {
		k_yield();
		returned_before_irq = (k_sem_take(&poll_thread_done, K_NO_WAIT) == 0);
		avail_while_irq_disabled = tx_avail_idx();
	} else {
		returned_before_irq = true;
		avail_while_irq_disabled = 0U;
	}
	first_completed = wait_tx_used(used_before_first);
	if (first_completed) {
		used_after_first = sys_le16_to_cpu(tx_vq->used->idx);
		irq_offload(call_virtio_pci_isr, (void *)console_vdev);
	}
	wait_result = (thread_id != NULL) && (started == 0) && first_completed
			      ? k_sem_take(&poll_thread_done, K_MSEC(TX_IDLE_TIMEOUT_MS))
			      : -EIO;
	if (wait_result == 0) {
		second_completed = wait_tx_used(used_after_first);
		if (second_completed) {
			irq_offload(call_virtio_pci_isr, (void *)console_vdev);
		}
	}
	irq_enable(irq);
	if (wait_result == 0) {
		join_result = k_thread_join(&poll_thread, K_MSEC(TX_IDLE_TIMEOUT_MS));
	} else if (thread_id != NULL) {
		k_thread_abort(&poll_thread);
	}

	zassert_not_null(thread_id, "could not create the second poll_out thread");
	zassert_ok(started, "the second poll_out thread did not start");
	zassert_false(returned_before_irq, "poll_out returned before TX completion was processed");
	zassert_true(first_completed, "the device did not complete TX while its IRQ was masked");
	zassert_equal(avail_while_irq_disabled, avail_after_first,
		      "the second character was submitted before the descriptor was reclaimed");
	zassert_ok(wait_result, "TX completion did not wake the waiting poll_out thread");
	zassert_true(second_completed, "the second character was not completed");
	zassert_ok(join_result, "the second poll_out thread did not exit");
	zassert_equal(poll_thread_avail_after, (uint16_t)(avail_after_first + 1),
		      "the waiting thread did not submit its character after the wakeup");
	wait_tx_idle();
}

ZTEST(uart_virtio_console, test_fifo_fill_reports_accepted_bytes_and_retries)
{
	uint16_t avail_held;
	uint16_t avail_before;
	uint16_t avail_after;

	wait_tx_idle();
	hold_tx_queue();
	avail_held = tx_avail_idx();
	irq_offload(read_irq_tx_status_from_isr, NULL);
	zassert_false(irq_tx_ready_result, "TX reported ready with its descriptor reserved");
	zassert_false(irq_tx_complete_result, "TX reported complete with a descriptor reserved");

	irq_offload(fifo_fill_two_from_isr, NULL);
	zassert_equal(fifo_fill_result, 0,
		      "fifo_fill reported bytes while no descriptor was available");
	zassert_equal(fifo_fill_avail_after, avail_held,
		      "fifo_fill submitted bytes without a descriptor");

	release_tx_queue();
	wait_tx_idle();
	irq_offload(read_irq_tx_status_from_isr, NULL);
	zassert_true(irq_tx_ready_result, "TX did not become ready after descriptor completion");
	zassert_true(irq_tx_complete_result,
		     "TX did not become complete after descriptor completion");

	avail_before = tx_avail_idx();
	irq_offload(fifo_fill_two_from_isr, NULL);
	avail_after = tx_avail_idx();
	zassert_equal(fifo_fill_result, (int)(uint16_t)(avail_after - avail_before),
		      "fifo_fill count did not match the number of submitted bytes");
	wait_tx_idle();

	zassert_ok(uart_irq_callback_user_data_set(console, retry_one_char_from_tx_callback, NULL));
	atomic_set(&tx_callback_count, 0);
	atomic_set(&tx_callback_outside_isr, 0);
	atomic_set(&retry_pending, 0);
	atomic_set(&retry_result, -1);
	irq_offload(enable_tx_irq_from_isr, NULL);
	atomic_set(&retry_pending, 1);
	avail_before = tx_avail_idx();
	irq_offload(fifo_fill_one_from_isr, NULL);
	wait_tx_idle();
	avail_after = tx_avail_idx();

	zassert_equal(fifo_fill_result, 1, "fifo_fill did not report its accepted byte");
	zassert_equal(atomic_get(&retry_result), 1,
		      "the TX callback did not retry the remaining byte");
	zassert_equal(avail_after, (uint16_t)(avail_before + 2),
		      "the retry byte was not submitted");
	zassert_true(atomic_get(&tx_callback_count) > 1,
		     "TX completion did not invoke the callback");
	zassert_equal(atomic_get(&tx_callback_outside_isr), 0,
		      "the retry callback ran outside an ISR");
	uart_irq_tx_disable(console);
	zassert_ok(uart_irq_callback_user_data_set(console, NULL, NULL));
}

ZTEST(uart_virtio_console, test_tx_irq_enable_disable)
{
	atomic_val_t callbacks_disabled;
	atomic_val_t callbacks_enabled;
	atomic_val_t callbacks_disabled_again;
	atomic_val_t callbacks_reenabled = 0;
	atomic_val_t callbacks_outside_isr;
	uint16_t used_before;
	int pending_disabled;
	int pending_enabled;
	int pending_disabled_again;
	unsigned int irq = CONSOLE_IRQN;
	bool completed;

	wait_tx_idle();
	uart_irq_tx_disable(console);
	atomic_set(&tx_callback_count, 0);
	atomic_set(&tx_callback_outside_isr, 0);
	zassert_ok(uart_irq_callback_user_data_set(console, count_tx_callback, NULL));
	(void)send_and_get_addr('A');
	callbacks_disabled = atomic_get(&tx_callback_count);
	irq_offload(read_irq_tx_status_from_isr, NULL);
	pending_disabled = irq_pending_result;

	irq_offload(enable_tx_irq_from_isr, NULL);
	(void)send_and_get_addr('B');
	callbacks_enabled = atomic_get(&tx_callback_count);
	irq_offload(read_irq_tx_status_from_isr, NULL);
	pending_enabled = irq_pending_result;

	/* Disable TX notifications after the device completes but before its ISR runs. */
	used_before = sys_le16_to_cpu(tx_vq->used->idx);
	irq_disable(irq);
	uart_poll_out(console, 'C');
	completed = wait_tx_used(used_before);
	uart_irq_tx_disable(console);
	if (completed) {
		irq_offload(call_virtio_pci_isr, (void *)console_vdev);
	}
	irq_enable(irq);
	callbacks_disabled_again = atomic_get(&tx_callback_count);
	irq_offload(read_irq_tx_status_from_isr, NULL);
	pending_disabled_again = irq_pending_result;
	if (completed) {
		wait_tx_idle();
		irq_offload(enable_tx_irq_from_isr, NULL);
		(void)send_and_get_addr('D');
		callbacks_reenabled = atomic_get(&tx_callback_count);
	}
	callbacks_outside_isr = atomic_get(&tx_callback_outside_isr);
	uart_irq_tx_disable(console);
	zassert_ok(uart_irq_callback_user_data_set(console, NULL, NULL));

	zassert_true(completed, "the device did not complete TX while its IRQ was masked");
	zassert_equal(callbacks_disabled, 0, "TX completion notified while TX IRQ was disabled");
	zassert_equal(callbacks_enabled, 2, "TX enable or completion notification was missing");
	zassert_equal(callbacks_disabled_again, callbacks_enabled,
		      "a pending completion notified after TX IRQ was disabled");
	zassert_equal(callbacks_reenabled, 4, "TX notifications did not resume after re-enabling");
	zassert_equal(callbacks_outside_isr, 0, "a TX callback ran outside an ISR");
	zassert_equal(pending_disabled, 0, "TX IRQ reported pending while disabled");
	zassert_equal(pending_enabled, 1, "enabled TX-ready IRQ did not report pending");
	zassert_equal(pending_disabled_again, 0, "disabled TX IRQ remained pending");
}

struct process_used_ctx {
	struct virtq *vq;
	uint8_t buf[2][8];
	int calls;
	bool requeue_failed;
};

static void process_used_requeue_cb(void *opaque, uint32_t used_len)
{
	struct process_used_ctx *ctx = opaque;
	struct virtq_buf vqbuf = {.addr = ctx->buf[1], .len = sizeof(ctx->buf[1])};
	uint16_t avail, head;

	ARG_UNUSED(used_len);
	ctx->calls++;
	if (ctx->calls != 1) {
		return;
	}

	if (virtq_add_buffer_chain(ctx->vq, &vqbuf, 1, 0, process_used_requeue_cb, ctx,
				   K_NO_WAIT) != 0) {
		ctx->requeue_failed = true;
		return;
	}

	avail = sys_le16_to_cpu(ctx->vq->avail->idx);
	head = sys_le16_to_cpu(ctx->vq->avail->ring[(avail - 1U) & (ctx->vq->num - 1U)]);
	ctx->vq->used->ring[1].id = sys_cpu_to_le32(head);
	ctx->vq->used->ring[1].len = sys_cpu_to_le32(sizeof(ctx->buf[1]));
	barrier_dmem_fence_full();
	ctx->vq->used->idx = sys_cpu_to_le16(2U);
}

ZTEST(uart_virtio_console, test_process_used_pass_is_bounded)
{
	struct process_used_ctx ctx = {0};
	struct virtq vq;
	struct virtq_buf vqbuf = {.addr = ctx.buf[0], .len = sizeof(ctx.buf[0])};
	uint16_t head;

	zassert_ok(virtq_create(&vq, 4U), "could not create a test virtqueue");
	ctx.vq = &vq;

	zassert_ok(
		virtq_add_buffer_chain(&vq, &vqbuf, 1, 0, process_used_requeue_cb, &ctx, K_NO_WAIT),
		"could not submit the first test buffer");
	head = sys_le16_to_cpu(vq.avail->ring[0]);
	vq.used->ring[0].id = sys_cpu_to_le32(head);
	vq.used->ring[0].len = sys_cpu_to_le32(sizeof(ctx.buf[0]));
	barrier_dmem_fence_full();
	vq.used->idx = sys_cpu_to_le16(1U);

	virtq_process_used(&vq);
	zassert_false(ctx.requeue_failed, "could not requeue the test buffer");
	zassert_equal(vq.last_used_idx, 1U,
		      "a pass consumed a completion produced by its own callback");
	zassert_equal(ctx.calls, 1, "the first completion callback did not run");

	virtq_process_used(&vq);
	zassert_equal(vq.last_used_idx, 2U,
		      "the next pass did not consume the remaining completion");
	zassert_equal(ctx.calls, 2, "the second completion callback did not run");

	virtq_free(&vq);
}

static void count_completion_cb(void *opaque, uint32_t used_len)
{
	int *calls = opaque;

	ARG_UNUSED(used_len);
	(*calls)++;
}

ZTEST(uart_virtio_console, test_process_used_faults_on_oversized_used_idx)
{
	struct virtq vq;
	uint8_t buf[8];
	struct virtq_buf vqbuf = {.addr = buf, .len = sizeof(buf)};
	uint16_t head;
	int calls = 0;

	zassert_ok(virtq_create(&vq, 4U), "could not create a test virtqueue");
	zassert_ok(
		virtq_add_buffer_chain(&vq, &vqbuf, 1, 0, count_completion_cb, &calls, K_NO_WAIT),
		"could not submit the test buffer");
	head = sys_le16_to_cpu(vq.avail->ring[0]);

	/* Every used slot points at the in-flight buffer. */
	for (uint16_t i = 0U; i < vq.num; i++) {
		vq.used->ring[i].id = sys_cpu_to_le32(head);
		vq.used->ring[i].len = sys_cpu_to_le32(sizeof(buf));
	}
	barrier_dmem_fence_full();

	/*
	 * A compliant device cannot complete more buffers than the queue holds.
	 * A far-ahead used index must reject the whole batch instead of
	 * processing slots that may belong to older completions.
	 */
	vq.used->idx = sys_cpu_to_le16(0xFFFFU);
	virtq_process_used(&vq);

	zassert_true(vq.faulted, "the oversized used index did not fault the queue");
	zassert_equal(vq.last_used_idx, 0U, "the oversized batch was processed");
	zassert_equal(calls, 0, "a callback ran for the oversized batch");
	zassert_equal(virtq_get_free_desc_count(&vq), vq.num - 1U,
		      "the in-flight descriptor was returned");
	zassert_true(vq.invalid_used_count > 0U, "the oversized used index was not counted");

	virtq_free(&vq);
}

ZTEST(uart_virtio_console, test_process_used_rejects_cyclic_chain)
{
	struct virtq vq;
	uint8_t buf[2][8];
	struct virtq_buf vqbuf[] = {
		{.addr = buf[0], .len = sizeof(buf[0])},
		{.addr = buf[1], .len = sizeof(buf[1])},
	};
	uint16_t head, second;
	int calls = 0;

	zassert_ok(virtq_create(&vq, 4U), "could not create a test virtqueue");
	zassert_ok(virtq_add_buffer_chain(&vq, vqbuf, ARRAY_SIZE(vqbuf), 0, count_completion_cb,
					  &calls, K_NO_WAIT),
		   "could not submit the test chain");
	head = sys_le16_to_cpu(vq.avail->ring[0]);
	second = sys_le16_to_cpu(vq.desc[head].next);

	/* Close the chain into a cycle. */
	vq.desc[second].flags = sys_cpu_to_le16(VIRTQ_DESC_F_NEXT);
	vq.desc[second].next = sys_cpu_to_le16(head);
	vq.used->ring[0].id = sys_cpu_to_le32(head);
	vq.used->ring[0].len = sys_cpu_to_le32(sizeof(buf[0]) + sizeof(buf[1]));
	barrier_dmem_fence_full();
	vq.used->idx = sys_cpu_to_le16(1U);

	virtq_process_used(&vq);

	zassert_equal(vq.last_used_idx, 1U, "the cyclic entry was not consumed");
	zassert_false(vq.faulted, "a malformed chain faulted the whole queue");
	zassert_true(vq.invalid_used_count > 0U, "the cyclic chain was not counted");
	zassert_equal(virtq_get_free_desc_count(&vq), 2U,
		      "the malformed chain modified the free descriptor stack");
	zassert_equal(calls, 0, "the callback ran for a cyclic chain");

	virtq_free(&vq);
}

static void *suite_setup(void)
{
	zassert_true(device_is_ready(console), "console device is not ready");
	zassert_true(device_is_ready(console_vdev), "virtio transport is not ready");

	tx_vq = virtio_get_virtqueue(console_vdev, CONSOLE_TX_VQ);
	zassert_not_null(tx_vq, "console TX virtqueue is not available");

	if (IS_ENABLED(CONFIG_LOG_BACKEND_UART) && !IS_ENABLED(CONFIG_LOG_BACKEND_UART_AUTOSTART)) {
		const struct log_backend *backend = log_backend_get_by_name("log_backend_uart");

		zassert_not_null(backend, "UART log backend is not available");
		log_backend_init(backend);
		log_backend_enable(backend, backend->cb->ctx, LOG_LEVEL_DBG);
	}

	return NULL;
}

ZTEST_SUITE(uart_virtio_console, NULL, suite_setup, NULL, NULL, NULL);
