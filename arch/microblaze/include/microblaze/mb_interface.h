/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#ifndef ZEPHYR_ARCH_MICROBLAZE_INCLUDE_MICROBLAZE_MB_INTERFACE_H_
#define ZEPHYR_ARCH_MICROBLAZE_INCLUDE_MICROBLAZE_MB_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif
/* Enable Interrupts */
extern void microblaze_enable_interrupts(void);
/* Disable Interrupts */
extern void microblaze_disable_interrupts(void);
/* Enable Instruction Cache */
extern void microblaze_enable_icache(void);
/* Disable Instruction Cache */
extern void microblaze_disable_icache(void);
/* Enable Instruction Cache */
extern void microblaze_enable_dcache(void);
/* Disable Instruction Cache */
extern void microblaze_disable_dcache(void);
/* Enable hardware exceptions */
extern void microblaze_enable_exceptions(void);
/* Disable hardware exceptions */
extern void microblaze_disable_exceptions(void);

/* necessary for pre-processor */
#define stringify(s) tostring(s)
#define tostring(s)  #s

/* Simplified Cache instruction macros that use only single register */
#define wdc(v) ({ __asm__ __volatile__("wdc\t%0,r0\n" ::"d"(v)); })
#define wdc_flush(v) ({ __asm__ __volatile__("wdc.flush\t%0,r0\n" ::"d"(v)); })
#define wdc_clear(v) ({ __asm__ __volatile__("wdc.clear\t%0,r0\n" ::"d"(v)); })
#define wic(v) ({ __asm__ __volatile__("wic\t%0,r0\n" ::"d"(v)); })

/* FSL Access Macros */
/* Blocking Data Read and Write to FSL no. id */
#define getfsl(val, id) asm volatile("get\t%0,rfsl" stringify(id) : "=d"(val))
#define putfsl(val, id) asm volatile("put\t%0,rfsl" stringify(id)::"d"(val))

/* Non-blocking Data Read and Write to FSL no. id */
#define ngetfsl(val, id) asm volatile("nget\t%0,rfsl" stringify(id) : "=d"(val))
#define nputfsl(val, id) asm volatile("nput\t%0,rfsl" stringify(id)::"d"(val))

/* Blocking Control Read and Write to FSL no. id */
#define cgetfsl(val, id) asm volatile("cget\t%0,rfsl" stringify(id) : "=d"(val))
#define cputfsl(val, id) asm volatile("cput\t%0,rfsl" stringify(id)::"d"(val))

/* Non-blocking Control Read and Write to FSL no. id */
#define ncgetfsl(val, id) asm volatile("ncget\t%0,rfsl" stringify(id) : "=d"(val))
#define ncputfsl(val, id) asm volatile("ncput\t%0,rfsl" stringify(id)::"d"(val))

/* Polling versions of FSL access macros. This makes the FSL access interruptible */
#define getfsl_interruptible(val, id)                                                              \
	asm volatile("\n1:\n\tnget\t%0,rfsl" stringify(id) "\n\t"                                  \
							   "addic\tr18,r0,0\n\t"                   \
							   "bnei\tr18,1b\n"                        \
		     : "=d"(val)::"r18")

#define putfsl_interruptible(val, id)                                                              \
	asm volatile("\n1:\n\tnput\t%0,rfsl" stringify(id) "\n\t"                                  \
							   "addic\tr18,r0,0\n\t"                   \
							   "bnei\tr18,1b\n" ::"d"(val)             \
		     : "r18")

#define cgetfsl_interruptible(val, id)                                                             \
	asm volatile("\n1:\n\tncget\t%0,rfsl" stringify(id) "\n\t"                                 \
							    "addic\tr18,r0,0\n\t"                  \
							    "bnei\tr18,1b\n"                       \
		     : "=d"(val)::"r18")

#define cputfsl_interruptible(val, id)                                                             \
	asm volatile("\n1:\n\tncput\t%0,rfsl" stringify(id) "\n\t"                                 \
							    "addic\tr18,r0,0\n\t"                  \
							    "bnei\tr18,1b\n" ::"d"(val)            \
		     : "r18")
/* FSL valid and error check macros. */
#define fsl_isinvalid(result) asm volatile("addic\t%0,r0,0" : "=d"(result))
#define fsl_iserror(error)                                                                         \
	asm volatile("mfs\t%0,rmsr\n\t"                                                            \
		     "andi\t%0,%0,0x10"                                                            \
		     : "=d"(error))

/* Pseudo assembler instructions */
#define clz(v)                                                                                     \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("clz\t%0,%1\n" : "=d"(_rval) : "d"(v));                       \
		_rval;                                                                             \
	})

#define mbar(mask) ({ __asm__ __volatile__("mbar\t" stringify(mask)); })

/* Arm like macros for calling different types of memory barriers */
#define isb()	mbar(0b01)
#define dmb()	mbar(0b10)
#define dsb()	mbar(0b11)
#define __ISB() isb()
#define __DMB() dmb()
#define __DSB() dsb()

#define mb_sleep()     ({ __asm__ __volatile__("sleep\t"); })
#define mb_hibernate() ({ __asm__ __volatile__("hibernate\t"); })
#define mb_suspend()   ({ __asm__ __volatile__("suspend\t"); })
#define mb_swapb(v)                                                                                \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("swapb\t%0,%1\n" : "=d"(_rval) : "d"(v));                     \
		_rval;                                                                             \
	})

#define mb_swaph(v)                                                                                \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("swaph\t%0,%1\n" : "=d"(_rval) : "d"(v));                     \
		_rval;                                                                             \
	})

#define mfgpr(rn)                                                                                  \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("or\t%0,r0," stringify(rn) "\n" : "=d"(_rval));               \
		_rval;                                                                             \
	})

#define mfmsr()                                                                                    \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("mfs\t%0,rmsr\n" : "=d"(_rval));                              \
		_rval;                                                                             \
	})

#define mfear()                                                                                    \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("mfs\t%0,rear\n" : "=d"(_rval));                              \
		_rval;                                                                             \
	})

#define mfeare()                                                                                   \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("mfse\t%0,rear\n" : "=d"(_rval));                             \
		_rval;                                                                             \
	})

#define mfesr()                                                                                    \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("mfs\t%0,resr\n" : "=d"(_rval));                              \
		_rval;                                                                             \
	})

#define mffsr()                                                                                    \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("mfs\t%0,rfsr\n" : "=d"(_rval));                              \
		_rval;                                                                             \
	})

#define mfpvr(rn)                                                                                  \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("mfs\t%0,rpvr" stringify(rn) "\n" : "=d"(_rval));             \
		_rval;                                                                             \
	})

#define mfpvre(rn)                                                                                 \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("mfse\t%0,rpvr" stringify(rn) "\n" : "=d"(_rval));            \
		_rval;                                                                             \
	})

#define mfbtr()                                                                                    \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("mfs\t%0,rbtr\n" : "=d"(_rval));                              \
		_rval;                                                                             \
	})

#define mfedr()                                                                                    \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("mfs\t%0,redr\n" : "=d"(_rval));                              \
		_rval;                                                                             \
	})

#define mfpid()                                                                                    \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("mfs\t%0,rpid\n" : "=d"(_rval));                              \
		_rval;                                                                             \
	})

#define mfzpr()                                                                                    \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("mfs\t%0,rzpr\n" : "=d"(_rval));                              \
		_rval;                                                                             \
	})

#define mftlbx()                                                                                   \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("mfs\t%0,rtlbx\n" : "=d"(_rval));                             \
		_rval;                                                                             \
	})

#define mftlblo()                                                                                  \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("mfs\t%0,rtlblo\n" : "=d"(_rval));                            \
		_rval;                                                                             \
	})

#define mftlbhi()                                                                                  \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("mfs\t%0,rtlbhi\n" : "=d"(_rval));                            \
		_rval;                                                                             \
	})

#define mfslr()                                                                                    \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("mfs\t%0,rslr\n" : "=d"(_rval));                              \
		_rval;                                                                             \
	})

#define mfshr()                                                                                    \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("mfs\t%0,rshr\n" : "=d"(_rval));                              \
		_rval;                                                                             \
	})

#define mtgpr(rn, v) ({ __asm__ __volatile__("or\t" stringify(rn) ",r0,%0\n" ::"d"(v)); })

#define mtmsr(v) ({ __asm__ __volatile__("mts\trmsr,%0\n\tnop\n" ::"d"(v)); })

#define mtfsr(v) ({ __asm__ __volatile__("mts\trfsr,%0\n\tnop\n" ::"d"(v)); })

#define mtpid(v) ({ __asm__ __volatile__("mts\trpid,%0\n\tnop\n" ::"d"(v)); })

#define mtzpr(v) ({ __asm__ __volatile__("mts\trzpr,%0\n\tnop\n" ::"d"(v)); })

#define mttlbx(v) ({ __asm__ __volatile__("mts\trtlbx,%0\n\tnop\n" ::"d"(v)); })

#define mttlblo(v) ({ __asm__ __volatile__("mts\trtlblo,%0\n\tnop\n" ::"d"(v)); })

#define mttlbhi(v) ({ __asm__ __volatile__("mts\trtlbhi,%0\n\tnop\n" ::"d"(v)); })

#define mttlbsx(v) ({ __asm__ __volatile__("mts\trtlbsx,%0\n\tnop\n" ::"d"(v)); })

#define mtslr(v) ({ __asm__ __volatile__("mts\trslr,%0\n\tnop\n" ::"d"(v)); })

#define mtshr(v) ({ __asm__ __volatile__("mts\trshr,%0\n\tnop\n" ::"d"(v)); })

#define lwx(address)                                                                               \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("lwx\t%0,%1,r0\n" : "=d"(_rval) : "d"(address));              \
		_rval;                                                                             \
	})

#define lwr(address)                                                                               \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("lwr\t%0,%1,r0\n" : "=d"(_rval) : "d"(address));              \
		_rval;                                                                             \
	})

#define lwea(lladdr)                                                                               \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("lwea\t%0,%M1,%L1\n" : "=d"(_rval) : "d"(lladdr));            \
		_rval;                                                                             \
	})

#define lhur(address)                                                                              \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("lhur\t%0,%1,r0\n" : "=d"(_rval) : "d"(address));             \
		_rval;                                                                             \
	})

#define lhuea(lladdr)                                                                              \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("lhuea\t%0,%M1,%L1\n" : "=d"(_rval) : "d"(lladdr));           \
		_rval;                                                                             \
	})

#define lbur(address)                                                                              \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("lbur\t%0,%1,r0\n" : "=d"(_rval) : "d"(address));             \
		_rval;                                                                             \
	})

#define lbuea(lladdr)                                                                              \
	({                                                                                         \
		unsigned int _rval = 0U;                                                           \
		__asm__ __volatile__("lbuea\t%0,%M1,%L1\n" : "=d"(_rval) : "d"(lladdr));           \
		_rval;                                                                             \
	})

#define swx(address, data) ({ __asm__ __volatile__("swx\t%0,%1,r0\n" ::"d"(data), "d"(address)); })

#define swr(address, data) ({ __asm__ __volatile__("swr\t%0,%1,r0\n" ::"d"(data), "d"(address)); })

#define swea(lladdr, data)                                                                         \
	({ __asm__ __volatile__("swea\t%0,%M1,%L1\n" ::"d"(data), "d"(lladdr)); })

#define shr(address, data) ({ __asm__ __volatile__("shr\t%0,%1,r0\n" ::"d"(data), "d"(address)); })

#define shea(lladdr, data)                                                                         \
	({ __asm__ __volatile__("shea\t%0,%M1,%L1\n" ::"d"(data), "d"(lladdr)); })

#define sbr(address, data) ({ __asm__ __volatile__("sbr\t%0,%1,r0\n" ::"d"(data), "d"(address)); })

#define sbea(lladdr, data)                                                                         \
	({ __asm__ __volatile__("sbea\t%0,%M1,%L1\n" ::"d"(data), "d"(lladdr)); })

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_MICROBLAZE_INCLUDE_MICROBLAZE_MB_INTERFACE_H_ */
