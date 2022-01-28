// SPDX-License-Identifier: GPL-2.0
/*
 * FTNANDC024 NAND General Layer
 *
 * Copyright (C) 2019-2021 Faraday Technology Corp.
 */
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/rawnand.h>
#include <linux/mtd/partitions.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/ktime.h>
#include <linux/scatterlist.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/mtd/bbm.h>

#include "ftnandc024.h"

struct ftnandc024_nand_data *ftnandc024_init_param(void)
{
	struct ftnandc024_nand_data *data;
	data = kzalloc(sizeof(struct ftnandc024_nand_data), GFP_KERNEL);

	data->set_param = NULL;
	data->terminate = NULL;
	data->buf = NULL;

	return data;
}

