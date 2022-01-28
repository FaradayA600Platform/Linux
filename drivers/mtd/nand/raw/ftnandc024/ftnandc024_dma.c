// SPDX-License-Identifier: GPL-2.0
/*
 * FTNANDC024 NAND DMA API
 *
 * Copyright (C) 2019-2021 Faraday Technology Corp.
 */
#include <linux/mtd/mtd.h>
#include <linux/mtd/rawnand.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/scatterlist.h>

#include "ftnandc024.h"

#ifdef CONFIG_FTNANDC024_USE_AHBDMA
static void ftnandc024_dma_callback(void *param)
{
	struct ftnandc024_nand_data *data = (struct ftnandc024_nand_data *)param;
	data->dma_sts = DMA_COMPLETE;
	wake_up(&data->dma_wait_queue);
}

static int ftnandc024_dma_wait(struct nand_chip *chip, u8 dir)
{
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	int wait_queue_status;

	wait_queue_status = wait_event_timeout(data->dma_wait_queue,
						data->dma_sts == DMA_COMPLETE, HZ);
	if(wait_queue_status == 0) {
		if(dir == DMA_TO_DEVICE)
			dev_err(data->dev, "Timeout(DMA write)PG:0x%x\n", data->page_addr);
		else if(dir == DMA_FROM_DEVICE)
			dev_err(data->dev, "Timeout(DMA read) PG:0x%x\n", data->page_addr);
		return 1;
	}
	return 0;
}

int ftnandc024_nand_dma_wr(struct nand_chip *chip, const uint8_t *buf)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	struct dma_chan *chan = data->dma_chan;
	struct dma_slave_config cfg;
	struct dma_async_tx_descriptor *desc;
	struct scatterlist sg;
	dma_addr_t dma_addr;
	int ret;

	data->dma_sts = DMA_ONGOING;

	// buf isn't from kmalloc or get_free_page(s)
	if(!virt_addr_valid(buf)) {
		DBGLEVEL2(ftnandc024_dbg("Buf isn't from kmalloc or get_free_page(s)\n"));
		memcpy(data->dma_buf, buf, mtd->writesize);
		sg_init_one(&sg, (void *)data->dmaaddr, mtd->writesize);
		sg_dma_address(&sg) = data->dmaaddr;
		cfg.direction = DMA_TO_DEVICE;
		cfg.src_addr = (dma_addr_t)data->dmaaddr;
		cfg.dst_addr = (dma_addr_t)data->chip.legacy.IO_ADDR_R;
		cfg.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		cfg.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		/* This must be same as the length of prefetch(128 words)*/
		cfg.src_maxburst = cfg.dst_maxburst = 128;
		if (dmaengine_slave_config(chan, &cfg)) {
			dev_err(data->dev, "slave_config(w) is failed\n");
			return 1;
		}
		
		desc = dmaengine_prep_slave_sg(chan, &sg, 1, DMA_TO_DEVICE,
				DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
		if (desc == NULL) {
			dev_err(data->dev, "prep_slave_sg(w) is failed\n");
			return 1;
		}
		desc->callback = ftnandc024_dma_callback;
		desc->callback_param = data; 
		// Start Tx
		dmaengine_submit(desc);
		dma_async_issue_pending(chan);
		// Wait for completion of DMA engine
		ret = ftnandc024_dma_wait(chip, DMA_TO_DEVICE);
	}
	else {
		DBGLEVEL2(ftnandc024_dbg("Buf is from kmalloc or get_free_page(s)\n"));
		dma_addr = dma_map_single( data->dev, (void *)buf, mtd->writesize, DMA_TO_DEVICE);	
		if(dma_mapping_error(data->dev, dma_addr)) {
			dev_err(data->dev, "dma_map_single(w) is failed\n");
			dma_unmap_single(data->dev, dma_addr, mtd->writesize, DMA_TO_DEVICE);
			return 1;
		}
		sg_init_one(&sg, (void *)buf, mtd->writesize);
		sg_dma_address(&sg) = dma_addr;
		cfg.direction = DMA_TO_DEVICE;
		cfg.src_addr = dma_addr;
		cfg.dst_addr = (dma_addr_t)data->chip.legacy.IO_ADDR_R;
		cfg.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		cfg.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		/* This must be same as the length of prefetch(128 words)*/
		cfg.src_maxburst = cfg.dst_maxburst = 128;
		if (dmaengine_slave_config(chan, &cfg)) {
			dev_err(data->dev, "slave_config(w) is failed\n");
			return 1;
		}
		
		desc = dmaengine_prep_slave_sg(chan, &sg, 1, DMA_TO_DEVICE,
				DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
		if (desc == NULL) {
			dev_err(data->dev, "prep_slave_sg(w) is failed\n");
			dma_unmap_single(data->dev, dma_addr, mtd->writesize, DMA_TO_DEVICE);
			return 1;
		}
		desc->callback = ftnandc024_dma_callback;
		desc->callback_param = data; 
		// Start Tx
		dmaengine_submit(desc);
		dma_async_issue_pending(chan);
		// Wait for completion of DMA engine
		ret = ftnandc024_dma_wait(chip, DMA_TO_DEVICE);
		dma_unmap_single(data->dev, dma_addr, mtd->writesize, DMA_TO_DEVICE);
	}
	if(ret)
		dmaengine_terminate_async(chan);
	return ret;
}

int ftnandc024_nand_dma_rd(struct nand_chip *chip, uint8_t *buf)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	struct dma_chan *chan = data->dma_chan;
	struct dma_slave_config cfg;
	struct scatterlist sg;
	struct dma_async_tx_descriptor *desc;
	dma_addr_t dma_addr;
	int ret;

	data->dma_sts = DMA_ONGOING;
	
	// buf isn't from kmalloc or get_free_page(s)
	if(!virt_addr_valid(buf)) {
		DBGLEVEL2(ftnandc024_dbg("Buf isn't from kmalloc or get_free_page(s)\n"));
		sg_init_one(&sg, (void *)data->dmaaddr, mtd->writesize);
		sg_dma_address(&sg) = data->dmaaddr;
		cfg.direction = DMA_FROM_DEVICE;
		cfg.src_addr = (dma_addr_t)data->chip.legacy.IO_ADDR_R;
		cfg.dst_addr = (dma_addr_t)data->dmaaddr;
		cfg.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		cfg.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		/* This must be same as the length of prefetch(128 words)*/
		cfg.src_maxburst = cfg.dst_maxburst = 128;
		if (dmaengine_slave_config(chan, &cfg)) {
			dev_err(data->dev, "slave_config(r) is failed\n");
			return 1;
		}
		
		desc = dmaengine_prep_slave_sg(chan, &sg, 1, DMA_FROM_DEVICE,
				DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
		if (desc == NULL) {
			dev_err(data->dev, "prep_slave_sg(r) is failed\n");
			//goto out;
			return 1;
		}
		desc->callback = ftnandc024_dma_callback;
		desc->callback_param = data; 
		// Start Tx
		dmaengine_submit(desc);
		dma_async_issue_pending(chan);
		// Wait for completion of DMA engine
		ret = ftnandc024_dma_wait(chip, DMA_FROM_DEVICE);
		memcpy(buf, data->dma_buf, mtd->writesize);
	}
	else {
		DBGLEVEL2(ftnandc024_dbg("Buf is from kmalloc or get_free_page(s)\n"));
		dma_addr = dma_map_single( data->dev, (void *)buf, mtd->writesize, DMA_FROM_DEVICE);	
		if(dma_mapping_error(data->dev, dma_addr)) {
			dev_err(data->dev, "dma_map_single(r) is failed\n");
			dma_unmap_single(data->dev, dma_addr, mtd->writesize, DMA_FROM_DEVICE);
			//goto out;
			return 1;
		}
		sg_init_one(&sg, (void *)buf, mtd->writesize);
		sg_dma_address(&sg) = dma_addr;
		cfg.direction = DMA_FROM_DEVICE;
		cfg.src_addr = (dma_addr_t)data->chip.legacy.IO_ADDR_R;
		cfg.dst_addr = dma_addr;
		cfg.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		cfg.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		/* This must be same as the length of prefetch(128 words)*/
		cfg.src_maxburst = cfg.dst_maxburst = 128;
		if (dmaengine_slave_config(chan, &cfg)) {
			dev_err(data->dev, "slave_config(r) is failed\n");
			return 1;
		}
		
		desc = dmaengine_prep_slave_sg(chan, &sg, 1, DMA_FROM_DEVICE,
				DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
		if (desc == NULL) {
			dma_unmap_single(data->dev, dma_addr, mtd->writesize, DMA_FROM_DEVICE);
			dev_err(data->dev, "prep_slave_sg(r) is failed\n");
			//goto out;
			return 1;
		}
		desc->callback = ftnandc024_dma_callback;
		desc->callback_param = data; 
		// Start Tx
		dmaengine_submit(desc);
		dma_async_issue_pending(chan);
		// Wait for completion of DMA engine
		ret = ftnandc024_dma_wait(chip, DMA_FROM_DEVICE);
		dma_unmap_single(data->dev, dma_addr, mtd->writesize, DMA_FROM_DEVICE);
	}
	if(ret)
		dmaengine_terminate_async(chan);
	return ret;
}
#endif
