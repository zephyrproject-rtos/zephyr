/* Copyright 2025 The ChromiumOS Authors
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/util.h>
#include <ksched.h>

/* The basic exception frame, popped by the hardware during return */
struct hw_frame_base {
	uint32_t r0, r1, r2, r3;
	uint32_t r12;
	uint32_t lr;
	uint32_t pc;
	uint32_t apsr;
};

/* The hardware frame pushed when entry is taken with FPU active */
struct hw_frame_fpu {
	struct hw_frame_base base;
	uint32_t s_regs[16];
	uint32_t fpscr;
	uint32_t reserved;
};

/* The hardware frame pushed when entry happens with a misaligned stack */
struct hw_frame_align {
	struct hw_frame_base base;
	uint32_t align_pad;
};

/* Both of the above */
struct hw_frame_align_fpu {
	struct hw_frame_fpu base;
	uint32_t align_pad;
};

/* Zephyr's synthesized frame used during context switch on interrupt
 * exit.  It's a minimal hardware frame plus storage for r4-11.
 */
struct synth_frame {
	uint32_t r7, r8, r9, r10, r11;
	uint32_t r4, r5, r6; /* these match switch format */
	struct hw_frame_base base;
};

/* Zephyr's custom frame used for suspended threads, not hw-compatible */
struct switch_frame {
#ifdef CONFIG_BUILTIN_STACK_GUARD
	uint32_t psplim;
#endif
	uint32_t apsr;
	uint32_t r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12;
	uint32_t lr;
	uint32_t pc;
};

/* Union of synth and switch frame, used during context switch */
union u_frame {
	struct {
		char pad[sizeof(struct switch_frame) - sizeof(struct synth_frame)];
		struct synth_frame hw;
	};
	struct switch_frame sw;
};

/* u_frame with have_fpu flag prepended (zero value), no FPU state */
struct z_frame {
#ifdef CONFIG_FPU_SHARING
	uint32_t have_fpu;
#endif
	union u_frame u;
};

/* u_frame + FPU data, with have_fpu (non-zero) */
struct z_frame_fpu {
	uint32_t have_fpu;
	uint32_t s_regs[32];
	uint32_t fpscr;
	union u_frame u;
};

/* Union of all possible stack frame formats, aligned at the top (!).
 * Note that FRAMESZ is constructed to be larger than any of them to
 * avoid having a zero-length array.  The code doesn't ever use the
 * size of this struct, it just wants to be have compiler-visible
 * offsets for in-place copies.
 */
/* clang-format off */
#define FRAMESZ (4 + MAX(sizeof(struct z_frame_fpu), sizeof(struct hw_frame_align_fpu)))
#define PAD(T)  char pad_##T[FRAMESZ - sizeof(struct T)]
union frame {
	struct { PAD(hw_frame_base);      struct hw_frame_base hw;          };
	struct { PAD(hw_frame_fpu);       struct hw_frame_fpu hwfp;         };
	struct { PAD(hw_frame_align);     struct hw_frame_align hw_a;       };
	struct { PAD(hw_frame_align_fpu); struct hw_frame_align_fpu hwfp_a; };
	struct { PAD(z_frame);            struct z_frame z;                 };
	struct { PAD(z_frame_fpu);        struct z_frame_fpu zfp;           };
};
/* clang-format on */

/* Validate the structs are correctly top-aligned */
#define FRAME_FIELD_END(F) ((void *)&(&(((union frame *)0)->F))[1])
BUILD_ASSERT(FRAME_FIELD_END(hw) == FRAME_FIELD_END(hwfp));
BUILD_ASSERT(FRAME_FIELD_END(hw) == FRAME_FIELD_END(hw_a));
BUILD_ASSERT(FRAME_FIELD_END(hw) == FRAME_FIELD_END(hwfp_a));
BUILD_ASSERT(FRAME_FIELD_END(hw) == FRAME_FIELD_END(z));
BUILD_ASSERT(FRAME_FIELD_END(hw) == FRAME_FIELD_END(zfp));

#ifdef CONFIG_FPU_SHARING
uint32_t arm_m_switch_stack_buffer = sizeof(struct z_frame_fpu) - sizeof(struct hw_frame_base);
#else
uint32_t arm_m_switch_stack_buffer = sizeof(struct z_frame) - sizeof(struct hw_frame_base);
#endif

/* Global pointers to the frame locations for the callee-saved
 * registers.  Set in arm_m_must_switch(), and used by the fixup
 * assembly in arm_m_exc_exit.
 */
struct {
	void *out, *in;
} arm_m_cs_ptrs;

#ifdef CONFIG_LTO
/* Toolchain workaround: when building with LTO, gcc seems unable to
 * notice the reference to arm_m_cs_ptrs in the assembly for
 * arm_m_exc_exit below, and drops the symbol before the final link.
 * Use this global to store a pointer in arm_m_new_stack(), wasting a
 * few bytes of code & data.
 */
void *arm_m_lto_dummy;
#endif

/* Unit test hook, unused in production */
void *arm_m_last_switch_handle;

/* Global holder for the location of the saved LR in the entry frame. */
uint32_t *arm_m_exc_lr_ptr;

/* Dummy used in arch_switch() when USERSPACE=y */
uint32_t arm_m_switch_control;

/* clang-format off */
/* Emits an in-place copy from a hw_frame_base to a switch_frame */
#define HW_TO_SWITCH(hw, sw) do {					\
	uint32_t r0 = hw.r0, r1 = hw.r1, r2 = hw.r2, r3 = hw.r3;        \
	uint32_t r12 = hw.r12, lr = hw.lr, pc = hw.pc, apsr = hw.apsr;  \
	pc |= 1; /* thumb bit! */                                       \
	sw.r0 = r0; sw.r1 = r1; sw.r2 = r2; sw.r3 = r3;                 \
	sw.r12 = r12; sw.lr = lr; sw.pc = pc; sw.apsr = apsr;           \
} while (false)

/* Emits an in-place copy from a switch_frame to a synth_frame */
#define SWITCH_TO_SYNTH(sw, syn) do {					\
	struct synth_frame syntmp = {                                   \
		.r4 = sw.r4, .r5 = sw.r5, .r6 = sw.r6, .r7 = sw.r7,     \
		.r8 = sw.r8, .r9 = sw.r9, .r10 = sw.r10, .r11 = sw.r11, \
		.base.r0 = sw.r0, .base.r1 = sw.r1, .base.r2 = sw.r2,   \
		.base.r3 = sw.r3, .base.r12 = sw.r12, .base.lr = sw.lr, \
		.base.pc = sw.pc, .base.apsr = sw.apsr,                 \
	};                                                              \
	syn = syntmp;                                                   \
} while (false)
/* clang-format on */

/* Reports if the passed return address is a valid EXC_RETURN (high
 * four bits set) that will restore to the PSP running in thread mode
 * (low four bits == 0xd).  That is an interrupted Zephyr thread
 * context.  For everything else, we just return directly via the
 * hardware-pushed stack frame with no special handling. See ARMv7M
 * manual B1.5.8.
 */
static bool arm_m_is_thread_return(uint32_t lr)
{
	return (lr & 0xf000000f) == 0xf000000d;
}

/* Returns true if the EXC_RETURN address indicates a FPU subframe was
 * pushed to the stack.  See ARMv7M manual B1.5.8.
 */
static bool arm_m_fpu_state_pushed(uint32_t lr)
{
	return IS_ENABLED(CONFIG_CPU_HAS_FPU) ? !!(lr & 0x08000000) : false;
}

#ifdef CONFIG_BUILTIN_STACK_GUARD
#define PSPLIM(f) ((f)->z.u.sw.psplim)
#else
#define PSPLIM(f) 0
#endif

/* Converts, in place, a pickled "switch" frame from a suspended
 * thread to a "synthesized" format that can be restored by the CPU
 * hardware on exception exit.
 */
static void *arm_m_switch_to_cpu(void *sp)
{
	union frame *f;
	uint32_t splim;

#ifdef CONFIG_FPU_SHARING
	/* When FPU switching is enabled, the suspended handle always
	 * points to the have_fpu word, which will be followed by FPU
	 * state if non-zero.
	 */
	bool have_fpu = (*(uint32_t *)sp) != 0;

	if (have_fpu) {
		f = CONTAINER_OF(sp, union frame, zfp.have_fpu);
		splim = PSPLIM(f);
		__asm__ volatile("vldm %0, {s0-s31}" ::"r"(&f->zfp.s_regs[0]));
		SWITCH_TO_SYNTH(f->zfp.u.sw, f->zfp.u.hw);
	} else {
		f = CONTAINER_OF(sp, union frame, z.have_fpu);
		splim = PSPLIM(f);
		SWITCH_TO_SYNTH(f->z.u.sw, f->zfp.u.hw);
	}
#else
	f = CONTAINER_OF(sp, union frame, z.u.sw);
	splim = PSPLIM(f);
	SWITCH_TO_SYNTH(f->z.u.sw, f->z.u.hw);
#endif

#ifdef CONFIG_BUILTIN_STACK_GUARD
	__asm__ volatile("msr psplim, %0" ::"r"(splim));
#endif

	/* Mark the callee-saved pointer for the fixup assembly.  Note
	 * funny layout that puts r7 first!
	 */
	arm_m_cs_ptrs.in = &f->z.u.hw.r7;

	return &f->z.u.hw.base;
}

static void fpu_cs_copy(struct hw_frame_fpu *src, struct z_frame_fpu *dst)
{
	for (int i = 0; IS_ENABLED(CONFIG_FPU_SHARING) && i < 16; i++) {
		dst->s_regs[i] = src->s_regs[i];
	}
}

/* Converts, in-place, a CPU-spilled ("hardware") exception entry
 * frame to our ("zephyr") switch handle format such that the thread
 * can be suspended
 */
static void *arm_m_cpu_to_switch(void *sp, bool fpu)
{
	union frame *f = NULL;
	struct hw_frame_base *base = sp;
	bool padded = (base->apsr & 0x200);
	uint32_t fpscr;

	if (fpu && IS_ENABLED(CONFIG_FPU_SHARING)) {
		uint32_t dummy = 0;

		/* Lazy FPU stacking is enabled, so before we touch
		 * the stack frame we have to tickle the FPU to force
		 * it to spill the caller-save registers.  Then clear
		 * CONTROL.FPCA which gets set again by that instruction.
		 */
		__asm__ volatile("vmov %0, s0;"
				 "mrs %0, control;"
				 "bic %0, %0, #4;"
				 "msr control, %0;" ::"r"(dummy));
	}

	if (IS_ENABLED(CONFIG_FPU_SHARING) && fpu) {
		fpscr = CONTAINER_OF(sp, struct hw_frame_fpu, base)->fpscr;
	}

	/* There are four (!) different offsets from the interrupted
	 * stack at which the hardware frame might be found at
	 * runtime.  These expansions let the compiler generate
	 * optimized in-place copies for each.  In practice it does a
	 * pretty good job, much better than a double-copy via an
	 * intermediate buffer.  Note that when FPU state is spilled
	 * we must copy the 16 spilled registers first, to make room
	 * for the copy.
	 */
	if (!fpu && !padded) {
		f = CONTAINER_OF(sp, union frame, hw.r0);
		HW_TO_SWITCH(f->hw, f->z.u.sw);
	} else if (!fpu && padded) {
		f = CONTAINER_OF(sp, union frame, hw_a.base.r0);
		HW_TO_SWITCH(f->hw_a.base, f->z.u.sw);
	} else if (fpu && !padded) {
		f = CONTAINER_OF(sp, union frame, hwfp.base.r0);
		fpu_cs_copy(&f->hwfp, &f->zfp);
		HW_TO_SWITCH(f->hwfp.base, f->z.u.sw);
	} else if (fpu && padded) {
		f = CONTAINER_OF(sp, union frame, hwfp_a.base.base.r0);
		fpu_cs_copy(&f->hwfp_a.base, &f->zfp);
		HW_TO_SWITCH(f->hwfp_a.base.base, f->z.u.sw);
	}

#ifdef CONFIG_BUILTIN_STACK_GUARD
	__asm__ volatile("mrs %0, psplim" : "=r"(f->z.u.sw.psplim));
#endif

	/* Mark the callee-saved pointer for the fixup assembly */
	arm_m_cs_ptrs.out = &f->z.u.sw.r4;

#ifdef CONFIG_FPU_SHARING
	if (fpu) {
		__asm__ volatile("vstm %0, {s16-s31}" ::"r"(&f->zfp.s_regs[16]));
		f->zfp.fpscr = fpscr;
		f->zfp.have_fpu = true;
		return &f->zfp.have_fpu;
	}
	f->z.have_fpu = false;
	return &f->z.have_fpu;
#else
	return &f->z.u.sw;
#endif
}

void *arm_m_new_stack(char *base, uint32_t sz, void *entry, void *arg0, void *arg1, void *arg2,
		      void *arg3)
{
	struct switch_frame *sw;
	uint32_t baddr;

#ifdef CONFIG_LTO
	arm_m_lto_dummy = &arm_m_cs_ptrs;
#endif

	/* Kludgey global initialization, put a computed pointer to
	 * the LR frame location into this variable for use by
	 * arm_m_exc_tail().  Should move to arch init somewhere once
	 * arch_switch is better integrated
	 */
	char *stack = (char *)K_KERNEL_STACK_BUFFER(z_interrupt_stacks[0]);
	uint32_t *s_top = (uint32_t *)(stack + K_KERNEL_STACK_SIZEOF(z_interrupt_stacks[0]));

	arm_m_exc_lr_ptr = &s_top[-1];

	baddr = ((uint32_t)base + 7) & ~7;
	sz = ((uint32_t)(base + sz) - baddr) & ~7;

	if (sz < sizeof(struct switch_frame)) {
		return NULL;
	}

	/* Note: a useful trick here would be to initialize LR to
	 * point to cleanup code, avoiding the need for the
	 * z_thread_entry wrapper, saving a few words of stack frame
	 * and a few cycles on thread entry.
	 */
	sw = (void *)(baddr + sz - sizeof(*sw));
	*sw = (struct switch_frame){
		IF_ENABLED(CONFIG_BUILTIN_STACK_GUARD, (.psplim = baddr,)) .r0 = (uint32_t)arg0,
			   .r1 = (uint32_t)arg1, .r2 = (uint32_t)arg2, .r3 = (uint32_t)arg3,
			   .pc = ((uint32_t)entry) | 1, /* set thumb bit! */
			   .apsr = 0x1000000,           /* thumb bit here too! */
		};

	if (IS_ENABLED(CONFIG_FPU_SHARING)) {
		return CONTAINER_OF(sw, struct z_frame, u.sw);
	}
	return sw;
}

bool arm_m_must_switch(uint32_t lr)
{
#if defined(CONFIG_USE_SWITCH) && defined(CONFIG_MULTITHREADING)
	struct k_thread *last_thread = _current;
#endif

	if (!arm_m_is_thread_return(lr)) {
		return false;
	}

	void *last, *next = z_get_next_switch_handle(NULL);

	if (next == NULL) {
		return false;
	}

	bool fpu = arm_m_fpu_state_pushed(lr);

	__asm__ volatile("mrs %0, psp" : "=r"(last));
	last = arm_m_cpu_to_switch(last, fpu);
	next = arm_m_switch_to_cpu(next);
	__asm__ volatile("msr psp, %0" ::"r"(next));

#if !defined(CONFIG_MULTITHREADING)
	arm_m_last_switch_handle = last;
#elif defined(CONFIG_USE_SWITCH)
	last_thread->switch_handle = last;
#endif

#ifdef CONFIG_USERSPACE
	uint32_t control;

	__asm__ volatile("mrs %0, control" : "=r"(control));
	last_thread->arch.mode &= (~1) | (control & 1);
	control = (control & ~1) | (_current->arch.mode & 1);
	__asm__ volatile("msr control, %0" ::"r"(control));
#endif

#if defined(CONFIG_USERSPACE) || defined(CONFIG_MPU_STACK_GUARD)
	z_arm_configure_dynamic_mpu_regions(_current);
#endif

#ifdef CONFIG_THREAD_LOCAL_STORAGE
	z_arm_tls_ptr = _current->tls;
#endif
	return true;
}

/* This is handled an inline now for C code, but there are a few spots
 * that need to get to it from assembly (but which IMHO should really
 * be ported to C)
 */
void arm_m_legacy_exit(void)
{
	arm_m_exc_tail();
}

/* We arrive here on "return" from exception handlers on a context
 * switch. Our job is to save the interrupted r4-r11 of the outgoing
 * thread (which can't be done in the C function context because we
 * can't control the compiler spill behavior of callee-saved
 * registers), restore the same registers from the incoming thread,
 * and then return via BX to a constructed exception return value.  We
 * can use any of r0-r3/r12/lr because they have already been saved.
 * FPU restore is handled in software, so we always use a constant
 * EXC_RETURN value indicating an integer-only restore.
 */
__asm__(".globl arm_m_exc_exit;"
	"arm_m_exc_exit:;"
	"  ldr r0, =arm_m_cs_ptrs;"
	"  ldm r0, {r0, r1};" /* fields: out, in */
	"  mov lr, #0xfffffffd;"
	"  stm r0, {r4-r11};"
	"  ldm r1!, {r7-r11};"
	"  ldm r1, {r4-r6};"
	"  bx lr;");
