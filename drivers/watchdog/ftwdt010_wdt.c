// SPDX-License-Identifier: GPL-2.0
/*
 * Faraday FTWDT010 WatchDog Timer
 *
 * Copyright (C) 2013-2021 Faraday Technology Corporation
 *
 * Author: Faraday CTD/SD Dept.
 *
 */

#include <linux/bitops.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/watchdog.h>
#include <linux/clk.h>

#define FTWDT010_WDCOUNTER      0x00
#define FTWDT010_WDLOAD         0x04
#define FTWDT010_WDRESTART      0x08
#define FTWDT010_WDCR           0x0c
#define FTWDT010_WDSTATUS       0x10
#define FTWDT010_WDCLEAR        0x14
#define FTWDT010_WDINTRLEN      0x18
#define FTWDT010_WDREVISON      0x1c

#define WDRESTART_MAGIC         0x5AB9

#define WDCR_CLOCK              BIT(4)
#define WDCR_EXT                BIT(3)
#define WDCR_INTR               BIT(2)
#define WDCR_RST                BIT(1)
#define WDCR_ENABLE             BIT(0)

#define FTWDT010_DEFAULT_TIMEOUT	60	/* in second */

struct ftwdt010_wdt {
	struct watchdog_device  wdd;
	struct device           *dev;
	void __iomem            *base;
	struct resource         *res;
	struct clk              *clk;
	int                     irq;
	spinlock_t              lock;
	unsigned int            counter;
};

static void ftwdt010_wdt_clear_interrupt(struct ftwdt010_wdt *ftwdt010)
{
	iowrite32(0, ftwdt010->base + FTWDT010_WDCLEAR);
}

static void __ftwdt010_wdt_stop(struct ftwdt010_wdt *ftwdt010)
{
	unsigned long cr;

	cr = ioread32(ftwdt010->base + FTWDT010_WDCR);
	cr &= ~WDCR_ENABLE;
	iowrite32(cr, ftwdt010->base + FTWDT010_WDCR);
}

static int ftwdt010_wdt_start(struct watchdog_device *wdd)
{
	struct ftwdt010_wdt *ftwdt010 = watchdog_get_drvdata(wdd);
	unsigned long cr;
	unsigned long flags;

	spin_lock_irqsave(&ftwdt010->lock, flags);

	__ftwdt010_wdt_stop(ftwdt010);

	cr = ioread32(ftwdt010->base + FTWDT010_WDCR);
	cr |= WDCR_ENABLE | WDCR_RST | WDCR_INTR;

	iowrite32(ftwdt010->counter, ftwdt010->base + FTWDT010_WDLOAD);
	iowrite32(WDRESTART_MAGIC, ftwdt010->base + FTWDT010_WDRESTART);
	iowrite32(cr, ftwdt010->base + FTWDT010_WDCR);

	spin_unlock_irqrestore(&ftwdt010->lock, flags);

	return 0;
}

static int ftwdt010_wdt_stop(struct watchdog_device *wdd)
{
	struct ftwdt010_wdt *ftwdt010 = watchdog_get_drvdata(wdd);
	unsigned long flags;

	spin_lock_irqsave(&ftwdt010->lock, flags);
	__ftwdt010_wdt_stop(ftwdt010);
	spin_unlock_irqrestore(&ftwdt010->lock, flags);

	return 0;
}

static int ftwdt010_wdt_ping(struct watchdog_device *wdd)
{
	struct ftwdt010_wdt *ftwdt010 = watchdog_get_drvdata(wdd);
	unsigned long flags;

	spin_lock_irqsave(&ftwdt010->lock, flags);
	iowrite32(WDRESTART_MAGIC, ftwdt010->base + FTWDT010_WDRESTART);
	spin_unlock_irqrestore(&ftwdt010->lock, flags);

	return 0;
}

static int ftwdt010_wdt_set_timeout(struct watchdog_device *wdd,
				unsigned int timeout)
{
	struct ftwdt010_wdt *ftwdt010 = watchdog_get_drvdata(wdd);
	unsigned long clkrate = clk_get_rate(ftwdt010->clk);
	unsigned int counter;
	unsigned long flags;

	/* WdCounter is 32bit register. If the counter is larger than
	   0xFFFFFFFF, set the counter as 0xFFFFFFFF*/
	if (clkrate > 0xFFFFFFFF / timeout)
		counter = 0xFFFFFFFF;
	else
		counter = clkrate * timeout;

	wdd->timeout = counter / clkrate;

	spin_lock_irqsave(&ftwdt010->lock, flags);
	ftwdt010->counter = counter;
	iowrite32(counter, ftwdt010->base + FTWDT010_WDLOAD);
	iowrite32(WDRESTART_MAGIC, ftwdt010->base + FTWDT010_WDRESTART);
	spin_unlock_irqrestore(&ftwdt010->lock, flags);

	return 0;
}


static const struct watchdog_info ftwdt010_wdt_info = {
	.options    = WDIOF_KEEPALIVEPING
	            | WDIOF_MAGICCLOSE
	            | WDIOF_SETTIMEOUT,
	.identity   = "FTWDT010 Watchdog",
};

static struct watchdog_ops ftwdt010_wdt_ops = {
	.owner  = THIS_MODULE,
	.start  = ftwdt010_wdt_start,
	.stop   = ftwdt010_wdt_stop,
	.ping   = ftwdt010_wdt_ping,
	.set_timeout = ftwdt010_wdt_set_timeout,
};

static irqreturn_t ftwdt010_wdt_interrupt(int irq, void *dev_id)
{
	struct ftwdt010_wdt *ftwdt010 = dev_id;

	dev_info(ftwdt010->dev, "watchdog timer expired\n");

	ftwdt010_wdt_ping(&ftwdt010->wdd);
	ftwdt010_wdt_clear_interrupt(ftwdt010);
	return IRQ_HANDLED;
}

static int ftwdt010_wdt_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ftwdt010_wdt *ftwdt010;
	struct resource *res;
	int irq;
	int ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "No mmio resource defined.\n");
		return -ENXIO;
	}

	if ((irq = platform_get_irq(pdev, 0)) < 0) {
		dev_err(dev, "Failed to get irq.\n");
		goto err_get_irq;
		return -ENXIO;
	}

	/* Allocate driver private data */
	ftwdt010 = kzalloc(sizeof(struct ftwdt010_wdt), GFP_KERNEL);
	if (!ftwdt010) {
		dev_err(dev, "Failed to allocate memory.\n");
		ret = -ENOMEM;
		goto err_alloc_priv;
	}

	/* Setup watchdog_device */
	ftwdt010->wdd.timeout = FTWDT010_DEFAULT_TIMEOUT;
	ftwdt010->wdd.info = &ftwdt010_wdt_info;
	ftwdt010->wdd.ops = &ftwdt010_wdt_ops;
	ftwdt010->dev = dev;
	platform_set_drvdata(pdev, ftwdt010);
	watchdog_set_drvdata(&ftwdt010->wdd, ftwdt010);

	/* This overrides the default timeout only if DT configuration was found */
	watchdog_init_timeout(&ftwdt010->wdd, 0, dev);

	/* Setup clock */
#ifdef CONFIG_OF
	ftwdt010->clk = devm_clk_get(dev, NULL);
#else
	ftwdt010->clk = devm_clk_get(dev, "pclk");
#endif
	if (IS_ERR(ftwdt010->clk)) {
		dev_err(dev, "Failed to get clock.\n");
		ret = PTR_ERR(ftwdt010->clk);
		goto err_clk_get;
	}
	ret = clk_prepare_enable(ftwdt010->clk);
	if (ret < 0) {
		dev_err(dev, "failed to enable clock\n");
		return ret;
	}

	/* Map io memory */
	ftwdt010->res = request_mem_region(res->start, resource_size(res),
	                                   dev_name(dev));
	if (!ftwdt010->res) {
		dev_err(dev, "Resources is unavailable.\n");
		ret = -EBUSY;
		goto err_req_mem_region;
	}

	ftwdt010->base = ioremap(res->start, resource_size(res));
	if (!ftwdt010->base) {
		dev_err(dev, "Failed to map registers.\n");
		ret = -ENOMEM;
		goto err_ioremap;
	}

	/* Register irq */
	ret = request_irq(irq, ftwdt010_wdt_interrupt, 0, pdev->name, ftwdt010);
	if (ret < 0) {
		dev_err(dev, "Failed to request irq %d\n", irq);
		goto err_req_irq;
	}
	ftwdt010->irq = irq;

	spin_lock_init(&ftwdt010->lock);
	/* Set the default timeout (60 sec). If the clock source (PCLK) is
	   larger than 71582788 Hz, the reset time will be less than 60 sec.
	   For A380, PCLK is 99 MHz, the maximum reset time is 43 sec.*/
	ftwdt010_wdt_set_timeout(&ftwdt010->wdd, ftwdt010->wdd.timeout);

	/* Register watchdog */
	ret = watchdog_register_device(&ftwdt010->wdd);
	if (ret) {
		dev_err(dev, "cannot register watchdog (%d)\n", ret);
		goto err_register_wdd;
	}

	dev_info(dev, "irq %d, mapped at %px\n", irq, ftwdt010->base);

//	dev_info(dev, "start watchdog timer\n");
//	ftwdt010_wdt_start(&ftwdt010->wdd);

	return 0;

err_register_wdd:
	free_irq(irq, ftwdt010);
err_req_irq:
	iounmap(ftwdt010->base);
err_ioremap:
	release_mem_region(res->start, resource_size(res));
err_req_mem_region:
	clk_disable_unprepare(ftwdt010->clk);
err_clk_get:
	platform_set_drvdata(pdev, NULL);
	kfree(ftwdt010);
err_alloc_priv:
err_get_irq:
	release_resource(res);
	return ret;
}

static int ftwdt010_wdt_remove(struct platform_device *pdev)
{
	struct ftwdt010_wdt *ftwdt010 = platform_get_drvdata(pdev);
	struct resource *res = ftwdt010->res;

	watchdog_unregister_device(&ftwdt010->wdd);

	free_irq(ftwdt010->irq, ftwdt010);
	iounmap(ftwdt010->base);
	clk_disable_unprepare(ftwdt010->clk);
	release_mem_region(res->start, resource_size(res));
	platform_set_drvdata(pdev, NULL);
	kfree(ftwdt010);
	release_resource(res);

	return 0;
}

static int __maybe_unused ftwdt010_wdt_suspend(struct device *dev)
{
	struct ftwdt010_wdt *ftwdt010 = dev_get_drvdata(dev);

	__ftwdt010_wdt_stop(ftwdt010);
	return 0;
}

static int __maybe_unused ftwdt010_wdt_resume(struct device *dev)
{
	struct ftwdt010_wdt *ftwdt010 = dev_get_drvdata(dev);
	unsigned int cr;

	if (watchdog_active(&ftwdt010->wdd)) {
		cr = ioread32(ftwdt010->base + FTWDT010_WDCR);
		cr |= WDCR_ENABLE;
		iowrite32(cr, ftwdt010->base + FTWDT010_WDCR);
	}

	return 0;
}

static const struct dev_pm_ops ftwdt010_wdt_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(ftwdt010_wdt_suspend,
	                        ftwdt010_wdt_resume)
};

static const struct of_device_id ftwdt010_wdt_of_match[] = {
	{ .compatible = "faraday,ftwdt010", },
	{ },
};
MODULE_DEVICE_TABLE(of, ftwdt010_wdt_of_match);

static struct platform_driver ftwdt010_wdt_driver = {
	.probe      = ftwdt010_wdt_probe,
	.remove     = ftwdt010_wdt_remove,
	.driver     = {
		.name   = "ftwdt010-wdt",
		.of_match_table = ftwdt010_wdt_of_match,
		.pm = &ftwdt010_wdt_dev_pm_ops,
	},
};

module_platform_driver(ftwdt010_wdt_driver);

MODULE_AUTHOR("Faraday Technology Corporation");
MODULE_DESCRIPTION("FTWDT010 WatchDog Timer Driver");
MODULE_LICENSE("GPL");
