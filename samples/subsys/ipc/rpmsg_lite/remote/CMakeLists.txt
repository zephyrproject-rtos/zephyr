set(BOARD lpcxpresso54114_m0)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/samples/subsys/ipc/rpmsg_lite)
target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/drivers)
target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/ext/lib/ipc/rpmsg_lite/lib/include)
if(CONFIG_SOC_SERIES_LPC54XXX)
target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/ext/lib/ipc/rpmsg_lite/lib/include/platform/lpc5411x)
endif()

if(CONFIG_IPC_RPMSG_LITE)
target_sources(app PRIVATE $ENV{ZEPHYR_BASE}/ext/lib/ipc/rpmsg_lite/lib/common/llist.c)
target_sources(app PRIVATE $ENV{ZEPHYR_BASE}/ext/lib/ipc/rpmsg_lite/lib/rpmsg_lite/rpmsg_lite.c)
target_sources(app PRIVATE $ENV{ZEPHYR_BASE}/ext/lib/ipc/rpmsg_lite/lib/rpmsg_lite/rpmsg_ns.c)
target_sources(app PRIVATE $ENV{ZEPHYR_BASE}/ext/lib/ipc/rpmsg_lite/lib/rpmsg_lite/rpmsg_queue.c)
target_sources(app PRIVATE $ENV{ZEPHYR_BASE}/ext/lib/ipc/rpmsg_lite/lib/rpmsg_lite/porting/environment/rpmsg_env_zephyr.c)
if(CONFIG_SOC_SERIES_LPC54XXX)
target_sources(app PRIVATE $ENV{ZEPHYR_BASE}/ext/lib/ipc/rpmsg_lite/lib/rpmsg_lite/porting/platform/lpc5411x/rpmsg_platform_zephyr_ipm.c)
endif()
target_sources(app PRIVATE $ENV{ZEPHYR_BASE}/ext/lib/ipc/rpmsg_lite/lib/virtio/virtqueue.c)
endif()

target_sources(app PRIVATE src/main_remote.c)

