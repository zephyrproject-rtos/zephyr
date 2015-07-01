/* static_idt.c - test static IDT APIs */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
Ensures interrupt and exception stubs are installed correctly.
 */

#include <tc_util.h>

#include <nanokernel.h>
#include <arch/cpu.h>
#include <nano_private.h>
#if defined(__GNUC__)
#include <test_asm_inline_gcc.h>
#else
#include <test_asm_inline_other.h>
#endif

#ifdef CONFIG_MICROKERNEL
#include <zephyr.h>
#endif

/* These vectors are somewhat arbitrary. We try and use unused vectors */
#define TEST_SOFT_INT 62
#define TEST_SPUR_INT 63

/* externs */

 /* the _idt_base_address symbol is generated via a linker script */

extern unsigned char _idt_base_address[];

extern void *nanoIntStub;
extern void *exc_divide_error_handlerStub;

NANO_CPU_INT_REGISTER(nanoIntStub, TEST_SOFT_INT, 0);

static volatile int    excHandlerExecuted;
static volatile int    intHandlerExecuted;
/* Assume the spurious interrupt handler will execute and abort the task/fiber */
static volatile int    spurHandlerAbortedContext = 1;

#ifdef CONFIG_NANOKERNEL
static char __stack fiberStack[512];
#endif


/**
 *
 *  isr_handler - handler to perform various actions from within an ISR context
 *
 * This routine is the ISR handler for _trigger_isrHandler().
 *
 * @return N/A
 */

void isr_handler(void)
{
	intHandlerExecuted++;
}

/**
 *
 * exc_divide_error_handler -
 *
 * This is the handler for the divde by zero exception.  The source of this
 * divide-by-zero error comes from the following line in main() ...
 *         error = error / excHandlerExecuted;
 * Where excHandlerExecuted is zero.
 * The disassembled code for it looks something like ....
 *         f7 fb                   idiv   %ecx
 * This handler is part of a test that is only interested in detecting the
 * error so that we know the exception connect code is working.  Therefore,
 * a very quick and dirty approach is taken for dealing with the exception;
 * we skip the offending instruction by adding 2 to the EIP.  (If nothing is
 * done, then control goes back to the offending instruction and an infinite
 * loop of divide-by-zero errors would be created.)
 *
 * @return N/A
 */

void exc_divide_error_handler(NANO_ESF *pEsf)
{
	pEsf->eip += 2;
	excHandlerExecuted = 1;    /* provide evidence that the handler executed */
}


/**
 *
 * @brief Check the IDT.
 *
 * This test examines the IDT and verifies that the static interrupt and
 * exception stubs are installed at the correct place.
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int nanoIdtStubTest(void)
{
	IDT_ENTRY *pIdtEntry;
	uint16_t offset;

	/* Check for the interrupt stub */
	pIdtEntry = (IDT_ENTRY *) (_idt_base_address + (TEST_SOFT_INT << 3));

	offset = (uint16_t)((uint32_t)(&nanoIntStub) & 0xFFFF);
	if (pIdtEntry->lowOffset != offset) {
		TC_ERROR("Failed to find low offset of nanoIntStub "
				 "(0x%x) at vector %d\n", (uint32_t)offset, TEST_SOFT_INT);
		return TC_FAIL;
	}

	offset = (uint16_t)((uint32_t)(&nanoIntStub) >> 16);
	if (pIdtEntry->hiOffset != offset) {
		TC_ERROR("Failed to find high offset of nanoIntStub "
				 "(0x%x) at vector %d\n", (uint32_t)offset, TEST_SOFT_INT);
		return TC_FAIL;
	}

	/* Check for the exception stub */
	pIdtEntry = (IDT_ENTRY *) (_idt_base_address + (IV_DIVIDE_ERROR << 3));

	offset = (uint16_t)((uint32_t)(&exc_divide_error_handlerStub) & 0xFFFF);
	if (pIdtEntry->lowOffset != offset) {
		TC_ERROR("Failed to find low offset of exc stub "
				 "(0x%x) at vector %d\n", (uint32_t)offset, IV_DIVIDE_ERROR);
		return TC_FAIL;
	}

	offset = (uint16_t)((uint32_t)(&exc_divide_error_handlerStub) >> 16);
	if (pIdtEntry->hiOffset != offset) {
		TC_ERROR("Failed to find high offset of exc stub "
				 "(0x%x) at vector %d\n", (uint32_t)offset, IV_DIVIDE_ERROR);
		return TC_FAIL;
	}

	/*
	 * If the other fields are wrong, the system will crash when the exception
	 * and software interrupt are triggered so we don't check them.
	 */
	return TC_PASS;
}

/**
 *
 * @brief Task/fiber to test spurious handlers
 *
 * @return 0
 */

#ifdef CONFIG_MICROKERNEL
void idtSpurTask(void)
#else
static void idtSpurFiber(int a1, int a2)
#endif
{
#ifndef CONFIG_MICROKERNEL
	ARG_UNUSED(a1);
	ARG_UNUSED(a2);
#endif /* !CONFIG_MICROKERNEL */

	TC_PRINT("- Expect to see unhandled interrupt/exception message\n");

	_trigger_spurHandler();

	/* Shouldn't get here */
	spurHandlerAbortedContext = 0;

}

/**
 *
 * @brief Entry point to static IDT tests
 *
 * This is the entry point to the static IDT tests.
 *
 * @return N/A
 */

#ifdef CONFIG_MICROKERNEL
void idtTestTask(void)
#else
void main(void)
#endif
{
	int           rv;       /* return value from tests */
	volatile int  error;    /* used to create a divide by zero error */

	TC_START("Test Nanokernel static IDT tests");


	TC_PRINT("Testing to see if IDT has address of test stubs()\n");
	rv = nanoIdtStubTest();
	if (rv != TC_PASS) {
		goto doneTests;
	}

	TC_PRINT("Testing to see interrupt handler executes properly\n");
	_trigger_isrHandler();

	if (intHandlerExecuted == 0) {
		TC_ERROR("Interrupt handler did not execute\n");
		rv = TC_FAIL;
		goto doneTests;
	} else if (intHandlerExecuted != 1) {
		TC_ERROR("Interrupt handler executed more than once! (%d)\n",
				 intHandlerExecuted);
		rv = TC_FAIL;
		goto doneTests;
	}

	TC_PRINT("Testing to see exception handler executes properly\n");

	/*
	 * Use excHandlerExecuted instead of 0 to prevent the compiler issuing a
	 * 'divide by zero' warning.
	 */
	error = error / excHandlerExecuted;

	if (excHandlerExecuted == 0) {
		TC_ERROR("Exception handler did not execute\n");
		rv = TC_FAIL;
		goto doneTests;
	} else if (excHandlerExecuted != 1) {
		TC_ERROR("Exception handler executed more than once! (%d)\n",
				 excHandlerExecuted);
		rv = TC_FAIL;
		goto doneTests;
	}

	/*
	 * Start fiber/task to trigger the spurious interrupt handler
	 */
	TC_PRINT("Testing to see spurious handler executes properly\n");
#ifdef CONFIG_MICROKERNEL
	task_start(tSpurTask);
#else
	task_fiber_start(fiberStack, sizeof(fiberStack), idtSpurFiber, 0, 0, 5, 0);
#endif
	/*
	 * The fiber/task should not run past where the spurious interrupt is
	 * generated. Therefore spurHandlerAbortedContext should remain at 1.
	 */
	if (spurHandlerAbortedContext == 0) {
		TC_ERROR("Spurious handler did not execute as expected\n");
		rv = TC_FAIL;
		goto doneTests;
	}

doneTests:
	TC_END(rv, "%s - %s.\n", rv == TC_PASS ? PASS : FAIL, __func__);
	TC_END_REPORT(rv);
}
