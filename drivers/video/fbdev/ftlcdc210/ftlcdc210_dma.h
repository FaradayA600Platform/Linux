/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020-2021 Faraday Technology Corporation
 * Driver for Faraday FTLCDC210 DMA Functions
 *
 * Author: Jack Chain <jack_ch@faraday-tech.com>
 */

#ifndef __FTLCDC210_DMA_H
#define __FTLCDC210_DMA_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>

struct ftlcdc210_dma {
	struct dma_slave_config slave_conf;
	struct dma_chan         *chan;
	struct device           *dev;
	dma_addr_t              src_dma_addr;
	dma_addr_t              dst_dma_addr;
	dma_cookie_t            cookie;
	size_t                  trans_size;
	unsigned int            value;
	unsigned char           is_running;
};

int ftlcdc210_DMA_imgblt(struct device *dev,dma_addr_t src,dma_addr_t dst,size_t xcnt,
           size_t ycnt,size_t sstride,size_t dstride);
           
int ftlcdc210_DMA_fillrec(struct device *dev,dma_addr_t dst,size_t xcnt,
           size_t ycnt,size_t dstride,unsigned int color);
           
int ftlcdc210_DMA_copyarea(struct device *dev,dma_addr_t src,dma_addr_t dst,size_t xcnt,
           size_t ycnt,size_t sstride,size_t dstride);           

#endif   /* __FTLCDC210_DMA_H */
