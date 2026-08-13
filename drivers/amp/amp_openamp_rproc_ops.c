/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Siemens Mobility GmbH
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/amp/amp_openamp_ops_rproc.h>

struct remoteproc *zephyr_openamp_init(struct remoteproc *rproc,
					const struct remoteproc_ops *ops, void *arg)
{
	rproc->priv = arg;
	return rproc;
}

int zephyr_openamp_config(struct remoteproc *rproc, void *data)
{
	const struct amp_full_identification *full_id = rproc->priv;
	return amp_prepare_core(full_id->dev, &full_id->core_id, data);
}

int zephyr_openamp_start(struct remoteproc *rproc)
{
	const struct amp_full_identification *full_id = rproc->priv;
	return amp_start_core(full_id->dev, &full_id->core_id);
}

int zephyr_openamp_stop(struct remoteproc *rproc)
{
	const struct amp_full_identification *full_id = rproc->priv;
	return amp_stop_core(full_id->dev, &full_id->core_id);
}
