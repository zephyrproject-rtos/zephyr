/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "xuk-config.h"
#include "x86_64-hw.h"
#include "xuk.h"
#include "serial.h"

#ifdef CONFIG_XUK_DEBUG
#include "vgacon.h"
#include "printf.h"
#else
#define printf(...)
#endif

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

/* Defined at the linker level in xuk-stubs.c */
extern char _xuk_stub16_start, _xuk_stub16_end;

/* 64 bit entry point.  Lives immediately after the 32 bit stub.
 * Expects to have its stack already set up.
 */
__asm__(".pushsection .xuk_start64\n"
	".align 16\n"
	"    jmp _cstart64\n"
	".popsection\n");

/* Interrupt/exception entry points stored in the IDT.
 *
 * FIXME: the assembly below uses XCHG r/m, because I'm lazy and this
 * was SO much easier than hand coding the musical chairs required to
 * emulate it.  But that instruction is outrageously slow (like 20+
 * cycle latency on most CPUs!), and this is interrupt entry.
 * Replace, once we have a test available to detect bad register
 * contents
 */
extern char _isr_entry_err, _isr_entry_noerr;
__asm__(/* Exceptions that push an error code arrive here. */
	".align 16\n"
	"_isr_entry_err:\n"
	"    xchg %rdx, (%rsp)\n"
	"    jmp _isr_entry2\n"

	/* IRQs with no error code land here, then fall through */
	".align 16\n"
	"_isr_entry_noerr:\n"
	"    push %rdx\n"

	/* Arrive here with RDX already pushed to the stack below the
	 * interrupt frame and (if needed) populated with the error
	 * code from the exception.  It will become the third argument
	 * to the C handler.  Stuff the return address from the call
	 * in the stub table into RDI (the first argument).
	 */
	"_isr_entry2:\n"
	"    xchg %rdi, 8(%rsp)\n"
	"    push %rax\n"
	"    push %rcx\n"
	"    push %rsi\n"
	"    push %r8\n"
	"    push %r9\n"
	"    push %r10\n"
	"    push %r11\n"
	"    mov %rsp, %rsi\n" /* stack in second arg */
	"    call _isr_c_top\n"

	/* We have pushed only the caller-save registers at this
	 * point.  Check return value to see if we are returning back
	 * into the same context or if we need to do a full dump and
	 * restore.
	 */
	"    test %rax, %rax\n"
	"    jnz _switch_bottom\n"
	"    pop %r11\n"
	"    pop %r10\n"
	"    pop %r9\n"
	"    pop %r8\n"
	"    pop %rsi\n"
	"    pop %rcx\n"
	"    pop %rax\n"
	"    pop %rdx\n"
	"    pop %rdi\n"
	"    iretq\n");

/* Top half of a context switch.  Arrive here with the "CPU pushed"
 * part of the exception frame (SS, RSP, RFLAGS, CS, RIP) already on
 * the stack, the context pointer to which to switch stored in RAX and
 * a pointer into which to store the current context in RDX (NOTE:
 * this will be a pointer to a 32 bit memory location if we are in x32
 * mode!).  It will push the first half of the register set (the same
 * caller-save registers pushed by an ISR) and then continue on to
 * _switch_bottom to finish up.
 */
__asm__(".align 16\n"
	".global _switch_top\n"
	"_switch_top:\n"
	"    push %rdi\n"
	"    push %rdx\n"
	"    push %rax\n"
	"    push %rcx\n"
	"    push %rsi\n"
	"    push %r8\n"
	"    push %r9\n"
	"    push %r10\n"
	"    push %r11\n"
	"    mov %rsp, %r8\n"
	"    sub $48, %r8\n"
#ifdef CONFIG_XUK_64_BIT_ABI
	"    movq %r8, (%rdx)\n"
#else
	"    movl %r8d, (%rdx)\n"
#endif
	/* Fall through... */
	/* Bottom half of a switch, used by both ISR return and
	 * context switching.  Arrive here with the exception frame
	 * and caller-saved registers already on the stack and the
	 * stack pointer to use for the restore in RAX.  It will push
	 * the remaining registers and then restore.
	 */
	".align 16\n"
	"_switch_bottom:\n"
	"    push %rbx\n"
	"    push %rbp\n"
	"    push %r12\n"
	"    push %r13\n"
	"    push %r14\n"
	"    push %r15\n"
	"    mov %rax, %rsp\n"
	"    pop %r15\n"
	"    pop %r14\n"
	"    pop %r13\n"
	"    pop %r12\n"
	"    pop %rbp\n"
	"    pop %rbx\n"
	"    pop %r11\n"
	"    pop %r10\n"
	"    pop %r9\n"
	"    pop %r8\n"
	"    pop %rsi\n"
	"    pop %rcx\n"
	"    pop %rax\n"
	"    pop %rdx\n"
	"    pop %rdi\n"
	"    iretq\n");

static unsigned int isr_stub_base;

struct vhandler {
	void (*fn)(void*, int);
	void *arg;
};

static struct vhandler *vector_handlers;

static void putchar(int c)
{
	serial_putc(c);
#ifdef XUK_DEBUG
	vgacon_putc(c);
#endif
}

long _isr_c_top(unsigned long vecret, unsigned long rsp,
		unsigned long err)
{
	/* The vector stubs are 8-byte-aligned, so to get the vector
	 * index from the return address we just shift off the bottom
	 * bits
	 */
	int vector = (vecret - isr_stub_base) >> 3;
	struct vhandler *h = &vector_handlers[vector];
	struct xuk_entry_frame *frame = (void *)rsp;

	_isr_entry();

	/* Set current priority in CR8 to the currently-serviced IRQ
	 * and re-enable interrupts
	 */
	unsigned long long cr8, cr8new = vector >> 4;

	__asm__ volatile("movq %%cr8, %0;"
			 "movq %1, %%cr8;"
			 "sti"
			 : "=r"(cr8) : "r"(cr8new));

	if (h->fn) {
		h->fn(h->arg, err);
	} else {
		_unhandled_vector(vector, err, frame);
	}

	/* Mask interrupts to finish processing (they'll get restored
	 * in the upcoming IRET) and restore CR8
	 */
	__asm__ volatile("cli; movq %0, %%cr8" : : "r"(cr8));

	/* Signal EOI if it's an APIC-managed interrupt */
	if (vector > 0x1f) {
		_apic.EOI = 0;
	}

	/* Subtle: for the "interrupted context pointer", we pass in
	 * the value our stack pointer WILL have once we finish
	 * spilling registers after this function returns.  If this
	 * hook doesn't want to switch, it will return null and never
	 * save the value of the pointer.
	 */
	return (long)_isr_exit_restore_stack((void *)(rsp - 48));
}

static long choose_isr_entry(int vector)
{
	/* Constructed with 1's in the vector indexes defined to
	 * generate an error code.  Couldn't find a clean way to make
	 * the compiler generate this code
	 */
	const int mask = 0x27d00; /* 0b00100111110100000000 */

	if (vector < 32 && ((1 << vector) & mask)) {
		return (long)&_isr_entry_err;
	} else {
		return (long)&_isr_entry_noerr;
	}
}

void xuk_set_isr(int interrupt, int priority,
		 void (*handler)(void *, int), void *arg)
{
	int v = interrupt - 0x100;

	/* Need to choose a vector number?  Try all vectors at the
	 * specified priority.  Clobber one if we have to.
	 */
	if (interrupt < 0x100 || interrupt > 0x1ff) {
		for (int pi = 0; pi <= 0xf; pi++) {
			v = (priority << 4) | pi;
			if (!vector_handlers[v].fn) {
				break;
			}
		}
	}

	/* Need to set up IO-APIC?  Set it up to deliver to all CPUs
	 * here (another API later will probably allow for IRQ
	 * affinity).  Do a read/write cycle to avoid clobbering
	 * settings like edge triggering & polarity that might have
	 * been set up by other platform layers.  We only want to muck
	 * with routing.
	 */
	if (interrupt < 0x100) {
		struct ioapic_red red;
		int regidx = 0x10 + 2 * interrupt;

		red.regvals[0] = ioapic_read(regidx);
		red.regvals[1] = ioapic_read(regidx + 1);
		red.vector = v;
		red.logical = 0;
		red.destination = 0xff;
		red.masked = 1;
		ioapic_write(regidx, red.regvals[0]);
		ioapic_write(regidx + 1, red.regvals[1]);
	}

	/* Is it a special interrupt? */
	if (interrupt == INT_APIC_LVT_TIMER) {
		struct apic_lvt lvt = {
			.vector = v,
			.mode = ONESHOT,
		};

		_apic.LVT_TIMER = lvt;
	}

	printf("set_isr v %d\n", v);

	vector_handlers[v].fn = handler;
	vector_handlers[v].arg = arg;
}

/* Note: "raw vector" interrupt numbers cannot be masked, as the APIC
 * doesn't have a per-vector mask bit.  Only specific LVT interrupts
 * (we handle timer below) and IOAPIC-generated interrupts can be
 * masked on x86.  In practice, this isn't a problem as that API is a
 * special-purpose kind of thing.  Real devices will always go through
 * the supported channel.
 */
void xuk_set_isr_mask(int interrupt, int masked)
{
	if (interrupt == INT_APIC_LVT_TIMER) {
		struct apic_lvt lvt = _apic.LVT_TIMER;

		lvt.masked = masked;
		_apic.LVT_TIMER = lvt;
	} else if (interrupt < 0x100) {
		struct ioapic_red red;
		int regidx = 0x10 + 2 * interrupt;

		red.regvals[0] = ioapic_read(regidx);
		red.regvals[1] = ioapic_read(regidx + 1);
		red.masked = masked;
		ioapic_write(regidx, red.regvals[0]);
		ioapic_write(regidx + 1, red.regvals[1]);
	}
}

/* Note: these base pointers live together in a big block.  Eventually
 * we will probably want one of them for userspace TLS, which means it
 * will need to be retargetted to point somewhere within the
 * application memory.  But this is fine for now.
 */
static void setup_fg_segs(int cpu)
{
	int fi = 3 + 2 * cpu, gi = 3 + 2 * cpu + 1;
	struct gdt64 *fs = &_shared.gdt[fi];
	struct gdt64 *gs = &_shared.gdt[gi];

	gdt64_set_base(fs, (long)&_shared.fs_ptrs[cpu]);
	gdt64_set_base(gs, (long)&_shared.gs_ptrs[cpu]);

	int fsel = GDT_SELECTOR(fi), gsel = GDT_SELECTOR(gi);

	__asm__("mov %0, %%fs; mov %1, %%gs" : : "r"(fsel), "r"(gsel));
}

static void init_gdt(void)
{
	printf("Initializing 64 bit IDT\n");

	/* Need a GDT for ourselves, not whatever the previous layer
	 * set up.  The scheme is that segment zero is the null
	 * segment (required and enforced architecturally), segment
	 * one (selector 8) is the code segment, two (16) is a
	 * data/stack segment (ignored by code at runtime, but
	 * required to be present in the L/GDT when executing an
	 * IRET), and remaining segments come in pairs to provide
	 * FS/GS segment bases for each CPU.
	 */
	_shared.gdt[0] = (struct gdt64) {};
	_shared.gdt[1] = (struct gdt64) {
		.readable = 1,
		.codeseg = 1,
		.notsystem = 1,
		.present = 1,
		.long64 = 1,
	};
	_shared.gdt[2] = (struct gdt64) {
		.readable = 1,
		.codeseg = 0,
		.notsystem = 1,
		.present = 1,
		.long64 = 1,
	};
	for (int i = 3; i < ARRAY_SIZE(_shared.gdt); i++) {
		_shared.gdt[i] = (struct gdt64) {
			.readable = 1,
			.codeseg = 0,
			.notsystem = 1,
			.present = 1,
			.long64 = 1,
		};
	}
}

static void init_idt(void)
{
	printf("Initializing 64 bit IDT\n");

	/* Make an IDT in the next unused page and fill in all 256
	 * entries
	 */
	struct idt64 *idt = alloc_page(0);

	_shared.idt_addr = (unsigned int)(long)idt;
	for (int i = 0; i < 256; i++) {
		idt[i] = (struct idt64) {
			.segment = GDT_SELECTOR(1),
			.type = 14, /* == 64 bit interrupt gate */
			.present = 1,
		};
	}

	/* Hand-encode stubs for each vector that are a simple 5-byte
	 * CALL instruction to the single handler entry point.  That's
	 * an opcode of 0xe8 followd by a 4-byte offset from the start
	 * of the next (!) instruction.  The call is used to push a
	 * return address on the stack that points into the stub,
	 * allowing us to extract the vector index by what stub it
	 * points into.
	 */
	struct istub {
		unsigned char opcode; /* 0xe8 == CALLQ */
		int off;
		unsigned char _unused[3];
	} __attribute__((packed)) *stubtab = alloc_page(0);

	isr_stub_base = (long)stubtab;

	/* FIXME: on x32, the entries in this handlers table are half
	 * the size as a native 64 bit build, and could be packed into
	 * the same page as the stubs above, saving the page of low
	 * memory.
	 */
	vector_handlers = alloc_page(1);

	for (int i = 0; i < 256; i++) {
		struct istub *st = &stubtab[i];

		st->opcode = 0xe8;
		st->off = choose_isr_entry(i) - (long)st - 5;
		idt64_set_isr(&idt[i], st);
	}
}

static void smp_init(void)
{
	/* Generate a GDT for the 16 bit stub to use when
	 * transitioning to 32 bit protected mode (so the poor thing
	 * doesn't have to do it itself).  It can live right here on
	 * our stack.
	 */
	struct gdt64 gdt16[] = {
		{},
		{
			.codeseg = 1,
			.default_size = 1,
			.readable = 1,
			.notsystem = 1,
			.present = 1,
			.limit_lo16 = 0xffff,
			.limit_hi4 = 0xf,
			.page_granularity = 1,
		},
		{
			.readable = 1,
			.default_size = 1,
			.notsystem = 1,
			.present = 1,
			.limit_lo16 = 0xffff,
			.limit_hi4 = 0xf,
			.page_granularity = 1,
		},
	};
	struct {
		short dummy;
		short limit;
		unsigned int addr;
	} gdtp16 = { .limit = sizeof(gdt16), .addr = (long)&gdt16[0] };
	_shared.gdt16_addr = (long)&gdtp16.limit;

	/* FIXME: this is only used at startup, and only for a ~150
	 * byte chunk of code.  Find a way to return it, or maybe put
	 * it in the low memory null guard instead?
	 */
	char *sipi_page = alloc_page(1);

	int s16bytes = &_xuk_stub16_end - &_xuk_stub16_start;

	printf("Copying %d bytes of 16 bit code into page %p\n",
	       s16bytes, (int)(long)sipi_page);
	for (int i = 0; i < s16bytes; i++) {
		sipi_page[i] = ((char *)&_xuk_stub16_start)[i];
	}

	/* First send an INIT interrupt to all CPUs.  This resets them
	 * regardless of whatever they were doing and they enter a
	 * "wait for SIPI" state
	 */
	printf("Sending INIT IPI\n");
	_apic.ICR_LO = (struct apic_icr_lo) {
		.delivery_mode = INIT,
		.shorthand = NOTSELF,
	};
	while (_apic.ICR_LO.send_pending) {
	}

	/* Now send the startup IPI (SIPI) to all CPUs.  They will
	 * begin executing in real mode with IP=0 and CS pointing to
	 * the page we allocated.
	 */
	_shared.smpinit_lock = 0;
	_shared.smpinit_stack = 0;
	_shared.num_active_cpus = 1;

	printf("Sending SIPI IPI\n");
	_apic.ICR_LO = (struct apic_icr_lo) {
		.delivery_mode = STARTUP,
		.shorthand = NOTSELF,
		.vector = ((long)sipi_page) >> 12,
	};
	while (_apic.ICR_LO.send_pending) {
	}

	for (int i = 1; i < CONFIG_MP_NUM_CPUS; i++) {
		_shared.smpinit_stack = _init_cpu_stack(i);
		printf("Granting stack @ %xh to CPU %d\n",
		       _shared.smpinit_stack, i);

		while (_shared.num_active_cpus <= i) {
			__asm__("pause");
		}
	}
}

void _cstart64(int cpu_id)
{
	if (cpu_id == 0) {
		extern char __bss_start, __bss_end;

		__builtin_memset(&__bss_start, 0, &__bss_end - &__bss_start);
	}

#ifdef CONFIG_XUK_DEBUG
	_putchar = putchar;
#endif
	printf("\n==\nHello from 64 bit C code on CPU%d (stack ~%xh)\n",
	       cpu_id, (int)(long)&cpu_id);
	printf("sizeof(int) = %d, sizeof(long) = %d, sizeof(void*) = %d\n",
	       sizeof(int), sizeof(long), sizeof(void *));

	if (cpu_id == 0) {
		init_gdt();
	}

	struct {
		unsigned short dummy[3];
		unsigned short limit;
		unsigned long long addr;
	} gdtp = { .limit = sizeof(_shared.gdt), .addr = (long)_shared.gdt };

	printf("Loading 64 bit GDT\n");
	__asm__ volatile("lgdt %0" : : "m"(gdtp.limit));

	/* Need to actually set the data & stack segments with those
	 * indexes.  Whatever we have in those hidden registers works
	 * for data access *now*, but the next interrupt will push
	 * whatever the selector index was, and we need to know that
	 * our table contains the same layout!
	 */
	int selector = GDT_SELECTOR(2);

	__asm__ volatile("mov %0, %%ds; mov %0, %%ss" : : "r"(selector));

	if (cpu_id == 0) {
		init_idt();
	}

	struct {
		unsigned short dummy[3];
		unsigned short limit;
		unsigned long long addr;
	} idtp = { .limit = 4096, .addr = _shared.idt_addr };

	printf("Loading IDT lim %d addr %xh\n", idtp.limit, idtp.addr);
	__asm__ volatile("lidt %0" : : "m"(idtp.limit));

	/* Classic PC architecture gotcha: disable 8259 PICs before
	 * they fires a timer interrupt into our exception table.
	 * Write 1's into the interrupt masks.
	 */
	if (cpu_id == 0) {
		printf("Disabling 8259 PICs\n");
		ioport_out8(0xa1, 0xff); /* slave  */
		ioport_out8(0x21, 0xff); /* master */
	}

	/* Enable APIC.  Set both the MSR bit and the "software
	 * enable" bit in the spurious interrupt vector register.
	 */
	const unsigned int IA32_APIC_BASE = 0x1b;

	printf("Enabling APIC id %xh ver %xh\n", _apic.ID, _apic.VER);
	set_msr_bit(IA32_APIC_BASE, 11);
	_apic.SPURIOUS |= 1<<8;
	_apic.LDR = cpu_id << 24;
	_apic.DIVIDE_CONF = APIC_DIVISOR(CONFIG_XUK_APIC_TSC_SHIFT);

	printf("Initializing FS/GS segments for local CPU%d\n", cpu_id);
	setup_fg_segs(cpu_id);

	if (cpu_id == 0) {
		printf("Brining up auxiliary CPUs...\n");
		smp_init();
	}

	printf("Calling _cpu_start on CPU %d\n", cpu_id);
	_cpu_start(cpu_id);
}

long xuk_setup_stack(long sp, void *fn, unsigned int eflags,
		     long *args, int nargs)
{
	long long *f = (long long *)(sp & ~7) - 20;

	/* FIXME: this should extend naturally to setting up usermode
	 * frames too: the frame should have a SS and RSP at the top
	 * that specifies the user stack into which to return (can be
	 * this same stack as long as the mapping is correct), and the
	 * CS should be a separate ring 3 segment.
	 */

	f[19] = GDT_SELECTOR(2);
	f[18] = sp;
	f[17] = eflags;
	f[16] = GDT_SELECTOR(1);
	f[15] = (long)fn;
	f[14] = nargs >= 1 ? args[0] : 0; /* RDI */
	f[13] = nargs >= 3 ? args[2] : 0; /* RDX */
	f[12] = 0;                        /* RAX */
	f[11] = nargs >= 4 ? args[3] : 0; /* RCX */
	f[10] = nargs >= 2 ? args[1] : 0; /* RSI */
	f[9]  = nargs >= 5 ? args[4] : 0; /* R8 */
	f[8]  = nargs >= 6 ? args[5] : 0; /* R9 */

	/* R10, R11, RBX, RBP, R12, R13, R14, R15 */
	for (int i = 7; i >= 0; i--) {
		f[i] = 0;
	}

	return (long)f;
}

int z_arch_printk_char_out(int c)
{
	putchar(c);
	return 0;
}
