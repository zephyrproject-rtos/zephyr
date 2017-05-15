#include <zephyr.h>
#include <irq.h>
#include <soc.h>

#include "fsl_os_abstraction.h"

unsigned int gIrqKey;

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_InterruptEnable
 * Description   : self explanatory.
 *
 *END**************************************************************************/
void OSA_InterruptEnable(void)
{
	irq_unlock(gIrqKey);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : OSA_InterruptDisable
 * Description   : self explanatory.
 *
 *END**************************************************************************/
void OSA_InterruptDisable(void)
{
	gIrqKey = irq_lock();
}
