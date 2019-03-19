target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/ext/lib/ipc/rpmsg_lite/lib/include)

target_sources(app PRIVATE $ENV{ZEPHYR_BASE}/ext/lib/ipc/rpmsg_lite/lib/common/llist.c)
target_sources(app PRIVATE $ENV{ZEPHYR_BASE}/ext/lib/ipc/rpmsg_lite/lib/rpmsg_lite/rpmsg_lite.c)
target_sources(app PRIVATE $ENV{ZEPHYR_BASE}/ext/lib/ipc/rpmsg_lite/lib/rpmsg_lite/rpmsg_ns.c)
target_sources(app PRIVATE $ENV{ZEPHYR_BASE}/ext/lib/ipc/rpmsg_lite/lib/rpmsg_lite/rpmsg_queue.c)
target_sources(app PRIVATE $ENV{ZEPHYR_BASE}/ext/lib/ipc/rpmsg_lite/lib/rpmsg_lite/porting/environment/rpmsg_env_zephyr.c)
target_sources(app PRIVATE $ENV{ZEPHYR_BASE}/ext/lib/ipc/rpmsg_lite/lib/virtio/virtqueue.c)
