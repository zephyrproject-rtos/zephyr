deps_config := \
	vxmicro.wrsconfig

include/config/auto.conf: \
	$(deps_config)

$(deps_config): ;
