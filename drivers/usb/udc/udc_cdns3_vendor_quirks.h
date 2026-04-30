/*
 * Copyright (C) 2026 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_CDNS3_VENDOR_QUIRKS_H_
#define ZEPHYR_DRIVERS_USB_UDC_CDNS3_VENDOR_QUIRKS_H_

#include <zephyr/devicetree.h>

#if DT_HAS_COMPAT_STATUS_OKAY(ti_am64_usb)
#include "udc_cdns3_vendor_ti_am64.h"
#endif

#define UDC_CDNS3_QUIRKS(n)                                                                        \
	COND_CODE_1(DT_NODE_HAS_COMPAT(DT_DRV_INST(n), ti_am64_usb),				   \
		(&udc_cdns3_ti_quirks_##n),							   \
		(/* Add more vendors here with nested COND_CODE_1 */ NULL))

#define UDC_CDNS3_FUNC_DEFINE(fn)                                                                  \
	static inline int udc_cdns3_quirks_##fn(const struct device *const dev)                    \
	{                                                                                          \
		struct udc_cdns3_data *data = udc_get_private(dev);                                \
		if (data->quirks && data->quirks->ops && data->quirks->ops->fn) {                  \
			return data->quirks->ops->fn(dev);                                         \
		}                                                                                  \
		return 0;                                                                          \
	}

/* Generate wrapper functions */
UDC_CDNS3_FUNC_DEFINE(preinit);
UDC_CDNS3_FUNC_DEFINE(init);
UDC_CDNS3_FUNC_DEFINE(enable);
UDC_CDNS3_FUNC_DEFINE(disable);
UDC_CDNS3_FUNC_DEFINE(shutdown);

#endif /* ZEPHYR_DRIVERS_USB_UDC_CDNS3_VENDOR_QUIRKS_H_ */
