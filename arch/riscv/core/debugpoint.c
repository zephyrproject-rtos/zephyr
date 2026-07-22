/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/debugpoint.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#define CSR_TSELECT  0x7a0
#define CSR_TDATA1   0x7a1
#define CSR_TDATA2   0x7a2
#define CSR_TINFO    0x7a4
#define CSR_TCONTROL 0x7a5

#define TDATA1_LOAD         BIT(0)
#define TDATA1_STORE        BIT(1)
#define TDATA1_EXECUTE      BIT(2)
#define TDATA1_U            BIT(3)
#define TDATA1_S            BIT(4)
#define TDATA1_M            BIT(6)
#define TDATA1_MATCH_SHIFT  7
#define TDATA1_MATCH_MASK   (0xfUL << TDATA1_MATCH_SHIFT)
#define TDATA1_MATCH_NAPOT  (1UL << TDATA1_MATCH_SHIFT)
#define TDATA1_CHAIN        BIT(11)
#define TDATA1_ACTION_SHIFT 12
#define TDATA1_ACTION_MASK  (0xfUL << TDATA1_ACTION_SHIFT)
#define TDATA1_MCONTROL_SIZELO_MASK    (0x3UL << 16)
#define TDATA1_MCONTROL_TIMING         BIT(18)
#define TDATA1_MCONTROL_SELECT         BIT(19)
#define TDATA1_MCONTROL_HIT            BIT(20)
#define TDATA1_MCONTROL6_V0_SIZE_MASK  (0xfUL << 16)
#define TDATA1_MCONTROL6_V1_SIZE_MASK  (0x7UL << 16)
#define TDATA1_MCONTROL6_V0_TIMING     BIT(20)
#define TDATA1_MCONTROL6_SELECT        BIT(21)
#define TDATA1_MCONTROL6_HIT0          BIT(22)
#define TDATA1_MCONTROL6_VU            BIT(23)
#define TDATA1_MCONTROL6_VS            BIT(24)
#define TDATA1_MCONTROL6_HIT1          BIT(25)
#define TDATA1_MCONTROL6_UNCERTAIN     BIT(26)
#define TDATA1_MCONTROL6_UNCERTAINEN   BIT(5)

#if __riscv_xlen >= 64
#define TDATA1_MCONTROL_SIZEHI_MASK (0x3UL << 21)
#else
#define TDATA1_MCONTROL_SIZEHI_MASK 0UL
#endif

#define TDATA1_DMODE        (1UL << (__riscv_xlen - 5))
#define TDATA1_TYPE_SHIFT   (__riscv_xlen - 4)
#define TDATA1_TYPE_MASK    (0xfUL << TDATA1_TYPE_SHIFT)
#define TDATA1_TYPE_NONE    (0UL << TDATA1_TYPE_SHIFT)
#define TDATA1_TYPE_MCONTROL  (2UL << TDATA1_TYPE_SHIFT)
#define TDATA1_TYPE_MCONTROL6 (6UL << TDATA1_TYPE_SHIFT)
#define TDATA1_TYPE_DISABLED  (15UL << TDATA1_TYPE_SHIFT)

#define TINFO_INFO_MASK       0xffffUL
#define TINFO_INFO_NONE       BIT(0)
#define TINFO_INFO_MCONTROL   BIT(2)
#define TINFO_INFO_MCONTROL6  BIT(6)
#define TINFO_VERSION_SHIFT   24
#define TINFO_VERSION_MASK    0xffUL

#define TCONTROL_MTE  BIT(3)
#define TCONTROL_MPTE BIT(7)

#define RISCV_EBREAK   0x00100073U
#define RISCV_C_EBREAK 0x9002U

enum riscv_trigger_type {
	RISCV_TRIGGER_MCONTROL,
	RISCV_TRIGGER_MCONTROL6,
};

/*
 * A logical debugpoint shared by all harts. tdata1 and tdata2 contain verified
 * values copied to one physical trigger per hart. active controls whether
 * mapped hardware may remain armed; g_lock serializes configuration updates.
 */
struct riscv_trigger_slot {
	atomic_t active;
	uint8_t tinfo_version;
	enum riscv_trigger_type trigger_type;
	z_debugpoint_handle_t handle;
	enum z_debugpoint_type type;
	uintptr_t addr;
	size_t size;
	uintptr_t tdata1;
	uintptr_t tdata2;
};

/* Snapshot kept after physical triggers are disabled for trap attribution. */
struct riscv_hit {
	z_debugpoint_handle_t handle;
	uint8_t type;
	uint8_t timing;
	bool access_addr_valid;
	bool rearm_required;
};

static struct riscv_trigger_slot g_slots[CONFIG_DEBUGPOINT_MAX_SLOTS];
/*
 * Logical capacity derived from free, compatible triggers on the initializing hart.
 * The generic core still requires installation on every online hart.
 */
static int g_slot_count;
/*
 * Physical trigger enumeration per hart. count bounds allocation searches;
 * scan_complete means the architectural end was found before the software cap.
 */
static uint8_t g_cpu_trigger_count[CONFIG_MP_MAX_NUM_CPUS];
static bool g_cpu_trigger_scan_complete[CONFIG_MP_MAX_NUM_CPUS];
/* Set after initializing logical capacity and all per-hart mapping entries. */
static atomic_t g_initialized;
/* [hart][logical slot] -> physical tselect index, or -1 when unmapped. */
static int8_t
	g_cpu_hw_index[CONFIG_MP_MAX_NUM_CPUS][CONFIG_DEBUGPOINT_MAX_SLOTS];
static struct k_spinlock g_lock;

extern int z_riscv_debugpoint_tinfo_read(uintptr_t *value);
extern int z_riscv_debugpoint_insn_read(const void *pc, uint32_t *instruction);

static bool cpu_has_trigger_mapping(unsigned int cpu, int slot)
{
	return g_cpu_hw_index[cpu][slot] >= 0;
}

static int logical_slot_for_trigger(unsigned int cpu, int hw_index)
{
	for (int i = 0; i < g_slot_count; i++) {
		if (cpu_has_trigger_mapping(cpu, i) &&
		    g_cpu_hw_index[cpu][i] == hw_index) {
			return i;
		}
	}

	return -1;
}

static void clear_trigger_mapping(unsigned int cpu, int slot)
{
	g_cpu_hw_index[cpu][slot] = -1;
}

static bool disable_selected_trigger(void)
{
	csr_write_imm(CSR_TDATA1, 0);

	/* Fixed-type implementations may retain control bits on a zero write. */
	uintptr_t tdata1 = csr_read_imm(CSR_TDATA1);
	uintptr_t enable = TDATA1_LOAD | TDATA1_STORE | TDATA1_EXECUTE;

	if ((tdata1 & enable) != 0U) {
		csr_write_imm(CSR_TDATA1, tdata1 & ~enable);
		tdata1 = csr_read_imm(CSR_TDATA1);
	}

	return (tdata1 & enable) == 0U;
}

static void reset_selected_trigger(void)
{
	(void)disable_selected_trigger();
	csr_write_imm(CSR_TDATA2, 0);
}

static int clear_selected_slot(unsigned int cpu, int slot)
{
	uintptr_t tdata1 = csr_read_imm(CSR_TDATA1);

	if ((tdata1 & TDATA1_DMODE) != 0U) {
		clear_trigger_mapping(cpu, slot);
		return 0;
	}
	if (!disable_selected_trigger()) {
		return -ENOTSUP;
	}

	csr_write_imm(CSR_TDATA2, 0);
	clear_trigger_mapping(cpu, slot);
	return 0;
}

static uintptr_t tdata1_type(uintptr_t value)
{
	return value & TDATA1_TYPE_MASK;
}

static bool is_watchpoint_type(enum z_debugpoint_type type)
{
	return type == Z_DEBUGPOINT_WATCH_READ ||
	       type == Z_DEBUGPOINT_WATCH_WRITE ||
	       type == Z_DEBUGPOINT_WATCH_RW;
}

static bool triggers_load(enum z_debugpoint_type type)
{
	return type == Z_DEBUGPOINT_WATCH_READ || type == Z_DEBUGPOINT_WATCH_RW;
}

static bool triggers_store(enum z_debugpoint_type type)
{
	return type == Z_DEBUGPOINT_WATCH_WRITE || type == Z_DEBUGPOINT_WATCH_RW;
}

static bool trigger_is_enabled(uintptr_t tdata1)
{
	uintptr_t type = tdata1_type(tdata1);

	return (type == TDATA1_TYPE_MCONTROL || type == TDATA1_TYPE_MCONTROL6) &&
	       (tdata1 & (TDATA1_LOAD | TDATA1_STORE | TDATA1_EXECUTE)) != 0U;
}

static bool trigger_may_fire(uintptr_t tdata1)
{
	uintptr_t type = tdata1_type(tdata1);

	if (type == TDATA1_TYPE_MCONTROL || type == TDATA1_TYPE_MCONTROL6) {
		return trigger_is_enabled(tdata1);
	}

	return type != TDATA1_TYPE_NONE && type != TDATA1_TYPE_DISABLED;
}

static bool selected_trigger_supports_watchpoint(uintptr_t tdata1)
{
	uintptr_t tinfo;

	if (z_riscv_debugpoint_tinfo_read(&tinfo) == 0) {
		return (tinfo & (TINFO_INFO_MCONTROL |
				 TINFO_INFO_MCONTROL6)) != 0U;
	}

	uintptr_t type = tdata1_type(tdata1);

	return type == TDATA1_TYPE_MCONTROL ||
	       type == TDATA1_TYPE_MCONTROL6 ||
	       type == TDATA1_TYPE_DISABLED;
}

static bool selected_trigger_available(unsigned int cpu, int slot,
				    int hw_index)
{
	/*
	 * A slot may reuse its own mapping. Otherwise candidates must be unowned,
	 * disabled, supported, and not reserved by Debug Mode.
	 */
	int owner = logical_slot_for_trigger(cpu, hw_index);

	if (owner >= 0) {
		return owner == slot;
	}

	uintptr_t tdata1 = csr_read_imm(CSR_TDATA1);

	if ((tdata1 & TDATA1_DMODE) != 0U || trigger_may_fire(tdata1)) {
		return false;
	}

	return selected_trigger_supports_watchpoint(tdata1);
}

static bool range_contains(uintptr_t addr, size_t size, uintptr_t value)
{
	return value >= addr && value - addr < size;
}

static bool ranges_overlap(uintptr_t a, size_t a_size, uintptr_t b, size_t b_size)
{
	return a <= b ? b - a < a_size : a - b < b_size;
}

static bool range_is_representable(uintptr_t addr, size_t size)
{
	if (size == 1U) {
		return true;
	}

	return IS_POWER_OF_TWO(size) && (addr & (size - 1U)) == 0U;
}

static uintptr_t napot_encode(uintptr_t addr, size_t size)
{
	return addr | ((size - 1U) >> 1);
}

static uintptr_t build_tdata1(const struct z_debugpoint_config *config,
			      enum riscv_trigger_type trigger_type)
{
	uintptr_t value = trigger_type == RISCV_TRIGGER_MCONTROL6 ?
			  TDATA1_TYPE_MCONTROL6 : TDATA1_TYPE_MCONTROL;

	value |= TDATA1_M;
	if (IS_ENABLED(CONFIG_USERSPACE)) {
		value |= TDATA1_U;
	}
	if (triggers_load(config->type)) {
		value |= TDATA1_LOAD;
	}
	if (triggers_store(config->type)) {
		value |= TDATA1_STORE;
	}
	if (config->size > 1U) {
		value |= TDATA1_MATCH_NAPOT;
	}
	if (trigger_type == RISCV_TRIGGER_MCONTROL) {
		value |= TDATA1_MCONTROL_TIMING;
	}

	return value;
}

static uintptr_t config_compare_mask(enum riscv_trigger_type trigger_type,
				     uint8_t version)
{
	uintptr_t mask = TDATA1_TYPE_MASK | TDATA1_DMODE |
			 TDATA1_LOAD | TDATA1_STORE | TDATA1_EXECUTE |
			 TDATA1_U | TDATA1_S | TDATA1_M |
			 TDATA1_MATCH_MASK | TDATA1_CHAIN |
			 TDATA1_ACTION_MASK;

	/* Timing is WARL; both before and after are supported. */
	if (trigger_type == RISCV_TRIGGER_MCONTROL) {
		return mask | TDATA1_MCONTROL_SELECT |
		       TDATA1_MCONTROL_SIZELO_MASK |
		       TDATA1_MCONTROL_SIZEHI_MASK;
	}

	mask |= TDATA1_MCONTROL6_SELECT | TDATA1_MCONTROL6_UNCERTAINEN |
		TDATA1_MCONTROL6_VU | TDATA1_MCONTROL6_VS;
	return mask | (version == 0U ? TDATA1_MCONTROL6_V0_SIZE_MASK :
		       TDATA1_MCONTROL6_V1_SIZE_MASK);
}

static bool config_matches(uintptr_t actual, uintptr_t requested,
			   enum riscv_trigger_type trigger_type,
			   uint8_t version)
{
	uintptr_t mask = config_compare_mask(trigger_type, version);

	if (!IS_ENABLED(CONFIG_USERSPACE)) {
		mask &= ~TDATA1_U;
	}

	return (actual & mask) == (requested & mask);
}

static bool selected_trigger_absent(uintptr_t tdata1)
{
	uintptr_t tinfo;

	if (z_riscv_debugpoint_tinfo_read(&tinfo) == 0) {
		return (tinfo & TINFO_INFO_MASK) == TINFO_INFO_NONE;
	}

	return tdata1_type(tdata1) == TDATA1_TYPE_NONE;
}

static uint8_t read_tinfo_version(void)
{
	uintptr_t value;

	if (z_riscv_debugpoint_tinfo_read(&value) != 0) {
		return 0U;
	}

	return (uint8_t)((value >> TINFO_VERSION_SHIFT) & TINFO_VERSION_MASK);
}

static unsigned int hit_value(const struct riscv_trigger_slot *slot,
			      uintptr_t tdata1)
{
	if (slot->trigger_type == RISCV_TRIGGER_MCONTROL) {
		return (tdata1 & TDATA1_MCONTROL_HIT) != 0U ? 1U : 0U;
	}
	if (slot->tinfo_version == 0U) {
		return (tdata1 & TDATA1_MCONTROL6_HIT0) != 0U ? 1U : 0U;
	}

	return ((tdata1 & TDATA1_MCONTROL6_HIT0) != 0U ? 1U : 0U) |
	       ((tdata1 & TDATA1_MCONTROL6_HIT1) != 0U ? 2U : 0U);
}

static enum z_debugpoint_timing trigger_timing(
	const struct riscv_trigger_slot *slot, uintptr_t tdata1,
	unsigned int hit)
{
	if (slot->trigger_type == RISCV_TRIGGER_MCONTROL) {
		return (tdata1 & TDATA1_MCONTROL_TIMING) != 0U ?
		       Z_DEBUGPOINT_TIMING_AFTER : Z_DEBUGPOINT_TIMING_BEFORE;
	}
	if (slot->tinfo_version == 0U) {
		return (tdata1 & TDATA1_MCONTROL6_V0_TIMING) != 0U ?
		       Z_DEBUGPOINT_TIMING_AFTER : Z_DEBUGPOINT_TIMING_BEFORE;
	}
	if (hit == 1U) {
		return Z_DEBUGPOINT_TIMING_BEFORE;
	}
	if (hit == 2U || hit == 3U) {
		return Z_DEBUGPOINT_TIMING_AFTER;
	}

	return Z_DEBUGPOINT_TIMING_UNKNOWN;
}

static int instruction_is_ebreak(uintptr_t pc)
{
	uint32_t instruction;
	int ret = z_riscv_debugpoint_insn_read((const void *)pc, &instruction);

	if (ret != 0) {
		return ret;
	}

	uint16_t low = (uint16_t)instruction;

	if (low == RISCV_C_EBREAK) {
		return 1;
	}
	if ((low & 0x3U) != 0x3U) {
		return 0;
	}

	return instruction == RISCV_EBREAK ? 1 : 0;
}

struct riscv_trigger_extent {
	int count;
	bool complete;
};

static struct riscv_trigger_extent probe_trigger_extent(void)
{
	/*
	 * Enumerate contiguous tselect values up to logical table capacity. If the
	 * limit is reached, complete remains false because more triggers may exist.
	 */
	uintptr_t saved_tselect = csr_read_imm(CSR_TSELECT);
	struct riscv_trigger_extent extent = {0};

	for (int i = 0; i <= ARRAY_SIZE(g_slots); i++) {
		csr_write_imm(CSR_TSELECT, i);
		if ((int)csr_read_imm(CSR_TSELECT) != i) {
			extent.complete = true;
			break;
		}

		if (selected_trigger_absent(csr_read_imm(CSR_TDATA1))) {
			extent.complete = true;
			break;
		}
		if (i < ARRAY_SIZE(g_slots)) {
			extent.count++;
		}
	}

	csr_write_imm(CSR_TSELECT, saved_tselect);
	return extent;
}

static void cache_cpu_trigger_extent(unsigned int cpu)
{
	struct riscv_trigger_extent extent = probe_trigger_extent();

	g_cpu_trigger_count[cpu] = extent.count;
	g_cpu_trigger_scan_complete[cpu] = extent.complete;
}

static int probe_trigger_slots(void)
{
	/* Free compatible triggers on the initializing hart define capacity. */
	struct riscv_trigger_extent extent = probe_trigger_extent();
	uintptr_t saved_tselect = csr_read_imm(CSR_TSELECT);
	unsigned int cpu = arch_curr_cpu()->id;
	int usable = 0;

	for (int i = 0; i < extent.count; i++) {
		csr_write_imm(CSR_TSELECT, i);

		uintptr_t tdata1 = csr_read_imm(CSR_TDATA1);

		if ((tdata1 & TDATA1_DMODE) != 0U ||
		    trigger_may_fire(tdata1) ||
		    !selected_trigger_supports_watchpoint(tdata1)) {
			continue;
		}

		usable++;
	}

	g_slot_count = usable;
	g_cpu_trigger_count[cpu] = extent.count;
	g_cpu_trigger_scan_complete[cpu] = extent.complete;
	csr_write_imm(CSR_TSELECT, saved_tselect);
	return usable > 0 ? 0 : -ENOTSUP;
}

#if defined(CONFIG_RISCV_HAS_TCONTROL)
static int configure_tcontrol(void)
{
	uintptr_t enable = TCONTROL_MPTE | TCONTROL_MTE;

	csr_write_imm(CSR_TCONTROL, csr_read_imm(CSR_TCONTROL) | enable);
	return (csr_read_imm(CSR_TCONTROL) & enable) == enable ? 0 : -ENOTSUP;
}
#endif

static int riscv_debugpoint_init(void)
{
	/* BSS zero would incorrectly map every logical slot to physical trigger 0. */
	for (int cpu = 0; cpu < CONFIG_MP_MAX_NUM_CPUS; cpu++) {
		for (int slot = 0; slot < ARRAY_SIZE(g_slots); slot++) {
			g_cpu_hw_index[cpu][slot] = -1;
		}
	}

	int ret = probe_trigger_slots();

	if (ret != 0) {
		return ret;
	}

#if defined(CONFIG_RISCV_HAS_TCONTROL)
	ret = configure_tcontrol();
	if (ret != 0) {
		return ret;
	}
#endif
	return 0;
}

static uintptr_t clear_trigger_status(uintptr_t tdata1,
				      enum riscv_trigger_type trigger_type,
				      uint8_t version)
{
	if (trigger_type == RISCV_TRIGGER_MCONTROL) {
		return tdata1 & ~TDATA1_MCONTROL_HIT;
	}
	if (version == 0U) {
		return tdata1 & ~TDATA1_MCONTROL6_HIT0;
	}

	return tdata1 & ~(TDATA1_MCONTROL6_HIT0 |
			  TDATA1_MCONTROL6_HIT1 |
			  TDATA1_MCONTROL6_UNCERTAIN);
}

static int arm_selected_trigger(const struct riscv_trigger_slot *slot,
				uintptr_t *actual)
{
	/*
	 * active can change in trap context. Check it around the WARL write so a
	 * retiring slot cannot leave hardware armed.
	 */
	uintptr_t value = slot->tdata1;

	if (atomic_get(&slot->active) == 0) {
		reset_selected_trigger();
		goto out;
	}

	csr_write_imm(CSR_TDATA1, value);
	value = csr_read_imm(CSR_TDATA1);

	if (atomic_get(&slot->active) == 0) {
		reset_selected_trigger();
		goto out;
	}
	if (!config_matches(value, slot->tdata1, slot->trigger_type,
			    slot->tinfo_version) ||
	    csr_read_imm(CSR_TDATA2) != slot->tdata2) {
		reset_selected_trigger();
		return -ENOTSUP;
	}

out:
	if (actual != NULL) {
		*actual = value;
	}
	return 0;
}

static int program_selected_trigger(const struct riscv_trigger_slot *slot,
				    uintptr_t *actual)
{
	if (!disable_selected_trigger()) {
		return -ENOTSUP;
	}

	csr_write_imm(CSR_TDATA2, slot->tdata2);
	return arm_selected_trigger(slot, actual);
}

static int try_program_trigger_type(struct riscv_trigger_slot *slot,
			const struct z_debugpoint_config *config,
			enum riscv_trigger_type trigger_type)
{
	uintptr_t requested = build_tdata1(config, trigger_type);
	uintptr_t tdata2 = config->size == 1U ? (uintptr_t)config->addr :
			   napot_encode((uintptr_t)config->addr,
					config->size);
	uint8_t version = 0U;

	if (trigger_type == RISCV_TRIGGER_MCONTROL6) {
		version = read_tinfo_version();
		if (version > 1U) {
			return -ENOTSUP;
		}
	}

	if (trigger_type == RISCV_TRIGGER_MCONTROL6 && version == 0U) {
		requested |= TDATA1_MCONTROL6_V0_TIMING;
	}

	uintptr_t access = requested &
			   (TDATA1_LOAD | TDATA1_STORE | TDATA1_EXECUTE);
	uintptr_t disabled = requested & ~access;

	/*
	 * Verify address and WARL control fields with access disabled, then save
	 * the verified encoding and arm it.
	 */
	if (!disable_selected_trigger()) {
		return -ENOTSUP;
	}
	csr_write_imm(CSR_TDATA2, tdata2);
	csr_write_imm(CSR_TDATA1, disabled);

	uintptr_t actual = csr_read_imm(CSR_TDATA1);

	if (!config_matches(actual, disabled, trigger_type, version) ||
	    csr_read_imm(CSR_TDATA2) != tdata2) {
		reset_selected_trigger();
		return -ENOTSUP;
	}

	slot->trigger_type = trigger_type;
	slot->tinfo_version = version;
	slot->tdata1 = clear_trigger_status(actual | access, trigger_type,
					    version);
	slot->tdata2 = tdata2;

	int ret = arm_selected_trigger(slot, &actual);

	if (ret == 0) {
		slot->tdata1 = clear_trigger_status(actual, trigger_type, version);
	}
	return ret;
}

static int try_program_trigger_types(struct riscv_trigger_slot *slot,
			    const struct z_debugpoint_config *config,
			    uintptr_t initial_tdata1)
{
	uintptr_t initial_type = tdata1_type(initial_tdata1);
	enum riscv_trigger_type first = RISCV_TRIGGER_MCONTROL6;
	enum riscv_trigger_type second = RISCV_TRIGGER_MCONTROL;

	if (initial_type == TDATA1_TYPE_MCONTROL) {
		first = RISCV_TRIGGER_MCONTROL;
		second = RISCV_TRIGGER_MCONTROL6;
	}
	int ret = try_program_trigger_type(slot, config, first);

	if (ret == 0) {
		return 0;
	}

	return try_program_trigger_type(slot, config, second);
}

static int allocate_cpu_trigger(struct riscv_trigger_slot *slot,
				const struct z_debugpoint_config *config,
				unsigned int cpu, int index)
{
	/* Initial installation claims any compatible trigger on this hart. */
	int ret = -ENOSPC;

	for (int hw_index = 0; hw_index < g_cpu_trigger_count[cpu]; hw_index++) {
		csr_write_imm(CSR_TSELECT, hw_index);
		if ((int)csr_read_imm(CSR_TSELECT) != hw_index ||
		    !selected_trigger_available(cpu, index, hw_index)) {
			continue;
		}

		uintptr_t initial_tdata1 = csr_read_imm(CSR_TDATA1);

		g_cpu_hw_index[cpu][index] = hw_index;
		ret = try_program_trigger_types(slot, config, initial_tdata1);
		if (ret == 0) {
			return 0;
		}

		clear_trigger_mapping(cpu, index);
	}

	return ret;
}

static int try_install_cpu_trigger_at(const struct riscv_trigger_slot *slot,
				     unsigned int cpu, int index,
				     int hw_index)
{
	csr_write_imm(CSR_TSELECT, hw_index);
	if ((int)csr_read_imm(CSR_TSELECT) != hw_index ||
	    !selected_trigger_available(cpu, index, hw_index)) {
		return -ENOSPC;
	}

	uintptr_t previous = csr_read_imm(CSR_TDATA1);

	if (cpu_has_trigger_mapping(cpu, index) &&
	    (previous & TDATA1_DMODE) != 0U) {
		clear_trigger_mapping(cpu, index);
		return -ENOSPC;
	}
	if (slot->trigger_type == RISCV_TRIGGER_MCONTROL6 &&
	    read_tinfo_version() != slot->tinfo_version) {
		return -ENOTSUP;
	}

	g_cpu_hw_index[cpu][index] = hw_index;

	int ret = program_selected_trigger(slot, NULL);

	if (ret != 0) {
		clear_trigger_mapping(cpu, index);
		return ret;
	}
	if (atomic_get(&slot->active) == 0) {
		reset_selected_trigger();
		clear_trigger_mapping(cpu, index);
	}

	return 0;
}

static int install_cpu_trigger(const struct riscv_trigger_slot *slot,
				 unsigned int cpu, int index)
{
	int mapped = g_cpu_hw_index[cpu][index];
	int ret = -ENOSPC;

	/* Reuse the cached mapping when possible, then search for a replacement. */
	if (mapped >= 0) {
		ret = try_install_cpu_trigger_at(slot, cpu, index, mapped);
		if (ret == 0) {
			return 0;
		}
	}

	for (int hw_index = 0; hw_index < g_cpu_trigger_count[cpu]; hw_index++) {
		if (hw_index == mapped) {
			continue;
		}

		int candidate_ret =
			try_install_cpu_trigger_at(slot, cpu, index, hw_index);

		if (candidate_ret == 0) {
			return 0;
		}
		if (candidate_ret == -ENOTSUP) {
			ret = candidate_ret;
		}
	}

	return ret;
}

static int find_free_slot(void)
{
	for (int i = 0; i < g_slot_count; i++) {
		if (atomic_get(&g_slots[i].active) == 0) {
			return i;
		}
	}

	return -ENOSPC;
}

int arch_debugpoint_validate(const struct z_debugpoint_config *config)
{
	if (!is_watchpoint_type(config->type)) {
		return -ENOTSUP;
	}
	if (!range_is_representable((uintptr_t)config->addr, config->size)) {
		return -ENOTSUP;
	}

	return 0;
}

int arch_debugpoint_install_local(const struct z_debugpoint_config *config,
				  z_debugpoint_handle_t handle)
{
	/*
	 * Create the logical slot and map this hart. The generic core synchronizes
	 * other online harts before z_debugpoint_add() returns.
	 */
	k_spinlock_key_t key = k_spin_lock(&g_lock);
	unsigned int cpu = arch_curr_cpu()->id;
	int ret = -ENOSPC;

	if (atomic_get(&g_initialized) == 0) {
		ret = riscv_debugpoint_init();
		if (ret != 0) {
			goto out;
		}
		atomic_set(&g_initialized, 1);
	}

	for (int i = 0; i < g_slot_count; i++) {
		if (atomic_get(&g_slots[i].active) != 0 &&
		    ranges_overlap((uintptr_t)config->addr, config->size,
				   g_slots[i].addr, g_slots[i].size)) {
			ret = -ENOTSUP;
			goto out;
		}
	}

	int index = find_free_slot();

	if (index < 0) {
		ret = index;
		goto out;
	}

	struct riscv_trigger_slot *slot = &g_slots[index];
	uintptr_t saved_tselect = csr_read_imm(CSR_TSELECT);

	slot->handle = handle;
	slot->type = config->type;
	slot->addr = (uintptr_t)config->addr;
	slot->size = config->size;
	atomic_set(&slot->active, 1);

	ret = allocate_cpu_trigger(slot, config, cpu, index);
	if (ret != 0) {
		atomic_set(&slot->active, 0);
	}

	csr_write_imm(CSR_TSELECT, saved_tselect);

out:
	k_spin_unlock(&g_lock, key);
	return ret;
}

int arch_debugpoint_uninstall_local(z_debugpoint_handle_t handle)
{
	/*
	 * Disable this hart before publishing the slot inactive. Generic CPU sync
	 * then removes stale mappings on the remaining harts.
	 */
	k_spinlock_key_t key = k_spin_lock(&g_lock);
	unsigned int cpu = arch_curr_cpu()->id;
	uintptr_t saved_tselect = csr_read_imm(CSR_TSELECT);
	int ret = -ENOENT;

	for (int i = 0; i < g_slot_count; i++) {
		struct riscv_trigger_slot *slot = &g_slots[i];

		if (atomic_get(&slot->active) == 0 ||
		    slot->handle != handle) {
			continue;
		}

		if (cpu_has_trigger_mapping(cpu, i)) {
			int hw_index = g_cpu_hw_index[cpu][i];

			csr_write_imm(CSR_TSELECT, hw_index);
			if ((int)csr_read_imm(CSR_TSELECT) != hw_index) {
				ret = -ENOTSUP;
				break;
			}

			ret = clear_selected_slot(cpu, i);
			if (ret != 0) {
				break;
			}
		}
		atomic_set(&slot->active, 0);
		ret = 0;
		break;
	}

	csr_write_imm(CSR_TSELECT, saved_tselect);
	k_spin_unlock(&g_lock, key);
	return ret;
}

int arch_debugpoint_cpu_sync(void)
{
	if (atomic_get(&g_initialized) == 0) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&g_lock);
	unsigned int cpu = arch_curr_cpu()->id;
	uintptr_t saved_tselect = csr_read_imm(CSR_TSELECT);
	int ret = 0;

	cache_cpu_trigger_extent(cpu);

#if defined(CONFIG_RISCV_HAS_TCONTROL)
	ret = configure_tcontrol();
	if (ret != 0) {
		goto out;
	}
#endif

	/*
	 * Reconcile this hart: install every active logical slot and remove mappings
	 * whose logical slot has been retired.
	 */
	for (int i = 0; i < g_slot_count; i++) {
		struct riscv_trigger_slot *slot = &g_slots[i];

		if (atomic_get(&slot->active) == 0 &&
		    !cpu_has_trigger_mapping(cpu, i)) {
			continue;
		}

		if (atomic_get(&slot->active) == 0) {
			if (cpu_has_trigger_mapping(cpu, i)) {
				int hw_index = g_cpu_hw_index[cpu][i];

				csr_write_imm(CSR_TSELECT, hw_index);
				if ((int)csr_read_imm(CSR_TSELECT) !=
				    hw_index) {
					ret = -ENOTSUP;
					break;
				}

				ret = clear_selected_slot(cpu, i);
				if (ret != 0) {
					break;
				}
			}
			continue;
		}

		ret = install_cpu_trigger(slot, cpu, i);
		if (ret != 0) {
			break;
		}

		if (atomic_get(&slot->active) == 0 &&
		    cpu_has_trigger_mapping(cpu, i)) {
			ret = clear_selected_slot(cpu, i);
			if (ret != 0) {
				break;
			}
		}
	}

#if defined(CONFIG_RISCV_HAS_TCONTROL)
out:
#endif
	csr_write_imm(CSR_TSELECT, saved_tselect);
	k_spin_unlock(&g_lock, key);
	return ret;
}

static bool select_mapped_trigger(unsigned int cpu, int slot)
{
	if (!cpu_has_trigger_mapping(cpu, slot)) {
		return false;
	}

	int hw_index = g_cpu_hw_index[cpu][slot];

	csr_write_imm(CSR_TSELECT, hw_index);
	if ((int)csr_read_imm(CSR_TSELECT) != hw_index) {
		clear_trigger_mapping(cpu, slot);
		return false;
	}

	return true;
}

static bool has_enabled_foreign_trigger(unsigned int cpu)
{
	/* A capped scan cannot exclude an unowned trigger beyond our table. */
	if (!g_cpu_trigger_scan_complete[cpu]) {
		return true;
	}

	for (int hw_index = 0;
	     hw_index < g_cpu_trigger_count[cpu]; hw_index++) {
		csr_write_imm(CSR_TSELECT, hw_index);
		if ((int)csr_read_imm(CSR_TSELECT) != hw_index) {
			return true;
		}

		uintptr_t tdata1 = csr_read_imm(CSR_TDATA1);

		if (logical_slot_for_trigger(cpu, hw_index) < 0 &&
		    trigger_may_fire(tdata1)) {
			return true;
		}
	}

	return false;
}

static int restore_cpu_triggers(unsigned int cpu)
{
	/*
	 * Re-arm active logical slots after attribution and clear mappings for slots
	 * retired by removal or one-shot handling.
	 */
	for (int i = 0; i < g_slot_count; i++) {
		struct riscv_trigger_slot *slot = &g_slots[i];

		if (!select_mapped_trigger(cpu, i)) {
			continue;
		}
		if (atomic_get(&slot->active) == 0) {
			int ret = clear_selected_slot(cpu, i);

			if (ret != 0) {
				return ret;
			}
			continue;
		}

		if ((csr_read_imm(CSR_TDATA1) & TDATA1_DMODE) != 0U) {
			clear_trigger_mapping(cpu, i);
			return -EBUSY;
		}
		uintptr_t actual;
		int ret = program_selected_trigger(slot, &actual);

		if (ret != 0) {
			int clear_ret = clear_selected_slot(cpu, i);

			return clear_ret != 0 ? clear_ret : ret;
		}
		if (atomic_get(&slot->active) == 0) {
			ret = clear_selected_slot(cpu, i);

			if (ret != 0) {
				return ret;
			}
			continue;
		}
		if (hit_value(slot, actual) != 0U) {
			ret = clear_selected_slot(cpu, i);

			return ret != 0 ? ret : -ENOTSUP;
		}
	}

	return 0;
}

static bool prepare_hit(struct riscv_trigger_slot *slot, uintptr_t tdata1,
			unsigned int hit_value, bool access_addr_valid,
			struct riscv_hit *record)
{
	enum z_debugpoint_timing timing =
		trigger_timing(slot, tdata1, hit_value);
	bool rearm_required = timing != Z_DEBUGPOINT_TIMING_AFTER;

	if (rearm_required) {
		if (!atomic_cas(&slot->active, 1, 0)) {
			return false;
		}
	} else if (atomic_get(&slot->active) == 0) {
		return false;
	}

	*record = (struct riscv_hit) {
		.handle = slot->handle,
		.type = (uint8_t)slot->type,
		.timing = (uint8_t)timing,
		.access_addr_valid = access_addr_valid,
		.rearm_required = rearm_required,
	};
	return true;
}

int z_riscv_debugpoint_handle(struct arch_esf *esf)
{
	if (atomic_get(&g_initialized) == 0) {
		return -ENOENT;
	}

	uintptr_t access_addr;

	__asm__ volatile("csrr %0, mtval" : "=r"(access_addr));

	uintptr_t saved_tselect = csr_read_imm(CSR_TSELECT);
	unsigned int cpu = arch_curr_cpu()->id;
	uintptr_t trigger_state[CONFIG_DEBUGPOINT_MAX_SLOTS] = {0};
	struct riscv_hit hits[CONFIG_DEBUGPOINT_MAX_SLOTS];
	int hit_count = 0;
	bool owned_trap = false;

	/*
	 * Disable and snapshot every mapped trigger before attribution. Prefer
	 * hit bits, then mtval, then a sole enabled Zephyr trigger when no
	 * foreign trigger could have raised the exception.
	 */
	for (int i = 0; i < g_slot_count; i++) {
		if (!select_mapped_trigger(cpu, i)) {
			continue;
		}

		trigger_state[i] = csr_read_imm(CSR_TDATA1);
		if (!disable_selected_trigger()) {
			int ret = restore_cpu_triggers(cpu);

			csr_write_imm(CSR_TSELECT, saved_tselect);
			return ret != 0 ? ret : -EIO;
		}
	}

	for (int i = 0; i < g_slot_count; i++) {
		struct riscv_trigger_slot *slot = &g_slots[i];

		if (!cpu_has_trigger_mapping(cpu, i)) {
			continue;
		}

		unsigned int value = hit_value(slot, trigger_state[i]);

		if (value == 0U) {
			continue;
		}
		owned_trap = true;

		bool addr_valid =
			range_contains(slot->addr, slot->size, access_addr);

		if (prepare_hit(slot, trigger_state[i], value, addr_valid,
				&hits[hit_count])) {
			hit_count++;
		}
	}

	int ebreak_status =
		owned_trap ? 0 : instruction_is_ebreak(esf->mepc);
	bool ebreak = ebreak_status > 0;
	bool instruction_known = ebreak_status >= 0;

	if (!owned_trap && !ebreak && instruction_known) {
		for (int i = 0; i < g_slot_count; i++) {
			struct riscv_trigger_slot *slot = &g_slots[i];

			if (!cpu_has_trigger_mapping(cpu, i) ||
			    !range_contains(slot->addr, slot->size,
					    access_addr) ||
			    !trigger_is_enabled(trigger_state[i])) {
				continue;
			}
			owned_trap = true;
			if (prepare_hit(slot, trigger_state[i], 0U, true,
					&hits[0])) {
				hit_count = 1;
			}
			break;
		}
	}

	if (!owned_trap && !ebreak && instruction_known &&
	    !has_enabled_foreign_trigger(cpu)) {
		int candidate = -1;
		int candidate_count = 0;

		for (int i = 0; i < g_slot_count; i++) {
			if (!cpu_has_trigger_mapping(cpu, i) ||
			    !trigger_is_enabled(trigger_state[i])) {
				continue;
			}

			candidate = i;
			candidate_count++;
		}

		if (candidate_count == 1) {
			struct riscv_trigger_slot *slot = &g_slots[candidate];

			owned_trap = true;
			if (prepare_hit(slot, trigger_state[candidate], 0U,
					false, &hits[0])) {
				hit_count = 1;
			}
		}
	}

	if (hit_count == 0) {
		int ret = restore_cpu_triggers(cpu);

		csr_write_imm(CSR_TSELECT, saved_tselect);
		if (ret != 0) {
			return ret;
		}
		return owned_trap ? 0 : -ENOENT;
	}

	csr_write_imm(CSR_TSELECT, saved_tselect);

	/* Invoke callbacks while all Zephyr-owned physical triggers are disabled. */
	for (int i = 0; i < hit_count; i++) {
		struct z_debugpoint_event event = {
			.pc = (void *)esf->mepc,
			.access_addr = hits[i].access_addr_valid ?
				       (void *)access_addr : NULL,
			.access_addr_valid = hits[i].access_addr_valid,
			.access_size = 0U,
			.type = (enum z_debugpoint_type)hits[i].type,
			.timing = (enum z_debugpoint_timing)hits[i].timing,
			.rearm_required = hits[i].rearm_required,
			.esf = esf,
		};

		z_debugpoint_hit(hits[i].handle, &event);
	}

	saved_tselect = csr_read_imm(CSR_TSELECT);
	int ret = restore_cpu_triggers(cpu);

	csr_write_imm(CSR_TSELECT, saved_tselect);
	return ret;
}
