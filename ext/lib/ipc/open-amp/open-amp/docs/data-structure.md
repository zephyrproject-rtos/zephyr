Libmetal helper data struct
===========================
```
struct metal_io_region {
	char name[64];                      /**< I/O region name */
	void                    *virt;      /**< base virtual address */
	const metal_phys_addr_t *physmap;   /**< table of base physical address
	                                         of each of the pages in the I/O
	                                         region */
	size_t                  size;       /**< size of the I/O region */
	unsigned long           page_shift; /**< page shift of I/O region */
	metal_phys_addr_t       page_mask;  /**< page mask of I/O region */
	unsigned int            mem_flags;  /**< memory attribute of the
	                                         I/O region */
	struct metal_io_ops     ops;        /**< I/O region operations */
};


/** Libmetal device structure. */
struct metal_device {
	const char             *name;       /**< Device name */
	struct metal_bus       *bus;        /**< Bus that contains device */
	unsigned               num_regions; /**< Number of I/O regions in
	                                      device */
	struct metal_io_region regions[METAL_MAX_DEVICE_REGIONS]; /**< Array of
	                                                I/O regions in device*/
	struct metal_list      node;       /**< Node on bus' list of devices */
	int                    irq_num;    /**< Number of IRQs per device */
	void                   *irq_info;  /**< IRQ ID */
};
```

Remoteproc data struct
===========================
```
struct remoteproc {
	struct metal_device dev;       /**< Each remoteproc has a device, each device knows its memories regions */
	metal_mutex_t lock;            /**< mutex lock */
	void *rsc_table;               /**< pointer to resource table */
	size_t rsc_len;                /**< length of the resoruce table */
	struct remoteproc_ops *ops;    /**< pointer to remoteproc operation */
	metal_phys_addr_t bootaddr;    /**< boot address */
	struct loader_ops *loader_ops; /**< image loader operation */
	unsigned int state;            /**< remoteproc state */
	struct metal_list vdevs;       /**< list of vdevs  (can we limited to one for code size but linux and resource table supports multiple */
	void *priv;                    /**< remoteproc private data */
};

struct remoteproc_vdev {
	struct metal_list node;          /**< node */
	struct remoteproc *rproc;        /**< pointer to the remoteproc instance */
	struct virtio_dev;               /**< virtio device */
	uint32_t notify_id;              /**< virtio device notification ID */
	void *vdev_rsc;                  /**< pointer to the vdev space in resource table */
	struct metal_io_region *vdev_io; /**< pointer to the vdev space I/O region */ 
	int vrings_num;                  /**< number of vrings */
	struct rproc_vrings[1];          /**< vrings array */
};

struct remoteproc_vring {
	struct remoteproc_vdev *rpvdev;  /**< pointer to the remoteproc vdev */
	uint32_t notify_id;              /**< vring notify id */
	size_t len;                      /**< vring length */
	uint32_t alignment;              /**< vring alignment */
	void *va;                        /**< vring start virtual address */
	struct metal_io_region *io;      /**< pointer to the vring I/O region */
};
```

Virtio Data struct
===========================
```
struct virtio_dev {
	int index;                               /**< unique position on the virtio bus */
	struct virtio_device_id id;              /**< the device type identification (used to match it with a driver). */
	struct metal_device *dev;                /**< do we need this in virtio device ? */
	metal_spinlock lock;                     /**< spin lock */
	uint64_t features;                       /**< the features supported by both ends. */
	unsigned int role;                       /**< if it is virtio backend or front end. */
	void (*rst_cb)(struct virtio_dev *vdev); /**< user registered virtio device callback */
	void *priv;                              /**< pointer to virtio_dev private data */
	int vrings_num;                          /**< number of vrings */
	struct virtqueue vqs[1];                 /**< array of virtqueues */
};

struct virtqueue {
	char vq_name[VIRTQUEUE_MAX_NAME_SZ];    /**< virtqueue name */
	struct virtio_device *vdev;             /**< pointer to virtio device */
	uint16_t vq_queue_index;
	uint16_t vq_nentries;
	uint32_t vq_flags;
	int vq_alignment;
	int vq_ring_size;
	boolean vq_inuse;
	void *vq_ring_mem;
	void (*callback) (struct virtqueue * vq); /**< virtqueue callback */
	void (*notify) (struct virtqueue * vq);   /**< virtqueue notify remote function */
	int vq_max_indirect_size;
	int vq_indirect_mem_size;
	struct vring vq_ring;
	uint16_t vq_free_cnt;
	uint16_t vq_queued_cnt;
	struct metal_io_region *buffers_io; /**< buffers shared memory */

	/*
	 * Head of the free chain in the descriptor table. If
	 * there are no free descriptors, this will be set to
	 * VQ_RING_DESC_CHAIN_END.
	 */
	uint16_t vq_desc_head_idx;

	/*
	 * Last consumed descriptor in the used table,
	 * trails vq_ring.used->idx.
	 */
	uint16_t vq_used_cons_idx;

	/*
	 * Last consumed descriptor in the available table -
	 * used by the consumer side.
	 */
	uint16_t vq_available_idx;

	uint8_t padd;
	/*
	 * Used by the host side during callback. Cookie
	 * holds the address of buffer received from other side.
	 * Other fields in this structure are not used currently.
	 * Do we needed??/
	struct vq_desc_extra {
		void *cookie;
		struct vring_desc *indirect;
		uint32_t indirect_paddr;
		uint16_t ndescs;
	} vq_descx[0];
};

struct vring {
	unsigned int num;   /**< number of buffers of the vring */
	struct vring_desc *desc;
	struct vring_avail *avail;
	struct vring_used *used;
};
```
RPMsg Data struct
===========================
```
struct rpmsg_virtio_device {
	struct virtio_dev *vdev;           /**< pointer to the virtio device */
	struct virtqueue *rvq;             /**< pointer to receive virtqueue */
	struct virtqueue *svq;             /**< pointer to send virtqueue */
	int buffers_number;                /**< number of shared buffers */
	struct metal_io_region *shbuf_io;  /**< pointer to the shared buffer I/O region */
	void *shbuf;
	int (*new_endpoint_cb)(const char *name, uint32_t addr); /**< name service announcement user designed callback which is used for when there is a name service announcement, there is no local endpoints waiting to bind */
	struct metal_list endpoints;       /**< list of endpoints */
};

struct rpmsg_endpoint {
	char name[SERVICE_NAME_SIZE];
	struct rpmsg_virtio_dev *rvdev;                                                                           /**< pointer to the RPMsg virtio device */
	uint32_t addr;                                                                                            /**< endpoint local address */
	uint32_t dest_addr;                                                                                       /**< endpoint default target address */
	int (*cb)(struct rpmsg_endpoint *ept, void *data, struct metal_io_region *io, size_t len, uint32_t addr); /**< endpoint callback */
	void (*destroy)(struct rpmsg_endpoint *ept);                                                              /**< user registerd endpoint destory callback */
	/* Whether we need another callback for ack ns announcement? */
};
```
