ifeq ($(CONFIG_IEEE154_NRF5),y)
ZEPHYRINCLUDE += -I$(srctree)/ext/hal/nordic/drivers

# A common prefix used for placing radio buffer in a named
# memory section.
CFLAGS += -DRADIO_SECTION_PREFIX=".top_of_image_ram"

# Number of slots containing short addresses of nodes for which pending data is stored.
CFLAGS += -DRADIO_PENDING_SHORT_ADDRESSES=1

# Number of slots containing extended addresses of nodes for which pending data is stored.
CFLAGS += -DRADIO_PENDING_EXTENDED_ADDRESSES=1

# Number of buffers in receive queue.
CFLAGS += -DRADIO_RX_BUFFERS=1

# CCA mode
ifeq ($(CONFIG_IEEE154_NRF5_CCA_MODE_ED,y)
CFLAGS += -DRADIO_CCA_MODE=NRF_RADIO_CCA_MODE_ED
else ifeq ($(CONFIG_IEEE154_NRF5_CCA_MODE_CARRIER),y)
CFLAGS += -DRADIO_CCA_MODE=NRF_RADIO_CCA_MODE_CARRIER
else ifeq ($(IEEE154_NRF5_CCA_MODE_CARRIER_AND_ED),y)
CFLAGS += -DRADIO_CCA_MODE=NRF_RADIO_CCA_MODE_CARRIER_AND_ED
else ifeq ($(IEEE154_NRF5_CCA_MODE_CARRIER_OR_ED),y)
CFLAGS += -DRADIO_CCA_MODE=NRF_RADIO_CCA_MODE_CARRIER_OR_ED
endif

# CCA mode options
CFLAGS += -DRADIO_CCA_CORR_LIMIT=$(CONFIG_IEEE154_NRF5_CCA_CORR_LIMIT)
CFLAGS += -DRADIO_CCA_CORR_THRESHOLD=$(CONFIG_IEEE154_NRF5_CCA_CORR_THRESHOLD)
CFLAGS += -DRADIO_CCA_ED_THRESHOLD=$(CONFIG_IEEE154_NRF5_CCA_ED_THRESHOLD)

endif