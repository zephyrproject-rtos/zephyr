#include <zephyr/types.h>

#include "hal/ccm.h"
#include "hal/radio.h"

#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

void chan_set(u32_t chan)
{
	switch (chan) {
	case 37:
		radio_freq_chan_set(2);
		break;

	case 38:
		radio_freq_chan_set(26);
		break;

	case 39:
		radio_freq_chan_set(80);
		break;

	default:
		if (chan < 11) {
			radio_freq_chan_set(4 + (2 * chan));
		} else if (chan < 40) {
			radio_freq_chan_set(28 + (2 * (chan - 11)));
		} else {
			LL_ASSERT(0);
		}
		break;
	}

	radio_whiten_iv_set(chan);
}
