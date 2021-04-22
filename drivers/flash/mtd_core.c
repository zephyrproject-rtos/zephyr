/*
 * Copyright (c) 2021 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/flash.h>
#include <drivers/mtd.h>

#define MTD_ITEM_CONFIG(part) ((part)->cfg)
#define MTD_ITEM_STATE(part) ((part)->state)

/* Helper functions */
static int bounds_check(off_t off, size_t len, size_t size)
{
	if ((off < 0) || ((off + len) > size)) {
		return -EINVAL;
	}

	return 0;
}

static int read_arguments_check(const struct mtd_info_cfg *cfg, off_t off,
				size_t len)
{
	int rc;

	rc = bounds_check(off, len, cfg->size);

	return rc;
}

static int writeable_check(const struct mtd_info_cfg *cfg)
{
	if (cfg->read_only) {
		return -EROFS;
	}

	return 0;
}

static int write_arguments_check(const struct mtd_info_cfg *cfg, off_t off,
				 size_t len)
{
	int rc;

	rc = read_arguments_check(cfg, off, len);
	if (rc) {
		return rc;
	}

	rc = writeable_check(cfg);

	return rc;
}

static struct mtd_info *mtd_get_master(const struct mtd_info *mtd)
{
	struct mtd_info *master = (struct mtd_info *)mtd;

	while (master->cfg->parent) {
		master = (struct mtd_info *)master->cfg->parent;
	}

	return master;
}

static off_t mtd_get_master_offset(const struct mtd_info *mtd, off_t off)
{
	struct mtd_info *master = (struct mtd_info *)mtd;
	off_t ret = off;

	while (master->cfg->parent) {
		ret += master->cfg->off;
		master = (struct mtd_info *)master->cfg->parent;
	}

	ret += master->cfg->off;

	return ret;
}
/* End helper functions */

/* Public functions */
int mtd_read(const struct mtd_info *mtd, off_t off, void *dst, size_t len)
{
	if (mtd == NULL) {
		return -EINVAL;
	}

	int rc;

	rc = read_arguments_check(mtd->cfg, off, len);
	if (rc) {
		return rc;
	}

	struct mtd_info *master = mtd_get_master(mtd);

	if (!device_is_ready(master->cfg->device)) {
		return -EIO;
	}

	off = mtd_get_master_offset(mtd, off);

	return flash_read(master->cfg->device, off, dst, len);
}

int mtd_write(const struct mtd_info *mtd, off_t off, const void *src,
	      size_t len)
{
	int rc;

	if (mtd == NULL) {
		return -EINVAL;
	}

	rc = write_arguments_check(mtd->cfg, off, len);
	if (rc) {
		return rc;
	}

	struct mtd_info *master = mtd_get_master(mtd);

	if (!device_is_ready(master->cfg->device)) {
		return -EIO;
	}

	off = mtd_get_master_offset(mtd, off);

	return flash_write(master->cfg->device, off, (void *)src, len);
}

int mtd_erase(const struct mtd_info *mtd, off_t off, size_t len)
{
	int rc;

	if (mtd == NULL) {
		return -EINVAL;
	}

	rc = write_arguments_check(mtd->cfg, off, len);
	if (rc) {
		return rc;
	}

	struct mtd_info *master = mtd_get_master(mtd);

	if (!device_is_ready(master->cfg->device)) {
		return -EIO;
	}
	off = mtd_get_master_offset(mtd, off);

	return flash_erase(master->cfg->device, off, len);
}

int mtd_get_wbs(const struct mtd_info *mtd, uint8_t *wbs)
{
	if ((mtd == NULL) || (wbs == NULL)) {
		return -EINVAL;
	}

	struct mtd_info *master = mtd_get_master(mtd);

	*wbs = flash_get_write_block_size(master->cfg->device);

	return 0;
}

struct foreachpage_ctx {
	off_t start;
	off_t end;
	off_t page_start;
	uint32_t page_size;
	uint32_t page_cnt;
};

static bool foreachpage_cb(const struct flash_pages_info *info, void *ctxp)
{
	struct foreachpage_ctx *ctx = (struct foreachpage_ctx *)ctxp;

	if (ctx->end < info->start_offset) {
		return false;
	}

	if (ctx->start > info->start_offset) {
		return true;
	}

	ctx->page_start = info->start_offset;
	ctx->page_size = info->size;
	ctx->page_cnt++;
	return true;
}

int mtd_get_ebs(const struct mtd_info *mtd, size_t *ebs)
{
	if ((mtd == NULL) || (ebs == NULL)) {

		return -EINVAL;
	}

	/* erase-block-size specified in dts */
	if (mtd->cfg->erase_block_size != mtd->cfg->size) {
		*ebs = mtd->cfg->erase_block_size;
		return 0;
	}

	/* get erase-block-size from master device, only return value if
	 * erase block size is constant over the mtd
	 */
	struct mtd_info *master = mtd_get_master(mtd);
	struct foreachpage_ctx ctx;

	ctx.start = mtd_get_master_offset(mtd, 0);
	ctx.end = mtd_get_master_offset(mtd, 0) + mtd->cfg->size - 1U;
	ctx.page_cnt = 0;

	flash_page_foreach(master->cfg->device, foreachpage_cb, &ctx);
	if ((ctx.page_cnt * ctx.page_size) == mtd->cfg->size) {
		*ebs = ctx.page_size;
		return 0;
	}

	return -EINVAL;
}

int mtd_get_block_at(const struct mtd_info *mtd, off_t off,
		     struct mtd_block *block)
{
	if ((mtd == NULL) || (block == NULL)) {
		return -EINVAL;
	}

	/* erase-block-size specified in dts, use it as block-size */
	if (mtd->cfg->erase_block_size != mtd->cfg->size) {
		block->offset = off - (off % mtd->cfg->erase_block_size);
		block->size = mtd->cfg->erase_block_size;
		return 0;
	}

	/* get block-size from master device */
	struct mtd_info *master = mtd_get_master(mtd);
	struct foreachpage_ctx ctx;

	ctx.start = mtd_get_master_offset(mtd, 0);
	ctx.end = mtd_get_master_offset(mtd, off);
	ctx.page_cnt = 0;

	flash_page_foreach(master->cfg->device, foreachpage_cb, &ctx);

	if (ctx.page_cnt == 0) {
		return -EINVAL;
	}

	block->offset = ctx.page_start - mtd_get_master_offset(mtd, 0);
	block->size = ctx.page_size;
	return 0;
}

int mtd_get_edv(const struct mtd_info *mtd, uint8_t *edv)
{
	const struct flash_parameters *param;

	if ((mtd == NULL) || (edv == NULL)) {
		return -EINVAL;
	}

	const struct mtd_info_cfg *cfg = MTD_ITEM_CONFIG(mtd);

	param = flash_get_parameters(cfg->device);
	*edv = param->erase_value;

	return 0;
}
/* End public functions */

#define DT_DRV_COMPAT fixed_partitions

#define GEN_MTD_INFO(_name, _dev, _parent, _off, _size, _ebs, _ro) \
	static const struct mtd_info_cfg UTIL_CAT(_name, _cfg) = { \
		.device = _dev, \
		.parent = _parent, \
		.off = _off, \
		.size = _size, \
		.erase_block_size = _ebs, \
		.read_only = _ro, \
	}; \
	static struct mtd_info_state UTIL_CAT(_name, _state); \
	const struct mtd_info _name = { \
		.cfg = &UTIL_CAT(_name, _cfg), \
		.state = &UTIL_CAT(_name, _state), \
	};

#define GET_PART_SIZE(n) DT_PROP_OR(n, size, DT_REG_SIZE(n))

#define GET_ERASE_BLOCK_SIZE(n) \
	DT_PROP_OR(n, erase_block_size, GET_PART_SIZE(n))

#define ASSERT_ERASE_BLOCK_SIZE(n) \
	BUILD_ASSERT(GET_PART_SIZE(n) % GET_ERASE_BLOCK_SIZE(n) == 0U, \
		     "Partition size not a multiple of erase block size")

#define GET_MTD_PARTITION_STRUCT(n) UTIL_CAT(mtd_, DT_NODELABEL(n))

#define GGPARENT(n) DT_PARENT(DT_GPARENT(n))

#define GET_MTD_PARTITION_DEVICE(n) \
	COND_CODE_1(DT_NODE_HAS_COMPAT(GGPARENT(n), fixed_partitions), \
		(NULL), (DEVICE_DT_GET(DT_MTD_FROM_FIXED_PARTITION(n))))

#define GET_MTD_PARTITION_PARENT_PTR(n) \
	COND_CODE_1(DT_NODE_HAS_COMPAT(GGPARENT(n), fixed_partitions), \
		(&GET_MTD_PARTITION_STRUCT(DT_GPARENT(n))), (NULL))

#define GEN_MTD_PARTITION_STRUCT(n) \
	ASSERT_ERASE_BLOCK_SIZE(n); \
	GEN_MTD_INFO(UTIL_CAT(mtd_, DT_NODELABEL(n)), \
		     GET_MTD_PARTITION_DEVICE(n), \
		     GET_MTD_PARTITION_PARENT_PTR(n), \
		     DT_REG_ADDR(n), GET_PART_SIZE(n), \
		     GET_ERASE_BLOCK_SIZE(n), \
		     DT_PROP(n, read_only))

#define FOREACH_MTD_PARTITION(n) \
	DT_FOREACH_CHILD(DT_DRV_INST(n), GEN_MTD_PARTITION_STRUCT)

DT_INST_FOREACH_STATUS_OKAY(FOREACH_MTD_PARTITION)

#define GEN_MTD_MASTER_STRUCT(n) \
	GEN_MTD_INFO( \
		UTIL_CAT(mtd_, DT_PARENT(n)), \
		COND_CODE_1(DT_NODE_HAS_COMPAT(DT_PARENT(n), soc_nv_flash), \
			(DEVICE_DT_GET(DT_GPARENT(n))), \
			(DEVICE_DT_GET(DT_PARENT(n)))), \
		NULL, 0, GET_PART_SIZE(DT_PARENT(n)), \
		GET_ERASE_BLOCK_SIZE(DT_PARENT(n)), false \
	)

#define MTD_MASTER_FILTER(n)  COND_CODE_1( \
	DT_NODE_HAS_COMPAT(DT_PARENT(n), fixed_partitions), \
	(), (GEN_MTD_MASTER_STRUCT(n))) \

#define FOREACH_FIXEDPARTITIONS(n) \
	MTD_MASTER_FILTER(DT_INST(n, fixed_partitions))

DT_INST_FOREACH_STATUS_OKAY(FOREACH_FIXEDPARTITIONS)
