/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <soc_init.h>
#include <flash_init.h>
#include <esp_err.h>
#include <esp_private/cache_utils.h>
#include <esp_private/system_internal.h>
#include <esp_timer.h>
#include <efuse_virtual.h>
#include <esp_rom_serial_output.h>
#include <zephyr/init.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/arch/common/init.h>
#include <zephyr/arch/riscv/arch.h>
#include <hal/interrupt_clic_ll.h>
#include <riscv/interrupt.h>
#include <riscv/csr.h>
#include <soc/interrupts.h>
#include <psram.h>
#include <zephyr/sys/printk.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/pm_cpu_ops.h>
#include <esp_cpu.h>
#include <soc/hp_sys_clkrst_reg.h>

#if defined(CONFIG_NOCACHE_MEMORY)
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/util.h>
#include "cache.h"
#endif

extern void esp_reset_reason_init(void);
extern FUNC_NORETURN void z_cstart(void);

/*
 * The ESP32-P4 clock generator only produces a discrete set of CPU
 * frequencies, and the set differs by silicon revision: pre-v3 (v1.3) parts
 * support 90/180/360 MHz, while the v3.x family supports 100/200/400 MHz.
 * A devicetree clock-frequency outside the selected revision's set builds
 * cleanly but will not boot, so validate set membership at compile time.
 */
#if defined(CONFIG_SOC_ESP32P4_REV_1_3)
#define ESP32P4_CPU_FREQ_VALID(hz)                                                        \
	((hz) == 90000000 || (hz) == 180000000 || (hz) == 360000000)
#define ESP32P4_CPU_FREQ_MSG "ESP32-P4 rev v1.3 CPU frequency must be 90, 180, or 360 MHz"
#else
#define ESP32P4_CPU_FREQ_VALID(hz)                                                        \
	((hz) == 100000000 || (hz) == 200000000 || (hz) == 400000000)
#define ESP32P4_CPU_FREQ_MSG "ESP32-P4 rev v3.x CPU frequency must be 100, 200, or 400 MHz"
#endif

BUILD_ASSERT(ESP32P4_CPU_FREQ_VALID(DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)),
	     ESP32P4_CPU_FREQ_MSG);
BUILD_ASSERT(ESP32P4_CPU_FREQ_VALID(DT_PROP(DT_PATH(cpus, cpu_1), clock_frequency)),
	     ESP32P4_CPU_FREQ_MSG);

#ifdef CONFIG_SMP
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <soc/hp_system_reg.h>
#include <ipi.h>

#define ESP32P4_MAX_NUM_CPUS_MSG "ESP32-P4 has 2 HP cores"
BUILD_ASSERT(CONFIG_MP_MAX_NUM_CPUS <= 2, ESP32P4_MAX_NUM_CPUS_MSG);

#endif

#if defined(CONFIG_NOCACHE_MEMORY)
void nocache_region_init(void)
{
	const uintptr_t start = (uintptr_t)_nocache_ram_start;
	const size_t used = (size_t)(uintptr_t)_nocache_ram_size;
	size_t size;
	uintptr_t napot_addr;
	uint32_t cfg;

	if (used == 0) {
		return;
	}

	/* A PMA NAPOT entry can only describe a power-of-two region that is
	 * aligned to its own size. The linker sizes the section to its content,
	 * so round the length up to the next power of two and make sure the
	 * base is aligned to it, otherwise the entry would leave part of the
	 * section cached and silently break DMA coherency.
	 */
	size = BIT(LOG2CEIL(used));

	__ASSERT((start & (size - 1)) == 0U,
		 "nocache region 0x%lx not aligned to its NAPOT size 0x%zx", start, size);

	napot_addr = (start | ((size >> 1) - 1)) >> PMA_SHIFT;
	cfg = PMA_NAPOT | PMA_EN | PMA_R | PMA_W | PMA_NONCACHEABLE;

	sys_cache_data_flush_and_invd_range(_nocache_ram_start, used);

	RV_WRITE_CSR(CSR_PMAADDR0 + NOCACHE_PMA_ENTRY, napot_addr);
	RV_WRITE_CSR(CSR_PMACFG0 + NOCACHE_PMA_ENTRY, cfg);
}
#endif /* CONFIG_NOCACHE_MEMORY */

static void core_intr_matrix_clear(void)
{
	for (int i = 0; i < ETS_MAX_INTR_SOURCE; i++) {
		interrupt_clic_ll_route(0, i, ETS_INVALID_INUM);
	}

	for (int i = 0; i < 32; i++) {
		esprv_int_set_vectored(i, true);
	}
}

void IRAM_ATTR __esp_platform_app_start(void)
{
#ifdef CONFIG_FPU
	/* The Espressif boot path enters via __start, bypassing the
	 * generic riscv reset.S that initialises mstatus.FS. Enable
	 * the FPU here before any float instruction is executed.
	 */
	__asm__ volatile("li t0, %0\n"
			 "csrs mstatus, t0\n"
			 "fscsr zero\n" ::"i"(MSTATUS_FS_INIT)
			 : "t0");
#endif

	core_intr_matrix_clear();

	esp_reset_reason_init();

	esp_timer_early_init();

	esp_flash_config();

	esp_efuse_init_virtual();

#if defined(CONFIG_NOCACHE_MEMORY)
	nocache_region_init();
#endif

	z_cstart();

	CODE_UNREACHABLE;
}

void IRAM_ATTR __esp_platform_mcuboot_start(void)
{
	z_cstart();

	CODE_UNREACHABLE;
}

int IRAM_ATTR arch_printk_char_out(int c)
{
	if (c == '\n') {
		esp_rom_output_tx_one_char('\r');
	}
	esp_rom_output_tx_one_char(c);
	return 0;
}

void sys_arch_reboot(int type)
{
	esp_restart();
}

#ifdef CONFIG_SMP
/* Entry stub in soc/espressif/esp32p4/start_core1.S */
extern void __esp32p4_core1_start(void);

enum { IPI_SCHED = 0 };
static atomic_t cpu_pending_ipi[CONFIG_MP_MAX_NUM_CPUS];

static const uintptr_t ipi_reg[] = {
	HP_SYSTEM_CPU_INT_FROM_CPU_0_REG,
	HP_SYSTEM_CPU_INT_FROM_CPU_1_REG,
};

static inline void ipi_raise(unsigned int hart)
{
	REG_WRITE(ipi_reg[hart], BIT(0));
}

static inline void ipi_clear(unsigned int hart)
{
	REG_WRITE(ipi_reg[hart], 0);
}

int pm_cpu_on(unsigned long cpuid, uintptr_t entry_point)
{
	/* boot core 1 at our own stub and ignore entry_point
	 * (generic code passes &__start, which is core 0's boot path).
	 */
	uintptr_t boot_addr = (uintptr_t)&__esp32p4_core1_start;

	ARG_UNUSED(entry_point);

	if (cpuid != 1U) {
		return -EINVAL;
	}

	/* clock on */
	REG_SET_BIT(HP_SYS_CLKRST_SOC_CLK_CTRL0_REG,
		    HP_SYS_CLKRST_REG_CORE1_CPU_CLK_EN);

	/* put core 1 in reset: also forces a clean restart if it was
	 * already running (warm reset case)
	 */
	REG_SET_BIT(HP_SYS_CLKRST_HP_RST_EN0_REG,
		    HP_SYS_CLKRST_REG_RST_EN_CORE1_GLOBAL);

	/* unstall and set the boot address while core 1 is in reset */
	esp_cpu_unstall(1);
	ets_set_appcpu_boot_addr((uint32_t)boot_addr);

	/* release reset: core 1's ROM now starts with the address in place */
	REG_CLR_BIT(HP_SYS_CLKRST_HP_RST_EN0_REG,
		    HP_SYS_CLKRST_REG_RST_EN_CORE1_GLOBAL);

	return 0;
}

int pm_cpu_off(void)
{
	/* Core 1 is never powered down in the SMP bring-up. */
	return -ENOTSUP;
}

void arch_sched_directed_ipi(uint32_t cpu_bitmap)
{
	unsigned int key = arch_irq_lock();
	unsigned int id = _current_cpu->id;
	unsigned int num_cpus = arch_num_cpus();

	for (unsigned int i = 0; i < num_cpus; i++) {
		if ((i != id) && _kernel.cpus[i].arch.online && ((cpu_bitmap & BIT(i)) != 0)) {
			atomic_set_bit(&cpu_pending_ipi[i], IPI_SCHED);
			ipi_raise(_kernel.cpus[i].arch.hartid);
		}
	}

	arch_irq_unlock(key);
}

static void sched_ipi_handler(void *unused)
{
	ARG_UNUSED(unused);

	/* clear pending interrupt first */
	ipi_clear(csr_read(mhartid));
	atomic_val_t pending_ipi = atomic_clear(&cpu_pending_ipi[_current_cpu->id]);

	if (pending_ipi & ATOMIC_MASK(IPI_SCHED)) {
		z_sched_ipi();
	}
}

/* Must run on the core that will receive the interrupt */
static void ipi_init_this_cpu(void)
{
	/*
	 * Each CPU listens to its own FROM_CPU source, so that
	 * arch_sched_directed_ipi() can target one CPU. Interrupt sources
	 * ETS_FROM_CPU_INTR0 and INTR1 are consecutive.
	 */
	int ret = esp_intr_alloc(ETS_FROM_CPU_INTR0_SOURCE + csr_read(mhartid),
			0, /*let the driver choose the level*/
			sched_ipi_handler,
			NULL,
			NULL);

	__ASSERT(ret == 0,
		"CPU%lu: could not allocate IPI interrupt (%d)",
		(unsigned long)csr_read(mhartid), ret);
	ARG_UNUSED(ret);
}

int arch_smp_init(void)
{
	ipi_init_this_cpu();
	return 0;
}

void smp_timer_init(void)
{
	/* Runs on the secondary CPU after z_dummy_thread_init(). */
	ipi_init_this_cpu();
}

#endif /* CONFIG_SMP */

#ifdef CONFIG_SOC_PER_CORE_INIT_HOOK
void soc_per_core_init_hook(void)
{
	if (csr_read(mhartid) == 0) {
		/*
		 * The boot CPU 0 did its per-hart setup in __esp_platform_app_start(),
		 * so only secondary CPU 1 needs to run the code below.
		 */
		return;
	}

#if defined(CONFIG_NOCACHE_MEMORY)
	/* PMA CSRs are per hart.*/
	nocache_region_init();
#endif

	/* clear the boot address so a later reset does not start core 1
	 * at a stale entry
	 */
	ets_set_appcpu_boot_addr(0);
}
#endif /* CONFIG_SOC_PER_CORE_INIT_HOOK */
