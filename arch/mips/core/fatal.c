/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sys/printk.h>

FUNC_NORETURN void z_mips_fatal_error(unsigned int reason,
					  const z_arch_esf_t *esf)
{
	if (esf != NULL) {
		printk("$ 0   : %08x %08x %08x %08x\n",
			0, esf->r[0], esf->r[1], esf->r[2]);
		printk("$ 4   : %08x %08x %08x %08x\n",
			esf->r[3], esf->r[4], esf->r[5], esf->r[6]);
		printk("$ 8   : %08x %08x %08x %08x\n",
			esf->r[7], esf->r[8], esf->r[9], esf->r[10]);
		printk("$12   : %08x %08x %08x %08x\n",
			esf->r[11], esf->r[12], esf->r[13], esf->r[14]);
		printk("$16   : %08x %08x %08x %08x\n",
			esf->r[15], esf->r[16], esf->r[17], esf->r[18]);
		printk("$20   : %08x %08x %08x %08x\n",
			esf->r[19], esf->r[20], esf->r[21], esf->r[22]);
		printk("$24   : %08x %08x\n",
			esf->r[23], esf->r[24]);
		printk("$28   : %08x %08x %08x %08x\n",
			esf->r[27], esf->r[28], esf->r[29], esf->r[30]);

		printk("epc   : %08x\n", esf->epc);

		printk("Status: %08x\n", esf->status);
		printk("Cause : %08x\n", esf->cause);
		printk("BadVA : %08x\n", esf->badvaddr);
	}

	z_fatal_error(reason, esf);
	CODE_UNREACHABLE;
}
