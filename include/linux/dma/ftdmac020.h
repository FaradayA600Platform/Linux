// SPDX-License-Identifier: GPL-2.0
/*
 * Faraday FTDMAC020 DMA engine driver
 *
 * Copyright (C) 2013-2021 Faraday Technology Corporation
 *
 * Author: Faraday CTD/SD Dept.
 *
 */

#ifndef __MACH_FTDMAC020_H
#define __MACH_FTDMAC020_H

enum ftdmac020_channels {
	FTDMAC020_CHANNEL_0 = (1 << 0),
	FTDMAC020_CHANNEL_1 = (1 << 1),
	FTDMAC020_CHANNEL_2 = (1 << 2),
	FTDMAC020_CHANNEL_3 = (1 << 3),
	FTDMAC020_CHANNEL_4 = (1 << 4),
	FTDMAC020_CHANNEL_5 = (1 << 5),
	FTDMAC020_CHANNEL_6 = (1 << 6),
	FTDMAC020_CHANNEL_7 = (1 << 7),
	FTDMAC020_CHANNEL_ALL = 0xff,
};

/**
 * struct ftdmac020_dma_slave - DMA slave data
 * @common: physical address and register width...
 * @id: specify which ftdmac020 device to use, -1 for wildcard
 * @channels: bitmap of usable DMA channels
 * @handshake: hardware handshake number, -1 to disable handshake mode
 */
struct ftdmac020_dma_slave {
	struct dma_slave_config common;
	int id;
	enum ftdmac020_channels channels;
	int handshake;
};

/**
 * ftdmac020_chan_filter() - filter function for dma_request_channel().
 * @chan: DMA channel
 * @data: pointer to ftdmac020_dma_slave
 */
bool ftdmac020_chan_filter(struct dma_chan *chan, void *data);
#endif /* __MACH_FTDMAC020_H */
