#include <device.h>
#include <init.h>
#include <soc.h>


static int nxp_mcimx7_init(struct device *arg)
{
    ARG_UNUSED(arg);

    return 0;
}

SYS_INIT(nxp_mcimx7_init, PRE_KERNEL_1, 0);
