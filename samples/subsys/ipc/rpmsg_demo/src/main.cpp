#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include <drivers/ipm.h>
#include <metal/device.h>
#include <openamp/open_amp.h>
#include "rsc_table.hpp"
#include <string>

#define SHM_START_ADDR (DT_REG_ADDR(DT_CHOSEN(zephyr_ipc_shm)))
#define SHM_SIZE (DT_REG_SIZE(DT_CHOSEN(zephyr_ipc_shm)))

static K_SEM_DEFINE(ipm_sem, 0, 1);
static K_SEM_DEFINE(ept_sem, 0, 1);
static struct rpmsg_virtio_device rvdev;
static const device *ipm;
static std::string rcv_msg;

static void ipm_callback(const device *dev, void *context, uint32_t id, volatile void *data)
{
    // signal to M4 to look into the inbox
    printk("%s\n", __func__);
    k_sem_give(&ipm_sem);
}

int rpvdev_notify_func_callback(void *priv, uint32_t id)
{
    // signal to A53 to look into the inbox
    printk("%s\n", __func__);
    ipm_send(ipm, 1, RPMSG_MU_CHANNEL, NULL, 0);
    return 0;
}

void virtio_dev_reset_cb_callback(virtio_device *vdev)
{

}

void rpmsg_ns_bind_cb_callback(rpmsg_device *rdev, const char *name, uint32_t dest)
{

}

int rpmsg_ept_cb_callback(rpmsg_endpoint *ept, void *data, size_t len, uint32_t src, void *priv)
{
    rcv_msg = (const char*)data;
    printk("%s: message in callback is: %s\n", __func__, rcv_msg.c_str());

    k_sem_give(&ept_sem);
    return RPMSG_SUCCESS;
}

void rpmsg_ns_unbind_cb_callback(rpmsg_endpoint *ept)
{

}

static std::string receive_message()
{
    printk("\n\nWait for message\n");
    while (k_sem_take(&ept_sem, K_NO_WAIT) != 0)
    {
        int status = k_sem_take(&ipm_sem, K_FOREVER);
        if (status == 0)
        {
            rproc_virtio_notified(rvdev.vdev, 1);
        }
    }
    printk("message in the buffer is: %s \n", rcv_msg.c_str());
    return rcv_msg;
}

void app_task(void *unused0, void *unused1, void *unused2)
{
    (void)unused0;
    (void)unused1;
    (void)unused2;

    copyResourceTable();

    // metal stuff
    metal_init_params metal_params = METAL_INIT_DEFAULTS;

    if (metal_init(&metal_params))
    {
        return;
    }

    const char* shm_device_name = "shm";
    const char* bus_name = "generic";

    metal_device shm_device = {
        .name = shm_device_name,
        .num_regions = 2,
        .regions = {
            {.virt = NULL}, /* shared memory */
            {.virt = NULL}, /* rsc_table memory */
        },
        .node = { NULL },
        .irq_num = 0,
        .irq_info = NULL
    };

    if (metal_register_generic_device(&shm_device))
    {
        return;
    }

    metal_device* device_;
    if (metal_device_open(bus_name, shm_device_name, &device_))
    {
        return;
    }

    void* shm_virt = (void *)SHM_START_ADDR;
    metal_phys_addr_t shm_physmap = SHM_START_ADDR;
    size_t shm_size = SHM_SIZE;
    metal_io_init(&device_->regions[0], shm_virt, &shm_physmap, shm_size, -1, 0, NULL);

    metal_io_region* shm_io = metal_device_io_region(device_, 0);
    if (!shm_io)
    {
        return;
    }

    remote_resource_table* rsc_tbl = getResourceTable();

    void* rsc_virt = rsc_tbl;
    metal_phys_addr_t rsc_physmap = (metal_phys_addr_t)rsc_tbl;
    size_t rsc_size = sizeof(remote_resource_table);
    metal_io_init(&device_->regions[1], rsc_virt, &rsc_physmap, rsc_size, -1, 0, NULL);

    metal_io_region* rsc_io = metal_device_io_region(device_, 1);
    if (!rsc_io)
    {
        return;
    }

    // ipm stuff
    ipm = DEVICE_DT_GET(DT_CHOSEN(zephyr_ipc));
    if (!device_is_ready(ipm))
    {
        return;
    }

    ipm_register_callback(ipm, ipm_callback, NULL);
    ipm_set_enabled(ipm, 1);

    printk("IPM initialized, data size = %d\n", CONFIG_IPM_IMX_MAX_DATA_SIZE);

    // vdev/vring stuff
    virtio_device* vdev = rproc_virtio_create_vdev(VIRTIO_DEV_SLAVE, VIRTIO_ID_RPMSG, (void *)&rsc_tbl->user_vdev, rsc_io, NULL, rpvdev_notify_func_callback, virtio_dev_reset_cb_callback);
    if (!vdev)
    {
        return;
    }

    rproc_virtio_wait_remote_ready(vdev);

    fw_rsc_vdev_vring* vring0 = &rsc_tbl->user_vring0;
    if (rproc_virtio_init_vring(vdev, 0, vring0->notifyid, (void *)vring0->da, rsc_io, vring0->num, vring0->align))
    {
        return;
    }

    fw_rsc_vdev_vring* vring1 = &rsc_tbl->user_vring1;
    if (rproc_virtio_init_vring(vdev, 1, vring1->notifyid, (void *)vring1->da, rsc_io, vring1->num, vring1->align))
    {
        return;
    }

    rpmsg_virtio_shm_pool shpool;
    rpmsg_virtio_init_shm_pool(&shpool, shm_virt, shm_size);

    if (rpmsg_init_vdev(&rvdev, vdev, rpmsg_ns_bind_cb_callback, shm_io, &shpool))
    {
        return;
    }

    rpmsg_device* rpdev = rpmsg_virtio_get_rpmsg_device(&rvdev);
    if (!rpdev)
    {
        return;
    }

    const char* rpmsg_channel_name = "rpmsg-virtual-tty-channel-1";

    rpmsg_endpoint rcv_ept;
    if (rpmsg_create_ept(&rcv_ept, rpdev, rpmsg_channel_name, RPMSG_ADDR_ANY, RPMSG_ADDR_ANY, rpmsg_ept_cb_callback, rpmsg_ns_unbind_cb_callback))
    {
        return;
    }

    printk("So far, so good!\n");

    while (1)
    {
        std::string msg = receive_message();
        printk("Sending message : %s \n", msg.c_str());
        rpmsg_send(&rcv_ept, msg.c_str(), msg.size());
    }
}
K_THREAD_DEFINE(app_id, 1024, app_task, NULL, NULL, NULL, K_PRIO_COOP(7), 0, 0);
