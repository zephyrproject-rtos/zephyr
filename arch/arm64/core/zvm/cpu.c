/*
 * Copyright 2021-2022 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/spinlock.h>
#include <zephyr/toolchain.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm64/lib_helpers.h>
#include <zephyr/drivers/pm_cpu_ops/psci.h>
#include <zephyr/zvm/zvm.h>
#include <zephyr/zvm/vm_cpu.h>
#include <zephyr/zvm/vdev/vgic_v3.h>
#include <zephyr/zvm/arm/timer.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

extern uint64_t cpu_vmpidr_el2_list[CONFIG_MP_MAX_NUM_CPUS];

/**
 * @brief Check whether Hyp and vhe are supported
 */
static bool is_basic_hardware_support(void)
{
	if (!is_el_implemented(MODE_EL2)) {
		ZVM_LOG_ERR("Hyp mode not available on this system.\n");
		return false;
	}

	if (is_el2_vhe_supported()) {
		return true;
	}
	return false;
}

static bool is_gicv3_device_support(void)
{
#if defined(CONFIG_GIC_V3)
	return true;
#else
	return false;
#endif
}

static int vcpu_virq_init(struct z_vcpu *vcpu)
{
	struct gicv3_vcpuif_ctxt *ctxt;

	/* init vgicv3 context */
	ctxt = (struct gicv3_vcpuif_ctxt *)k_malloc(sizeof(struct gicv3_vcpuif_ctxt));
	if (!ctxt) {
		ZVM_LOG_ERR("Init vcpu context failed");
		return -ENXIO;
	}
	memset(ctxt, 0, sizeof(struct gicv3_vcpuif_ctxt));

	vcpu_gicv3_init(ctxt);
	vcpu->arch->virq_data = ctxt;

	return 0;
}

static int vcpu_virq_deinit(struct z_vcpu *vcpu)
{
	struct gicv3_vcpuif_ctxt *ctxt;

	ctxt = vcpu->arch->virq_data;
	k_free(ctxt);

	return 0;
}

static void vcpu_vgic_save(struct z_vcpu *vcpu)
{
	vgicv3_state_save(vcpu, (struct gicv3_vcpuif_ctxt *)vcpu->arch->virq_data);
}

static void vcpu_vgic_load(struct z_vcpu *vcpu)
{
	vgicv3_state_load(vcpu, (struct gicv3_vcpuif_ctxt *)vcpu->arch->virq_data);
}

static void vcpu_vtimer_save(struct z_vcpu *vcpu)
{
	uint64_t vcycles, pcycles;
	k_timeout_t vticks, pticks;
	struct virt_timer_context *timer_ctxt = vcpu->arch->vtimer_context;

#ifdef CONFIG_HAS_ARM_VHE
	/* virt timer save */
	timer_ctxt->cntv_ctl = read_cntv_ctl_el02();
	write_cntv_ctl_el02(timer_ctxt->cntv_ctl & ~CNTV_CTL_ENABLE_BIT);
	timer_ctxt->cntv_cval = read_cntv_cval_el02();
	/* phys timer save */
	timer_ctxt->cntp_ctl = read_cntp_ctl_el02();
	write_cntp_ctl_el02(timer_ctxt->cntp_ctl & ~CNTP_CTL_ENABLE_BIT);
	timer_ctxt->cntp_cval = read_cntp_cval_el02();

	if (timer_ctxt->cntv_ctl & CNTV_CTL_ENABLE_BIT) {
		if (!(timer_ctxt->cntv_ctl & CNTV_CTL_IMASK_BIT)) {
			vcycles = read_cntvct_el0();
			if (timer_ctxt->cntv_cval <= vcycles) {
				vticks.ticks = 0;
			} else {
				vticks.ticks =
				(timer_ctxt->cntv_cval - vcycles) / HOST_CYC_PER_TICK;
			}
			z_add_timeout(
				&timer_ctxt->vtimer_timeout,
				timer_ctxt->vtimer_timeout.fn,
				vticks);
		}
	}
	if (timer_ctxt->cntp_ctl & CNTP_CTL_ENABLE_BIT) {
		if (!(timer_ctxt->cntp_ctl & CNTP_CTL_IMASK_BIT)) {
			pcycles = read_cntpct_el0();
			if (timer_ctxt->cntp_cval <= pcycles) {
				pticks.ticks = 0;
			} else {
				pticks.ticks =
				(timer_ctxt->cntp_cval - pcycles) / HOST_CYC_PER_TICK;
			}
			z_add_timeout(
				&timer_ctxt->ptimer_timeout,
				timer_ctxt->ptimer_timeout.fn,
				pticks
				);
		}
	}
#else
	timer_ctxt->cntv_ctl = read_cntv_ctl_el0();
	write_cntv_ctl_el0(timer_ctxt->cntv_ctl & ~CNTV_CTL_ENABLE_BIT);
	timer_ctxt->cntv_cval = read_cntv_cval_el0();
#endif
	barrier_dsync_fence_full();
}

static void vcpu_vtimer_load(struct z_vcpu *vcpu)
{
	struct virt_timer_context *timer_ctxt = vcpu->arch->vtimer_context;

	z_abort_timeout(&timer_ctxt->vtimer_timeout);
	z_abort_timeout(&timer_ctxt->ptimer_timeout);

#ifdef CONFIG_HAS_ARM_VHE
	write_cntvoff_el2(timer_ctxt->timer_offset);
#else
	write_cntvoff_el2(timer_ctxt->timer_offset);
	write_cntv_cval_el0(timer_ctxt->cntv_cval);
	write_cntv_ctl_el0(timer_ctxt->cntv_ctl);
#endif
	barrier_dsync_fence_full();
}

static void arch_vcpu_sys_regs_init(struct z_vcpu *vcpu)
{
	struct zvm_vcpu_context *aarch64_c = &vcpu->arch->ctxt;

	/*Each vcpu's mpidr_el1 is equal to physical cpu begin from 0 to n. */
	aarch64_c->sys_regs[VCPU_MPIDR_EL1] = cpu_vmpidr_el2_list[vcpu->vcpu_id];

	aarch64_c->sys_regs[VCPU_CPACR_EL1] = 0x03 << 20;
	aarch64_c->sys_regs[VCPU_VPIDR] = 0x410fc050;

	aarch64_c->sys_regs[VCPU_TTBR0_EL1] = 0;
	aarch64_c->sys_regs[VCPU_TTBR1_EL1] = 0;
	aarch64_c->sys_regs[VCPU_MAIR_EL1] = 0;
	aarch64_c->sys_regs[VCPU_TCR_EL1] = 0;
	aarch64_c->sys_regs[VCPU_PAR_EL1] = 0;
	aarch64_c->sys_regs[VCPU_AMAIR_EL1] = 0;

	aarch64_c->sys_regs[VCPU_TPIDR_EL0] = read_tpidr_el0();
	aarch64_c->sys_regs[VCPU_TPIDRRO_EL0] = read_tpidrro_el0();
	aarch64_c->sys_regs[VCPU_CSSELR_EL1] = read_csselr_el1();
	aarch64_c->sys_regs[VCPU_SCTLR_EL1] = 0x30C50838;
	aarch64_c->sys_regs[VCPU_ESR_EL1] = 0;
	aarch64_c->sys_regs[VCPU_AFSR0_EL1] = 0;
	aarch64_c->sys_regs[VCPU_AFSR1_EL1] = 0;
	aarch64_c->sys_regs[VCPU_FAR_EL1] = 0;
	aarch64_c->sys_regs[VCPU_VBAR_EL1] = 0;
	aarch64_c->sys_regs[VCPU_CONTEXTIDR_EL1] = 0;
	aarch64_c->sys_regs[VCPU_CNTKCTL_EL1] = 0;
	aarch64_c->sys_regs[VCPU_ELR_EL1] = 0;
	aarch64_c->sys_regs[VCPU_SPSR_EL1] = SPSR_MODE_EL1H;
}

static void arch_vcpu_sys_regs_deinit(struct z_vcpu *vcpu)
{
	int i;
	struct zvm_vcpu_context *aarch64_c = &vcpu->arch->ctxt;

	for (i = 0; i < VCPU_SYS_REG_NUM; i++) {
		aarch64_c->sys_regs[i] = 0;
	}
}

static void arch_vcpu_common_regs_init(struct z_vcpu *vcpu)
{
	struct zvm_vcpu_context *ctxt;

	ctxt = &vcpu->arch->ctxt;
	memset(&ctxt->regs, 0, sizeof(struct zvm_vcpu_context));

	ctxt->regs.pc = vcpu->vm->os->info.entry_point;
	ctxt->regs.pstate = (SPSR_MODE_EL1H | DAIF_DBG_BIT | DAIF_ABT_BIT |
							DAIF_IRQ_BIT | DAIF_FIQ_BIT);
}

static void arch_vcpu_common_regs_deinit(struct z_vcpu *vcpu)
{
	ARG_UNUSED(vcpu);
}

static void arch_vcpu_fp_regs_init(struct z_vcpu *vcpu)
{
	ARG_UNUSED(vcpu);
}

static void arch_vcpu_fp_regs_deinit(struct z_vcpu *vcpu)
{
	ARG_UNUSED(vcpu);
}

uint64_t *find_index_reg(uint16_t index, arch_commom_regs_t *regs)
{
	uint64_t *value;

	if (index == 31) {
		value = NULL;
	} else if (index > 18 && index < 30) {
		value = &regs->callee_saved_regs.x19 + index - 19;
	} else {
		if (index == 30) {
			value = &regs->esf_handle_regs.lr;
		} else {
			value = &regs->esf_handle_regs.x0 + index;
		}
	}
	return value;
}

void vcpu_sysreg_load(struct z_vcpu *vcpu)
{
	struct zvm_vcpu_context *g_context = &vcpu->arch->ctxt;

	write_csselr_el1(g_context->sys_regs[VCPU_CSSELR_EL1]);
	write_vmpidr_el2(g_context->sys_regs[VCPU_MPIDR_EL1]);
	write_sctlr_el12(g_context->sys_regs[VCPU_SCTLR_EL1]);
	write_tcr_el12(g_context->sys_regs[VCPU_TCR_EL1]);
	write_cpacr_el12(g_context->sys_regs[VCPU_CPACR_EL1]);
	write_ttbr0_el12(g_context->sys_regs[VCPU_TTBR0_EL1]);
	write_ttbr1_el12(g_context->sys_regs[VCPU_TTBR1_EL1]);
	write_esr_el12(g_context->sys_regs[VCPU_ESR_EL1]);
	write_afsr0_el12(g_context->sys_regs[VCPU_AFSR0_EL1]);
	write_afsr1_el12(g_context->sys_regs[VCPU_AFSR1_EL1]);
	write_far_el12(g_context->sys_regs[VCPU_FAR_EL1]);
	write_mair_el12(g_context->sys_regs[VCPU_MAIR_EL1]);
	write_vbar_el12(g_context->sys_regs[VCPU_VBAR_EL1]);
	write_contextidr_el12(g_context->sys_regs[VCPU_CONTEXTIDR_EL1]);
	write_amair_el12(g_context->sys_regs[VCPU_AMAIR_EL1]);
	write_cntkctl_el12(g_context->sys_regs[VCPU_CNTKCTL_EL1]);
	write_par_el1(g_context->sys_regs[VCPU_PAR_EL1]);
	write_tpidr_el1(g_context->sys_regs[VCPU_TPIDR_EL1]);
	write_sp_el1(g_context->sys_regs[VCPU_SP_EL1]);
	write_elr_el12(g_context->sys_regs[VCPU_ELR_EL1]);
	write_spsr_el12(g_context->sys_regs[VCPU_SPSR_EL1]);

	vcpu->arch->vcpu_sys_register_loaded = true;
	write_hstr_el2(BIT(15));
	vcpu->arch->host_mdcr_el2 = read_mdcr_el2();
	write_mdcr_el2(vcpu->arch->guest_mdcr_el2);
}

void vcpu_sysreg_save(struct z_vcpu *vcpu)
{
	struct zvm_vcpu_context *g_context = &vcpu->arch->ctxt;

	g_context->sys_regs[VCPU_MPIDR_EL1] = read_vmpidr_el2();
	g_context->sys_regs[VCPU_CSSELR_EL1] = read_csselr_el1();
	g_context->sys_regs[VCPU_ACTLR_EL1] = read_actlr_el1();

	g_context->sys_regs[VCPU_SCTLR_EL1] = read_sctlr_el12();
	g_context->sys_regs[VCPU_CPACR_EL1] = read_cpacr_el12();
	g_context->sys_regs[VCPU_TTBR0_EL1] = read_ttbr0_el12();
	g_context->sys_regs[VCPU_TTBR1_EL1] = read_ttbr1_el12();
	g_context->sys_regs[VCPU_ESR_EL1] = read_esr_el12();
	g_context->sys_regs[VCPU_TCR_EL1] = read_tcr_el12();
	g_context->sys_regs[VCPU_AFSR0_EL1] = read_afsr0_el12();
	g_context->sys_regs[VCPU_AFSR1_EL1] = read_afsr1_el12();
	g_context->sys_regs[VCPU_FAR_EL1] = read_far_el12();
	g_context->sys_regs[VCPU_MAIR_EL1] = read_mair_el12();
	g_context->sys_regs[VCPU_VBAR_EL1] = read_vbar_el12();
	g_context->sys_regs[VCPU_CONTEXTIDR_EL1] = read_contextidr_el12();
	g_context->sys_regs[VCPU_AMAIR_EL1] = read_amair_el12();
	g_context->sys_regs[VCPU_CNTKCTL_EL1] = read_cntkctl_el12();

	g_context->sys_regs[VCPU_PAR_EL1] = read_par_el1();
	g_context->sys_regs[VCPU_TPIDR_EL1] = read_tpidr_el1();
	g_context->regs.esf_handle_regs.elr = read_elr_el12();
	g_context->regs.esf_handle_regs.spsr = read_spsr_el12();
	vcpu->arch->vcpu_sys_register_loaded = false;
}

void switch_to_guest_sysreg(struct z_vcpu *vcpu)
{
	uint32_t vcpu_reg_val;

	struct zvm_vcpu_context *gcontext = &vcpu->arch->ctxt;
	struct zvm_vcpu_context *hcontext = &vcpu->arch->host_ctxt;

	/* save host context */
	hcontext->running_vcpu = vcpu;
	hcontext->sys_regs[VCPU_SPSR_EL1] = read_spsr_el1();
	hcontext->sys_regs[VCPU_MDSCR_EL1] = read_mdscr_el1();

	/* load stage-2 pgd for vm */
	write_vtcr_el2(vcpu->vm->arch->vtcr_el2);
	write_vttbr_el2(vcpu->vm->arch->vttbr);
	barrier_isync_fence_full();

	/* enable hyperviosr trap */
	write_hcr_el2(vcpu->arch->hcr_el2);
	vcpu_reg_val = read_cpacr_el1();
	vcpu_reg_val |= CPACR_EL1_TTA;
	vcpu_reg_val &= ~CPACR_EL1_ZEN;
	vcpu_reg_val |= CPTR_EL2_TAM;
	vcpu_reg_val |= CPACR_EL1_FPEN_NOTRAP;
	write_cpacr_el1(vcpu_reg_val);
	write_vbar_el2((uint64_t)_hyp_vector_table);

	hcontext->sys_regs[VCPU_TPIDRRO_EL0] = read_tpidrro_el0();
	write_tpidrro_el0(gcontext->sys_regs[VCPU_TPIDRRO_EL0]);
	write_elr_el2(gcontext->regs.pc);
	write_spsr_el2(gcontext->regs.pstate);
	vcpu_reg_val = ((struct gicv3_vcpuif_ctxt *)vcpu->arch->virq_data)->icc_ctlr_el1;
	vcpu_reg_val &= ~(0x02);
	write_sysreg(vcpu_reg_val, ICC_CTLR_EL1);
}

void switch_to_host_sysreg(struct z_vcpu *vcpu)
{
	uint32_t vcpu_reg_val;
	struct zvm_vcpu_context *gcontext = &vcpu->arch->ctxt;
	struct zvm_vcpu_context *hcontext = &vcpu->arch->host_ctxt;

	gcontext->sys_regs[VCPU_TPIDRRO_EL0] = read_tpidrro_el0();
	write_tpidrro_el0(hcontext->sys_regs[VCPU_TPIDRRO_EL0]);
	gcontext->regs.pc = read_elr_el2();
	gcontext->regs.pstate = read_spsr_el2();
	vcpu_reg_val = ((struct gicv3_vcpuif_ctxt *)vcpu->arch->virq_data)->icc_ctlr_el1;
	vcpu_reg_val |= (0x02);
	write_sysreg(vcpu_reg_val, ICC_CTLR_EL1);

	/* disable hyperviosr trap */
	if (vcpu->arch->hcr_el2 & HCR_VSE_BIT) {
		vcpu->arch->hcr_el2 = read_hcr_el2();
	}
	write_hcr_el2(HCR_VHE_FLAGS);
	write_vbar_el2((uint64_t)_vector_table);

	/* save vm's stage-2 pgd */
	vcpu->vm->arch->vtcr_el2 = read_vtcr_el2();
	vcpu->vm->arch->vttbr = read_vttbr_el2();
	barrier_isync_fence_full();

	/* load host context */
	write_mdscr_el1(hcontext->sys_regs[VCPU_MDSCR_EL1]);
	write_spsr_el1(hcontext->sys_regs[VCPU_SPSR_EL1]);
}

void arch_vcpu_context_save(struct z_vcpu *vcpu)
{
	vcpu_vgic_save(vcpu);
	vcpu_vtimer_save(vcpu);
	vcpu_sysreg_save(vcpu);
}

void arch_vcpu_context_load(struct z_vcpu *vcpu)
{
	int cpu;

	cpu = _current_cpu->id;
	vcpu->cpu = cpu;

	vcpu_sysreg_load(vcpu);
	vcpu_vtimer_load(vcpu);
	vcpu_vgic_load(vcpu);

	vcpu->arch->hcr_el2 &= ~HCR_TWE_BIT;
	vcpu->arch->hcr_el2 &= ~HCR_TWI_BIT;
}

int arch_vcpu_init(struct z_vcpu *vcpu)
{
	int ret = 0;
	struct vcpu_arch *vcpu_arch = vcpu->arch;
	struct vm_arch *vm_arch = vcpu->vm->arch;

	vcpu_arch->hcr_el2 = HCR_VM_FLAGS;
	vcpu_arch->guest_mdcr_el2 = 0;
	vcpu_arch->host_mdcr_el2 = 0;
	vcpu_arch->list_regs_map = 0;
	vcpu_arch->pause = 0;
	vcpu_arch->vcpu_sys_register_loaded = false;

	/* init vm_arch here */
	vm_arch->vtcr_el2 = (0x20 | BIT(6) | BIT(8) | BIT(10) | BIT(12) | BIT(13) | BIT(31));
	vm_arch->vttbr = (vcpu->vm->vmid | vm_arch->vm_pgd_base);

	arch_vcpu_common_regs_init(vcpu);
	arch_vcpu_sys_regs_init(vcpu);
	arch_vcpu_fp_regs_init(vcpu);

	ret = vcpu_virq_init(vcpu);
	if (ret) {
		return ret;
	}

	ret = arch_vcpu_timer_init(vcpu);
	if (ret) {
		return ret;
	}

#ifdef CONFIG_VM_DTB_FILE_INPUT
	/* passing argu to linux, like fdt and others */
	vcpu_arch->ctxt.regs.esf_handle_regs.x0 = LINUX_DTB_MEM_BASE;
	vcpu_arch->ctxt.regs.esf_handle_regs.x1 = 0;
	vcpu_arch->ctxt.regs.esf_handle_regs.x2 = 0;
	vcpu_arch->ctxt.regs.esf_handle_regs.x3 = 0;
	vcpu_arch->ctxt.regs.callee_saved_regs.x20 = LINUX_DTB_MEM_BASE;
	vcpu_arch->ctxt.regs.callee_saved_regs.x21 = 0;
	vcpu_arch->ctxt.regs.callee_saved_regs.x22 = 0;
	vcpu_arch->ctxt.regs.callee_saved_regs.x23 = 0;
#endif
	return ret;
}

int arch_vcpu_deinit(struct z_vcpu *vcpu)
{
	int ret = 0;

	ret = arch_vcpu_timer_deinit(vcpu);
	if (ret) {
		ZVM_LOG_WARN("Deinit arch timer failed.\n");
		return ret;
	}

	ret = vcpu_virq_deinit(vcpu);
	if (ret) {
		ZVM_LOG_WARN("Deinit virt cpu irq failed.\n");
		return ret;
	}

	arch_vcpu_fp_regs_deinit(vcpu);
	arch_vcpu_sys_regs_deinit(vcpu);
	arch_vcpu_common_regs_deinit(vcpu);

	return ret;
}

int zvm_arch_init(void *op)
{
	ARG_UNUSED(op);
	int ret = 0;

	/* Is hyp、vhe available? */
	if (!is_basic_hardware_support()) {
		return -ESRCH;
	}
	if (!is_gicv3_device_support()) {
		return -ENODEV;
	}
	return ret;
}
