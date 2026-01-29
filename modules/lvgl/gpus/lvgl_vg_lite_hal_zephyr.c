/*
 * Copyright (c) 2025 Felipe Neves <ryukokki.felipe@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/sys_io.h>
#include <lvgl.h>
#include <libs/vg_lite_driver/VGLiteKernel/vg_lite_kernel.h>
#include <libs/vg_lite_driver/VGLiteKernel/vg_lite_hal.h>
#include <libs/vg_lite_driver/VGLiteKernel/vg_lite_hw.h>

#define VG_SYSTEM_RESERVE_COUNT 2

static int z_vg_lite_init(void);

typedef struct list_head {
    struct list_head * next;
    struct list_head * prev;
} list_head_t;

typedef struct heap_node {
    list_head_t list;
    uint32_t offset;
    unsigned long size;
    int32_t status;
    vg_lite_vidmem_pool_t pool;
} heap_node_t;

struct memory_heap {
    uint32_t free;
    list_head_t list;
};

struct vg_lite_device {
    /* void * gpu; */
    uint32_t register_base;    /* Always use physical for register access in RTOS. */
    /* struct page * pages; */
    volatile void * contiguous[VG_SYSTEM_RESERVE_COUNT];
    unsigned int order;
    unsigned int heap_size[VG_SYSTEM_RESERVE_COUNT];
    void * virtual[VG_SYSTEM_RESERVE_COUNT];
    uint32_t physical[VG_SYSTEM_RESERVE_COUNT];
    uint32_t size[VG_SYSTEM_RESERVE_COUNT];
    struct memory_heap heap[VG_SYSTEM_RESERVE_COUNT];
    volatile uint32_t int_flags;
    struct k_sem sync;
    void * device;
    vg_lite_gpu_execute_state_t gpu_execute_state;
};

static struct vg_lite_device * device;

static volatile void * contiguous_mem[VG_SYSTEM_RESERVE_COUNT] = {
    [0 ... VG_SYSTEM_RESERVE_COUNT - 1] = NULL
};

static uint32_t gpu_mem_base[VG_SYSTEM_RESERVE_COUNT] = {
    [0 ... VG_SYSTEM_RESERVE_COUNT - 1] = 0
};

static uint32_t heap_size[VG_SYSTEM_RESERVE_COUNT] = {
    [0 ... VG_SYSTEM_RESERVE_COUNT - 1] = MAX_CONTIGUOUS_SIZE
};

/**
 * VG_LITE Driver continguous memory allocator implementation below.
 * TODO: Cleanup the VGLite driver from LVGL and remove the code below,
 *       since unfortunately it is not possible since this allocator custom
 *       is required by the VGLite upper layers. This allocator was extracted
 *       from Verisilicon reference implementation.
 */

#define VG_LITE_HAL_NODE_MARKER 0xABBAF00D

#define INIT_LIST_HEAD(entry) \
    (entry)->next = (entry);\
    (entry)->prev = (entry);

static inline void add_list(list_head_t * to_add, list_head_t * head)
{
    /* Link the new item. */
    to_add->next = head;
    to_add->prev = head->prev;

    /* Modify the neighbor. */
    head->prev = to_add;
    if(to_add->prev != NULL) {
        to_add->prev->next = to_add;
    }
}

static inline void delete_list(list_head_t * entry)
{
    if(entry->prev != NULL) {
        entry->prev->next = entry->next;
    }
    if(entry->next != NULL) {
        entry->next->prev = entry->prev;
    }
}

static int split_node(heap_node_t * node, unsigned long size)
{
    heap_node_t * split;

    /* Allocate a new node. */
    vg_lite_hal_allocate(sizeof(heap_node_t), (void **)&split);
    if(split == NULL)
        return -1;

    /* Fill in the data of this node of the remaning size. */
    split->offset = node->offset + size;
    split->size = node->size - size;
    split->status = 0;

    /* Add the new node behind the current node. */
    add_list(&split->list, &node->list);

    /* Adjust the size of the current node. */
    node->size = size;
    return 0;
}

vg_lite_error_t vg_lite_hal_allocate(unsigned long size, void ** memory)
{
    vg_lite_error_t error = VG_LITE_SUCCESS;

    /* TODO: Allocate some memory. No more kernel mode in RTOS. */
    *memory = lv_malloc(size);
    if(NULL == *memory)
        error = VG_LITE_OUT_OF_MEMORY;

    return error;
}

vg_lite_error_t vg_lite_hal_free(void * memory)
{
    vg_lite_error_t error = VG_LITE_SUCCESS;

    lv_free(memory);

    return error;
}

vg_lite_error_t vg_lite_hal_allocate_contiguous(unsigned long size, vg_lite_vidmem_pool_t pool, void ** logical,
                                                void ** klogical, uint32_t * physical, void ** node)
{
    unsigned long aligned_size;
    heap_node_t * pos;

    /* Judge if it exceeds the range of pool */
    if(pool >= VG_SYSTEM_RESERVE_COUNT)
        pool = VG_SYSTEM_RESERVE_COUNT - 1;

    /* Align the size to 64 bytes. */
    aligned_size = (size + 63) & ~63;

    /* Check if there is enough free memory available. */
    if(aligned_size > device->heap[pool].free) {
        return VG_LITE_OUT_OF_MEMORY;
    }

    /* Walk the heap backwards. */
    for(pos = (heap_node_t *)device->heap[pool].list.prev;
        &pos->list != &device->heap[pool].list;
        pos = (heap_node_t *) pos->list.prev) {
        /* Check if the current node is free and is big enough. */
        if(pos->status == 0 && pos->size >= aligned_size) {
            /* See if we the current node is big enough to split. */
            if(0 != split_node(pos, aligned_size)) {
                return VG_LITE_OUT_OF_RESOURCES;
            }
            /* Mark the current node as used. */
            pos->status = 0xABBAF00D;

            /*  Return the logical/physical address. */
            /* *logical = (uint8_t *) private_data->contiguous_mapped + pos->offset; */
            *logical = (uint8_t *)device->virtual[pool] + pos->offset;
            *klogical = *logical;
            *physical = gpu_mem_base[pool] + (uint32_t)(*logical);/* device->physical + pos->offset; */

            /* Mark which pool the pos belong to */
            pos->pool = pool;

            device->heap[pool].free -= aligned_size;

            *node = pos;
            return VG_LITE_SUCCESS;
        }
    }

    /* Out of memory. */
    return VG_LITE_OUT_OF_MEMORY;
}

void vg_lite_hal_free_contiguous(void * memory_handle)
{
    heap_node_t * pos, * node;
    vg_lite_vidmem_pool_t pool;

    /* Get pointer to node. */
    node = memory_handle;

    if(node->status != VG_LITE_HAL_NODE_MARKER) {
        return;
    }

    /* Determine which pool the node belongs to */
    pool = node->pool;

    /* Mark node as free. */
    node->status = 0;

    /* Add node size to free count. */
    device->heap[pool].free += node->size;

    /* Check if next node is free. */
    pos = node;
    for(pos = (heap_node_t *)pos->list.next;
        &pos->list != &device->heap[pool].list;
        pos = (heap_node_t *)pos->list.next) {
        if(pos->status == 0) {
            /* Merge the nodes. */
            node->size += pos->size;
            if(node->offset > pos->offset)
                node->offset = pos->offset;
            /* Delete the next node from the list. */
            delete_list(&pos->list);
            vg_lite_hal_free(pos);
        }
        break;
    }

    /* Check if the previous node is free. */
    pos = node;
    for(pos = (heap_node_t *)pos->list.prev;
        &pos->list != &device->heap[pool].list;
        pos = (heap_node_t *)pos->list.prev) {
        if(pos->status == 0) {
            /* Merge the nodes. */
            pos->size += node->size;
            if(pos->offset > node->offset)
                pos->offset = node->offset;
            /* Delete the current node from the list. */
            delete_list(&node->list);
            vg_lite_hal_free(node);
        }
        break;
    }
}

void vg_lite_hal_free_os_heap(void)
{
    struct heap_node * pos, * n;
    uint32_t i;

    /* Check for valid device. */
    if(device != NULL) {
        /* Process each node. */
        for(i = 0; i < VG_SYSTEM_RESERVE_COUNT; i++) {
            for(pos = (heap_node_t *)device->heap[i].list.next,
                n = (heap_node_t *)pos->list.next;
                &pos->list != &device->heap[i].list;
                pos = n, n = (heap_node_t *)n->list.next) {
                /* Remove it from the linked list. */
                delete_list(&pos->list);
                /* Free up the memory. */
                vg_lite_hal_free(pos);
            }
        }
    }
}

/**
 * End of the VG Lite Continguous custom allocator.
 */

static void z_vg_lite_isr(const void *arg)
{
    struct vg_lite_device *vg_dev = (struct vg_lite_device *)arg;
    uint32_t flags = vg_lite_hal_peek(VG_LITE_INTR_STATUS);

    if(flags) {
        /* Combine with current interrupt flags. */
        vg_dev->int_flags |= flags;
        /* Wake up any waiters. */
        k_sem_give(&vg_dev->sync);
    }
}

static int z_vg_lite_init(void)
{
    uint32_t gpu_irq_line = DT_PROP(DT_PARENT(DEVICE_DT_GET_ONE(verisilicon_vglite)), irq);
    uint32_t gpu_base_addr = DT_REG_ADDR(DT_PARENT(DEVICE_DT_GET_ONE(verisilicon_vglite)));
    uint32_t i;

    vg_lite_hal_allocate(sizeof(struct vg_lite_device), (void **)&device);
    if(NULL == device)
        return -1;

    /* Zero out the enture structure. */
    memset(device, 0, sizeof(struct vg_lite_device));

    /* Setup register memory. **********************************************/
    device->register_base = gpu_base_addr;

    /* Initialize contiguous memory. ***************************************/
    /* Allocate the contiguous memory. */
    for(i = 0; i < VG_SYSTEM_RESERVE_COUNT; i++) {
        device->heap_size[i] = heap_size[i];
        device->contiguous[i] = (volatile void *)contiguous_mem[i];
        /* Make 64byte aligned. */
        while((((uint32_t)device->contiguous[i]) & 63) != 0) {
            device->contiguous[i] = ((unsigned char *)device->contiguous[i]) + 4;
            device->heap_size[i] -= 4;
        }

        device->virtual[i] = (void *)device->contiguous[i];
        device->physical[i] = gpu_mem_base[i] + (uint32_t)device->virtual[i];
        device->size[i] = device->heap_size[i];

        /* Create the heap. */
        INIT_LIST_HEAD(&device->heap[i].list);
        device->heap[i].free = device->size[i];

        vg_lite_hal_allocate(sizeof(heap_node_t), (void **)&node);
        if(node == NULL) {
            vg_lite_exit();
            return -1;
        }
        node->offset = 0;
        node->size = device->size[i];
        node->status = 0;
        add_list(&node->list, &device->heap[i].list);
    }

    k_sem_init(&device->sync, 0, 1);
    device->int_flags = 0;

    IRQ_CONNECT(gpu_irq_line, CONFIG_LV_Z_VG_LITE_INTERRUPT_PRIORITY, z_vg_lite_isr, device, 0);
    irq_enable(gpu_irq_line);

    return 0;
}
SYS_INIT(z_vg_lite_init, POST_KERNEL, CONFIG_LV_Z_INIT_PRIORITY);

/**
 * Below the public functions of the VG-Lite hal required by the driver
 */
void vg_lite_hal_delay(uint32_t ms)
{
    k_msleep(ms);
}

uint32_t vg_lite_hal_peek(uint32_t address)
{
    return sys_read32((mem_addr_t)(device->register_base + address));
}

void vg_lite_hal_poke(uint32_t address, uint32_t data)
{
    sys_write32(data, (mem_addr_t)(device->register_base + address));
}

int32_t vg_lite_hal_wait_interrupt(uint32_t timeout, uint32_t mask, uint32_t * value)
{
    int ret = k_sem_take(&device->sync, K_MSEC(timeout));

    if(value)
        *value = device->int_flags & mask;
    device->int_flags = 0;

    return ret;
}

vg_lite_error_t vg_lite_hal_query_mem(vg_lite_kernel_mem_t * mem)
{
    if(device != NULL) {
        mem->bytes  = device->heap[mem->pool].free;
        return VG_LITE_SUCCESS;
    }
    mem->bytes = 0;
    return VG_LITE_NO_CONTEXT;
}

vg_lite_error_t vg_lite_hal_map_memory(vg_lite_kernel_map_memory_t * node)
{
    node->logical = (void *)node->physical;
    return VG_LITE_SUCCESS;
}

vg_lite_error_t vg_lite_hal_unmap_memory(vg_lite_kernel_unmap_memory_t * node)
{
    return VG_LITE_SUCCESS;
}

void vg_lite_set_gpu_execute_state(vg_lite_gpu_execute_state_t state)
{
    device->gpu_execute_state = state;
}
