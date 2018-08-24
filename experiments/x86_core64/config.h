#ifndef _CONFIG_H
#define _CONFIG_H

/* Placeholder configuration file */

#define CONFIG_MP_NUM_CPUS 2

#define CONFIG_DEBUG_STUB32 1

/* The APIC timer will run 2^X times slower than the TSC. (X = 0-7) */
#define CONFIG_APIC_TSC_SHIFT 6

#endif /* _CONFIG_H */
