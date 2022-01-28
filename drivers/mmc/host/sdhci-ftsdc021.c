/*
 * Faraday FTSDC021 controller driver.
 *
 * Copyright (c) 2012 Faraday Technology Corporation.
 *
 * Authors: Bing-Jiun, Luo <bjluo@faraday-tech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/reset.h>

#include <linux/platform_data/sd-faraday.h>

#include "sdhci-pltfm.h"
#include "cqhci.h"

#define  DRIVER_NAME "ftsdc021"

#ifdef CONFIG_A500_PLATFORM
#define Transfer_Mode_PIO
#endif
//#define Transfer_Mode_SDMA

#ifdef CONFIG_MMC_CQHCI
#define SDHCI_FTSDC021_CQE_BASE_ADDR    0x200
#define SDHCI_FTSDC021_CQE_ARBSEL       0x260
#endif

#define SDHCI_FTSDC021_VENDOR0          0x100
#define SDHCI_PULSE_LATCH_OFF_MASK      0x3F00
#define SDHCI_PULSE_LATCH_OFF_SHIFT     8
#define SDHCI_PULSE_LATCH_EN            0x1

#define SDHCI_FTSDC021_VENDOR1          0x104
#define SDHCI_FTSDC021_RST_N_FUNCTION   8
#define SDHCI_FTSDC021_RST_N_MASK       0xFFFFFFF7

#define SDHCI_FTSDC021_VENDOR3          0x10C

#define FTSDC021_BASE_CLOCK             100000000

unsigned int pulse_latch_offset = 1;
unsigned int cqe_task_arbitration = 0;

static unsigned int ftsdc021_get_max_clk(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);

	return pltfm_host->clock;
}

static void ftsdc021_set_clock(struct sdhci_host *host, unsigned int clock)
{
	int vendor0;

	sdhci_set_clock(host, clock);

	vendor0 = sdhci_readl(host, SDHCI_FTSDC021_VENDOR0);
	if (host->mmc->actual_clock < 100000000)
		/* Enable pulse latch */
		vendor0 |= SDHCI_PULSE_LATCH_EN;
	else
		vendor0 &= ~(SDHCI_PULSE_LATCH_OFF_MASK | SDHCI_PULSE_LATCH_EN);

	vendor0 &= ~(0x3f << SDHCI_PULSE_LATCH_OFF_SHIFT);
	vendor0 |= (pulse_latch_offset << SDHCI_PULSE_LATCH_OFF_SHIFT);
	sdhci_writel(host, vendor0, SDHCI_FTSDC021_VENDOR0);
}

static void ftsdc021_hw_reset(struct sdhci_host *host)
{
	unsigned int value;

	value = sdhci_readl(host, SDHCI_FTSDC021_VENDOR1);

	if ((value & SDHCI_FTSDC021_RST_N_FUNCTION) == 0) {
		sdhci_writel(host, value | SDHCI_FTSDC021_RST_N_FUNCTION, SDHCI_FTSDC021_VENDOR1);
	}
	else {
		sdhci_writel(host, value & SDHCI_FTSDC021_RST_N_MASK, SDHCI_FTSDC021_VENDOR1);
		udelay(10);
		sdhci_writel(host, value | SDHCI_FTSDC021_RST_N_FUNCTION, SDHCI_FTSDC021_VENDOR1);
	}
}

void ftsdc021_wait_dll_lock(void)
{
	platform_sdhci_wait_dll_lock();
}
EXPORT_SYMBOL(ftsdc021_wait_dll_lock);

void ftsdc021_reset_dll(void)
{
#if 0
	int timeout = 1000;
	void __iomem *gpio_va;

	gpio_va = ioremap(0xa8500000, 0x10000);

	//Enable gpio bit4 and bit14 write permission
	writel((readl(gpio_va + 0x8) | 0x4010), gpio_va + 0x8);
	//reset wrapper
	writel((readl(gpio_va) | 0x4000), gpio_va);
	udelay(1);; //need some delay
	writel((readl(gpio_va) & 0xffffbfff), gpio_va);
	//reset DLL
	writel((readl(gpio_va) | 0x10), gpio_va);
	udelay(1);; //need some delay
	writel((readl(gpio_va) & 0xffffffef), gpio_va);
	//wait reset ok
	timeout = 1000;
	while((readl(gpio_va + 0x4) & 0x8) == 0x8) {
		if (timeout) {
			udelay(1);
			timeout--;
		}
		else {
			printk("Reset DLL timeout\n");
			break;
		}
	}

	printk("Reset DLL done\n");
	iounmap(gpio_va);
#endif
}
EXPORT_SYMBOL(ftsdc021_reset_dll);

#ifdef CONFIG_MMC_CQHCI
static u32 sdhci_ftsdc021_cqhci_irq(struct sdhci_host *host, u32 intmask)
{
	int cmd_error = 0;
	int data_error = 0;

	if (!sdhci_cqe_irq(host, intmask, &cmd_error, &data_error))
		return intmask;

	cqhci_irq(host->mmc, intmask, cmd_error, data_error);

	return 0;
}

static void sdhci_ftsdc021_dumpregs(struct mmc_host *mmc)
{
	sdhci_dumpregs(mmc_priv(mmc));
}

static const struct cqhci_host_ops sdhci_ftsdc021_cqhci_ops = {
	.enable     = sdhci_cqe_enable,
	.disable    = sdhci_cqe_disable,
	.dumpregs   = sdhci_ftsdc021_dumpregs,
};

#endif
static int sdhci_cqe_add_host(struct sdhci_host *host,
			      struct platform_device *pdev)
{
	int ret = 0;
#ifdef CONFIG_MMC_CQHCI
	struct cqhci_host *cq_host;

	host->mmc->caps2 |= MMC_CAP2_CQE | MMC_CAP2_CQE_DCMD;
	ret = sdhci_setup_host(host);
	if (ret)
		return ret;

	cq_host = devm_kzalloc(host->mmc->parent, sizeof(*cq_host), GFP_KERNEL);
	if (!cq_host) {
		ret = -ENOMEM;
		goto cleanup;
	}

	cq_host->mmio = host->ioaddr + SDHCI_FTSDC021_CQE_BASE_ADDR;
	cq_host->ops = &sdhci_ftsdc021_cqhci_ops;

	ret = cqhci_init(cq_host, host->mmc, false);
	if (ret)
		goto cleanup;
	/*
	 * Ready Task Arbitration Select
	 * 0: smalles ID task
	 * 1: earlier task
	 */
	sdhci_writel(host, cqe_task_arbitration, SDHCI_FTSDC021_CQE_ARBSEL);

	ret = __sdhci_add_host(host);
	if (ret)
		goto cleanup;

	return ret;
cleanup:
	sdhci_cleanup_host(host);

#endif
	return ret;
}

static struct sdhci_ops ftsdc021_ops = {
	.set_clock = ftsdc021_set_clock,
	.get_max_clock = ftsdc021_get_max_clk,
	.set_bus_width = sdhci_set_bus_width,
	.reset = sdhci_reset,
	.hw_reset = ftsdc021_hw_reset,
	.set_uhs_signaling = sdhci_set_uhs_signaling,
#ifdef CONFIG_MMC_CQHCI
	.irq = sdhci_ftsdc021_cqhci_irq,
#endif
};

static struct sdhci_pltfm_data ftsdc021_pdata = {
	.ops = &ftsdc021_ops,
	.quirks = SDHCI_QUIRK_CAP_CLOCK_BASE_BROKEN |
	          SDHCI_QUIRK_32BIT_ADMA_SIZE |
	          SDHCI_QUIRK_BROKEN_TIMEOUT_VAL |
	          SDHCI_QUIRK_DATA_TIMEOUT_USES_SDCLK,
	.quirks2 = SDHCI_QUIRK2_PRESET_VALUE_BROKEN,
};

static const struct of_device_id sdhci_ftsdc021_of_match_table[] = {
	{ .compatible = "faraday,ftsdc021-sdhci", .data = &ftsdc021_pdata},
	{ .compatible = "faraday,ftsdc021-sdhci-5.1", .data = &ftsdc021_pdata},
	{}
};

MODULE_DEVICE_TABLE(of, sdhci_ftsdc021_of_match_table);

static int ftsdc021_probe(struct platform_device *pdev)
{
	const struct of_device_id *match;
	const struct sdhci_pltfm_data *soc_data;
	struct sdhci_pltfm_host *pltfm_host;
	struct sdhci_host *host = NULL;
	int ret = 0;
	struct clk *clk;
	struct reset_control *rc;
	struct device_node *np = pdev->dev.of_node;
	const __be32 *pulse, *task_arb, *win_size;
	int cqe_on = 0, vendor3;

	pr_info("ftsdc021: probe\n");

	match = of_match_device(sdhci_ftsdc021_of_match_table, &pdev->dev);

	if (match)
		soc_data = match->data;
	else
		soc_data = &ftsdc021_pdata;

	host = sdhci_pltfm_init(pdev, soc_data, 0);
	if (IS_ERR(host))
		return PTR_ERR(host);
	pltfm_host = sdhci_priv(host);

	sdhci_get_of_property(pdev);

	/* Ungated sd sys clock */
	clk = devm_clk_get(&pdev->dev, "sysclk");
	if (!IS_ERR(clk)) {
		pltfm_host->sysclk = clk;
		clk_prepare_enable(clk);
	}

	/* Get sd clock (sdclk1x for FTSDC021) */
	clk = devm_clk_get(&pdev->dev, "sdclk1x");
	if (!IS_ERR(clk)) {
		pltfm_host->clk = clk;
		pltfm_host->clock = clk_get_rate(pltfm_host->clk);
		clk_prepare_enable(clk);
	}

	/* deasserted sdc reset */
	rc = devm_reset_control_get(&pdev->dev, "rstn");
	if (!IS_ERR_OR_NULL(rc)) {
		pltfm_host->rc = rc;
		reset_control_deassert(rc);
	}

	if (!match) {
		pltfm_host->clock = FTSDC021_BASE_CLOCK;
	}
	else {
		ret = mmc_of_parse(host->mmc);
		if (ret)
			return ret;
		/* Get default pulse latch offset */
		pulse = of_get_property(np, "pulse-latch", NULL);
		if (pulse)
			pulse_latch_offset = be32_to_cpup(pulse);

		/* Get minimum tuning window size */
		win_size = of_get_property(np, "tuning-window-size", NULL);

		if (of_property_read_bool(np, "supports-cqe"))
			cqe_on = 1;

		task_arb = of_get_property(np, "cqe-task-arbitration", NULL);
		if (task_arb)
			cqe_task_arbitration = be32_to_cpup(pulse);
	}

	/* Set transfer mode, default use ADMA */
#if defined(Transfer_Mode_PIO)
	host->quirks |= SDHCI_QUIRK_BROKEN_DMA;
	host->quirks |= SDHCI_QUIRK_BROKEN_ADMA;
#elif defined(Transfer_Mode_SDMA)
	host->quirks |= SDHCI_QUIRK_FORCE_DMA;
	host->quirks |= SDHCI_QUIRK_BROKEN_ADMA;
#endif

	host->mmc->caps |= MMC_CAP_HW_RESET;

	if (cqe_on)
		ret = sdhci_cqe_add_host(host, pdev);
	else
		ret = sdhci_add_host(host);

	if (ret)
		sdhci_pltfm_free(pdev);

	/* Set minimum tuning window size */
	if (win_size) {
		vendor3 = sdhci_readl(host, SDHCI_FTSDC021_VENDOR3);
		vendor3 &= 0xFFFFFFE0;
		vendor3 |= be32_to_cpup(win_size);
		sdhci_writel(host, vendor3, SDHCI_FTSDC021_VENDOR3);
	}

	return ret;
}

static int ftsdc021_remove(struct platform_device *pdev)
{
	return sdhci_pltfm_unregister(pdev);
}

static struct platform_driver ftsdc021_driver = {
	.driver     = {
		.name   = "ftsdc021",
		.owner  = THIS_MODULE,
		.of_match_table = sdhci_ftsdc021_of_match_table,
		.pm     = &sdhci_pltfm_pmops,
	},
	.probe      = ftsdc021_probe,
	.remove     = ftsdc021_remove,
};

module_platform_driver(ftsdc021_driver);

MODULE_DESCRIPTION("SDHCI driver for FTSDC021");
MODULE_AUTHOR("Bing-Yao, Luo <bjluo@faraday-tech.com>");
MODULE_LICENSE("GPL v2");
