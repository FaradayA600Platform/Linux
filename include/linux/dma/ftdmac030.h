/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020-2021 Faraday Technology Corporation
 * Driver for the Faraday FTDMAC030 DMA Controller 
 *
 * Author: Jack Chain <jack_ch@faraday-tech.com>
 */

#ifndef __MACH_FTDMAC030_H
#define __MACH_FTDMAC030_H

enum ftdmac030_channels {
	FTDMAC030_CHANNEL_0 = (1 << 0),
	FTDMAC030_CHANNEL_1 = (1 << 1),
	FTDMAC030_CHANNEL_2 = (1 << 2),
	FTDMAC030_CHANNEL_3 = (1 << 3),
	FTDMAC030_CHANNEL_4 = (1 << 4),
	FTDMAC030_CHANNEL_5 = (1 << 5),
	FTDMAC030_CHANNEL_6 = (1 << 6),
	FTDMAC030_CHANNEL_7 = (1 << 7),
	FTDMAC030_CHANNEL_ALL = 0xff,
};

/**
 * struct ftdmac030_dma_slave - DMA slave data
 * @common: physical address and register width...
 * @id: specify which ftdmac030 device to use, -1 for wildcard
 * @channels: bitmap of usable DMA channels
 * @handshake: hardware handshake number, -1 to disable handshake mode
 */
struct ftdmac030_dma_slave {
	struct dma_slave_config common;
	int id;
	enum ftdmac030_channels channels;
	int handshake;
};

/**
 * ftdmac030_chan_filter() - filter function for dma_request_channel().
 * @chan: DMA channel
 * @data: pointer to ftdmac030_dma_slave
 */
bool ftdmac030_chan_filter(struct dma_chan *chan, void *data);

#endif /* __MACH_FTDMAC030_H */
