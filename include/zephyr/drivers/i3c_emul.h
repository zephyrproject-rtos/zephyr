/**
 * @file
 *
 * @brief Public APIs for the I3C emulation drivers.
 */

/*
 * Copyright (c) 2026 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_I3C_EMUL_H_
#define ZEPHYR_INCLUDE_DRIVERS_I3C_EMUL_H_

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/types.h>

/**
 * @brief I3C Emulation Interface
 * @defgroup i3c_emul_interface I3C Emulation Interface
 * @ingroup io_emulators
 * @ingroup i3c_interface
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

struct i3c_emul_api;

/**
 * Node in a linked list of emulators for I3C target devices on an emulated bus.
 *
 * The peripheral emulator typically embeds this struct in its own data
 * structure, fills in @ref static_addr / @ref pid / @ref bcr / @ref dcr from
 * its devicetree node, then attaches via @ref i3c_emul_register.
 */
struct i3c_emul {
	/** Target emulator backing this bus node. REQUIRED. */
	const struct emul *target;

	/** API provided by the peripheral emulator. */
	const struct i3c_emul_api *api;

	/**
	 * Optional mock API. If non-NULL, the bus emulator routes operations
	 * through these callbacks before falling back to @ref api.  Returning
	 * @c -ENOSYS from a mock callback delegates to the real @ref api.
	 */
	struct i3c_emul_api *mock_api;

	/**
	 * Bus emulator controller device this peripheral is attached to.
	 * Populated by @ref i3c_emul_register so that the peripheral side
	 * (e.g. @ref i3c_emul_target_raise_ibi) can locate the bus without
	 * external state.
	 */
	const struct device *bus;

	/**
	 * Static address from devicetree (first cell of @c reg). May be 0 when
	 * the device only obtains an address through DAA.
	 */
	uint8_t static_addr;

	/**
	 * Dynamic address currently in use. The peripheral sets this itself in
	 * its @ref i3c_emul_api::do_daa handler during DAA, and via its
	 * SETDASA / SETNEWDA / SETAASA @c do_ccc handlers; cleared on RSTDAA.
	 */
	uint8_t dynamic_addr;

	/** 48-bit Provisioned ID. */
	uint64_t pid;

	/** Bus Characteristic Register (initial value). */
	uint8_t bcr;

	/** Device Characteristic Register. */
	uint8_t dcr;

	/**
	 * Per-event raise capability mask managed by the peripheral via
	 * ENEC/DISEC. Holds I3C_CCC_EVT_INTR / _CR / _HJ bits. The bus
	 * emulator's raise_ibi/raise_hj/raise_crr helpers consult the
	 * matching bit before driving the event.
	 */
	uint8_t enabled_events;

	/**
	 * Cached GETCAPS Format 1 byte 0 — peripheral's HDR-mode capability
	 * advertisement. Filled by the peripheral at init from the same
	 * source it uses for its GETCAPS handler. Bus emulator gates HDR
	 * private xfers (e.g. HDR-DDR) on the matching bit, mirroring what
	 * a real controller does after caching a target's GETCAPS reply.
	 */
	uint8_t getcaps1;
};

/**
 * Issue private SDR (or HDR) read/write transfers to a target emulator.
 *
 * @param target  The peripheral emulator instance.
 * @param msgs    Array of I3C messages.  For read messages, the callback fills
 *                @c msgs[i].buf and updates @c msgs[i].num_xfer.
 * @param num_msgs Number of messages in the array.
 *
 * @retval 0 on success.
 * @retval -EIO on bus error.
 */
typedef int (*i3c_emul_xfers_t)(const struct emul *target, struct i3c_msg *msgs, uint8_t num_msgs);

/**
 * Deliver a Common Command Code (CCC) to a target emulator.
 *
 * Broadcast CCCs are dispatched to every attached target; direct CCCs are
 * dispatched once per addressed target in @c payload->targets.payloads[]. Use
 * @c i3c_ccc_is_payload_broadcast() to distinguish them.
 *
 * @param target  The peripheral emulator instance.
 * @param payload The CCC payload (id, optional defining byte, target list).
 *
 * @retval 0 on success.
 * @retval -ENOTSUP if the target does not implement this CCC.
 * @retval -EIO on protocol error.
 */
typedef int (*i3c_emul_do_ccc_t)(const struct emul *target, struct i3c_ccc_payload *payload);

/**
 * Run Dynamic Address Assignment (DAA) against a target emulator.
 *
 * Called by the bus emulator from its @c do_daa controller hook when it offers
 * a target a dynamic address. The peripheral takes ownership of its own dynamic
 * address (sets its @ref i3c_emul::dynamic_addr) and reports back the basic
 * info it would have shifted out during the ENTDAA exchange (PID/BCR/DCR), from
 * the same source it uses for its GETPID / GETBCR / GETDCR handlers. The bus
 * emulator mirrors those into the controller-side @c i3c_device_desc.
 *
 * This is the peripheral-side counterpart to @c i3c_driver_api::do_daa: the
 * controller hook drives the bus DAA, and this callback handles a single
 * target's participation.
 *
 * @param target      The peripheral emulator instance.
 * @param dynamic_addr Dynamic address offered by the controller (non-zero).
 * @param pid         Out: 48-bit Provisioned ID.
 * @param bcr         Out: Bus Characteristic Register.
 * @param dcr         Out: Device Characteristic Register.
 *
 * @retval 0 on acceptance.
 * @retval -EALREADY if the target already has a dynamic address.
 * @retval <0 if the assignment is rejected by the peripheral.
 */
typedef int (*i3c_emul_do_daa_t)(const struct emul *target, uint8_t dynamic_addr, uint64_t *pid,
				 uint8_t *bcr, uint8_t *dcr);

/**
 * Per-target API exposed by an I3C peripheral emulator to the bus emulator.
 *
 * @ref xfers and @ref do_ccc are mandatory. @ref do_daa is required only for
 * targets that obtain a dynamic address through DAA; it may be @c NULL for
 * SETDASA-only or legacy-I2C-on-I3C targets that never enter ENTDAA.
 *
 * HDR-mode private xfers (e.g. HDR-DDR) flow through @ref xfers — the bus
 * emulator validates HDR mode entry and target BCR up front, then hands
 * the @c i3c_msg array (with the HDR flags set) to @ref xfers. Peripherals
 * that handle HDR identically to SDR need no extra code; peripherals with
 * separate HDR semantics dispatch on @c msgs[i].flags & I3C_MSG_HDR.
 */
struct i3c_emul_api {
	/** Private SDR / HDR transfers. See @ref i3c_emul_xfers_t. */
	i3c_emul_xfers_t xfers;
	/** Common Command Code dispatch. See @ref i3c_emul_do_ccc_t. */
	i3c_emul_do_ccc_t do_ccc;
	/** Dynamic Address Assignment handler. See @ref i3c_emul_do_daa_t. */
	i3c_emul_do_daa_t do_daa;
};

/**
 * Register an I3C target peripheral emulator with the bus emulator.
 *
 * Typically invoked from the peripheral emulator's @ref emul_init_t callback
 * via @c emul_init_for_bus.
 *
 * @param dev  The bus emulator device.
 * @param emul The peripheral's @ref i3c_emul node.
 *
 * @retval 0 always.
 */
int i3c_emul_register(const struct device *dev, struct i3c_emul *emul);

struct i2c_emul;

/**
 * Register a legacy-I2C-on-I3C peripheral with the bus emulator.
 *
 * I2C devices on an I3C bus are described in devicetree with
 * @c reg = <addr 0 lvr> (mid cell zero). The emul framework allocates
 * a standard @c struct i2c_emul for them, so existing i2c sensor
 * emulators (BMI160, AKM09918C, ...) register here unmodified when
 * their devicetree node is reparented from a
 * @c zephyr,i2c-emul-controller to a @c zephyr,i3c-emul.
 *
 * Typically invoked from @c emul_init_for_bus_from_list when the bus
 * emulator detects an i3c parent with an i2c-typed child emul.
 *
 * @param dev  The i3c bus emulator device.
 * @param emul The peripheral's @c i2c_emul node.
 *
 * @retval 0 always.
 */
int i3c_emul_register_i2c(const struct device *dev, struct i2c_emul *emul);

/**
 * @return true if and only if @p dev is a @c zephyr,i3c-emul
 *         instance.
 *
 * Helper for the shared emul framework so it can route i2c-typed
 * peripherals on an i3c bus to @ref i3c_emul_register_i2c instead of
 * the i2c controller's own register hook.
 */
bool i3c_emul_is_bus(const struct device *dev);

/**
 * Inject an in-band interrupt (IBI) from a peripheral emulator into the bus.
 *
 * The bus emulator ACKs the IBI only if the controller has previously enabled
 * IBI for this target.  On ACK the corresponding @c i3c_device_desc->ibi_cb
 * is invoked (directly, or via the IBI workqueue when
 * @kconfig{CONFIG_I3C_IBI_WORKQUEUE} is enabled).
 *
 * @param target      The peripheral emulator instance.
 * @param payload     IBI payload bytes (may be NULL when @c payload_len == 0).
 * @param payload_len Length of the payload in bytes.
 *
 * @retval 0 on ACK + delivery.
 * @retval -ENOTCONN if IBI is disabled for this target.
 * @retval -EINVAL if the target is not attached to a controller.
 */
int i3c_emul_target_raise_ibi(const struct emul *target, uint8_t *payload, uint8_t payload_len);

/**
 * Inject a hot-join request from a peripheral emulator.
 *
 * The bus emulator ACKs the request only if the controller previously called
 * @c ibi_hj_response(true).  When @kconfig{CONFIG_I3C_IBI_WORKQUEUE} is enabled
 * the IBI workqueue runs @c i3c_do_daa() automatically on ACK to attach the
 * device; otherwise application code must trigger a fresh @c i3c_do_daa() itself.
 *
 * @retval 0 on ACK (and successful IBI-workqueue enqueue, when enabled).
 * @retval -EINVAL if @p target is invalid or not attached to a controller.
 * @retval -EACCES if the target already has a dynamic address, or hot-join is
 *         not enabled for it (via ENEC/DISEC).
 * @retval -ENOTCONN if hot-join is currently NACKed by the controller.
 * @retval -ENOMEM if the IBI work item cannot be enqueued.
 * @retval -ENOSYS if @kconfig{CONFIG_I3C_USE_IBI} is disabled.
 */
int i3c_emul_target_raise_hj(const struct emul *target);

/**
 * Inject a controller-role-request (CRR) from a secondary-controller-capable
 * target emulator.
 *
 * @retval 0 on ACK (and successful IBI-workqueue enqueue, when enabled).
 * @retval -EINVAL if @p target is invalid, not attached to a controller, or has
 *         no dynamic address.
 * @retval -EACCES if the controller-role event is disabled for the target
 *         (via ENEC/DISEC).
 * @retval -ENOTCONN if CRR is currently NACKed by the controller.
 * @retval -ENOMEM if the IBI work item cannot be enqueued.
 * @retval -ENOSYS if @kconfig{CONFIG_I3C_USE_IBI} is disabled.
 */
int i3c_emul_target_raise_crr(const struct emul *target);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_I3C_EMUL_H_ */
