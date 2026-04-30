/*
 * Copyright (C) 2026 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_CDNS3_VENDOR_QUIRKS_H_
#define ZEPHYR_DRIVERS_USB_UDC_CDNS3_VENDOR_QUIRKS_H_

#define UDC_CDNS3_QUIRKS(n) (NULL)

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
