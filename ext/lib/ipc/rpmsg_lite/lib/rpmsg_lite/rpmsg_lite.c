/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * Copyright (c) 2015 Xilinx, Inc.
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright 2016 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "rpmsg_lite.h"
#include "rpmsg_platform.h"
#include "rpmsg_env.h"

RL_PACKED_BEGIN
/*!
 * Common header for all rpmsg messages.
 * Every message sent/received on the rpmsg bus begins with this header.
 */
struct rpmsg_std_hdr
{
    unsigned long src;      /*!< source endpoint address */
    unsigned long dst;      /*!< destination endpoint address */
    unsigned long reserved; /*!< reserved for future use */
    unsigned short len;     /*!< length of payload (in bytes) */
    unsigned short flags;   /*!< message flags */
} RL_PACKED_END;

RL_PACKED_BEGIN
/*!
 * Common message structure.
 * Contains the header and the payload.
 */
struct rpmsg_std_msg
{
    struct rpmsg_std_hdr hdr; /*!< RPMsg message header */
    unsigned char data[1];    /*!< bytes of message payload data */
} RL_PACKED_END;

/* rpmsg_std_hdr contains a reserved field,
 * this implementation of RPMSG uses this reserved
 * field to hold the idx and totlen of the buffer
 * not being returned to the vring in the receive
 * callback function. This way, the no-copy API
 * can use this field to return the buffer later.
 */
struct rpmsg_hdr_reserved
{
    unsigned short rfu; /* reserved for future usage */
    unsigned short idx;
};

/* Interface which is used to interact with the virtqueue layer,
 * a different interface is used, when the local processor is the MASTER
 * and when it is the REMOTE.
 */
struct virtqueue_ops
{
    void (*vq_tx)(struct virtqueue *vq, void *buffer, unsigned long len, unsigned short idx);
    void *(*vq_tx_alloc)(struct virtqueue *vq, unsigned long *len, unsigned short *idx);
    void *(*vq_rx)(struct virtqueue *vq, unsigned long *len, unsigned short *idx);
    void (*vq_rx_free)(struct virtqueue *vq, void *buffer, unsigned long len, unsigned short idx);
};

/* Zero-Copy extension macros */
#define RPMSG_STD_MSG_FROM_BUF(buf) (struct rpmsg_std_msg *)((char *)buf - offsetof(struct rpmsg_std_msg, data))

#if (!RL_BUFFER_COUNT) || (RL_BUFFER_COUNT & (RL_BUFFER_COUNT - 1))
#error "RL_BUFFER_COUNT must be power of two (2, 4, ...)"
#endif

/* Buffer is formed by payload and struct rpmsg_std_hdr */
#define RL_BUFFER_SIZE (RL_BUFFER_PAYLOAD_SIZE + 16)

#if (!RL_BUFFER_SIZE) || (RL_BUFFER_SIZE & (RL_BUFFER_SIZE - 1))
#error "RL_BUFFER_SIZE must be power of two (256, 512, ...)"\
       "RL_BUFFER_PAYLOAD_SIZE must be equal to (240, 496, 1008, ...) [2^n - 16]."
#endif


/*!
 * @brief
 * Create a new rpmsg endpoint, which can be used
 * for communication.
 *
 * @param rpmsg_lite_dev    RPMsg Lite instance
 * @param addr              Local endpoint address
 *
 * @return       RL_NULL if not found, node pointer containing the ept on success
 *
 */
struct llist *rpmsg_lite_get_endpoint_from_addr(struct rpmsg_lite_instance *rpmsg_lite_dev, unsigned long addr)
{
    struct llist *rl_ept_lut_head;

    rl_ept_lut_head = rpmsg_lite_dev->rl_endpoints;
    while (rl_ept_lut_head)
    {
        struct rpmsg_lite_endpoint *rl_ept = (struct rpmsg_lite_endpoint *)rl_ept_lut_head->data;
        if (rl_ept->addr == addr)
        {
            return rl_ept_lut_head;
        }
        rl_ept_lut_head = rl_ept_lut_head->next;
    }
    return RL_NULL;
}

/***************************************************************
   mmm    mm   m      m      mmmmm    mm     mmm  m    m  mmmm
 m"   "   ##   #      #      #    #   ##   m"   " #  m"  #"   "
 #       #  #  #      #      #mmmm"  #  #  #      #m#    "#mmm
 #       #mm#  #      #      #    #  #mm#  #      #  #m      "#
  "mmm" #    # #mmmmm #mmmmm #mmmm" #    #  "mmm" #   "m "mmm#"
****************************************************************/

/*!
 * @brief
 * Called when remote side calls virtqueue_kick()
 * at its transmit virtqueue.
 * In this callback, the buffer is read-out
 * of the rvq and user callback is called.
 *
 * @param vq  Virtqueue affected by the kick
 *
 */
static void rpmsg_lite_rx_callback(struct virtqueue *vq)
{
    struct rpmsg_std_msg *rpmsg_msg;
    unsigned long len;
    unsigned short idx;
    struct rpmsg_lite_endpoint *ept;
    int cb_ret;
    struct llist *node;
    struct rpmsg_hdr_reserved *rsvd;
    struct rpmsg_lite_instance *rpmsg_lite_dev = (struct rpmsg_lite_instance *)vq->priv;

    RL_ASSERT(rpmsg_lite_dev != NULL);

    /* Process the received data from remote node */
    rpmsg_msg = (struct rpmsg_std_msg *)rpmsg_lite_dev->vq_ops->vq_rx(rpmsg_lite_dev->rvq, &len, &idx);

    while (rpmsg_msg)
    {
        node = rpmsg_lite_get_endpoint_from_addr(rpmsg_lite_dev, rpmsg_msg->hdr.dst);

        cb_ret = RL_RELEASE;
        if (node != RL_NULL)
        {
            ept = (struct rpmsg_lite_endpoint *)node->data;
            cb_ret = ept->rx_cb(rpmsg_msg->data, rpmsg_msg->hdr.len, rpmsg_msg->hdr.src, ept->rx_cb_data);
        }

        if (cb_ret == RL_HOLD)
        {
            rsvd = (struct rpmsg_hdr_reserved *)&rpmsg_msg->hdr.reserved;
            rsvd->idx = idx;
        }
        else
        {
            rpmsg_lite_dev->vq_ops->vq_rx_free(rpmsg_lite_dev->rvq, rpmsg_msg, len, idx);
        }
        rpmsg_msg = (struct rpmsg_std_msg *)rpmsg_lite_dev->vq_ops->vq_rx(rpmsg_lite_dev->rvq, &len, &idx);
    }
}

/*!
 * @brief
 * Called when remote side calls virtqueue_kick()
 * at its receive virtqueue.
 *
 * @param vq  Virtqueue affected by the kick
 *
 */
static void rpmsg_lite_tx_callback(struct virtqueue *vq)
{
    struct rpmsg_lite_instance *rpmsg_lite_dev = (struct rpmsg_lite_instance *)vq->priv;

    RL_ASSERT(rpmsg_lite_dev != NULL);
    rpmsg_lite_dev->link_state = 1;
}

/****************************************************************************

 m    m  mmmm         m    m   mm   mm   m mmmm   m      mmmmm  mm   m   mmm
 "m  m" m"  "m        #    #   ##   #"m  # #   "m #        #    #"m  # m"   "
  #  #  #    #        #mmmm#  #  #  # #m # #    # #        #    # #m # #   mm
  "mm"  #    #        #    #  #mm#  #  # # #    # #        #    #  # # #    #
   ##    #mm#"        #    # #    # #   ## #mmm"  #mmmmm mm#mm  #   ##  "mmm"
            #
 In case this processor has the REMOTE role
*****************************************************************************/
/*!
 * @brief
 * Places buffer on the virtqueue for consumption by the other side.
 *
 * @param vq      Virtqueue to use
 * @param buffer  Buffer pointer
 * @param len     Buffer length
 * @idx           Buffer index
 *
 * @return Status of function execution
 *
 */
static void vq_tx_remote(struct virtqueue *tvq, void *buffer, unsigned long len, unsigned short idx)
{
    int status;
    status = virtqueue_add_consumed_buffer(tvq, idx, len);
    RL_ASSERT(status == VQUEUE_SUCCESS); /* must success here */

    /* As long as the length of the virtqueue ring buffer is not shorter
     * than the number of buffers in the pool, this function should not fail.
     * This condition is always met, so we don't need to return anything here */
}

/*!
 * @brief
 * Provides buffer to transmit messages.
 *
 * @param vq      Virtqueue to use
 * @param len     Length of returned buffer
 * @param idx     Buffer index
 *
 * return Pointer to buffer.
 */
static void *vq_tx_alloc_remote(struct virtqueue *tvq, unsigned long *len, unsigned short *idx)
{
    return virtqueue_get_available_buffer(tvq, idx, (uint32_t *)len);
}

/*!
 * @brief
 * Retrieves the received buffer from the virtqueue.
 *
 * @param vq   Virtqueue to use
 * @param len  Size of received buffer
 * @param idx  Index of buffer
 *
 * @return  Pointer to received buffer
 *
 */
static void *vq_rx_remote(struct virtqueue *rvq, unsigned long *len, unsigned short *idx)
{
    return virtqueue_get_available_buffer(rvq, idx, (uint32_t *)len);
}

/*!
 * @brief
 * Places the used buffer back on the virtqueue.
 *
 * @param vq   Virtqueue to use
 * @param len  Size of received buffer
 * @param idx  Index of buffer
 *
 */
static void vq_rx_free_remote(struct virtqueue *rvq, void *buffer, unsigned long len, unsigned short idx)
{
    int status;
#if defined(RL_CLEAR_USED_BUFFERS) && (RL_CLEAR_USED_BUFFERS == 1)
    env_memset(buffer, 0x00, len);
#endif
    status = virtqueue_add_consumed_buffer(rvq, idx, len);
    RL_ASSERT(status == VQUEUE_SUCCESS); /* must success here */
                                         /* As long as the length of the virtqueue ring buffer is not shorter
                                          * than the number of buffers in the pool, this function should not fail.
                                          * This condition is always met, so we don't need to return anything here */
}

/****************************************************************************

 m    m  mmmm         m    m   mm   mm   m mmmm   m      mmmmm  mm   m   mmm
 "m  m" m"  "m        #    #   ##   #"m  # #   "m #        #    #"m  # m"   "
  #  #  #    #        #mmmm#  #  #  # #m # #    # #        #    # #m # #   mm
  "mm"  #    #        #    #  #mm#  #  # # #    # #        #    #  # # #    #
   ##    #mm#"        #    # #    # #   ## #mmm"  #mmmmm mm#mm  #   ##  "mmm"
            #
 In case this processor has the MASTER role
*****************************************************************************/

/*!
 * @brief
 * Places buffer on the virtqueue for consumption by the other side.
 *
 * @param vq      Virtqueue to use
 * @param buffer  Buffer pointer
 * @param len     Buffer length
 * @idx           Buffer index
 *
 * @return Status of function execution
 *
 */
static void vq_tx_master(struct virtqueue *tvq, void *buffer, unsigned long len, unsigned short idx)
{
    int status;
    status = virtqueue_add_buffer(tvq, idx);
    RL_ASSERT(status == VQUEUE_SUCCESS); /* must success here */

    /* As long as the length of the virtqueue ring buffer is not shorter
     * than the number of buffers in the pool, this function should not fail.
     * This condition is always met, so we don't need to return anything here */
}

/*!
 * @brief
 * Provides buffer to transmit messages.
 *
 * @param vq      Virtqueue to use
 * @param len     Length of returned buffer
 * @param idx     Buffer index
 *
 * return Pointer to buffer.
 */
static void *vq_tx_alloc_master(struct virtqueue *tvq, unsigned long *len, unsigned short *idx)
{
    return virtqueue_get_buffer(tvq, (uint32_t *)len, idx);
}

/*!
 * @brief
 * Retrieves the received buffer from the virtqueue.
 *
 * @param vq   Virtqueue to use
 * @param len  Size of received buffer
 * @param idx  Index of buffer
 *
 * @return  Pointer to received buffer
 *
 */
static void *vq_rx_master(struct virtqueue *rvq, unsigned long *len, unsigned short *idx)
{
    return virtqueue_get_buffer(rvq, (uint32_t *)len, idx);
}

/*!
 * @brief
 * Places the used buffer back on the virtqueue.
 *
 * @param vq   Virtqueue to use
 * @param len  Size of received buffer
 * @param idx  Index of buffer
 *
 */
static void vq_rx_free_master(struct virtqueue *rvq, void *buffer, unsigned long len, unsigned short idx)
{
    int status;
#if defined(RL_CLEAR_USED_BUFFERS) && (RL_CLEAR_USED_BUFFERS == 1)
    env_memset(buffer, 0x00, len);
#endif
    status = virtqueue_add_buffer(rvq, idx);
    RL_ASSERT(status == VQUEUE_SUCCESS); /* must success here */

    /* As long as the length of the virtqueue ring buffer is not shorter
     * than the number of buffers in the pool, this function should not fail.
     * This condition is always met, so we don't need to return anything here */
}

/* Interface used in case this processor is MASTER */
static const struct virtqueue_ops master_vq_ops = {
    vq_tx_master, vq_tx_alloc_master, vq_rx_master, vq_rx_free_master,
};

/* Interface used in case this processor is REMOTE */
static const struct virtqueue_ops remote_vq_ops = {
    vq_tx_remote, vq_tx_alloc_remote, vq_rx_remote, vq_rx_free_remote,
};

/* helper function for virtqueue notification */
void virtqueue_notify(struct virtqueue *vq)
{
    platform_notify(vq->vq_queue_index);
}

/*************************************************

 mmmmmm mmmmm mmmmmmm        mm   m mmmmmmm     m
 #      #   "#   #           #"m  # #     #  #  #
 #mmmmm #mmm#"   #           # #m # #mmmmm" #"# #
 #      #        #           #  # # #      ## ##"
 #mmmmm #        #           #   ## #mmmmm #   #

**************************************************/
#if defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1)
struct rpmsg_lite_endpoint *rpmsg_lite_create_ept(struct rpmsg_lite_instance *rpmsg_lite_dev,
                                                  unsigned long addr,
                                                  rl_ept_rx_cb_t rx_cb,
                                                  void *rx_cb_data,
                                                  struct rpmsg_lite_ept_static_context *ept_context)
#else
struct rpmsg_lite_endpoint *rpmsg_lite_create_ept(struct rpmsg_lite_instance *rpmsg_lite_dev,
                                                  unsigned long addr,
                                                  rl_ept_rx_cb_t rx_cb,
                                                  void *rx_cb_data)
#endif
{
    struct rpmsg_lite_endpoint *rl_ept;
    struct llist *node;
    unsigned int i;
    
    if (!rpmsg_lite_dev)
        return RL_NULL;

    env_lock_mutex(rpmsg_lite_dev->lock);
    {
        if (addr == RL_ADDR_ANY)
        {
            /* find lowest free address */
            for (i = 1; i < 0xFFFFFFFF; i++)
            {
                if (rpmsg_lite_get_endpoint_from_addr(rpmsg_lite_dev, i) == RL_NULL)
                {
                    addr = i;
                    break;
                }
            }
            if (addr == RL_ADDR_ANY)
            {
                /* no address is free, cannot happen normally */
                env_unlock_mutex(rpmsg_lite_dev->lock);
                return RL_NULL;
            }
        }
        else
        {
            if (rpmsg_lite_get_endpoint_from_addr(rpmsg_lite_dev, addr) != RL_NULL)
            {
                /* Already exists! */
                env_unlock_mutex(rpmsg_lite_dev->lock);
                return RL_NULL;
            }
        }

#if defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1)
        if (!ept_context)
        {
            env_unlock_mutex(rpmsg_lite_dev->lock);
            return RL_NULL;
        }

        rl_ept = &(ept_context->ept);
        node = &(ept_context->node);
#else
        rl_ept = env_allocate_memory(sizeof(struct rpmsg_lite_endpoint));
        if (!rl_ept)
        {
            env_unlock_mutex(rpmsg_lite_dev->lock);
            return RL_NULL;
        }
        node = env_allocate_memory(sizeof(struct llist));
        if (!node)
        {
            env_free_memory(rl_ept);
            env_unlock_mutex(rpmsg_lite_dev->lock);
            return RL_NULL;
        }
#endif /* RL_USE_STATIC_API */

        env_memset(rl_ept, 0x00, sizeof(struct rpmsg_lite_endpoint));

        rl_ept->addr = addr;
        rl_ept->rx_cb = rx_cb;
        rl_ept->rx_cb_data = rx_cb_data;

        node->data = rl_ept;

        add_to_list((struct llist **)&rpmsg_lite_dev->rl_endpoints, node);
    }
    env_unlock_mutex(rpmsg_lite_dev->lock);

    return rl_ept;
}
/*************************************************

 mmmmmm mmmmm mmmmmmm        mmmm   mmmmmm m
 #      #   "#   #           #   "m #      #
 #mmmmm #mmm#"   #           #    # #mmmmm #
 #      #        #           #    # #      #
 #mmmmm #        #           #mmm"  #mmmmm #mmmmm

**************************************************/

int rpmsg_lite_destroy_ept(struct rpmsg_lite_instance *rpmsg_lite_dev, struct rpmsg_lite_endpoint *rl_ept)
{
    struct llist *node;

    if (!rpmsg_lite_dev)
        return RL_ERR_PARAM;
    
    if (!rl_ept)
        return RL_ERR_PARAM;
    
    env_lock_mutex(rpmsg_lite_dev->lock);
    node = rpmsg_lite_get_endpoint_from_addr(rpmsg_lite_dev, rl_ept->addr);
    if (node)
    {
        remove_from_list((struct llist **)&rpmsg_lite_dev->rl_endpoints, node);
        env_unlock_mutex(rpmsg_lite_dev->lock);
#if !(defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1))
        env_free_memory(node);
        env_free_memory(rl_ept);
#endif
        return RL_SUCCESS;
    }
    else
    {
        env_unlock_mutex(rpmsg_lite_dev->lock);
        return RL_ERR_PARAM;
    }
}

/******************************************

mmmmmmm m    m          mm   mmmmm  mmmmm
   #     #  #           ##   #   "#   #
   #      ##           #  #  #mmm#"   #
   #     m""m          #mm#  #        #
   #    m"  "m        #    # #      mm#mm

*******************************************/

int rpmsg_lite_is_link_up(struct rpmsg_lite_instance *rpmsg_lite_dev)
{
    if (!rpmsg_lite_dev)
        return 0;

    return rpmsg_lite_dev->link_state;
}

/*!
 * @brief
 * Internal function to format a RPMsg compatible
 * message and sends it
 *
 * @param rpmsg_lite_dev    RPMsg Lite instance
 * @param src               Local endpoint address
 * @param dst               Remote endpoint address
 * @param data              Payload buffer
 * @param size              Size of payload, in bytes
 * @param flags             Value of flags field
 * @param timeout           Timeout in ms, 0 if nonblocking
 *
 * @return  Status of function execution, RL_SUCCESS on success
 *
 */
int rpmsg_lite_format_message(struct rpmsg_lite_instance *rpmsg_lite_dev,
                              unsigned long src,
                              unsigned long dst,
                              char *data,
                              unsigned long size,
                              int flags,
                              unsigned long timeout)
{
    struct rpmsg_std_msg *rpmsg_msg;
    void *buffer;
    unsigned short idx;
    unsigned long tick_count = 0;
    unsigned long buff_len;

    if (!rpmsg_lite_dev)
        return RL_ERR_PARAM;

    if (!data)
        return RL_ERR_PARAM;

    if (!rpmsg_lite_dev->link_state)
        return RL_NOT_READY;

    /* Lock the device to enable exclusive access to virtqueues */
    env_lock_mutex(rpmsg_lite_dev->lock);
    /* Get rpmsg buffer for sending message. */
    buffer = rpmsg_lite_dev->vq_ops->vq_tx_alloc(rpmsg_lite_dev->tvq, &buff_len, &idx);
    env_unlock_mutex(rpmsg_lite_dev->lock);

    if (!buffer && !timeout)
        return RL_ERR_NO_MEM;

    while (!buffer)
    {
        env_sleep_msec(RL_MS_PER_INTERVAL);
        env_lock_mutex(rpmsg_lite_dev->lock);
        buffer = rpmsg_lite_dev->vq_ops->vq_tx_alloc(rpmsg_lite_dev->tvq, &buff_len, &idx);
        env_unlock_mutex(rpmsg_lite_dev->lock);
        tick_count += RL_MS_PER_INTERVAL;
        if ((tick_count >= timeout) && (!buffer))
        {
            return RL_ERR_NO_MEM;
        }
    }

    rpmsg_msg = (struct rpmsg_std_msg *)buffer;

    /* Initialize RPMSG header. */
    rpmsg_msg->hdr.dst = dst;
    rpmsg_msg->hdr.src = src;
    rpmsg_msg->hdr.len = size;
    rpmsg_msg->hdr.flags = flags;

    /* Copy data to rpmsg buffer. */
    env_memcpy(rpmsg_msg->data, data, size);

    env_lock_mutex(rpmsg_lite_dev->lock);
    /* Enqueue buffer on virtqueue. */
    rpmsg_lite_dev->vq_ops->vq_tx(rpmsg_lite_dev->tvq, buffer, buff_len, idx);
    /* Let the other side know that there is a job to process. */
    virtqueue_kick(rpmsg_lite_dev->tvq);
    env_unlock_mutex(rpmsg_lite_dev->lock);

    return RL_SUCCESS;
}

int rpmsg_lite_send(struct rpmsg_lite_instance *rpmsg_lite_dev,
                    struct rpmsg_lite_endpoint *ept,
                    unsigned long dst,
                    char *data,
                    unsigned long size,
                    unsigned long timeout)
{
    if (!ept)
        return RL_ERR_PARAM;

    // FIXME : may be just copy the data size equal to buffer length and Tx it.
    if (size > RL_BUFFER_PAYLOAD_SIZE)
        return RL_ERR_BUFF_SIZE;

    return rpmsg_lite_format_message(rpmsg_lite_dev, ept->addr, dst, data, size, 0, timeout);
}

#if defined(RL_API_HAS_ZEROCOPY) && (RL_API_HAS_ZEROCOPY == 1)

void *rpmsg_lite_alloc_tx_buffer(struct rpmsg_lite_instance *rpmsg_lite_dev, unsigned long *size, unsigned long timeout)
{
    struct rpmsg_std_msg *rpmsg_msg;
    struct rpmsg_hdr_reserved *reserved = RL_NULL;
    void *buffer;
    unsigned short idx;
    unsigned int tick_count = 0;

    if (!size)
        return NULL;

    if (!rpmsg_lite_dev->link_state)
    {
        *size = 0;
        return NULL;
    }

    /* Lock the device to enable exclusive access to virtqueues */
    env_lock_mutex(rpmsg_lite_dev->lock);
    /* Get rpmsg buffer for sending message. */
    buffer = rpmsg_lite_dev->vq_ops->vq_tx_alloc(rpmsg_lite_dev->tvq, size, &idx);
    env_unlock_mutex(rpmsg_lite_dev->lock);

    if (!buffer && !timeout)
    {
        *size = 0;
        return NULL;
    }

    while (!buffer)
    {
        env_sleep_msec(RL_MS_PER_INTERVAL);
        env_lock_mutex(rpmsg_lite_dev->lock);
        buffer = rpmsg_lite_dev->vq_ops->vq_tx_alloc(rpmsg_lite_dev->tvq, size, &idx);
        env_unlock_mutex(rpmsg_lite_dev->lock);
        tick_count += RL_MS_PER_INTERVAL;
        if ((tick_count >= timeout) && (!buffer))
        {
            *size = 0;
            return NULL;
        }
    }

    rpmsg_msg = (struct rpmsg_std_msg *)buffer;

    /* keep idx and totlen information for nocopy tx function */
    reserved = (struct rpmsg_hdr_reserved *)&rpmsg_msg->hdr.reserved;
    reserved->idx = idx;

    /* return the maximum payload size */
    *size -= sizeof(struct rpmsg_std_hdr);

    return rpmsg_msg->data;
}

int rpmsg_lite_send_nocopy(struct rpmsg_lite_instance *rpmsg_lite_dev,
                           struct rpmsg_lite_endpoint *ept,
                           unsigned long dst,
                           void *data,
                           unsigned long size)
{
    struct rpmsg_std_msg *rpmsg_msg;
    unsigned long src;
    struct rpmsg_hdr_reserved *reserved = RL_NULL;

    if (!ept || !data)
        return RL_ERR_PARAM;

    if (size > RL_BUFFER_PAYLOAD_SIZE)
        return RL_ERR_BUFF_SIZE;

    if (!rpmsg_lite_dev->link_state)
        return RL_NOT_READY;

    src = ept->addr;

    rpmsg_msg = RPMSG_STD_MSG_FROM_BUF(data);

    /* Initialize RPMSG header. */
    rpmsg_msg->hdr.dst = dst;
    rpmsg_msg->hdr.src = src;
    rpmsg_msg->hdr.len = size;
    rpmsg_msg->hdr.flags = 0;

    reserved = (struct rpmsg_hdr_reserved *)&rpmsg_msg->hdr.reserved;

    env_lock_mutex(rpmsg_lite_dev->lock);
    /* Enqueue buffer on virtqueue. */
    rpmsg_lite_dev->vq_ops->vq_tx(rpmsg_lite_dev->tvq, (void *)rpmsg_msg,
                                  (unsigned long)virtqueue_get_buffer_length(rpmsg_lite_dev->tvq, reserved->idx),
                                  reserved->idx);
    /* Let the other side know that there is a job to process. */
    virtqueue_kick(rpmsg_lite_dev->tvq);
    env_unlock_mutex(rpmsg_lite_dev->lock);

    return RL_SUCCESS;
}

/******************************************

 mmmmm  m    m          mm   mmmmm  mmmmm
 #   "#  #  #           ##   #   "#   #
 #mmmm"   ##           #  #  #mmm#"   #
 #   "m  m""m          #mm#  #        #
 #    " m"  "m        #    # #      mm#mm

 *******************************************/

int rpmsg_lite_release_rx_buffer(struct rpmsg_lite_instance *rpmsg_lite_dev, void *rxbuf)
{
    struct rpmsg_std_msg *rpmsg_msg;
    struct rpmsg_hdr_reserved *reserved = RL_NULL;

    if (!rpmsg_lite_dev)
        return RL_ERR_PARAM;
    if (!rxbuf)
        return RL_ERR_PARAM;

    rpmsg_msg = RPMSG_STD_MSG_FROM_BUF(rxbuf);

    /* Get the pointer to the reserved field that contains buffer size and the index */
    reserved = (struct rpmsg_hdr_reserved *)&rpmsg_msg->hdr.reserved;

    env_lock_mutex(rpmsg_lite_dev->lock);

    /* Return used buffer, with total length (header length + buffer size). */
    rpmsg_lite_dev->vq_ops->vq_rx_free(rpmsg_lite_dev->rvq, rpmsg_msg,
                                       (unsigned long)virtqueue_get_buffer_length(rpmsg_lite_dev->rvq, reserved->idx),
                                       reserved->idx);

    env_unlock_mutex(rpmsg_lite_dev->lock);

    return RL_SUCCESS;
}

#endif /* RL_API_HAS_ZEROCOPY */

/******************************

 mmmmm  mm   m mmmmm mmmmmmm
   #    #"m  #   #      #
   #    # #m #   #      #
   #    #  # #   #      #
 mm#mm  #   ## mm#mm    #

 *****************************/
#if defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1)
struct rpmsg_lite_instance *rpmsg_lite_master_init(
    void *shmem_addr, size_t shmem_length, int link_id, uint32_t init_flags, struct rpmsg_lite_instance *static_context)
#else
struct rpmsg_lite_instance *rpmsg_lite_master_init(void *shmem_addr,
                                                   size_t shmem_length,
                                                   int link_id,
                                                   uint32_t init_flags)
#endif
{
    int status;
    void (*callback[2])(struct virtqueue *vq);
    const char *vq_names[2];
    struct vring_alloc_info ring_info;
    struct virtqueue *vqs[2];
    void *buffer;
    int idx, j;
    struct rpmsg_lite_instance *rpmsg_lite_dev = NULL;

    if ((2 * RL_BUFFER_COUNT) > ((RL_WORD_ALIGN_DOWN(shmem_length - RL_VRING_OVERHEAD)) / RL_BUFFER_SIZE))
        return NULL;

    if (link_id > RL_PLATFORM_HIGHEST_LINK_ID)
        return NULL;

    if (!shmem_addr)
        return NULL;

#if defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1)
    if (!static_context)
        return NULL;
    rpmsg_lite_dev = static_context;
#else
    rpmsg_lite_dev = env_allocate_memory(sizeof(struct rpmsg_lite_instance));
    if (!rpmsg_lite_dev)
        return NULL;
#endif

    env_memset(rpmsg_lite_dev, 0, sizeof(struct rpmsg_lite_instance));

    status = env_init();
    if (status != RL_SUCCESS)
    {
#if !(defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1))
        env_free_memory(rpmsg_lite_dev);
#endif
        return NULL;
    }

    /*
     * Since device is RPMSG Remote so we need to manage the
     * shared buffers. Create shared memory pool to handle buffers.
     */
    rpmsg_lite_dev->sh_mem_base = (char *)RL_WORD_ALIGN_UP(shmem_addr + RL_VRING_OVERHEAD);
    rpmsg_lite_dev->sh_mem_remaining = (RL_WORD_ALIGN_DOWN(shmem_length - RL_VRING_OVERHEAD)) / RL_BUFFER_SIZE;
    rpmsg_lite_dev->sh_mem_total = rpmsg_lite_dev->sh_mem_remaining;

    /* Initialize names and callbacks*/
    vq_names[0] = "rx_vq";
    vq_names[1] = "tx_vq";
    callback[0] = rpmsg_lite_rx_callback;
    callback[1] = rpmsg_lite_tx_callback;
    rpmsg_lite_dev->vq_ops = &master_vq_ops;

    /* Create virtqueue for each vring. */
    for (idx = 0; idx < 2; idx++)
    {
        ring_info.phy_addr = (void *)((unsigned long)shmem_addr + (unsigned long)((idx == 0) ? (0) : (VRING_SIZE)));
        ring_info.align = VRING_ALIGN;
        ring_info.num_descs = RL_BUFFER_COUNT;

        env_memset((void *)ring_info.phy_addr, 0x00, vring_size(ring_info.num_descs, ring_info.align));

#if defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1)
        status = virtqueue_create_static(RL_GET_VQ_ID(link_id, idx), (char *)vq_names[idx], &ring_info, callback[idx],
                                         virtqueue_notify, &vqs[idx],
                                         (struct vq_static_context *)&rpmsg_lite_dev->vq_ctxt[idx]);
#else
        status = virtqueue_create(RL_GET_VQ_ID(link_id, idx), (char *)vq_names[idx], &ring_info, callback[idx],
                                  virtqueue_notify, &vqs[idx]);
#endif /* RL_USE_STATIC_API */

        if (status == RL_SUCCESS)
        {
            /* Initialize vring control block in virtqueue. */
            vq_ring_init(vqs[idx]);

            /* Disable callbacks - will be enabled by the application
            * once initialization is completed.
            */
            virtqueue_disable_cb(vqs[idx]);
        }
        else
        {
#if !(defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1))
            env_free_memory(rpmsg_lite_dev);
#endif
            return NULL;
        }

        /* virtqueue has reference to the RPMsg Lite instance */
        vqs[idx]->priv = (void *)rpmsg_lite_dev;
    }

    status = env_create_mutex((LOCK *)&rpmsg_lite_dev->lock, 1);
    if (status != RL_SUCCESS)
    {
#if !(defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1))
        env_free_memory(rpmsg_lite_dev);
#endif
        return NULL;
    }

    // FIXME - a better way to handle this , tx for master is rx for remote and vice versa.
    rpmsg_lite_dev->tvq = vqs[1];
    rpmsg_lite_dev->rvq = vqs[0];

    for (j = 0; j < 2; j++)
    {
        for (idx = 0; ((idx < vqs[j]->vq_nentries) && (idx < rpmsg_lite_dev->sh_mem_total)); idx++)
        {
            /* Initialize TX virtqueue buffers for remote device */
            buffer = rpmsg_lite_dev->sh_mem_remaining ?
                         (rpmsg_lite_dev->sh_mem_base +
                          RL_BUFFER_SIZE * (rpmsg_lite_dev->sh_mem_total - rpmsg_lite_dev->sh_mem_remaining--)) :
                         (RL_NULL);

            RL_ASSERT(buffer);

            env_memset(buffer, 0x00, RL_BUFFER_SIZE);
            if (vqs[j] == rpmsg_lite_dev->rvq)
                status = virtqueue_fill_avail_buffers(vqs[j], buffer, RL_BUFFER_SIZE);
            else if (vqs[j] == rpmsg_lite_dev->tvq)
                status = virtqueue_fill_used_buffers(vqs[j], buffer, RL_BUFFER_SIZE);
            else
                RL_ASSERT(0); /* should not happen */

            if (status != RL_SUCCESS)
            {
/* Clean up! */
#if !(defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1))
                env_free_memory(rpmsg_lite_dev);
#endif
                env_delete_mutex(rpmsg_lite_dev->lock);
                return NULL;
            }
        }
    }

    /* Install ISRs */
    platform_init_interrupt(rpmsg_lite_dev->rvq->vq_queue_index, rpmsg_lite_dev->rvq);
    platform_init_interrupt(rpmsg_lite_dev->tvq->vq_queue_index, rpmsg_lite_dev->tvq);
    env_disable_interrupt(rpmsg_lite_dev->rvq->vq_queue_index);
    env_disable_interrupt(rpmsg_lite_dev->tvq->vq_queue_index);
    rpmsg_lite_dev->link_state = 1;
    env_enable_interrupt(rpmsg_lite_dev->rvq->vq_queue_index);
    env_enable_interrupt(rpmsg_lite_dev->tvq->vq_queue_index);

    /*
     * Let the remote device know that Master is ready for
     * communication.
     */
    virtqueue_kick(rpmsg_lite_dev->rvq);

    return rpmsg_lite_dev;
}

#if defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1)
struct rpmsg_lite_instance *rpmsg_lite_remote_init(void *shmem_addr,
                                                   int link_id,
                                                   uint32_t init_flags,
                                                   struct rpmsg_lite_instance *static_context)
#else
struct rpmsg_lite_instance *rpmsg_lite_remote_init(void *shmem_addr, int link_id, uint32_t init_flags)
#endif
{
    int status;
    void (*callback[2])(struct virtqueue *vq);
    const char *vq_names[2];
    struct vring_alloc_info ring_info;
    struct virtqueue *vqs[2];
    int idx;
    struct rpmsg_lite_instance *rpmsg_lite_dev = NULL;

    if (link_id > RL_PLATFORM_HIGHEST_LINK_ID)
        return NULL;

    if (!shmem_addr)
        return NULL;

#if defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1)
    if (!static_context)
        return NULL;
    rpmsg_lite_dev = static_context;
#else
    rpmsg_lite_dev = env_allocate_memory(sizeof(struct rpmsg_lite_instance));
    if (!rpmsg_lite_dev)
        return NULL;
#endif

    env_memset(rpmsg_lite_dev, 0, sizeof(struct rpmsg_lite_instance));

    status = env_init();
    if (status != RL_SUCCESS)
    {
#if !(defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1))
        env_free_memory(rpmsg_lite_dev);
#endif
        return NULL;
    }

    vq_names[0] = "tx_vq"; /* swapped in case of remote */
    vq_names[1] = "rx_vq";
    callback[0] = rpmsg_lite_tx_callback;
    callback[1] = rpmsg_lite_rx_callback;
    rpmsg_lite_dev->vq_ops = &remote_vq_ops;

    /* Create virtqueue for each vring. */
    for (idx = 0; idx < 2; idx++)
    {
        ring_info.phy_addr = (void *)((unsigned long)shmem_addr + (unsigned long)((idx == 0) ? (0) : (VRING_SIZE)));
        ring_info.align = VRING_ALIGN;
        ring_info.num_descs = RL_BUFFER_COUNT;

#if defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1)
        status = virtqueue_create_static(RL_GET_VQ_ID(link_id, idx), (char *)vq_names[idx], &ring_info, callback[idx],
                                         virtqueue_notify, &vqs[idx],
                                         (struct vq_static_context *)&rpmsg_lite_dev->vq_ctxt[idx]);
#else
        status = virtqueue_create(RL_GET_VQ_ID(link_id, idx), (char *)vq_names[idx], &ring_info, callback[idx],
                                  virtqueue_notify, &vqs[idx]);
#endif /* RL_USE_STATIC_API */

        if (status != RL_SUCCESS)
        {
#if !(defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1))
            env_free_memory(rpmsg_lite_dev);
#endif
            return NULL;
        }

        /* virtqueue has reference to the RPMsg Lite instance */
        vqs[idx]->priv = (void *)rpmsg_lite_dev;
    }

    status = env_create_mutex((LOCK *)&rpmsg_lite_dev->lock, 1);
    if (status != RL_SUCCESS)
    {
#if !(defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1))
        env_free_memory(rpmsg_lite_dev);
#endif
        return NULL;
    }

    // FIXME - a better way to handle this , tx for master is rx for remote and vice versa.
    rpmsg_lite_dev->tvq = vqs[0];
    rpmsg_lite_dev->rvq = vqs[1];

    /* Install ISRs */
    platform_init_interrupt(rpmsg_lite_dev->rvq->vq_queue_index, rpmsg_lite_dev->rvq);
    platform_init_interrupt(rpmsg_lite_dev->tvq->vq_queue_index, rpmsg_lite_dev->tvq);
    env_disable_interrupt(rpmsg_lite_dev->rvq->vq_queue_index);
    env_disable_interrupt(rpmsg_lite_dev->tvq->vq_queue_index);
    rpmsg_lite_dev->link_state = 0;
    env_enable_interrupt(rpmsg_lite_dev->rvq->vq_queue_index);
    env_enable_interrupt(rpmsg_lite_dev->tvq->vq_queue_index);

    return rpmsg_lite_dev;
}

/*******************************************

 mmmm   mmmmmm mmmmm  mm   m mmmmm mmmmmmm
 #   "m #        #    #"m  #   #      #
 #    # #mmmmm   #    # #m #   #      #
 #    # #        #    #  # #   #      #
 #mmm"  #mmmmm mm#mm  #   ## mm#mm    #

********************************************/

int rpmsg_lite_deinit(struct rpmsg_lite_instance *rpmsg_lite_dev)
{
    if (!rpmsg_lite_dev)
        return RL_ERR_PARAM;

    if (!(rpmsg_lite_dev->rvq && rpmsg_lite_dev->tvq && rpmsg_lite_dev->lock))
    {
        /* ERROR - trying to initialize uninitialized RPMSG? */
        RL_ASSERT((rpmsg_lite_dev->rvq && rpmsg_lite_dev->tvq && rpmsg_lite_dev->lock));
        return RL_ERR_PARAM;
    }

    env_disable_interrupt(rpmsg_lite_dev->rvq->vq_queue_index);
    env_disable_interrupt(rpmsg_lite_dev->tvq->vq_queue_index);
    rpmsg_lite_dev->link_state = 0;
    env_enable_interrupt(rpmsg_lite_dev->rvq->vq_queue_index);
    env_enable_interrupt(rpmsg_lite_dev->tvq->vq_queue_index);

    platform_deinit_interrupt(rpmsg_lite_dev->rvq->vq_queue_index);
    platform_deinit_interrupt(rpmsg_lite_dev->tvq->vq_queue_index);

#if defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1)
    virtqueue_free_static(rpmsg_lite_dev->rvq);
    virtqueue_free_static(rpmsg_lite_dev->tvq);
#else
    virtqueue_free(rpmsg_lite_dev->rvq);
    virtqueue_free(rpmsg_lite_dev->tvq);
#endif /* RL_USE_STATIC_API */

    env_delete_mutex(rpmsg_lite_dev->lock);

#if !(defined(RL_USE_STATIC_API) && (RL_USE_STATIC_API == 1))
    env_free_memory(rpmsg_lite_dev);
#endif /* RL_USE_STATIC_API */

    env_deinit();

    return RL_SUCCESS;
}
