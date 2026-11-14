#include <errno.h>
#include <zephyr/drivers/virtualization/ivshmem.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/printk.h>

#define ROUNDS       100
#define READY        0x49565348U
#define PAYLOAD_BASE 0x5a5a0000U
#define CHECK_BASE   0xa5a5ffffU
#define TIMEOUT_MS   5000

struct peer_slot {
	uint32_t ready;
	uint32_t sequence;
	uint32_t payload;
	uint32_t checksum;
	uint32_t done;
};

static int wait_value(volatile uint32_t *value, uint32_t expected, struct k_poll_event *event)
{
	int64_t deadline = k_uptime_get() + TIMEOUT_MS;

	while (true) {
		int64_t remaining;
		int ret;

		/* Reset before checking memory so a notification cannot be lost here. */
		k_poll_signal_reset(event->signal);
		event->state = K_POLL_STATE_NOT_READY;
		barrier_dmem_fence_full();

		if (*value == expected) {
			barrier_dmem_fence_full();
			return 0;
		}
		remaining = deadline - k_uptime_get();
		if (remaining <= 0) {
			return -ETIMEDOUT;
		}
		ret = k_poll(event, 1, K_MSEC(remaining));
		if ((ret != 0) && (ret != -EAGAIN)) {
			return ret;
		}
	}
}

static int send_message(const struct device *dev, volatile struct peer_slot *slot, uint32_t peer,
			uint32_t sequence)
{
	slot->payload = PAYLOAD_BASE ^ sequence;
	slot->checksum = slot->payload ^ sequence ^ CHECK_BASE;
	barrier_dmem_fence_full();
	slot->sequence = sequence;

	return ivshmem_int_peer(dev, peer, 0);
}

static int receive_message(volatile struct peer_slot *slot, uint32_t sequence,
			   struct k_poll_event *event)
{
	int ret = wait_value(&slot->sequence, sequence, event);

	if (ret != 0) {
		return ret;
	}
	if ((slot->payload != (PAYLOAD_BASE ^ sequence)) ||
	    (slot->checksum != (slot->payload ^ sequence ^ CHECK_BASE))) {
		return -EIO;
	}

	return 0;
}

static int ping_pong(const struct device *dev, uint32_t id, volatile struct peer_slot *slots,
		     struct k_poll_event *event)
{
	uint32_t peer = 1 - id;
	volatile struct peer_slot *own = &slots[id];
	volatile struct peer_slot *remote = &slots[peer];
	int64_t deadline = k_uptime_get() + 30000;
	int ret;

	own->sequence = 0;
	own->done = 0;
	barrier_dmem_fence_full();
	own->ready = READY;
	printk("Peer %u ready\n", id);

	/* Peer eventfds may not exist yet. Do not ring until both VMs are ready. */
	while (remote->ready != READY) {
		if (k_uptime_get() >= deadline) {
			return -ETIMEDOUT;
		}
		k_msleep(1);
		barrier_dmem_fence_full();
	}

	for (uint32_t sequence = 1; sequence <= ROUNDS; sequence++) {
		if (id == 0) {
			ret = send_message(dev, own, peer, sequence);
			if (ret != 0) {
				return ret;
			}
		}
		ret = receive_message(remote, sequence, event);
		if (ret != 0) {
			return ret;
		}
		if (id == 1) {
			ret = send_message(dev, own, peer, sequence);
			if (ret != 0) {
				return ret;
			}
		}
	}

	/* The responder waits until the final echo has also been validated. */
	if (id == 1) {
		ret = wait_value(&remote->done, 1, event);
		if (ret != 0) {
			return ret;
		}
	}
	own->done = 1;
	ret = ivshmem_int_peer(dev, peer, 0);
	if (ret != 0) {
		return ret;
	}
	if (id == 0) {
		return wait_value(&remote->done, 1, event);
	}

	return 0;
}

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(ivshmem));
	struct k_poll_signal signal;
	struct k_poll_event event =
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &signal);
	uintptr_t memory;
	uint32_t id;
	int ret;
	int unregister_ret;

	if (!device_is_ready(dev)) {
		printk("FAIL: device not ready\n");
		return 0;
	}

	id = ivshmem_get_id(dev);
	if ((id > 1) || (ivshmem_get_vectors(dev) != 1) ||
	    (ivshmem_get_mem(dev, &memory) < 2 * sizeof(struct peer_slot))) {
		printk("FAIL: expected two peers, one vector and shared memory\n");
		return 0;
	}
	k_poll_signal_init(&signal);
	ret = ivshmem_register_handler(dev, &signal, 0);
	if (ret != 0) {
		printk("FAIL: register handler: %d\n", ret);
		return 0;
	}
	ret = ping_pong(dev, id, (volatile struct peer_slot *)memory, &event);
	unregister_ret = ivshmem_register_handler(dev, NULL, 0);
	if (ret != 0) {
		printk("FAIL: peer %u ping-pong: %d\n", id, ret);
	} else if (unregister_ret != 0) {
		printk("FAIL: unregister handler: %d\n", unregister_ret);
	} else {
		printk("PASS: peer %u completed %u rounds\n", id, ROUNDS);
	}

	return 0;
}
