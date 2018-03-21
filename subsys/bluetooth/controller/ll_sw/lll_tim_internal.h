/* NOTE: Definitions used internal to LLL implementations */

#define TIFS_US 150

static inline u32_t addr_us_get(u8_t phy)
{
	switch (phy) {
	default:
	case BIT(0):
		return 40;
	case BIT(1):
		return 24;
	case BIT(2):
		return 376;
	}
}
