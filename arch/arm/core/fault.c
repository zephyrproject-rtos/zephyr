/* fault.c - common fault handler for ARM Cortex-M */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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
Common fault handler for ARM Cortex-M processors.
*/

#include <toolchain.h>
#include <sections.h>

#include <nanokernel.h>
#include <nano_private.h>

#ifdef CONFIG_PRINTK
#include <misc/printk.h>
#define PR_EXC(...) printk(__VA_ARGS__)
#else
#define PR_EXC(...)
#endif /* CONFIG_PRINTK */

#if (CONFIG_FAULT_DUMP > 0)
#define FAULT_DUMP(esf, fault) _FaultDump(esf, fault)
#else
#define FAULT_DUMP(esf, fault) \
	do {                   \
		(void) esf;    \
		(void) fault;  \
	} while ((0))
#endif

#if (CONFIG_FAULT_DUMP == 1)
/*******************************************************************************
*
* _FaultDump - dump information regarding fault (FAULT_DUMP == 1)
*
* Dump information regarding the fault when CONFIG_FAULT_DUMP is set to 1
* (short form).
*
* eg. (precise bus error escalated to hard fault):
*
* Fault! EXC #3, Thread: 0x200000dc, instr: 0x000011d3
* HARD FAULT: Escalation (see below)!
* MMFSR: 0x00000000, BFSR: 0x00000082, UFSR: 0x00000000
* BFAR: 0xff001234
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void _FaultDump(const NANO_ESF *esf, int fault)
{
	int escalation = 0;

	PR_EXC("Fault! EXC #%d, Thread: %x, instr @ %x\n",
	       fault,
	       context_self_get(),
	       esf->pc);

	if (3 == fault) { /* hard fault */
		escalation = _ScbHardFaultIsForced();
		PR_EXC("HARD FAULT: %s\n",
		       escalation ? "Escalation (see below)!"
				  : "Bus fault on vector table read\n");
	}

	PR_EXC("MMFSR: %x, BFSR: %x, UFSR: %x\n",
	       __scs.scb.cfsr.byte.mmfsr.val,
	       __scs.scb.cfsr.byte.bfsr.val,
	       __scs.scb.cfsr.byte.ufsr.val);

	if (_ScbMemFaultIsMmfarValid()) {
		PR_EXC("MMFAR: %x\n", _ScbMemFaultAddrGet());
		if (escalation) {
			_ScbMemFaultMmfarReset();
		}
	}
	if (_ScbBusFaultIsBfarValid()) {
		PR_EXC("BFAR: %x\n", _ScbBusFaultAddrGet());
		if (escalation) {
			_ScbBusFaultBfarReset();
		}
	}

	/* clear USFR sticky bits */
	_ScbUsageFaultAllFaultsReset();
}
#endif

#if (CONFIG_FAULT_DUMP == 2)
/*******************************************************************************
*
* _FaultContextShow - dump context information
*
* See _FaultDump() for example.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

static void _FaultContextShow(const NANO_ESF *esf)
{
	PR_EXC("  Executing context ID (thread): 0x%x\n"
	       "  Faulting instruction address:  0x%x\n",
	       context_self_get(),
	       esf->pc);
}

/*******************************************************************************
*
* _MpuFault - dump MPU fault information
*
* See _FaultDump() for example.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

static void _MpuFault(const NANO_ESF *esf,
					    int fromHardFault)
{
	PR_EXC("***** MPU FAULT *****\n");

	_FaultContextShow(esf);

	if (_ScbMemFaultIsStacking()) {
		PR_EXC("  Stacking error\n");
	} else if (_ScbMemFaultIsUnstacking()) {
		PR_EXC("  Unstacking error\n");
	} else if (_ScbMemFaultIsDataAccessViolation()) {
		PR_EXC("  Data Access Violation\n");
		if (_ScbMemFaultIsMmfarValid()) {
			PR_EXC("  Address: 0x%x\n", _ScbMemFaultAddrGet());
			if (fromHardFault) {
				_ScbMemFaultMmfarReset();
			}
		}
	} else if (_ScbMemFaultIsInstrAccessViolation()) {
		PR_EXC("  Instruction Access Violation\n");
	}
}

/*******************************************************************************
*
* _BusFault - dump bus fault information
*
* See _FaultDump() for example.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

static void _BusFault(const NANO_ESF *esf,
					    int fromHardFault)
{
	PR_EXC("***** BUS FAULT *****\n");

	_FaultContextShow(esf);

	if (_ScbBusFaultIsStacking()) {
		PR_EXC("  Stacking error\n");
	} else if (_ScbBusFaultIsUnstacking()) {
		PR_EXC("  Unstacking error\n");
	} else if (_ScbBusFaultIsPrecise()) {
		PR_EXC("  Precise data bus error\n");
		if (_ScbBusFaultIsBfarValid()) {
			PR_EXC("  Address: 0x%x\n", _ScbBusFaultAddrGet());
			if (fromHardFault) {
				_ScbBusFaultBfarReset();
			}
		}
		/* it's possible to have both a precise and imprecise fault */
		if (_ScbBusFaultIsImprecise()) {
			PR_EXC("  Imprecise data bus error\n");
		}
	} else if (_ScbBusFaultIsImprecise()) {
		PR_EXC("  Imprecise data bus error\n");
	} else if (_ScbBusFaultIsInstrBusErr()) {
		PR_EXC("  Instruction bus error\n");
	}
}

/*******************************************************************************
*
* _UsageFault - dump usage fault information
*
* See _FaultDump() for example.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

static void _UsageFault(const NANO_ESF *esf)
{
	PR_EXC("***** USAGE FAULT *****\n");

	_FaultContextShow(esf);

	/* bits are sticky: they stack and must be reset */
	if (_ScbUsageFaultIsDivByZero()) {
		PR_EXC("  Division by zero\n");
	}
	if (_ScbUsageFaultIsUnaligned()) {
		PR_EXC("  Unaligned memory access\n");
	}
	if (_ScbUsageFaultIsNoCp()) {
		PR_EXC("  No coprocessor instructions\n");
	}
	if (_ScbUsageFaultIsInvalidPcLoad()) {
		PR_EXC("  Illegal load of EXC_RETURN into PC\n");
	}
	if (_ScbUsageFaultIsInvalidState()) {
		PR_EXC("  Illegal use of the EPSR\n");
	}
	if (_ScbUsageFaultIsUndefinedInstr()) {
		PR_EXC("  Attempt to execute undefined instruction\n");
	}

	_ScbUsageFaultAllFaultsReset();
}

/*******************************************************************************
*
* _HardFault - dump hard fault information
*
* See _FaultDump() for example.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

static void _HardFault(const NANO_ESF *esf)
{
	PR_EXC("***** HARD FAULT *****\n");
	if (_ScbHardFaultIsBusErrOnVectorRead()) {
		PR_EXC("  Bus fault on vector table read\n");
	} else if (_ScbHardFaultIsForced()) {
		PR_EXC("  Fault escalation (see below)\n");
		if (_ScbIsMemFault()) {
			_MpuFault(esf, 1);
		} else if (_ScbIsBusFault()) {
			_BusFault(esf, 1);
		} else if (_ScbIsUsageFault()) {
			_UsageFault(esf);
		}
	}
}

/*******************************************************************************
*
* _DebugMonitor - dump debug monitor exception information
*
* See _FaultDump() for example.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

static void _DebugMonitor(const NANO_ESF *esf)
{
	PR_EXC("***** Debug monitor exception (not implemented) *****\n");
}

/*******************************************************************************
*
* _ReservedException - dump reserved exception information
*
* See _FaultDump() for example.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

static void _ReservedException(const NANO_ESF *esf,
						     int fault)
{
	PR_EXC("***** %s %d) *****\n",
	       fault < 16 ? "Reserved Exception (" : "Spurious interrupt (IRQ ",
	       fault - 16);
}

/*******************************************************************************
*
* _FaultDump - dump information regarding fault (FAULT_DUMP == 2)
*
* Dump information regarding the fault when CONFIG_FAULT_DUMP is set to 2
* (long form).
*
* eg. (precise bus error escalated to hard fault):
*
* Executing context ID (thread): 0x200000dc
* Faulting instruction address:  0x000011d3
* ***** HARD FAULT *****
*    Fault escalation (see below)
* ***** BUS FAULT *****
*   Precise data bus error
*   Address: 0xff001234
*
* RETURNS: N/A
*
* \NOMANUAL
*/

static void _FaultDump(const NANO_ESF *esf, int fault)
{
	switch (fault) {
	case 3:
		_HardFault(esf);
		break;
	case 4:
		_MpuFault(esf, 0);
		break;
	case 5:
		_BusFault(esf, 0);
		break;
	case 6:
		_UsageFault(esf);
		break;
	case 12:
		_DebugMonitor(esf);
		break;
	default:
		_ReservedException(esf, fault);
		break;
	}
}
#endif /* FAULT_DUMP == 2 */

/*******************************************************************************
*
* _Fault - fault handler
*
* This routine is called when fatal error conditions are detected by hardware
* and is responsible only for reporting the error. Once reported, it then
* invokes the user provided routine _SysFatalErrorHandler() which is
* responsible for implementing the error handling policy.
*
* Since the ESF can be either on the MSP or PSP depending if an exception or
* interrupt was already being handled, it is passed a pointer to both and has
* to find out on which the ESP is present.
*
* RETURNS: This function does not return.
*
* \NOMANUAL
*/

void _Fault(
	const NANO_ESF *msp, /* pointer to potential ESF on MSP */
	const NANO_ESF *psp  /* pointer to potential ESF on PSP */
	)
{
	const NANO_ESF *esf = _ScbIsNestedExc() ? msp : psp;
	int fault = _ScbActiveVectorGet();

	FAULT_DUMP(esf, fault);

	_SysFatalErrorHandler(_NANO_ERR_HW_EXCEPTION, esf);
}

/*******************************************************************************
*
* _FaultInit - initialization of fault handling
*
* Turns on the desired hardware faults.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void _FaultInit(void)
{
	_ScbDivByZeroFaultEnable();
	_ScbUnalignedFaultEnable();
}
