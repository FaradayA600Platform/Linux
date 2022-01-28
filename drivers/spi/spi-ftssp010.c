// SPDX-License-Identifier: GPL-2.0
/**
 * Faraday FTSSP010 SPI flash driver.
 *
 * Copyright (c) 2021 Faraday Tech Corporation.
 */

#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/iopoll.h>
#include <linux/reset.h>
#include <linux/spi/spi.h>

#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>

#include <asm/io.h>


/******************************************************************************
 * SSP010 definitions
 *****************************************************************************/
#define FTSSP010_OFFSET_CR0                 0x00
#define FTSSP010_OFFSET_CR1                 0x04
#define FTSSP010_OFFSET_CR2                 0x08
#define FTSSP010_OFFSET_STATUS              0x0c
#define FTSSP010_OFFSET_ICR                 0x10
#define FTSSP010_OFFSET_ISR                 0x14
#define FTSSP010_OFFSET_DATA                0x18

#define FTSSP010_OFFSET_REVISION            0x60
#define FTSSP010_OFFSET_FEATURE             0x64

/*
 * Control Register 0
 */
#define FTSSP010_CR0_SCLKPH                 BIT(0)                  /* SPI SCLK phase */
#define FTSSP010_CR0_SCLKPO                 BIT(1)                  /* SPI SCLK polarity */
#define FTSSP010_CR0_OPM                    GENMASK(3, 2)
#define FTSSP010_CR0_OPM_SLAVE_MONO         (0x0)
#define FTSSP010_CR0_OPM_SLAVE_STEREO       (0x1)
#define FTSSP010_CR0_OPM_MASTER_MONO        (0x2)
#define FTSSP010_CR0_OPM_MASTER_STEREO      (0x3)
#define FTSSP010_CR0_FSJSTFY                BIT(4)
#define FTSSP010_CR0_FSPO                   BIT(5)
#define FTSSP010_CR0_LSB                    BIT(6)
#define FTSSP010_CR0_LOOPBACK               BIT(7)
#define FTSSP010_CR0_FSDIST                 GENMASK(9, 8)
#define FTSSP010_CR0_SPI_FLASH              BIT(11)                 /* SPI application is Flash */
#define	FTSSP010_CR0_FFMT                   GENMASK(14, 12)
#define FTSSP010_CR0_FFMT_TI_SSP            (0x0)
#define FTSSP010_CR0_FFMT_SPI               (0x1)                   /* Motorola  SPI */
#define FTSSP010_CR0_FFMT_MICROWIRE         (0x2)                   /* NS  Microwire */
#define FTSSP010_CR0_FFMT_I2S               (0x3)                   /* Philips   I2S */
#define FTSSP010_CR0_FFMT_ACLINK            (0x4)                   /* Intel AC-Link */
#define FTSSP010_CR0_SPI_FLASHTX            BIT(18)                 /* Flash Transmit Control */

/*
 * Control Register 1
 */
#define FTSSP010_CR1_SCLKDIV                GENMASK(15, 0)          /* SCLK divider */
#define FTSSP010_CR1_SDL                    GENMASK(22, 16)         /* serial  data length */
#define FTSSP010_CR1_PDL                    GENMASK(31, 24)         /* padding data length */

/*
 * Control Register 2
 */
#define FTSSP010_CR2_SSPEN                  BIT(0)                  /* SSP enable */
#define FTSSP010_CR2_TXDOE                  BIT(1)                  /* transmit data output enable */
#define FTSSP010_CR2_RXFCLR                 BIT(2)                  /* receive  FIFO clear */
#define FTSSP010_CR2_TXFCLR                 BIT(3)                  /* transmit FIFO clear */
#define FTSSP010_CR2_ACWRST                 BIT(4)                  /* AC-Link warm reset enable */
#define FTSSP010_CR2_ACCRST                 BIT(5)                  /* AC-Link cold reset enable */
#define FTSSP010_CR2_SSPRST                 BIT(6)                  /* SSP reset */
#define FTSSP010_CR2_RXEN                   BIT(7)                  /* Enable Rx */
#define FTSSP010_CR2_TXEN                   BIT(8)                  /* Enable Tx */
#define FTSSP010_CR2_FS                     BIT(9)                  /* CS line: 0 is low, 1 is high */
#define FTSSP010_CR2_CHIPSEL                GENMASK(11, 10)         /* Chip Select */

/*
 * Status Register
 */
#define FTSSP010_STATUS_RFF                 BIT(0)                  /* receive FIFO full */
#define FTSSP010_STATUS_TFNF                BIT(1)                  /* transmit FIFO not full */
#define FTSSP010_STATUS_BUSY                BIT(2)                  /* bus is busy */
#define FTSSP010_STATUS_GET_RFVE            GENMASK(9, 4)           /* receive  FIFO valid entries */
#define FTSSP010_STATUS_GET_TFVE            GENMASK(17, 12)         /* transmit FIFO valid entries */

/*
 * Interrupt Control Register
 */
#define FTSSP010_ICR_RFOR                   BIT(0)                  /* receive  FIFO overrun   interrupt */
#define FTSSP010_ICR_TFUR                   BIT(1)                  /* transmit FIFO underrun  interrupt */
#define FTSSP010_ICR_RFTH                   BIT(2)                  /* receive  FIFO threshold interrupt */
#define FTSSP010_ICR_TFTH                   BIT(3)                  /* transmit FIFO threshold interrupt */
#define FTSSP010_ICR_RFDMA                  BIT(4)                  /* receive  DMA request */
#define FTSSP010_ICR_TFDMA                  BIT(5)                  /* transmit DMA request */
#define FTSSP010_ICR_AC97FCEN               BIT(6)                  /* AC97 frame complete  */
#define FTSSP010_ICR_RFTHOD                 GENMASK(11, 7)          /* receive  FIFO threshold */
#define FTSSP010_ICR_TFTHOD                 GENMASK(16, 12)         /* transmit FIFO threshold */
#define FTSSP010_ICR_RFTHOD_UNIT            BIT(17)                 /* receive FIFO threshold unit */
#define FTSSP010_ICR_TXCIEN                 BIT(18)                 /* transmit complete interrupt */

/*
 * Interrupt Status Register
 */
#define FTSSP010_ISR_RFOR                   BIT(0)                  /* receive  FIFO overrun  */
#define FTSSP010_ISR_TFUR                   BIT(1)                  /* transmit FIFO underrun */
#define FTSSP010_ISR_RFTH                   BIT(2)                  /* receive  FIFO threshold */
#define FTSSP010_ISR_TFTH                   BIT(3)                  /* transmit FIFO threshold */
#define FTSSP010_ISR_TXCI                   BIT(5)                  /* transmit data complete */

/*
 * Revision Register
 */
#define FTSSP010_REVISION_RELEASE           GENMASK(7, 0)
#define FTSSP010_REVISION_MINOR             GENMASK(15, 8)
#define FTSSP010_REVISION_MAJOR             GENMASK(23, 16)

/*
 * Feature Register
 */
#define FTSSP010_FEATURE_WIDTH              GENMASK(7, 0)
#define FTSSP010_FEATURE_RXFIFO_DEPTH       GENMASK(15, 8)
#define FTSSP010_FEATURE_TXFIFO_DEPTH       GENMASK(23, 16)
#define FTSSP010_FEATURE_I2S                BIT(25)
#define FTSSP010_FEATURE_SPI_MWR            BIT(26)
#define FTSSP010_FEATURE_SSP                BIT(27)

/******************************************************************************
 * spi_master (controller) priveate data
 *****************************************************************************/
struct ftssp010_dma {
	int                     dma_status;
	struct dma_chan         *dma_chan;
	struct dma_slave_config dma_cfg;
	wait_queue_head_t       dma_waitq;
};

struct ftssp010_spi {
	struct device           *dev;
	struct clk              *pclk;
	struct clk              *refclk;
	struct reset_control    *rstc;
	void __iomem            *base;
	int                     irq;
	int                     rxfifo_depth;
	int                     txfifo_depth;

	wait_queue_head_t       waitq;

	struct ftssp010_dma     tx_dma;
	struct ftssp010_dma     rx_dma;
};

/******************************************************************************
 * DMA functions for FTSSP010
 *****************************************************************************/
#define DMA_COMPLETE            1
#define DMA_ONGOING             2

#define FTSSP010_DMA_BUF_SIZE   (4 * 1024)

u8 *tx_dummy_buf;

static void ftssp010_dma_callback(void *param)
{
	struct ftssp010_dma *dma = (struct ftssp010_dma *)param;

	BUG_ON(dma->dma_status == DMA_COMPLETE);
	dma->dma_status = DMA_COMPLETE;
	wake_up(&dma->dma_waitq);
}

static int ftssp010_dma_wait(struct ftssp010_dma *dma)
{
	wait_event(dma->dma_waitq,
	           dma->dma_status == DMA_COMPLETE);

	return 0;
}

static int ftssp010_dma_prepare(struct ftssp010_spi *ctrl, struct ftssp010_dma *dma,
				   resource_size_t phys_base, const char *name)
{
	struct dma_chan *dma_chan;

	dma_chan = dma_request_slave_channel(ctrl->dev, name);
	if (dma_chan) {
		dev_info(ctrl->dev, "Use DMA(%s) channel %d...\n", dev_name(dma_chan->device->dev),
		         dma_chan->chan_id);
	} else {
		dev_err(ctrl->dev, "allocate %s dma channel failed, fall back to PIO mode\n", name);
		return -EBUSY;
	}

	if (strcmp(name,"tx") == 0)
		dma->dma_cfg.direction = DMA_TO_DEVICE;
	else if (strcmp(name,"rx") == 0)
		dma->dma_cfg.direction = DMA_FROM_DEVICE;
	dma->dma_cfg.src_addr = (dma_addr_t)phys_base + FTSSP010_OFFSET_DATA;
	dma->dma_cfg.dst_addr = (dma_addr_t)phys_base + FTSSP010_OFFSET_DATA;
	dma->dma_cfg.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	dma->dma_cfg.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	dma->dma_cfg.src_maxburst = 1;
	dma->dma_cfg.dst_maxburst = 1;
	if (dmaengine_slave_config(dma_chan, &dma->dma_cfg)) {
		dev_err(ctrl->dev, "config %s dma slave failed, fall back to PIO mode\n", name);
		return -EBUSY;
	}

	dma->dma_chan = dma_chan;

	init_waitqueue_head(&dma->dma_waitq);

	return 0;
}

/******************************************************************************
 * internal functions for FTSSP010
 *****************************************************************************/
static void ftssp010_set_bits_per_word(void __iomem * base, int bpw)
{
	unsigned int cr1 = readl(base + FTSSP010_OFFSET_CR1);

	cr1 &= ~FTSSP010_CR1_SDL;
	cr1 |= FIELD_PREP(FTSSP010_CR1_SDL, (bpw - 1));

	writel(cr1, base + FTSSP010_OFFSET_CR1);
}

static void ftssp010_write_word(void __iomem * base, const void *data,
				   int wsize)
{
	unsigned int tmp = 0;

	if (data) {
		switch (wsize) {
		case 1:
			tmp = *(const u8 *)data;
			break;

		case 2:
			tmp = *(const u16 *)data;
			break;

		default:
			tmp = *(const u32 *)data;
			break;
		}
	}

	writel(tmp, base + FTSSP010_OFFSET_DATA);
}

static void ftssp010_read_word(void __iomem * base, void *buf, int wsize)
{
	unsigned int data = readl(base + FTSSP010_OFFSET_DATA);

	if (buf) {
		switch (wsize) {
		case 1:
			*(u8 *) buf = data;
			break;

		case 2:
			*(u16 *) buf = data;
			break;

		default:
			*(u32 *) buf = data;
			break;
		}
	}
}

static unsigned int ftssp010_read_status(void __iomem * base)
{
	return readl(base + FTSSP010_OFFSET_STATUS);
}

static int ftssp010_txfifo_not_full(void __iomem * base)
{
	return ftssp010_read_status(base) & FTSSP010_STATUS_TFNF;
}

static int ftssp010_rxfifo_valid_entries(void __iomem * base)
{
	return FIELD_GET(FTSSP010_STATUS_GET_RFVE, ftssp010_read_status(base));
}

static unsigned int ftssp010_read_feature(void __iomem *base)
{
	return readl(base + FTSSP010_OFFSET_FEATURE);
}

static int ftssp010_rxfifo_depth(void __iomem *base)
{
	return FIELD_GET(FTSSP010_FEATURE_RXFIFO_DEPTH,
	                 ftssp010_read_feature(base)) + 1;
}

static int ftssp010_txfifo_depth(void __iomem *base)
{
	return FIELD_GET(FTSSP010_FEATURE_TXFIFO_DEPTH,
	                 ftssp010_read_feature(base)) + 1;
}

/******************************************************************************
 * workqueue
 *****************************************************************************/
static int _ftssp010_spi_fill_fifo_dma(struct ftssp010_spi *ctrl,
				   struct spi_transfer *t,
				   int wsize)
{
	struct dma_async_tx_descriptor *tx_desc, *rx_desc;
	struct scatterlist tx_sg, rx_sg;
	dma_addr_t tx_dma_addr, rx_dma_addr;
	u32 len = t->len;
	const void *tx_buf = t->tx_buf;
	void *rx_buf = t->rx_buf;
	u32 cr2, mask;

	mask = FTSSP010_CR2_TXDOE | FTSSP010_CR2_TXEN;

	if (rx_buf)
		mask |= FTSSP010_CR2_RXEN;

	cr2 = readl(ctrl->base + FTSSP010_OFFSET_CR2);

	writel((cr2 | mask), ctrl->base + FTSSP010_OFFSET_CR2);

	// setup dma tx descriptor
	if (tx_buf) {
		tx_dma_addr = dma_map_single(ctrl->dev, (void *)tx_buf, len, DMA_TO_DEVICE);
		sg_init_one(&tx_sg, (void *)tx_buf, len);
	} else {
		tx_dma_addr = dma_map_single(ctrl->dev, (void *)tx_dummy_buf, len, DMA_TO_DEVICE);
		sg_init_one(&tx_sg, (void *)tx_dummy_buf, len);
	}

	if(dma_mapping_error(ctrl->dev, tx_dma_addr)) {
		dev_err(ctrl->dev, "dma_map_single(w) failed\n");
		dma_unmap_single(ctrl->dev, tx_dma_addr, len, DMA_TO_DEVICE);
		goto out;
	}

	sg_dma_address(&tx_sg) = tx_dma_addr;
	sg_dma_len(&tx_sg) = len;

	tx_desc = dmaengine_prep_slave_sg(ctrl->tx_dma.dma_chan, &tx_sg, 1,
	                                  DMA_TO_DEVICE, (DMA_PREP_INTERRUPT | DMA_CTRL_ACK));
	if (!tx_desc) {
		dma_unmap_single(ctrl->dev, tx_dma_addr, len, DMA_TO_DEVICE);
		dev_err(ctrl->dev, "prepare_slave_sg(w) failed\n");
		goto out;
	}

	tx_desc->callback = ftssp010_dma_callback;
	tx_desc->callback_param = &ctrl->tx_dma;

	// setup dma rx descriptor
	if (rx_buf) {
		rx_dma_addr = dma_map_single(ctrl->dev, (void *)rx_buf, len, DMA_FROM_DEVICE);
		if(dma_mapping_error(ctrl->dev, rx_dma_addr)) {
			dev_err(ctrl->dev, "dma_map_single(r) failed\n");
			dma_unmap_single(ctrl->dev, rx_dma_addr, len, DMA_FROM_DEVICE);
			goto out;
		}

		sg_init_one(&rx_sg, (void *)rx_buf, len);
		sg_dma_address(&rx_sg) = rx_dma_addr;
		sg_dma_len(&rx_sg) = len;

		rx_desc = dmaengine_prep_slave_sg(ctrl->rx_dma.dma_chan, &rx_sg, 1,
		                                  DMA_FROM_DEVICE, (DMA_PREP_INTERRUPT | DMA_CTRL_ACK));
		if (rx_desc == NULL) {
			dma_unmap_single(ctrl->dev, rx_dma_addr, len, DMA_FROM_DEVICE);
			dev_err(ctrl->dev, "preg_slave_sg(r) failed\n");
			goto out;
		}

		rx_desc->callback = ftssp010_dma_callback;
		rx_desc->callback_param = &ctrl->rx_dma;

		// start dma rx channel
		ctrl->rx_dma.dma_status = DMA_ONGOING;
		dmaengine_submit(rx_desc);
		dma_async_issue_pending(ctrl->rx_dma.dma_chan);
	}

	// start dma tx channel
	ctrl->tx_dma.dma_status = DMA_ONGOING;
	dmaengine_submit(tx_desc);
	dma_async_issue_pending(ctrl->tx_dma.dma_chan);

	// wait for completion of dma channel
	if (rx_buf) {
		ftssp010_dma_wait(&ctrl->rx_dma);
		dma_unmap_single(ctrl->dev, rx_dma_addr, len, DMA_FROM_DEVICE);
	}
	ftssp010_dma_wait(&ctrl->tx_dma);
	dma_unmap_single(ctrl->dev, tx_dma_addr, len, DMA_TO_DEVICE);
	len = 0;
out:
	writel((cr2 & ~mask), ctrl->base + FTSSP010_OFFSET_CR2);

	cr2 |= (FTSSP010_CR2_TXFCLR | FTSSP010_CR2_RXFCLR);
	writel(cr2, ctrl->base + FTSSP010_OFFSET_CR2);

	return t->len - len;
}

static int _ftssp010_spi_fill_fifo(struct ftssp010_spi *ctrl,
				   struct spi_transfer *t,
				   int wsize)
{
	int len = t->len;
	const void *tx_buf = t->tx_buf;
	void *rx_buf = t->rx_buf;
	int cr2, mask, ret, size, i;
	unsigned long timeout;

	mask = FTSSP010_CR2_TXEN;

	if (tx_buf)
		mask |= FTSSP010_CR2_TXDOE;

	if (rx_buf)
		mask |= FTSSP010_CR2_RXEN;

	cr2 = readl(ctrl->base + FTSSP010_OFFSET_CR2);

	writel((cr2 | mask), ctrl->base + FTSSP010_OFFSET_CR2);

	while (len > 0) {
		size = min_t(u32, min_t(u32, ctrl->txfifo_depth, ctrl->rxfifo_depth), len / wsize);

		for (i = 0; i < size; ++i) {
			ftssp010_write_word(ctrl->base, tx_buf, wsize);

			if (tx_buf)
				tx_buf += wsize;
		}

		timeout = jiffies + 1000;
		do {
			unsigned int sts;

			if (time_after(jiffies, timeout)) {
				dev_info(ctrl->dev, "xfer error: len %d, " \
					 "tx_buf %px rx_buf %px\n",
					 t->len, t->tx_buf, t->rx_buf);
				len = t->len;
				goto out;
			}

			sts = ftssp010_read_status(ctrl->base);
			if (FIELD_GET(FTSSP010_STATUS_BUSY, sts) == 0) {
				if (tx_buf) {
					if (FIELD_GET(FTSSP010_STATUS_GET_TFVE, sts) == 0)
						break;
				} else {
					break;
				}
			}
		} while (1);

		len -= (size * wsize);

		if (rx_buf) {
			for (i = 0; i < size; ++i) {
				timeout = jiffies + 1000;
				/* wait until sth. in rx fifo */
				ret = wait_event_timeout(ctrl->waitq,
				                         ftssp010_rxfifo_valid_entries(ctrl->base), timeout);
				if (!ftssp010_rxfifo_valid_entries(ctrl->base)) {
					dev_err(ctrl->dev, "wait rx fifo timeout(len:%d(%d))\n",
					        len, t->len);
					goto out;
				} else if (unlikely(!ret)) {
					dev_err(ctrl->dev, "wait interrupt timeout(len:%d(%d))\n",
					        len, t->len);
					goto out;
				}

				ftssp010_read_word(ctrl->base, rx_buf, wsize);

				rx_buf += wsize;
			}
		}
	}
out:
	writel((cr2 & ~mask), ctrl->base + FTSSP010_OFFSET_CR2);

	cr2 |= (FTSSP010_CR2_TXFCLR | FTSSP010_CR2_RXFCLR);
	writel(cr2, ctrl->base + FTSSP010_OFFSET_CR2);

	return t->len - len;
}

/******************************************************************************
 * interrupt handler
 *****************************************************************************/
static irqreturn_t ftssp010_spi_interrupt(int irq, void *dev_id)
{
	struct spi_master *master = dev_id;
	struct ftssp010_spi *ctrl;
	u32 isr;

	ctrl = spi_master_get_devdata(master);

	isr = readl(ctrl->base + FTSSP010_OFFSET_ISR);

	if (isr & FTSSP010_ISR_TFUR)
		dev_err(&master->dev, "Tx FIFO Underrun!\n");

	if (isr & FTSSP010_ISR_RFOR)
		dev_err(&master->dev, "Rx FIFO overrun!\n");

	wake_up(&ctrl->waitq);
	return IRQ_HANDLED;
}

/******************************************************************************
 * struct spi_master functions
 *****************************************************************************/

static void ftssp010_spi_chipselect(struct spi_device *spi, bool is_high)
{
	struct ftssp010_spi *ctrl;
	u32 cr2;

	ctrl = spi_master_get_devdata(spi->master);

	if (is_high) {
		//Pull up the CS line.
		cr2 = (FIELD_PREP(FTSSP010_CR2_CHIPSEL, spi->chip_select) |
		       FTSSP010_CR2_FS |
		       FTSSP010_CR2_SSPEN);
	} else {
		//Pull down the CS line.
		cr2 = (FIELD_PREP(FTSSP010_CR2_CHIPSEL, spi->chip_select) |
		       FTSSP010_CR2_SSPEN);
	}

	writel(cr2, ctrl->base + FTSSP010_OFFSET_CR2);

	dev_dbg(&spi->dev, "%s: CR2=0x%08x, CS%d %s\n", __func__,
	        readl(ctrl->base + FTSSP010_OFFSET_CR2),
	        spi->chip_select, is_high ? "High" : "Low");
}

/* the spi->mode bits understood by this driver: */
#define MODEBITS (SPI_CPOL | SPI_CPHA | SPI_LSB_FIRST)

static void ftssp010_spi_master_setup_mode(struct spi_device *spi)
{
	struct ftssp010_spi *ctrl = spi_master_get_devdata(spi->master);
	unsigned int cr0;

	cr0 = FTSSP010_CR0_FSPO |
	      FIELD_PREP(FTSSP010_CR0_FFMT, FTSSP010_CR0_FFMT_SPI) |
	      FIELD_PREP(FTSSP010_CR0_OPM, FTSSP010_CR0_OPM_MASTER_STEREO);
#ifdef CONFIG_SPI_FTSSP010_FLASH
	cr0 |= FTSSP010_CR0_SPI_FLASH | FTSSP010_CR0_SPI_FLASHTX;
#endif
	if (spi->mode & SPI_CPOL) {
		dev_dbg(&spi->master->dev, "setup: CPOL high\n");
		cr0 |= FTSSP010_CR0_SCLKPO;
	}

	if (spi->mode & SPI_CPHA) {
		dev_dbg(&spi->master->dev, "setup: CPHA high\n");
		cr0 |= FTSSP010_CR0_SCLKPH;
	}

	if (spi->mode & SPI_LSB_FIRST) {
		dev_dbg(&spi->master->dev, "setup: LSB first\n");
		cr0 |= FTSSP010_CR0_LSB;
	}

	writel(cr0, ctrl->base + FTSSP010_OFFSET_CR0);
}

static void ftssp010_spi_master_setup_clkdiv(struct spi_device *spi,
				   struct spi_transfer *t)
{
	struct ftssp010_spi *ctrl;
	u32 clk_hz, div = 0;
	unsigned int cr1;

	ctrl = spi_master_get_devdata(spi->master);

	clk_hz = clk_get_rate(ctrl->refclk);

	while (div < 0xFFFF) {

		if ((clk_hz / (2 * (div + 1))) <= t->speed_hz)
			break;

		div++;
	}

	cr1 = readl(ctrl->base + FTSSP010_OFFSET_CR1);
	cr1 &= ~FTSSP010_CR1_SCLKDIV;
	cr1 |= FIELD_PREP(FTSSP010_CR1_SCLKDIV, div);
	writel(cr1, ctrl->base + FTSSP010_OFFSET_CR1);
}

static int ftssp010_spi_master_setup(struct spi_device *spi)
{
	struct ftssp010_spi *ctrl;

	dev_dbg(&spi->dev, "%s\n", __func__);

	ctrl = spi_master_get_devdata(spi->master);

	if (spi->mode & ~MODEBITS) {
		dev_err(&spi->master->dev,
		        "setup: unsupported mode bits %x\n", spi->mode & ~MODEBITS);
		return -EINVAL;
	}

	if (spi->chip_select > spi->master->num_chipselect) {
		dev_err(&spi->dev,
		        "setup: invalid chipselect %u (%u defined)\n",
		        spi->chip_select, spi->master->num_chipselect);
		return -EINVAL;
	}

	/* check speed */
	if (!spi->max_speed_hz) {
		dev_err(&spi->dev, "setup: max speed not specified\n");
		return -EINVAL;
	}

	if (spi->max_speed_hz > spi->master->max_speed_hz) {
		dev_err(&spi->dev,
		        "setup: invalid max speed hz %u (%u defined)\n",
		        spi->max_speed_hz, spi->master->max_speed_hz);
		return -EINVAL;
	}

	ftssp010_spi_master_setup_mode(spi);

	ftssp010_set_bits_per_word(ctrl->base, spi->bits_per_word);

	dev_dbg(&spi->dev, "%s: CR0=0x%08x\n", __func__,
	        readl(ctrl->base + FTSSP010_OFFSET_CR0));
	dev_dbg(&spi->dev, "%s: CR1=0x%08x\n", __func__,
	        readl(ctrl->base + FTSSP010_OFFSET_CR1));

	return 0;
}

/**
 * ftssp010_spi_transfer_one - Initiates the SPI transfer
 * @master:	Pointer to the spi_master structure which provides
 *		information about the controller.
 * @qspi:	Pointer to the spi_device structure
 * @transfer:	Pointer to the spi_transfer structure which provide information
 *		about next transfer parameters
 *
 * This function fills the TX FIFO, starts the transfer, and waits for the
 * transfer to be completed.
 *
 * Return:	Number of bytes transferred in the last transfer
 */
static int ftssp010_spi_transfer_one(struct spi_master *master,
				    struct spi_device *spi,
				    struct spi_transfer *transfer)
{
	struct ftssp010_spi *ctrl = spi_master_get_devdata(master);
	int ret_len;

	dev_dbg(&spi->dev, "xfer_one: len %d, tx_buf %px rx_buf %px\n",
	        transfer->len, transfer->tx_buf, transfer->rx_buf);

	ftssp010_spi_master_setup_clkdiv(spi, transfer);

	if (ctrl->tx_dma.dma_chan && ctrl->rx_dma.dma_chan)
		ret_len = _ftssp010_spi_fill_fifo_dma(ctrl, transfer, 1);
	else
		ret_len = _ftssp010_spi_fill_fifo(ctrl, transfer, 1);
#if 0
{
	int i, len = transfer->len;
	char *buf;

	if (transfer->tx_buf)
		buf = (char *) transfer->tx_buf;

	if (transfer->rx_buf)
		buf = (char *) transfer->rx_buf;

	dev_info(&spi->dev, "xfer_one: buffer content:\n");
	for (i = 0; i < len; i++) {

		printk("%x ", buf[i]);

	}

	dev_info(&spi->dev, "\nxfer_one: ret_len %d\n", ret_len);
}
#endif
	spi_finalize_current_transfer(master);

	return ret_len;
}

/**
 * ftssp010_spi_prepare_transfer_hardware - Prepares hardware for transfer.
 * @master:	Pointer to the spi_master structure which provides
 *		information about the controller.
 *
 * This function enables SPI master controller.
 *
 * Return:	Always 0
 */
static int ftssp010_spi_prepare_transfer_hardware(struct spi_master *master)
{
	struct ftssp010_spi *ctrl = spi_master_get_devdata(master);

	writel(FTSSP010_CR2_TXFCLR | FTSSP010_CR2_RXFCLR | FTSSP010_CR2_SSPEN,
	       ctrl->base + FTSSP010_OFFSET_CR2);

	return 0;
}

static int ftssp010_hw_setup(struct ftssp010_spi *ctrl, resource_size_t phys_base)
{
#ifdef CONFIG_SPI_FTSSP010_USE_DMA
	uint32_t val;
#endif

	writel(FTSSP010_ICR_RFOR | FTSSP010_ICR_TFUR | 
	       FTSSP010_ICR_TXCIEN,
	       ctrl->base + FTSSP010_OFFSET_ICR);

#ifdef CONFIG_SPI_FTSSP010_USE_DMA
	tx_dummy_buf = kzalloc(FTSSP010_DMA_BUF_SIZE, GFP_KERNEL);
	if (!tx_dummy_buf) {
		dev_err(ctrl->dev, "allocate dma buffer failed, fall back to PIO mode\n");
		return 0;
	}

	ftssp010_dma_prepare(ctrl, &ctrl->tx_dma, phys_base, "tx");

	ftssp010_dma_prepare(ctrl, &ctrl->rx_dma, phys_base, "rx");

	val = readl(ctrl->base + FTSSP010_OFFSET_ICR);
	val |= FIELD_PREP(FTSSP010_ICR_TFTHOD, 1) | FIELD_PREP(FTSSP010_ICR_RFTHOD, 1);
	val |= FTSSP010_ICR_TFDMA | FTSSP010_ICR_RFDMA;
	writel(val, ctrl->base + FTSSP010_OFFSET_ICR);
#endif

	return 0;
}

/******************************************************************************
 * struct platform_driver functions
 *****************************************************************************/
static int ftssp010_spi_probe(struct platform_device *pdev)
{
	struct spi_master *master;
	struct ftssp010_spi *ctrl;
	struct resource *res;
	int ret = 0;
	u32 dev_id, num_cs, clk_hz, rev;

	if ((master =
	     spi_alloc_master(&pdev->dev, sizeof *ctrl)) == NULL) {
		return -ENOMEM;
	}

	ctrl = spi_master_get_devdata(master);
	master->dev.of_node = pdev->dev.of_node;
	platform_set_drvdata(pdev, master);

	ctrl->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ctrl->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(ctrl->base)) {
		ret = PTR_ERR(ctrl->base);
		goto err_put_master;
	}

	ctrl->pclk = devm_clk_get(&pdev->dev, "pclk");
	if (!IS_ERR(ctrl->pclk)) {
		clk_prepare_enable(ctrl->pclk);
	}

	ctrl->refclk = devm_clk_get(&pdev->dev, "sspclk");
	if (IS_ERR(ctrl->refclk)) {
		dev_err(&pdev->dev, "ref_clk clock not found.\n");
		ret = PTR_ERR(ctrl->refclk);
		goto err_put_master;
	}

	ret = clk_prepare_enable(ctrl->refclk);
	if (ret) {
		dev_err(&pdev->dev, "Unable to enable device clock.\n");
		goto err_put_master;
	}

	ctrl->rstc = devm_reset_control_get(&pdev->dev, "rstn");
	if (!IS_ERR_OR_NULL(ctrl->rstc)) {
		reset_control_deassert(ctrl->rstc);
	}

	clk_hz = clk_get_rate(ctrl->refclk);

	//Tx at least divide by 2, SCLKDIV value is 0
	//Rx at least divide by 6, SCLKDIV value is 2
	master->max_speed_hz = clk_hz / 3;

	ctrl->rxfifo_depth = ftssp010_rxfifo_depth(ctrl->base);
	ctrl->txfifo_depth = ftssp010_txfifo_depth(ctrl->base);

	if ((ctrl->irq = platform_get_irq(pdev, 0)) < 0) {
		ret = -ENXIO;
		dev_err(&pdev->dev, "irq resource not found\n");
		goto err_clk_disable;
	}

	if ((ret = devm_request_irq(&pdev->dev, ctrl->irq, ftssp010_spi_interrupt, 0,
	                            pdev->name, master)) != 0) {
		ret = -ENXIO;
		dev_err(&pdev->dev, "request_irq failed\n");
		goto err_clk_disable;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "num-cs",
	                           &num_cs);
	if (ret < 0)
		master->num_chipselect = 1;
	else
		master->num_chipselect = num_cs;

	rev = readl(ctrl->base + FTSSP010_OFFSET_REVISION);
	dev_info(&pdev->dev, "Faraday FTSSP010 Controller HW version %u.%u.%u\n",
	         (u32)FIELD_GET(FTSSP010_REVISION_MAJOR, rev),
	         (u32)FIELD_GET(FTSSP010_REVISION_MINOR, rev),
	         (u32)FIELD_GET(FTSSP010_REVISION_RELEASE, rev));

	dev_info(&pdev->dev, "Controller base address 0x%08x (irq %d)\n",
	         (unsigned int)res->start, ctrl->irq);

	dev_info(&pdev->dev, "Controller max speed Hz %u, num_cs %d\n",
	         master->max_speed_hz, master->num_chipselect);

	ret = of_property_read_u32(pdev->dev.of_node, "dev_id",
	                           &dev_id);
	if (ret < 0)
		master->bus_num = pdev->id;
	else
		master->bus_num = dev_id;

	master->setup = ftssp010_spi_master_setup;
	master->set_cs = ftssp010_spi_chipselect;
	master->transfer_one = ftssp010_spi_transfer_one;
	master->prepare_transfer_hardware = ftssp010_spi_prepare_transfer_hardware;

	master->mode_bits = MODEBITS;
	master->bits_per_word_mask = SPI_BPW_MASK(8);

	init_waitqueue_head(&ctrl->waitq);

	ret = ftssp010_hw_setup(ctrl, res->start);
	if (ret < 0) {
		dev_err(&pdev->dev, "setup hardware failed\n");
		goto err_clk_disable;
	}

	if ((ret = devm_spi_register_master(&pdev->dev, master)) != 0) {
		dev_err(&pdev->dev, "register master failed\n");
		goto err_clk_disable;
	}

	return 0;

err_clk_disable:
	clk_disable_unprepare(ctrl->refclk);
err_put_master:
	spi_master_put(master);
	return ret;
}

static int __exit ftssp010_spi_remove(struct platform_device *pdev)
{
	struct spi_master *master;

	master = platform_get_drvdata(pdev);
	spi_unregister_master(master);

	dev_info(&pdev->dev, "FTSSP010 unregistered\n");

	return 0;
}

#define ftssp010_spi_suspend NULL
#define ftssp010_spi_resume NULL
#define ftssp010_spi_runtime_suspend NULL
#define ftssp010_spi_runtime_resume NULL
#define ftssp010_spi_runtime_idle NULL

static const struct dev_pm_ops ftssp010_spi_ops = {
#ifdef CONFIG_PM_SLEEP
	.suspend    = ftssp010_spi_suspend,
	.resume     = ftssp010_spi_resume,
#endif
	SET_RUNTIME_PM_OPS(ftssp010_spi_runtime_suspend, ftssp010_spi_runtime_resume,
	                   ftssp010_spi_runtime_idle)
};

#ifdef CONFIG_OF
static const struct of_device_id ftssp010_of_match[] = {
	{ .compatible = "faraday,ftssp010-spi" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ftssp010_dt_ids);
#endif

static struct platform_driver ftssp010_spi_driver = {
	.remove     = __exit_p(ftssp010_spi_remove),
	.driver     = {
		.name   = "ftssp010_spi",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(ftssp010_of_match),
		.pm     = &ftssp010_spi_ops,
	},
};

module_platform_driver_probe(ftssp010_spi_driver, ftssp010_spi_probe);

MODULE_AUTHOR("Bing-Yao Luo <bjluo@faraday-tech.com>");
MODULE_DESCRIPTION("FTSSP010 SPI Flash Driver");
MODULE_LICENSE("GPL");
