/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Vendor-specific API for Infineon HPPASS MFD driver.
 *
 * The HPPASS (High Performance Programmable Analog Sub-System) contains
 * multiple analog peripherals (SAR ADC, CSG) coordinated by an Autonomous
 * Controller (AC).  This header defines the API used by applications and
 * child peripheral drivers to interact with the MFD parent, including
 * AC lifecycle, STT management, trigger control, and AC interrupt handling.
 */

#ifndef SOC_INFINEON_CAT1B_COMMON_INFINEON_HPPASS_H_
#define SOC_INFINEON_CAT1B_COMMON_INFINEON_HPPASS_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name HPPASS topology constants
 * Hardware-fixed sizes used in the public API.
 * @{
 */
/** Maximum number of State Transition Table entries (Arch TRM §27.2.4.3). */
#define IFX_HPPASS_AC_STT_SIZE   16
/** Number of CSG slices. */
#define IFX_HPPASS_CSG_NUM       5
/** Number of SAR Muxed Sampler channels. */
#define IFX_HPPASS_SAR_MUX_NUM   4
/** Number of HW/FW triggers. */
#define IFX_HPPASS_TRIG_NUM      8
/** @} */

/**
 * @name HPPASS trigger masks
 * For use with @ref ifx_hppass_ac_set_fw_trigger_pulse,
 * @ref ifx_hppass_ac_set_fw_trigger_level,
 * @ref ifx_hppass_ac_clear_fw_trigger_level.
 * @{
 */
#define IFX_HPPASS_TRIG_0_MSK    (1 << 0)
#define IFX_HPPASS_TRIG_1_MSK    (1 << 1)
#define IFX_HPPASS_TRIG_2_MSK    (1 << 2)
#define IFX_HPPASS_TRIG_3_MSK    (1 << 3)
#define IFX_HPPASS_TRIG_4_MSK    (1 << 4)
#define IFX_HPPASS_TRIG_5_MSK    (1 << 5)
#define IFX_HPPASS_TRIG_6_MSK    (1 << 6)
#define IFX_HPPASS_TRIG_7_MSK    (1 << 7)
/** @} */

/**
 * @name HPPASS interrupt source masks
 * For use with @ref ifx_hppass_event_callback::event_mask.
 * Values match HPPASS_MMIO_HPPASS_INTR_* register bit positions.
 * @{
 */
#define IFX_HPPASS_INTR_FIFO_0_OVERFLOW       (1 << 0)
#define IFX_HPPASS_INTR_FIFO_1_OVERFLOW       (1 << 1)
#define IFX_HPPASS_INTR_FIFO_2_OVERFLOW       (1 << 2)
#define IFX_HPPASS_INTR_FIFO_3_OVERFLOW       (1 << 3)
#define IFX_HPPASS_INTR_FIFO_0_UNDERFLOW      (1 << 4)
#define IFX_HPPASS_INTR_FIFO_1_UNDERFLOW      (1 << 5)
#define IFX_HPPASS_INTR_FIFO_2_UNDERFLOW      (1 << 6)
#define IFX_HPPASS_INTR_FIFO_3_UNDERFLOW      (1 << 7)
/** Any FIFO overflow (FIFO_0..3) */
#define IFX_HPPASS_INTR_FIFO_OVERFLOW         (0xF << 0)
/** Any FIFO underflow (FIFO_0..3) */
#define IFX_HPPASS_INTR_FIFO_UNDERFLOW        (0xF << 4)
/** Result overflow (HPPASS_MMIO_HPPASS_INTR bit 8) */
#define IFX_HPPASS_INTR_RESULT_OVERFLOW       (1 << 8)
/** Sequencer Group Trigger Collision (HPPASS_MMIO_HPPASS_INTR bit 9) */
#define IFX_HPPASS_INTR_GROUP_TR_COLLISION    (1 << 9)
/** Sequencer Group Hold Violation (HPPASS_MMIO_HPPASS_INTR bit 10) */
#define IFX_HPPASS_INTR_GROUP_HOLD_VIOLATION  (1 << 10)
/** Autonomous Controller interrupt (HPPASS_MMIO_HPPASS_INTR bit 12) */
#define IFX_HPPASS_INTR_AC_INT                (1 << 12)
/** All HPPASS top-level interrupt sources combined */
#define IFX_HPPASS_INTR_ALL                   (IFX_HPPASS_INTR_FIFO_OVERFLOW | \
					       IFX_HPPASS_INTR_FIFO_UNDERFLOW | \
					       IFX_HPPASS_INTR_RESULT_OVERFLOW | \
					       IFX_HPPASS_INTR_GROUP_TR_COLLISION | \
					       IFX_HPPASS_INTR_GROUP_HOLD_VIOLATION | \
					       IFX_HPPASS_INTR_AC_INT)
/** @} */

/**
 * @brief AC State Transition Table condition codes.
 *
 * The Condition field selects the predicate evaluated by an AC state when
 * its action is WAIT_FOR or BRANCH_IF_*.  Values map directly to the COND
 * field of the TT_CFG1 register and therefore must remain bit-stable.
 */
enum ifx_hppass_condition {
	IFX_HPPASS_CONDITION_FALSE              = 0,  /**< Hardcoded FALSE. */
	IFX_HPPASS_CONDITION_TRUE               = 1,  /**< Hardcoded TRUE. */
	IFX_HPPASS_CONDITION_BLOCK_READY        = 2,  /**< Enabled blocks ready. */
	IFX_HPPASS_CONDITION_CNT_DONE           = 3,  /**< Counter / timer runout. */
	IFX_HPPASS_CONDITION_SAR_GROUP_0_DONE   = 4,
	IFX_HPPASS_CONDITION_SAR_GROUP_1_DONE   = 5,
	IFX_HPPASS_CONDITION_SAR_GROUP_2_DONE   = 6,
	IFX_HPPASS_CONDITION_SAR_GROUP_3_DONE   = 7,
	IFX_HPPASS_CONDITION_SAR_GROUP_4_DONE   = 8,
	IFX_HPPASS_CONDITION_SAR_GROUP_5_DONE   = 9,
	IFX_HPPASS_CONDITION_SAR_GROUP_6_DONE   = 10,
	IFX_HPPASS_CONDITION_SAR_GROUP_7_DONE   = 11,
	IFX_HPPASS_CONDITION_SAR_LIMIT_0        = 12,
	IFX_HPPASS_CONDITION_SAR_LIMIT_1        = 13,
	IFX_HPPASS_CONDITION_SAR_LIMIT_2        = 14,
	IFX_HPPASS_CONDITION_SAR_LIMIT_3        = 15,
	IFX_HPPASS_CONDITION_SAR_LIMIT_4        = 16,
	IFX_HPPASS_CONDITION_SAR_LIMIT_5        = 17,
	IFX_HPPASS_CONDITION_SAR_LIMIT_6        = 18,
	IFX_HPPASS_CONDITION_SAR_LIMIT_7        = 19,
	IFX_HPPASS_CONDITION_SAR_BUSY           = 20,
	IFX_HPPASS_CONDITION_SAR_FIR_0_DONE     = 21,
	IFX_HPPASS_CONDITION_SAR_FIR_1_DONE     = 22,
	IFX_HPPASS_CONDITION_SAR_QUEUE_HI_EMPTY = 23,
	IFX_HPPASS_CONDITION_SAR_QUEUE_LO_EMPTY = 24,
	IFX_HPPASS_CONDITION_SAR_QUEUES_EMPTY   = 25,
	IFX_HPPASS_CONDITION_TRIG_0             = 32,
	IFX_HPPASS_CONDITION_TRIG_1             = 33,
	IFX_HPPASS_CONDITION_TRIG_2             = 34,
	IFX_HPPASS_CONDITION_TRIG_3             = 35,
	IFX_HPPASS_CONDITION_TRIG_4             = 36,
	IFX_HPPASS_CONDITION_TRIG_5             = 37,
	IFX_HPPASS_CONDITION_TRIG_6             = 38,
	IFX_HPPASS_CONDITION_TRIG_7             = 39,
	IFX_HPPASS_CONDITION_FIFO_0_LEVEL       = 42,
	IFX_HPPASS_CONDITION_FIFO_1_LEVEL       = 43,
	IFX_HPPASS_CONDITION_FIFO_2_LEVEL       = 44,
	IFX_HPPASS_CONDITION_FIFO_3_LEVEL       = 45,
	IFX_HPPASS_CONDITION_CSG_0_DAC_DONE     = 48,
	IFX_HPPASS_CONDITION_CSG_1_DAC_DONE     = 49,
	IFX_HPPASS_CONDITION_CSG_2_DAC_DONE     = 50,
	IFX_HPPASS_CONDITION_CSG_3_DAC_DONE     = 51,
	IFX_HPPASS_CONDITION_CSG_4_DAC_DONE     = 52,
	IFX_HPPASS_CONDITION_CSG_0_COMP         = 56,
	IFX_HPPASS_CONDITION_CSG_1_COMP         = 57,
	IFX_HPPASS_CONDITION_CSG_2_COMP         = 58,
	IFX_HPPASS_CONDITION_CSG_3_COMP         = 59,
	IFX_HPPASS_CONDITION_CSG_4_COMP         = 60,
};

/**
 * @brief AC State Transition Table action codes.
 *
 * Selects the AC's behaviour on entering a state.  Values map directly to
 * the ACTION field of TT_CFG1 and must remain bit-stable.
 */
enum ifx_hppass_action {
	IFX_HPPASS_ACTION_STOP            = 0, /**< Stop the AC. */
	IFX_HPPASS_ACTION_NEXT            = 1, /**< Proceed to next state. */
	IFX_HPPASS_ACTION_WAIT_FOR        = 2, /**< Wait for condition. */
	IFX_HPPASS_ACTION_BRANCH_IF_TRUE  = 3, /**< Branch to branch_state_idx if true. */
	IFX_HPPASS_ACTION_BRANCH_IF_FALSE = 4, /**< Branch to branch_state_idx if false. */
};

/**
 * @brief Per-state SAR Muxed Sampler control.
 */
struct ifx_hppass_stt_mux {
	bool unlock;       /**< Unlock to apply chan_idx; ignored when false. */
	uint8_t chan_idx;  /**< Muxed Sampler channel selection (0..3). */
};

/**
 * @brief One State Transition Table entry.
 *
 * Field semantics mirror the AC state machine description in Arch TRM
 * §27.2.4 (see §27.2.4.3 Tables 254-256 for the bit layout, conditions,
 * and actions).
 * Populate one of these per STT slot, then pass an array of them to
 * @ref ifx_hppass_ac_load_stt or @ref ifx_hppass_ac_update_stt.
 */
struct ifx_hppass_ac_stt_entry {
	/** AC predicate (see @ref ifx_hppass_condition). */
	enum ifx_hppass_condition condition;
	/** AC action on state entry (see @ref ifx_hppass_action). */
	enum ifx_hppass_action action;
	/** Branch target state index (0..15), used when action is BRANCH_IF_*. */
	uint8_t branch_state_idx;
	/** Generate output pulse / CPU interrupt on state entry. */
	bool interrupt;
	/** Counter value (1..4096) for WAIT_FOR/BRANCH_IF_* with CNT_DONE. */
	uint16_t count;
	/** Unlock GPIO out update; when false gpio_out_msk is ignored. */
	bool gpio_out_unlock;
	/** GPIO output bitmask. */
	uint8_t gpio_out_msk;
	/** Per-CSG unlock; when false csg_enable[c] is ignored for CSG c. */
	bool csg_unlock[IFX_HPPASS_CSG_NUM];
	/** Per-CSG slice enable. */
	bool csg_enable[IFX_HPPASS_CSG_NUM];
	/** Per-CSG DAC start/update trigger. */
	bool csg_dac_trig[IFX_HPPASS_CSG_NUM];
	/** Unlock SAR enable; when false sar_enable is ignored. */
	bool sar_unlock;
	/** SAR enable. */
	bool sar_enable;
	/** SAR Sequencer Groups trigger mask. */
	uint8_t sar_grp_msk;
	/** Per-SAR-mux control. */
	struct ifx_hppass_stt_mux sar_mux[IFX_HPPASS_SAR_MUX_NUM];
};

struct ifx_hppass_event_callback;

/**
 * @brief HPPASS event callback handler signature.
 *
 * Invoked from ISR context when any of the events matching the
 * callback's event_mask fire on the combined MCPASS interrupt.
 *
 * @param dev    HPPASS MFD device
 * @param cb     Callback structure that triggered this invocation.
 *               Use CONTAINER_OF() to recover private context.
 * @param events Bitmask of events that matched (event_mask & status)
 */
typedef void (*ifx_hppass_event_handler_t)(const struct device *dev,
					   struct ifx_hppass_event_callback *cb,
					   uint32_t events);

/**
 * @brief HPPASS event callback structure.
 *
 * Allocate one instance per consumer (child driver or application).
 * Populate event_mask and handler before passing to
 * @ref ifx_hppass_add_callback.  Use IFX_HPPASS_INTR_* masks
 * to select interrupt sources.
 */
struct ifx_hppass_event_callback {
	/** @cond INTERNAL_HIDDEN */
	sys_snode_t node;
	/** @endcond */
	/** Events this callback is interested in (IFX_HPPASS_INTR_*) */
	uint32_t event_mask;
	/** Handler invoked from ISR context */
	ifx_hppass_event_handler_t handler;
};

/* Lifecycle */

/**
 * @brief Start the HPPASS Autonomous Controller
 *
 * Start the AC state machine from the given STT state index. The HPPASS
 * must have been previously initialised at device init.
 *
 * If @p timeout_us is non-zero the call blocks until the BLOCK_READY
 * condition is asserted or the timeout expires.  Use timeout_us == 0 for
 * a non-blocking start (e.g. when restarting after a cooperative stop
 * where the block is already powered).
 *
 * @param dev        HPPASS MFD device
 * @param start_state STT state index (0..15) to begin execution
 * @param timeout_us  Startup timeout in microseconds, 0 = non-blocking
 *
 * @retval 0        Success
 * @retval -EINVAL  Invalid start_state
 * @retval -EALREADY AC is already running
 * @retval -EIO     VDDA is below minimum threshold
 * @retval -ETIMEDOUT Timeout waiting for BLOCK_READY
 */
int ifx_hppass_ac_start(const struct device *dev, uint8_t start_state,
			uint16_t timeout_us);

/**
 * @brief Ensure the HPPASS Autonomous Controller is running.
 *
 * If the AC is stopped (typical for STTs that end in ACTION_STOP,
 * including the basic-mode default STT), kick it via @ref
 * ifx_hppass_ac_start starting at state index 0.  If it is already
 * running, do nothing.
 *
 * The HPPASS MFD's late-init hook (APPLICATION-priority SYS_INIT)
 * already runs the AC once through the configured STT after all
 * POST_KERNEL child drivers have programmed their slice/channel
 * configuration registers.  Most consumers therefore never need to
 * call this directly — call it only when a one-shot AC run is needed
 * after the boot pass (for example, walking a custom STT for a
 * runtime SAR sequence or BIST).
 *
 * No paired "stop" call is provided.  CMD_RUN.RUN is RW1S for SW
 * (Regs TRM 002-39445 §20.1.65); the AC stops only via STT STOP action
 * (Arch TRM §27.2.4.3 Table 256).  Callers that need the AC to
 * halt must arrange a STOP path in their STT (e.g. a BRANCH_IF_TRUE
 * to a STOP state, driven by a firmware level trigger).
 *
 * Calls from multiple children are serialised against each other.
 *
 * @param dev HPPASS MFD device (e.g. @c cfg->parent for a child driver).
 *
 * @retval 0          Success (AC running on return).
 * @retval -EINVAL    @p dev is NULL.
 * @retval -EIO       VDDA below minimum threshold.
 * @retval -ETIMEDOUT AC failed to reach BLOCK_READY in time.
 */
int ifx_hppass_ensure_running(const struct device *dev);

/* STT management */

/**
 * @brief Load the full State Transition Table
 *
 * Replace the entire STT starting from index 0.  The AC must not be
 * running (either not yet started, self-stopped via ACTION_STOP, or
 * hard-stopped).
 *
 * @param dev         HPPASS MFD device
 * @param num_entries Number of STT entries (1..16)
 * @param stt         Pointer to array of STT entry structures
 *
 * @retval 0       Success
 * @retval -EINVAL Invalid parameter
 * @retval -EBUSY  AC is currently running
 */
int ifx_hppass_ac_load_stt(const struct device *dev, uint8_t num_entries,
			   const struct ifx_hppass_ac_stt_entry *stt);

/**
 * @brief Update a contiguous slice of the State Transition Table
 *
 * Patch one or more STT entries starting at @p start_idx without
 * affecting other entries.  The AC must not be running.
 *
 * This is the primary mechanism for runtime STT reconfiguration after
 * the AC has self-stopped via an ACTION_STOP state.
 *
 * @param dev         HPPASS MFD device
 * @param num_entries Number of entries to write
 * @param stt         Pointer to array of STT entry structures
 * @param start_idx   First STT index to update (0..15)
 *
 * @retval 0       Success
 * @retval -EINVAL Invalid parameter or range overflow
 * @retval -EBUSY  AC is currently running
 */
int ifx_hppass_ac_update_stt(const struct device *dev, uint8_t num_entries,
			     const struct ifx_hppass_ac_stt_entry *stt,
			     uint8_t start_idx);

/* AC state query */

/**
 * @brief Check whether the AC is currently running
 *
 * @param dev HPPASS MFD device
 *
 * @retval true  AC is running
 * @retval false AC is stopped
 */
bool ifx_hppass_ac_is_running(const struct device *dev);

/**
 * @brief Check whether VDDA is within operating range
 *
 * @param dev HPPASS MFD device
 *
 * @retval true  VDDA is at or above minimum threshold
 * @retval false VDDA is below threshold
 */
bool ifx_hppass_is_vdda_ok(const struct device *dev);

/* FW trigger control (safe to call while AC is running) */

/**
 * @brief Fire a firmware pulse trigger
 *
 * Generates a single-cycle pulse trigger.  This is only effective if the
 * corresponding trigger input is configured as the firmware-pulse source.
 *
 * @param dev  HPPASS MFD device
 * @param mask Trigger mask (bitwise OR of IFX_HPPASS_TRIG_n_MSK)
 */
void ifx_hppass_ac_set_fw_trigger_pulse(const struct device *dev,
					uint8_t mask);

/**
 * @brief Assert a firmware level trigger
 *
 * Sets a latching level trigger.  The trigger remains asserted until
 * explicitly cleared via @ref ifx_hppass_ac_clear_fw_trigger_level.
 * Suitable for cooperative stop patterns where the AC must see the
 * trigger regardless of its current state.
 *
 * @param dev  HPPASS MFD device
 * @param mask Trigger mask (bitwise OR of IFX_HPPASS_TRIG_n_MSK)
 */
void ifx_hppass_ac_set_fw_trigger_level(const struct device *dev,
					uint8_t mask);

/**
 * @brief De-assert a firmware level trigger
 *
 * Clears a previously asserted level trigger.
 *
 * @param dev  HPPASS MFD device
 * @param mask Trigger mask (bitwise OR of IFX_HPPASS_TRIG_n_MSK)
 */
void ifx_hppass_ac_clear_fw_trigger_level(const struct device *dev,
					  uint8_t mask);

/* Interrupt callback management */

/**
 * @brief Register an event callback
 *
 * Add a callback to the HPPASS MFD interrupt dispatch list.  The
 * driver automatically updates the hardware interrupt mask to include
 * events from all registered callbacks.
 *
 * Callbacks are invoked from ISR context.  Each callback receives
 * only the events matching its event_mask.
 *
 * Not safe to call from ISR context, and not concurrency-safe against
 * itself or @ref ifx_hppass_remove_callback.  Callers must serialize
 * registration (typical: register from device init or single thread).
 *
 * @param dev      HPPASS MFD device
 * @param callback Callback structure with event_mask and handler set
 *
 * @retval 0       Success
 * @retval -EINVAL NULL callback, NULL handler, or zero event_mask
 */
int ifx_hppass_add_callback(const struct device *dev,
			    struct ifx_hppass_event_callback *callback);

/**
 * @brief Unregister an event callback
 *
 * Remove a previously registered callback.  The hardware interrupt
 * mask is updated to disable events no longer needed by any callback.
 *
 * Not safe to call from ISR context, and not concurrency-safe against
 * itself or @ref ifx_hppass_add_callback.  Callers must serialize.
 *
 * @param dev      HPPASS MFD device
 * @param callback Callback structure previously passed to
 *                 @ref ifx_hppass_add_callback
 *
 * @retval 0       Success
 * @retval -EINVAL NULL callback
 * @retval -ENOENT Callback was not registered
 */
int ifx_hppass_remove_callback(const struct device *dev,
			       struct ifx_hppass_event_callback *callback);

#ifdef __cplusplus
}
#endif

#endif /* SOC_INFINEON_CAT1B_COMMON_INFINEON_HPPASS_H_ */
