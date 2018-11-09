/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 * Copyright (c) 2015 Xilinx, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <metal/alloc.h>
#include <metal/log.h>
#include <metal/utilities.h>
#include <openamp/elf_loader.h>
#include <openamp/remoteproc.h>
#include <openamp/remoteproc_loader.h>
#include <openamp/remoteproc_virtio.h>
#include <openamp/rsc_table_parser.h>

/******************************************************************************
 *  static functions
 *****************************************************************************/
static struct loader_ops *
remoteproc_check_fw_format(const void *img_data, size_t img_len)
{
	if (img_len <= 0)
		return NULL;
	else if (elf_identify(img_data, img_len) == 0)
		return &elf_ops;
	else
		return NULL;
}

static struct remoteproc_mem *
remoteproc_get_mem(struct remoteproc *rproc, const char *name,
		   metal_phys_addr_t pa, metal_phys_addr_t da,
		   void *va, size_t size)
{
	struct metal_list *node;
	struct remoteproc_mem *mem;

	metal_list_for_each(&rproc->mems, node) {
		mem = metal_container_of(node, struct remoteproc_mem, node);
		if (name) {
			if (!strncmp(name, mem->name, sizeof(mem->name)))
				return mem;
		} else if (pa != METAL_BAD_PHYS) {
			metal_phys_addr_t pa_start, pa_end;

			pa_start = mem->pa;
			pa_end = pa_start + mem->size;
			if (pa >= pa_start && (pa + size) <= pa_end)
				return mem;
		} else if (da != METAL_BAD_PHYS) {
			metal_phys_addr_t da_start, da_end;

			da_start = mem->da;
			da_end = da_start + mem->size;
			if (da >= da_start && (da + size) <= da_end)
				return mem;
		} else if (va) {
			if (metal_io_virt_to_offset(mem->io, va) !=
			    METAL_BAD_OFFSET)
				return mem;

		} else {
			return NULL;
		}
	}
	return NULL;
}

static metal_phys_addr_t
remoteproc_datopa(struct remoteproc_mem *mem, metal_phys_addr_t da)
{
	metal_phys_addr_t pa;

	pa = mem->pa + da - mem->da;
	return pa;
}

static metal_phys_addr_t
remoteproc_patoda(struct remoteproc_mem *mem, metal_phys_addr_t pa)
{
	metal_phys_addr_t da;

	da = mem->da + pa - mem->pa;
	return da;
}

static void *remoteproc_get_rsc_table(struct remoteproc *rproc,
				      void *store,
				      struct image_store_ops *store_ops,
				      size_t offset,
				      size_t len)
{
	int ret;
	void *rsc_table = NULL;
	const void *img_data;

	/* Copy the resource table to local memory,
	 * the caller should be responsible to release the memory
	 */
	rsc_table = metal_allocate_memory(len);
	if (!rsc_table) {
		return RPROC_ERR_PTR(-RPROC_ENOMEM);
	}
	ret = store_ops->load(store, offset, len, &img_data, RPROC_LOAD_ANYADDR,
			      NULL, 1);
	if (ret < 0 || ret < (int)len || img_data == NULL) {
		metal_log(METAL_LOG_ERROR,
			  "get rsc failed: 0x%llx, 0x%llx\r\n", offset, len);
		rsc_table = RPROC_ERR_PTR(-RPROC_EINVAL);
		goto error;
	}
	memcpy(rsc_table, img_data, len);

	ret = handle_rsc_table(rproc, rsc_table, len, NULL);
	if (ret < 0) {
		rsc_table = RPROC_ERR_PTR(ret);
		goto error;
	}
	return rsc_table;

error:
	metal_free_memory(rsc_table);
	return rsc_table;
}

int remoteproc_parse_rsc_table(struct remoteproc *rproc,
			       struct resource_table *rsc_table,
			       size_t rsc_size)
{
	struct metal_io_region *io;

	io = remoteproc_get_io_with_va(rproc, (void *)rsc_table);
	return handle_rsc_table(rproc, rsc_table, rsc_size, io);
}

int remoteproc_set_rsc_table(struct remoteproc *rproc,
			     struct resource_table *rsc_table,
			     size_t rsc_size)
{
	int ret;
	struct metal_io_region *io;

	io = remoteproc_get_io_with_va(rproc, (void *)rsc_table);
	if (!io)
		return -EINVAL;
	ret = remoteproc_parse_rsc_table(rproc, rsc_table, rsc_size);
	if (!ret) {
		rproc->rsc_table = rsc_table;
		rproc->rsc_len = rsc_size;
		rproc->rsc_io = io;
	}
	return ret;

}

struct remoteproc *remoteproc_init(struct remoteproc *rproc,
				   struct remoteproc_ops *ops, void *priv)
{
	if (rproc) {
		memset(rproc, 0, sizeof (*rproc));
		rproc->state = RPROC_OFFLINE;
		metal_mutex_init(&rproc->lock);
		metal_list_init(&rproc->mems);
		metal_list_init(&rproc->vdevs);
	}
	rproc = ops->init(rproc, ops, priv);
	return rproc;
}

int remoteproc_remove(struct remoteproc *rproc)
{
	int ret;

	if (rproc) {
		metal_mutex_acquire(&rproc->lock);
		if (rproc->state == RPROC_OFFLINE)
			rproc->ops->remove(rproc);
		else
			ret = -EBUSY;
		metal_mutex_release(&rproc->lock);
	} else {
		ret = -EINVAL;
	}
	return ret;
}

int remoteproc_config(struct remoteproc *rproc, void *data)
{
	int ret = -RPROC_ENODEV;

	if (rproc) {
		metal_mutex_acquire(&rproc->lock);
		if (rproc->state == RPROC_OFFLINE) {
			/* configure operation is allowed if the state is
			 * offline or ready. This function can be called
			 * mulitple times before start the remote.
			 */
			if (rproc->ops->config)
				ret = rproc->ops->config(rproc, data);
			rproc->state = RPROC_READY;
		} else {
			ret = -RPROC_EINVAL;
		}
		metal_mutex_release(&rproc->lock);
	}
	return ret;
}

int remoteproc_start(struct remoteproc *rproc)
{
	int ret = -RPROC_ENODEV;

	if (rproc) {
		metal_mutex_acquire(&rproc->lock);
		if (rproc->state == RPROC_READY) {
			ret = rproc->ops->start(rproc);
			rproc->state = RPROC_RUNNING;
		} else {
			ret = -RPROC_EINVAL;
		}
		metal_mutex_release(&rproc->lock);
	}
	return ret;
}

int remoteproc_stop(struct remoteproc *rproc)
{
	int ret = -RPROC_ENODEV;

	if (rproc) {
		metal_mutex_acquire(&rproc->lock);
		if (rproc->state != RPROC_STOPPED &&
		    rproc->state != RPROC_OFFLINE) {
			if (rproc->ops->stop)
				ret = rproc->ops->stop(rproc);
			rproc->state = RPROC_STOPPED;
		} else {
			ret = 0;
		}
		metal_mutex_release(&rproc->lock);
	}
	return ret;
}

int remoteproc_shutdown(struct remoteproc *rproc)
{
	int ret = -RPROC_ENODEV;

	if (rproc) {
		ret = 0;
		metal_mutex_acquire(&rproc->lock);
		if (rproc->state != RPROC_OFFLINE) {
			if (rproc->state != RPROC_STOPPED) {
				if (rproc->ops->stop)
					ret = rproc->ops->stop(rproc);
			}
			if (!ret) {
				if (rproc->ops->shutdown)
					ret = rproc->ops->shutdown(rproc);
				if (!ret) {
					rproc->state = RPROC_OFFLINE;
				}
			}
		}
		metal_mutex_release(&rproc->lock);
	}
	return ret;
}

struct metal_io_region *
remoteproc_get_io_with_name(struct remoteproc *rproc,
			    const char *name)
{
	struct remoteproc_mem *mem;

	mem = remoteproc_get_mem(rproc, name,
				 METAL_BAD_PHYS, METAL_BAD_PHYS, NULL, 0);
	if (mem)
		return mem->io;
	else
		return NULL;
}

struct metal_io_region *
remoteproc_get_io_with_pa(struct remoteproc *rproc,
			  metal_phys_addr_t pa)
{
	struct remoteproc_mem *mem;

	mem = remoteproc_get_mem(rproc, NULL, pa, METAL_BAD_PHYS, NULL, 0);
	if (mem)
		return mem->io;
	else
		return NULL;
}

struct metal_io_region *
remoteproc_get_io_with_da(struct remoteproc *rproc,
			  metal_phys_addr_t da,
			  unsigned long *offset)
{
	struct remoteproc_mem *mem;

	mem = remoteproc_get_mem(rproc, NULL, METAL_BAD_PHYS, da, NULL, 0);
	if (mem) {
		struct metal_io_region *io;
		metal_phys_addr_t pa;

		io = mem->io;
		pa = remoteproc_datopa(mem, da);
		*offset = metal_io_phys_to_offset(io, pa);
		return io;
	} else {
		return NULL;
	}
}

struct metal_io_region *
remoteproc_get_io_with_va(struct remoteproc *rproc, void *va)
{
	struct remoteproc_mem *mem;

	mem = remoteproc_get_mem(rproc, NULL, METAL_BAD_PHYS, METAL_BAD_PHYS,
				 va, 0);
	if (mem)
		return mem->io;
	else
		return NULL;
}

void *remoteproc_mmap(struct remoteproc *rproc,
		      metal_phys_addr_t *pa, metal_phys_addr_t *da,
		      size_t size, unsigned int attribute,
		      struct metal_io_region **io)
{
	void *va = NULL;
	metal_phys_addr_t lpa, lda;
	struct remoteproc_mem *mem;

	if (!rproc)
		return NULL;
	else if (!pa && !da)
		return NULL;
	if (pa)
		lpa = *pa;
	else
		lpa = METAL_BAD_PHYS;
	if (da)
		lda =  *da;
	else
		lda = METAL_BAD_PHYS;
	mem = remoteproc_get_mem(rproc, NULL, lpa, lda, NULL, size);
	if (mem) {
		if (lpa != METAL_BAD_PHYS)
			lda = remoteproc_patoda(mem, lpa);
		else if (lda != METAL_BAD_PHYS)
			lpa = remoteproc_datopa(mem, lda);
		if (io)
			*io = mem->io;
		va = metal_io_phys_to_virt(mem->io, lpa);
	} else if (rproc->ops->mmap) {
		va = rproc->ops->mmap(rproc, &lpa, &lda, size, attribute, io);
	}

	if (pa)
		*pa  = lpa;
	if (da)
		*da = lda;
	return va;
}

int remoteproc_load(struct remoteproc *rproc, const char *path,
		    void *store, struct image_store_ops *store_ops,
		    void **img_info)
{
	int ret;
	struct loader_ops *loader;
	const void *img_data;
	void *limg_info = NULL;
	size_t offset, noffset;
	size_t len, nlen;
	int last_load_state;
	metal_phys_addr_t da, rsc_da;
	int rsc_len;
	size_t rsc_size;
	void *rsc_table = NULL;
	struct metal_io_region *io = NULL;

	if (!rproc)
		return -RPROC_ENODEV;

	metal_mutex_acquire(&rproc->lock);
	metal_log(METAL_LOG_DEBUG, "%s: check remoteproc status\r\n", __func__);
	/* If remoteproc is not in ready state, cannot load executable */
	if (rproc->state != RPROC_READY && rproc->state != RPROC_CONFIGURED) {
		metal_log(METAL_LOG_ERROR,
			  "load failure: invalid rproc state %d.\r\n",
			  rproc->state);
		metal_mutex_release(&rproc->lock);
		return -RPROC_EINVAL;
	}

	if (!store_ops) {
		metal_log(METAL_LOG_ERROR,
			  "load failure: loader ops is not set.\r\n");
		metal_mutex_release(&rproc->lock);
		return -RPROC_EINVAL;
	}

	/* Open exectuable to get ready to parse */
	metal_log(METAL_LOG_DEBUG, "%s: open exectuable image\r\n", __func__);
	ret = store_ops->open(store, path, &img_data);
	if (ret <= 0) {
		metal_log(METAL_LOG_ERROR,
			  "load failure: failed to open firmware %d.\n",
			  ret);
		metal_mutex_release(&rproc->lock);
		return -RPROC_EINVAL;
	}
	len = ret;
	metal_assert(img_data != NULL);

	/* Check executable format to select a parser */
	loader = rproc->loader;
	if (!loader) {
		metal_log(METAL_LOG_DEBUG, "%s: check loader\r\n", __func__);
		loader = remoteproc_check_fw_format(img_data, len);
		if (!loader) {
			metal_log(METAL_LOG_ERROR,
			       "load failure: failed to get store ops.\n");
			ret = -RPROC_EINVAL;
			goto error1;
		}
		rproc->loader = loader;
	}

	/* Load exectuable headers */
	metal_log(METAL_LOG_DEBUG, "%s: loading headers\r\n", __func__);
	offset = 0;
	last_load_state = RPROC_LOADER_NOT_READY;
	while(1) {
		ret = loader->load_header(img_data, offset, len,
					  &limg_info, last_load_state,
					  &noffset, &nlen);
		last_load_state = (unsigned int)ret;
		metal_log(METAL_LOG_DEBUG,
			  "%s, load header 0x%lx, 0x%x, next 0x%lx, 0x%x\r\n",
			  __func__, offset, len, noffset, nlen);
		if (ret < 0) {
			metal_log(METAL_LOG_ERROR,
				  "load header failed 0x%lx,%d.\r\n",
				  offset, len);

			goto error2;
		} else if ((ret & RPROC_LOADER_READY_TO_LOAD) != 0) {
			if (nlen == 0)
				break;
			else if ((noffset > (offset + len)) &&
				 (store_ops->features & SUPPORT_SEEK) == 0) {
				/* Required data is not continued, however
				 * seek is not supported, stop to load
				 * headers such as ELF section headers which
				 * is usually located to the end of image.
				 * Continue to load binary data to target
				 * memory.
				 */
				break;
			}
		}
		/* Continue to load headers image data */
		img_data = NULL;
		ret = store_ops->load(store, noffset, nlen,
				      &img_data,
				      RPROC_LOAD_ANYADDR,
				      NULL, 1);
		if (ret < (int)nlen) {
			metal_log(METAL_LOG_ERROR,
				  "load image data failed 0x%x,%d\r\n",
				  noffset, nlen);
			goto error2;
		}
		offset = noffset;
		len = nlen;
	}
	ret = elf_locate_rsc_table(limg_info, &rsc_da, &offset, &rsc_size);
	if (ret == 0 && rsc_size > 0) {
		/* parse resource table */
		rsc_len = (int)rsc_size;
		rsc_table = remoteproc_get_rsc_table(rproc, store, store_ops,
						     offset, rsc_len);
	} else {
		rsc_len = ret;
	}

	/* load executable data */
	metal_log(METAL_LOG_DEBUG, "%s: load executable data\r\n", __func__);
	offset = 0;
	len = 0;
	ret = -EINVAL;
	while(1) {
		unsigned char padding;
		size_t nmemsize;
		metal_phys_addr_t pa;

		da = RPROC_LOAD_ANYADDR;
		nlen = 0;
		nmemsize = 0;
		noffset = 0;
		ret = loader->load_data(rproc, img_data, offset, len,
					&limg_info, last_load_state, &da,
					&noffset, &nlen, &padding, &nmemsize);
		if (ret < 0) {
			metal_log(METAL_LOG_ERROR,
				  "load data failed,0x%lx,%d\r\n",
				  noffset, nlen);
			goto error3;
		}
		metal_log(METAL_LOG_DEBUG,
			  "load data: da 0x%lx, offset 0x%lx, len = 0x%lx, memsize = 0x%lx, state 0x%x\r\n",
			  da, noffset, nlen, nmemsize, ret);
		last_load_state = ret;
		if (da != RPROC_LOAD_ANYADDR) {
			/* Data is supposed to be loaded to target memory */
			img_data = NULL;
			/* get the I/O region from remoteproc */
			pa = METAL_BAD_PHYS;
			(void)remoteproc_mmap(rproc, &pa, &da, nmemsize, 0, &io);
			if (pa == METAL_BAD_PHYS || io == NULL) {
				metal_log(METAL_LOG_ERROR,
					  "load failed, no mapping for 0x%llx.\r\n",
					  da);
				ret = -RPROC_EINVAL;
				goto error3;
			}
			if (nlen > 0) {
				ret = store_ops->load(store, noffset, nlen,
						      &img_data, pa, io, 1);
				if (ret != (int)nlen) {
					metal_log(METAL_LOG_ERROR,
						  "load data failed 0x%lx, 0x%lx, 0x%x\r\n",
						  pa, noffset, nlen);
					ret = -RPROC_EINVAL;
					goto error3;
				}
			}
			if (nmemsize > nlen) {
				size_t tmpoffset;

				tmpoffset = metal_io_phys_to_offset(io,
								    pa + nlen);
				metal_io_block_set(io, tmpoffset,
						   padding, (nmemsize - nlen));
			}
		} else if (nlen != 0) {
			ret = store_ops->load(store, noffset, nlen,
					      &img_data,
					      RPROC_LOAD_ANYADDR,
					      NULL, 1);
			if (ret < (int)nlen) {
				if ((last_load_state &
				    RPROC_LOADER_POST_DATA_LOAD) != 0) {
					metal_log(METAL_LOG_WARNING,
						  "not all the headers are loaded\r\n");
					break;
				}
				metal_log(METAL_LOG_ERROR,
					  "post-load image data failed 0x%x,%d\r\n",
					  noffset, nlen);
				goto error3;
			}
			offset = noffset;
			len = nlen;
		} else {
			/* (last_load_state & RPROC_LOADER_LOAD_COMPLETE) != 0 */
			break;
		}
	}

	if (rsc_len < 0) {
		ret = elf_locate_rsc_table(limg_info, &rsc_da,
					   &offset, &rsc_size);
		if (ret == 0 && rsc_size > 0) {
			/* parse resource table */
			rsc_len = (int)rsc_size;
			rsc_table = remoteproc_get_rsc_table(rproc, store,
							     store_ops,
							     offset,
							     rsc_len);
		}
	}

	/* Update resource table */
	if (rsc_len && rsc_da != METAL_BAD_PHYS) {
		void *rsc_table_cp = rsc_table;

		metal_log(METAL_LOG_DEBUG,
			  "%s, update resource table\r\n", __func__);
		rsc_table = remoteproc_mmap(rproc, NULL, &rsc_da,
					    rsc_len, 0, &io);
		if (rsc_table) {
			size_t rsc_io_offset;

			/* Update resource table */
			rsc_io_offset = metal_io_virt_to_offset(io, rsc_table);
			ret = metal_io_block_write(io, rsc_io_offset,
						   rsc_table_cp, rsc_len);
			if (ret != rsc_len) {
				metal_log(METAL_LOG_WARNING,
					  "load: failed to update rsc\r\n");
			}
			rproc->rsc_table = rsc_table;
			rproc->rsc_len = rsc_len;
		} else {
			metal_log(METAL_LOG_WARNING,
				  "load: not able to update rsc table.\n");
		}
		metal_free_memory(rsc_table_cp);
		/* So that the rsc_table will not get released */
		rsc_table = NULL;
	}

	metal_log(METAL_LOG_DEBUG, "%s: successfully load firmware\r\n",
		  __func__);
	/* get entry point from the firmware */
	rproc->bootaddr = loader->get_entry(limg_info);
	rproc->state = RPROC_READY;

	metal_mutex_release(&rproc->lock);
	if (img_info)
		*img_info = limg_info;
	else
		loader->release(limg_info);
	store_ops->close(store);
	return 0;

error3:
	if (rsc_table)
		metal_free_memory(rsc_table);
error2:
	loader->release(limg_info);
error1:
	store_ops->close(store);
	metal_mutex_release(&rproc->lock);
	return ret;
}

int remoteproc_load_noblock(struct remoteproc *rproc,
			    const void *img_data, size_t offset, size_t len,
			    void **img_info,
			    metal_phys_addr_t *pa, struct metal_io_region **io,
			    size_t *noffset, size_t *nlen,
			    size_t *nmlen, unsigned char *padding)
{
	int ret;
	struct loader_ops *loader;
	void *limg_info = NULL;
	int last_load_state;
	metal_phys_addr_t da, rsc_da;
	size_t rsc_size;
	void *rsc_table = NULL, *lrsc_table = NULL;

	if (!rproc)
		return -RPROC_ENODEV;

	metal_assert(pa != NULL);
	metal_assert(io != NULL);
	metal_assert(noffset != NULL);
	metal_assert(nlen != NULL);
	metal_assert(nmlen != NULL);
	metal_assert(padding != NULL);

	metal_mutex_acquire(&rproc->lock);
	metal_log(METAL_LOG_DEBUG, "%s: check remoteproc status\r\n", __func__);
	/* If remoteproc is not in ready state, cannot load executable */
	if (rproc->state != RPROC_READY) {
		metal_log(METAL_LOG_ERROR,
			  "load failure: invalid rproc state %d.\r\n",
			  rproc->state);
		metal_mutex_release(&rproc->lock);
		return -RPROC_EINVAL;
	}

	/* Check executable format to select a parser */
	loader = rproc->loader;
	if (!loader) {
		metal_log(METAL_LOG_DEBUG, "%s: check loader\r\n", __func__);
		if (img_data == NULL || offset != 0 || len == 0) {
			metal_log(METAL_LOG_ERROR,
				  "load failure, invalid inputs, not able to identify image.\r\n");
			metal_mutex_release(&rproc->lock);
			return -RPROC_EINVAL;
		}
		loader = remoteproc_check_fw_format(img_data, len);
		if (!loader) {
			metal_log(METAL_LOG_ERROR,
			       "load failure: failed to identify image.\n");
			ret = -RPROC_EINVAL;
			metal_mutex_release(&rproc->lock);
			return -RPROC_EINVAL;
		}
		rproc->loader = loader;
	}
	if (img_info == NULL || *img_info == NULL ) {
		last_load_state = 0;
	} else {
		limg_info = *img_info;
		last_load_state = loader->get_load_state(limg_info);
		if (last_load_state < 0) {
			metal_log(METAL_LOG_ERROR,
				  "load failure, not able get load state.\r\n");
			metal_mutex_release(&rproc->lock);
			return -RPROC_EINVAL;
		}
	}
	da = RPROC_LOAD_ANYADDR;
	*nlen = 0;
	if ((last_load_state & RPROC_LOADER_READY_TO_LOAD) == 0 &&
	    (last_load_state & RPROC_LOADER_LOAD_COMPLETE) == 0) {
		/* Get the mandatory executable headers */
		ret = loader->load_header(img_data, offset, len,
					  &limg_info, last_load_state,
					  noffset, nlen);
		last_load_state = (unsigned int)ret;
		metal_log(METAL_LOG_DEBUG,
			  "%s, load header 0x%lx, 0x%x, next 0x%lx, 0x%x\r\n",
			  __func__, offset, len, *noffset, *nlen);
		if (ret < 0) {
			metal_log(METAL_LOG_ERROR,
				  "load header failed 0x%lx,%d.\r\n",
				  offset, len);

			goto error1;
		}
		last_load_state = loader->get_load_state(limg_info);
		if (*nlen != 0 &&
		    (last_load_state & RPROC_LOADER_READY_TO_LOAD) == 0)
			goto out;
	}
	if ((last_load_state & RPROC_LOADER_READY_TO_LOAD) != 0 ||
	    (last_load_state & RPROC_LOADER_POST_DATA_LOAD) != 0) {
		/* Enough information to know which target memory for
		 * which data.
		 */
		ret = loader->load_data(rproc, img_data, offset, len,
					&limg_info, last_load_state, &da,
					noffset, nlen, padding, nmlen);
		metal_log(METAL_LOG_DEBUG,
			  "%s, load data 0x%lx, 0x%x, next 0x%lx, 0x%x\r\n",
			  __func__, offset, len, *noffset, *nlen);
		if (ret < 0) {
			metal_log(METAL_LOG_ERROR,
				  "load data failed,0x%lx,%d\r\n",
				  offset, len);
			goto error1;
		}
		if (da != RPROC_LOAD_ANYADDR) {
			/* get the I/O region from remoteproc */
			*pa = METAL_BAD_PHYS;
			(void)remoteproc_mmap(rproc, pa, &da, *nmlen, 0, io);
			if (*pa == METAL_BAD_PHYS || io == NULL) {
				metal_log(METAL_LOG_ERROR,
					  "load failed, no mapping for 0x%llx.\r\n",
					  da);
				ret = -RPROC_EINVAL;
				goto error1;
			}
		}
		if (*nlen != 0)
			goto out;
		else
			last_load_state = loader->get_load_state(limg_info);
	}
	if ((last_load_state & RPROC_LOADER_LOAD_COMPLETE) != 0) {
		/* Get resource table */
		size_t rsc_offset;
		size_t rsc_io_offset;

		ret = elf_locate_rsc_table(limg_info, &rsc_da,
					   &rsc_offset, &rsc_size);
		if (ret == 0 && rsc_size > 0) {
			lrsc_table = metal_allocate_memory(rsc_size);
			if (lrsc_table == NULL) {
				ret = -RPROC_ENOMEM;
				goto error1;
			}
			rsc_table = remoteproc_mmap(rproc, NULL, &rsc_da,
						    rsc_size, 0, io);
			if (*io == NULL) {
				metal_log(METAL_LOG_ERROR,
					  "load failed: failed to mmap rsc\r\n");
				metal_free_memory(lrsc_table);
				goto error1;
			}
			rsc_io_offset = metal_io_virt_to_offset(*io, rsc_table);
			ret = metal_io_block_read(*io, rsc_io_offset,
						  lrsc_table, (int)rsc_size);
			if (ret != (int)rsc_size) {
				metal_log(METAL_LOG_ERROR,
					  "load failed: failed to get rsc\r\n");
				metal_free_memory(lrsc_table);
				goto error1;
			}
			/* parse resource table */
			ret = remoteproc_parse_rsc_table(rproc, lrsc_table,
							 rsc_size);
			if (ret == (int)rsc_size) {
				metal_log(METAL_LOG_ERROR,
					  "load failed: failed to parse rsc\r\n");
				metal_free_memory(lrsc_table);
				goto error1;
			}
			/* Update resource table */
			ret = metal_io_block_write(*io, rsc_io_offset,
						  lrsc_table, (int)rsc_size);
			if (ret != (int)rsc_size) {
				metal_log(METAL_LOG_WARNING,
					  "load exectuable, failed to update rsc\r\n");
			}
			rproc->rsc_table = rsc_table;
			rproc->rsc_len = (int)rsc_size;
			metal_free_memory(lrsc_table);
		}
	}
out:
	if (img_info != NULL)
		*img_info = limg_info;
	else
		loader->release(limg_info);
	metal_mutex_release(&rproc->lock);
	return 0;

error1:
	loader->release(limg_info);
	metal_mutex_release(&rproc->lock);
	return ret;
}

unsigned int remoteproc_allocate_id(struct remoteproc *rproc,
				    unsigned int start,
				    unsigned int end)
{
	unsigned int notifyid;

	if (start == RSC_NOTIFY_ID_ANY)
		start = 0;
	if (end == RSC_NOTIFY_ID_ANY)
		end = METAL_BITS_PER_ULONG;
	notifyid = metal_bitmap_next_set_bit(&rproc->bitmap,
					     start, end);
	if (notifyid != end)
		metal_bitmap_set_bit(&rproc->bitmap, notifyid);
	return notifyid;
}

static int remoteproc_virtio_notify(void *priv, uint32_t id)
{
	struct remoteproc *rproc = priv;

	return rproc->ops->notify(rproc, id);
}

struct virtio_device *
remoteproc_create_virtio(struct remoteproc *rproc,
			 int vdev_id, unsigned int role,
			 void (*rst_cb)(struct virtio_device *vdev))
{
	char *rsc_table;
	struct fw_rsc_vdev *vdev_rsc;
	struct metal_io_region *vdev_rsc_io;
	struct virtio_device *vdev;
	struct remoteproc_virtio *rpvdev;
	size_t vdev_rsc_offset;
	unsigned int notifyid;
	unsigned int num_vrings, i;
	struct metal_list *node;

	metal_assert(rproc);
	metal_mutex_acquire(&rproc->lock);
	rsc_table = rproc->rsc_table;
	vdev_rsc_io = rproc->rsc_io;
	vdev_rsc_offset = find_rsc(rsc_table, RSC_VDEV, vdev_id);
	if (!vdev_rsc_offset) {
		metal_mutex_release(&rproc->lock);
		return NULL;
	}
	vdev_rsc = (struct fw_rsc_vdev *)(rsc_table + vdev_rsc_offset);
	notifyid = vdev_rsc->notifyid;
	/* Check if the virtio device is already created */
	metal_list_for_each(&rproc->vdevs, node) {
		rpvdev = metal_container_of(node, struct remoteproc_virtio,
					    node);
		if (rpvdev->vdev.index == notifyid)
			return &rpvdev->vdev;
	}
	vdev = rproc_virtio_create_vdev(role, notifyid,
					vdev_rsc, vdev_rsc_io, rproc,
					remoteproc_virtio_notify,
					rst_cb);
	rpvdev = metal_container_of(vdev, struct remoteproc_virtio, vdev);
	metal_list_add_tail(&rproc->vdevs, &rpvdev->node);
	num_vrings = vdev_rsc->num_of_vrings;
	/* set the notification id for vrings */
	for (i = 0; i < num_vrings; i++) {
		struct fw_rsc_vdev_vring *vring_rsc;
		metal_phys_addr_t da;
		unsigned int num_descs, align;
		struct metal_io_region *io;
		void *va;
		size_t size;
		int ret;

		vring_rsc = &vdev_rsc->vring[i];
		notifyid = vring_rsc->notifyid;
		da = vring_rsc->da;
		num_descs = vring_rsc->num;
		align = vring_rsc->align;
		size = vring_size(num_descs, align);
		va = remoteproc_mmap(rproc, NULL, &da, size, 0, &io);
		if (!va)
			goto err1;
		ret = rproc_virtio_init_vring(vdev, i, notifyid,
					      va, io, num_descs, align);
		if (ret)
			goto err1;
	}
	metal_mutex_release(&rproc->lock);
	return vdev;

err1:
	remoteproc_remove_virtio(rproc, vdev);
	metal_mutex_release(&rproc->lock);
	return NULL;
}

void remoteproc_remove_virtio(struct remoteproc *rproc,
			      struct virtio_device *vdev)
{
	struct remoteproc_virtio *rpvdev;

	(void)rproc;
	metal_assert(vdev);
	rpvdev = metal_container_of(vdev, struct remoteproc_virtio, vdev);
	metal_list_del(&rpvdev->node);
	rproc_virtio_remove_vdev(&rpvdev->vdev);
}

int remoteproc_get_notification(struct remoteproc *rproc, uint32_t notifyid)
{
	struct remoteproc_virtio *rpvdev;
	struct metal_list *node;
	int ret;

	metal_list_for_each(&rproc->vdevs, node) {
		rpvdev = metal_container_of(node, struct remoteproc_virtio,
					    node);
		ret = rproc_virtio_notified(&rpvdev->vdev, notifyid);
		if (ret)
			return ret;
	}
	return 0;
}
