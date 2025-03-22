/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/mem_manage.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/util.h>
#include <zephyr/zvm/zvm.h>
#include <zephyr/zvm/arm/mmu.h>
#include <zephyr/zvm/os.h>
#include <zephyr/zvm/vm.h>
#include <zephyr/zvm/vm_mm.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

struct k_spinlock vm_mem_domain_lock;
static uint8_t vm_max_partitions = CONFIG_MAX_DOMAIN_PARTITIONS;
static struct k_spinlock z_vm_domain_lock;


/**
 * @brief add vpart_space to vm's unused list area.
 */
static int add_idle_vpart(struct vm_mem_domain *vmem_domain,
			struct vm_mem_partition *vpart)
{

	/* add vpart free list to mem_domain unused list */
	if (vpart->vpart_node.next == NULL) {
		sys_dlist_append(&vmem_domain->idle_vpart_list, &vpart->vpart_node);
	}

	return 0;
}

/**
 * @brief add vpart_space to vm's used list area.
 */
int add_mapped_vpart(struct vm_mem_domain *vmem_domain,
			struct vm_mem_partition *vpart)
{
	if (!vpart) {
		return -1;
	}

	sys_dlist_append(&vmem_domain->mapped_vpart_list, &vpart->vpart_node);
	return 0;
}

static struct vm_mem_partition *alloc_vm_mem_partition(uint64_t hpbase,
			uint64_t ipbase, uint64_t size, uint32_t attrs)
{
	struct vm_mem_partition *vpart;

	/* allocate memory for vpart */
	vpart = (struct vm_mem_partition *)k_malloc(sizeof(struct vm_mem_partition));
	if (!vpart) {
		return NULL;
	}
	vpart->vm_mm_partition = (struct k_mem_partition *)
			k_malloc(sizeof(struct k_mem_partition));
	if (!vpart->vm_mm_partition) {
		k_free(vpart);
		return NULL;
	}
	vpart->vm_mm_partition->start = ipbase;
	vpart->vm_mm_partition->size = size;
	vpart->vm_mm_partition->attr.attrs = attrs;

	vpart->part_hpa_base = hpbase;
	vpart->part_hpa_size = size;

	sys_dnode_init(&vpart->vpart_node);
	sys_dlist_init(&vpart->blk_list);

	return vpart;
}

/**
 * @brief init vpart from default device tree.
 */
static int create_vm_mem_vpart(struct vm_mem_domain *vmem_domain, uint64_t hpbase,
					uint64_t ipbase, uint64_t size, uint32_t attrs)
{
	int ret = 0;
	struct vm_mem_partition *vpart;

	vpart = alloc_vm_mem_partition(hpbase, ipbase, size, attrs);
	if (vpart == NULL) {
		return -ENXIO;
	}
	vpart->vmem_domain = vmem_domain;

	ret = add_idle_vpart(vmem_domain, vpart);

	return ret;
}

static int vm_ram_mem_create(struct vm_mem_domain *vmem_domain)
{
	int ret = 0;
	uint64_t va_base, pa_base, size;
	struct z_vm *vm = vmem_domain->vm;

	va_base = vm->os->info.vm_mem_base;
	size = vm->os->info.vm_mem_size;
	pa_base = vm->os->info.vm_load_base;

	ret =  create_vm_mem_vpart(vmem_domain, pa_base, va_base, size, MT_VM_NORMAL_MEM);
	return ret;
}

/**
 * @brief Create the dtb memory partition.
 */
static int vm_dtb_mem_create(struct vm_mem_domain *vmem_domain)
{
	int ret = 0;
	uint32_t vm_dtb_size = LINUX_DTB_MEM_SIZE;
	uint64_t vm_dtb_base = LINUX_DTB_MEM_BASE;

	ARG_UNUSED(ret);
	/* Attribute 'MT_VM_DEVICE_MEM' was occer a address size trap, replace with normal memory */
	return create_vm_mem_vpart(vmem_domain, vm_dtb_base, vm_dtb_base,
			vm_dtb_size, MT_VM_NORMAL_MEM);
}

static int vm_init_mem_create(struct vm_mem_domain *vmem_domain)
{
	int ret = 0;
	struct z_vm *vm = vmem_domain->vm;

	ret = vm_ram_mem_create(vmem_domain);
	if (ret) {
		return ret;
	}

#ifdef CONFIG_VM_DTB_FILE_INPUT
	if (vm->os->info.os_type == OS_TYPE_LINUX) {
		ret = vm_dtb_mem_create(vmem_domain);
	}
#endif /* CONFIG_VM_DTB_FILE_INPUT */

	return ret;
}

static int vm_domain_init(struct k_mem_domain *domain, uint8_t num_parts,
			  struct k_mem_partition *parts[], struct z_vm *vm)
{
	k_spinlock_key_t key;
	int ret = 0;
	uint32_t vmid = vm->vmid;

	if (domain == NULL) {
		ret = -EINVAL;
		goto out;
	}

	if (!(num_parts == 0U || parts != NULL)) {
		ret = -EINVAL;
		goto out;
	}

	if (!(num_parts <= vm_max_partitions)) {
		ret = -EINVAL;
		goto out;
	}

	key = k_spin_lock(&z_vm_domain_lock);
	domain->num_partitions = 0U;
	(void)memset(domain->partitions, 0, sizeof(domain->partitions));
	sys_dlist_init(&domain->mem_domain_q);

	ret = arch_vm_mem_domain_init(domain, vmid);
	k_spin_unlock(&z_vm_domain_lock, key);

out:
	return ret;
}

static bool check_vm_add_partition(struct k_mem_domain *domain,
				struct k_mem_partition *part)
{
	int i;
	uintptr_t pstart, pend, dstart, dend;

	if (part->size == 0U) {
		return false;
	}

	pstart = part->start;
	pend = part->start + part->size;

	if (pend <= pstart) {
		return false;
	}

	/* Check that this partition doesn't overlap any existing ones already
	 * in the domain
	 */
	for (i = 0; i < domain->num_partitions; i++) {
		struct k_mem_partition *dpart = &domain->partitions[i];

		if (dpart->size == 0U) {
			/* Unused slot */
			continue;
		}

		dstart = dpart->start;
		dend = dstart + dpart->size;

		if (pend > dstart && dend > pstart) {
			ZVM_LOG_WARN("Partition overlaps\n");
			ZVM_LOG_WARN(
				"addr: %p base %lx (size %zu)\n",
				part, part->start, part->size);

			ZVM_LOG_WARN(
				"existing: %p base %lx (size %zu)\n",
				dpart->start, dpart->size);
			return false;
		}
	}

	return true;
}

static int vm_mem_domain_partition_add(struct vm_mem_domain *vmem_dm,
							 struct vm_mem_partition *vpart)
{
	int p_idx;
	int ret = 0;
	uintptr_t phys_start;
	struct k_mem_domain *domain;
	struct k_mem_partition *part;
	struct z_vm *vm;
	k_spinlock_key_t key;

	phys_start = vpart->part_hpa_base;
	domain = vmem_dm->vm_mm_domain;
	part = vpart->vm_mm_partition;
	vm = vmem_dm->vm;

	if (!check_vm_add_partition(domain, part)) {
		ret = -EINVAL;
		goto out;
	}

	key = k_spin_lock(&vm_mem_domain_lock);

	for (p_idx = 0; p_idx < vm_max_partitions; p_idx++) {
		/* A zero-sized partition denotes it's a free partition */
		if (domain->partitions[p_idx].size == 0U) {
			break;
		}
	}

	if (p_idx >= vm_max_partitions) {
		ret = -ENOSPC;
		goto unlock_out;
	}
	domain->partitions[p_idx].start = part->start;
	domain->partitions[p_idx].size = part->size;
	domain->partitions[p_idx].attr = part->attr;
	domain->num_partitions++;

#ifdef CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API
	ret = arch_vm_mem_domain_partition_add(domain, p_idx, phys_start, vm->vmid);
#endif /* CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API */

unlock_out:
	k_spin_unlock(&vm_mem_domain_lock, key);

out:
	return ret;
}

static int vm_mem_domain_partition_remove(struct vm_mem_domain *vmem_dm)
{
	int p_idx;
	int ret = 0;
	uintptr_t phys_start;
	struct k_mem_domain *domain;
	struct z_vm *vm;
	k_spinlock_key_t key;

	ARG_UNUSED(phys_start);
	domain = vmem_dm->vm_mm_domain;
	vm = vmem_dm->vm;
	key = k_spin_lock(&vm_mem_domain_lock);

#ifdef CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API
	for (p_idx = 0; p_idx < vm_max_partitions; p_idx++) {
		if (domain->partitions[p_idx].size != 0U) {
			ret = arch_vm_mem_domain_partition_remove(domain, p_idx, vm->vmid);
		}
	}
#endif
	k_free(domain);
	k_spin_unlock(&vm_mem_domain_lock, key);

	return ret;
}

int vm_mem_domain_partitions_add(struct vm_mem_domain *vmem_dm)
{
	int ret = 0;
	k_spinlock_key_t key;
	struct  _dnode *d_node, *ds_node;
	struct vm_mem_partition *vpart;

	key = k_spin_lock(&vmem_dm->spin_mmlock);
	SYS_DLIST_FOR_EACH_NODE_SAFE(&vmem_dm->idle_vpart_list, d_node, ds_node) {
		vpart = CONTAINER_OF(d_node, struct vm_mem_partition, vpart_node);
		ret = vm_mem_domain_partition_add(vmem_dm, vpart);
		if (ret) {
			ZVM_LOG_ERR("vpart memory map failed:\n");
			ZVM_LOG_ERR(
				"vpart.base 0x%llx, vpart.size 0x%llx.\n",
				vpart->part_hpa_base,
				vpart->part_hpa_size);

			k_spin_unlock(&vmem_dm->spin_mmlock, key);
			return ret;
		}

		sys_dlist_remove(&vpart->vpart_node);
		sys_dlist_append(&vmem_dm->mapped_vpart_list, &vpart->vpart_node);
	}

	k_spin_unlock(&vmem_dm->spin_mmlock, key);
	return ret;
}

int vm_mem_apart_remove(struct vm_mem_domain *vmem_dm)
{
	int ret = 0;
	k_spinlock_key_t key;
	struct _dnode *d_node, *ds_node;
	struct vm_mem_partition *vpart;
	struct k_mem_partition  *vmpart;
	struct k_mem_domain  *vm_mem_dm;
	struct z_vm *vm;

	vm = vmem_dm->vm;
	key = k_spin_lock(&vmem_dm->spin_mmlock);

	vm_mem_dm = vmem_dm->vm_mm_domain;
	ret = vm_mem_domain_partition_remove(vmem_dm);
	SYS_DLIST_FOR_EACH_NODE_SAFE(&vmem_dm->mapped_vpart_list, d_node, ds_node) {
		vpart = CONTAINER_OF(d_node, struct vm_mem_partition, vpart_node);
		vmpart = vpart->vm_mm_partition;
		sys_dlist_remove(&vpart->vpart_node);
		k_free(vmpart);
		k_free(vpart);
	}

	k_spin_unlock(&vmem_dm->spin_mmlock, key);
	return ret;
}

int vm_dynmem_apart_add(struct vm_mem_domain *vmem_dm)
{
	int ret = 0;
	uint64_t vm_mem_blk_size;
	k_spinlock_key_t key;
	struct  _dnode *d_node, *ds_node;
	struct vm_mem_partition *vpart;
	struct z_vm *vm = vmem_dm->vm;

	switch (vm->os->info.os_type) {
	case OS_TYPE_LINUX:
		vm_mem_blk_size = LINUX_VM_BLOCK_SIZE;
		break;
	case OS_TYPE_ZEPHYR:
		vm_mem_blk_size = ZEPHYR_VM_BLOCK_SIZE;
		break;
	default:
		vm_mem_blk_size = DEFAULT_VM_BLOCK_SIZE;
		ZVM_LOG_WARN("Unknown os type!\n");
		break;
	}

	key = k_spin_lock(&vmem_dm->spin_mmlock);

	SYS_DLIST_FOR_EACH_NODE_SAFE(&vmem_dm->idle_vpart_list, d_node, ds_node) {
		/*TODO: Need a judge for all.*/
		vpart = CONTAINER_OF(d_node, struct vm_mem_partition, vpart_node);
		if (ret) {
			ZVM_LOG_WARN("Init vm memory failed!\n");
			return ret;
		}

		sys_dlist_remove(&vpart->vpart_node);
		sys_dlist_append(&vmem_dm->mapped_vpart_list, &vpart->vpart_node);

	}
	k_spin_unlock(&vmem_dm->spin_mmlock, key);

	return ret;
}

int vm_mem_domain_create(struct z_vm *vm)
{
	int ret;
	k_spinlock_key_t key;
	struct vm_mem_partition *vpart;

	ARG_UNUSED(vpart);
	struct vm_mem_domain *vmem_dm = vm->vmem_domain;

	/* vm'mm struct init here */
	vmem_dm = (struct vm_mem_domain *)k_malloc(sizeof(struct vm_mem_domain));
	if (!vmem_dm) {
		ZVM_LOG_WARN("Allocate mm memory for vm mm struct failed!\n");
		return -ENXIO;
	}
	vmem_dm->vm_mm_domain = (struct k_mem_domain *)k_malloc(sizeof(struct k_mem_domain));
	if (!vmem_dm->vm_mm_domain) {
		ZVM_LOG_WARN("Allocate mm memory domain failed!\n");
		return -ENXIO;
	}
	vmem_dm->is_init = false;
	ZVM_SPINLOCK_INIT(&vmem_dm->spin_mmlock);
	/* init the list of used and unused vpart */
	sys_dlist_init(&vmem_dm->idle_vpart_list);
	sys_dlist_init(&vmem_dm->mapped_vpart_list);
	ret = vm_domain_init(vmem_dm->vm_mm_domain, 0, NULL, vm);
	if (ret) {
		ZVM_LOG_WARN("Init vm domain failed!\n");
		return -ENXIO;
	}

	vmem_dm->vm = vm;
	vm->vmem_domain = vmem_dm;

	key = k_spin_lock(&vmem_dm->spin_mmlock);
	ret = vm_init_mem_create(vmem_dm);
	if (ret) {
		ZVM_LOG_WARN("Init vm areas failed!\n");
		k_spin_unlock(&vmem_dm->spin_mmlock, key);
		return ret;
	}
	k_spin_unlock(&vmem_dm->spin_mmlock, key);

	return 0;
}

int vm_vdev_mem_create(struct vm_mem_domain *vmem_domain, uint64_t hpbase,
					uint64_t ipbase, uint64_t size, uint32_t attrs)
{
	return create_vm_mem_vpart(vmem_domain, hpbase, ipbase, size, attrs);
}

uint64_t vm_gpa_to_hpa(struct z_vm *vm, uint64_t gpa, struct vm_mem_partition *vpart)
{
	struct vm_mem_domain *vmem_domain = vm->vmem_domain;
	sys_dnode_t *d_node, *ds_node;
	uint64_t vpart_gpa_start, vpart_gpa_end, vpart_hpa_start;

	SYS_DLIST_FOR_EACH_NODE_SAFE(&vmem_domain->mapped_vpart_list, d_node, ds_node) {
		vpart = CONTAINER_OF(d_node, struct vm_mem_partition, vpart_node);

		vpart_gpa_start = (uint64_t)(vpart->vm_mm_partition->start);
		vpart_gpa_end = vpart_gpa_start + ((uint64_t)vpart->vm_mm_partition->size);

		if (vpart_gpa_start <= gpa && gpa <= vpart_gpa_end) {
			vpart_hpa_start = vpart->part_hpa_base;
			return (gpa - vpart_gpa_start + vpart_hpa_start);
		}
	}
	return -ESRCH;
}

void vm_host_memory_read(uint64_t hpa, void *dst, size_t len)
{
	size_t len_actual = len;
	uint64_t *hva;

	if (len == 1) {
		len = 4;
	}
	k_mem_map_phys_bare((uint8_t **)&hva, hpa, len, K_MEM_CACHE_NONE | K_MEM_PERM_RW);
	memcpy(dst, hva, len_actual);
	k_mem_unmap_phys_bare((uint8_t *)hva, len);
}

void vm_host_memory_write(uint64_t hpa, void *src, size_t len)
{
	size_t len_actual = len;
	uint64_t *hva;

	if (len == 1) {
		len = 4;
	}
	k_mem_map_phys_bare(
		(uint8_t **)&hva, (uintptr_t)hpa, len,
		K_MEM_CACHE_NONE | K_MEM_PERM_RW);
	memcpy(hva, src, len_actual);
	k_mem_unmap_phys_bare((uint8_t *)hva, len);
}

void vm_guest_memory_read(struct z_vm *vm, uint64_t gpa, void *dst, size_t len)
{
	uint64_t hpa;
	struct vm_mem_partition *vpart;

	vpart = (struct vm_mem_partition *)k_malloc(sizeof(struct vm_mem_partition));
	if (!vpart) {
		return;
	}
	hpa = vm_gpa_to_hpa(vm, gpa, vpart);
	if (hpa < 0) {
		printk("Guest memory read: gpa to hpa failed!\n");
		return;
	}
	vm_host_memory_read(hpa, dst, len);
}

void vm_guest_memory_write(struct z_vm *vm, uint64_t gpa, void *src, size_t len)
{
	uint64_t hpa;
	struct vm_mem_partition *vpart;

	vpart = (struct vm_mem_partition *)k_malloc(sizeof(struct vm_mem_partition));
	if (!vpart) {
		return;
	}
	hpa = vm_gpa_to_hpa(vm, gpa, vpart);
	if (hpa < 0) {
		printk("Guest memory write: gpa to hpa failed!\n");
		return;
	}
	vm_host_memory_write(hpa, src, len);
}
