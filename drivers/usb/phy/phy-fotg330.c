// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2015-2019 Faraday Technology Corp.
 *
 * Author: Faraday CTD/SD Dept.
 *
 * This driver helps to switch Faraday controller function between host
 * and peripheral. It works with XHCI driver and Faraday peripheral controller
 * driver together.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/clk.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include <linux/usb.h>
#include <linux/usb/ch9.h>
#include <linux/usb/otg.h>
#include <linux/usb/gadget.h>
#include <linux/usb/hcd.h>

#include "phy-fotg330.h"

#define	DRIVER_DESC	"Faraday USB OTG transceiver driver"

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

static const char driver_name[] = "fotg330-otg";

static char *state_string[] = {
	"undefined",
	"b_device", //"b_idle",	//lichun@modify
	"b_srp_init",
	"b_peripheral",
	"b_wait_acon",
	"b_host",
	"a_host", //"a_idle",	//lichun@modify
	"a_wait_vrise",
	"a_wait_bcon",
	"a_host",
	"a_suspend",
	"a_peripheral",
	"a_wait_vfall",
	"a_vbus_err"
};

static int fotg330_otg_set_host(struct usb_otg *otg,
			   struct usb_bus *host)
{
	otg->host = host;

	return 0;
}

static int fotg330_otg_set_peripheral(struct usb_otg *otg,
				 struct usb_gadget *gadget)
{
	otg->gadget = gadget;

	return 0;
}

/* get current role (0: host; 1: peripheral) */
static u8 fotg330_get_role(struct fotg330_otg *fotg330)
{
	u32 temp = ioread32(fotg330->regs + FOTG330_OCSR);

	if (temp & OCSR_CROLE)
		return 1;	//Peripheral
	else
		return 0;
}

/* get current id (0: A-type; 1: B-type) */
static u8 fotg330_get_id(struct fotg330_otg *fotg330)
{
	u32 temp = ioread32(fotg330->regs + FOTG330_OCSR);

	if (temp & OCSR_CID)
		return 1;	//B-type
	else
		return 0;
}

/* set device vbus debounce: 0x3ff (vbus gating), < 0x3ff (has vbus)*/
static void fotg330_set_vbus(struct fotg330_otg *fotg330,
				u16 deb_time)
{
        u32 reg;

        reg = ioread32(fotg330->regs + FOTG330_DEV_EFCS);
        reg &= ~(0x3ff << 0);
        reg |= EFCS_VBUS_DEBOUNCE(deb_time);
        iowrite32(reg, fotg330->regs + FOTG330_DEV_EFCS);
}

static void fotg330_run_state_machine(struct fotg330_otg *fotg330,
					unsigned long delay)
{
	dev_dbg(&fotg330->pdev->dev, "transceiver is updated\n");
	if (!fotg330->qwork)
		return;

	queue_delayed_work(fotg330->qwork, &fotg330->work, delay);
}

static void fotg330_otg_update_state(struct fotg330_otg *fotg330)
{
	int old_state = fotg330->phy.otg->state;

	switch (old_state) {
	case OTG_STATE_UNDEFINED:
		fotg330->phy.otg->state = OTG_STATE_B_IDLE;
		/* FALL THROUGH */
	case OTG_STATE_B_IDLE:
		if (fotg330_get_role(fotg330) == CROLE_Host)
			fotg330->phy.otg->state = OTG_STATE_A_IDLE;
		break;

	case OTG_STATE_A_IDLE:
		if (fotg330_get_role(fotg330) == CROLE_Peripheral)
			fotg330->phy.otg->state = OTG_STATE_B_IDLE;
		break;

	default:
		break;
	}
}

static void fotg330_otg_work(struct work_struct *work)
{
	struct fotg330_otg *fotg330;
	struct usb_phy *phy;
	struct usb_otg *otg;
	int old_state;
	u32 val;

	fotg330 = container_of(to_delayed_work(work), struct fotg330_otg, work);

run:
	/* work queue is single thread, or we need spin_lock to protect */
	phy = &fotg330->phy;
	otg = fotg330->phy.otg;
	old_state = otg->state;

	fotg330_otg_update_state(fotg330);

	if (old_state != fotg330->phy.otg->state) {
		dev_info(&fotg330->pdev->dev, "change from state %s to %s\n",
			 state_string[old_state],
			 state_string[fotg330->phy.otg->state]);

		switch (fotg330->phy.otg->state) {
		case OTG_STATE_B_IDLE:
			otg->default_a = 0;

			//STOP host
			val = ioread32(fotg330->regs + FOTG330_XHCI_USBCMD);
			val &= ~USBCMD_RUN;
			iowrite32(val, fotg330->regs + FOTG330_XHCI_USBCMD);

			while ((ioread32(fotg330->regs + FOTG330_XHCI_USBSTS) & USBSTS_HCHALT) == 0) ;

			//set device global interrupt
			iowrite32(GIER_DEV_INT_EN | GIER_OTG_INT_EN,
				fotg330->regs + FOTG330_GIER);

			//enable device grp1 interrupt
			iowrite32(0xcfffff9f, fotg330->regs + FOTG330_DEV_IGER1);

			//release device vbus gating
			fotg330_set_vbus(fotg330, 0x300);

			break;

		case OTG_STATE_A_IDLE:
			otg->default_a = 1;

			//set device vbus gating
			fotg330_set_vbus(fotg330, 0x3ff);

			//disable device grp1 interrupt
			iowrite32(0x0, fotg330->regs + FOTG330_DEV_IGER1);

			//set host global interrupt
			iowrite32(GIER_HOST_INT_EN | GIER_OTG_INT_EN,
				fotg330->regs + FOTG330_GIER);

			//RUN host
			val = ioread32(fotg330->regs + FOTG330_XHCI_USBCMD);
			val |= USBCMD_RUN;
			iowrite32(val, fotg330->regs + FOTG330_XHCI_USBCMD);

			while ((ioread32(fotg330->regs + FOTG330_XHCI_USBSTS) & USBSTS_HCHALT) == 1) ;
			break;

		default:
			break;
		}
		goto run;
	}
}


static irqreturn_t fotg330_otg_irq(int irq, void *dev)
{
	struct fotg330_otg *fotg330 = dev;
	u32 otgsc;

	/* Write 1 to clear */
	otgsc = ioread32(fotg330->regs + FOTG330_OISR);
	if (otgsc)
		iowrite32(otgsc, fotg330->regs + FOTG330_OISR);

	if ((otgsc & fotg330->irq_en) == 0)
		return IRQ_NONE;

	fotg330_run_state_machine(fotg330, 0);

	return IRQ_HANDLED;
}

static void fotg330_init(struct fotg330_otg *fotg330)
{
	u32 val;

	//clear otg interrupt status (write '1' clear)
	val = ioread32(fotg330->regs + FOTG330_OISR);
	iowrite32(val, fotg330->regs + FOTG330_OISR);

	//set otg interrupt enable
	fotg330->irq_en = OIER_RLCHG_EN;
	iowrite32(fotg330->irq_en, fotg330->regs + FOTG330_OIER);

	fotg330_otg_update_state(fotg330);

	/* set global interrupt */
	if (fotg330_get_role(fotg330) == CROLE_Host)
		iowrite32(GIER_HOST_INT_EN | GIER_OTG_INT_EN,
			fotg330->regs + FOTG330_GIER);
	else
		iowrite32(GIER_DEV_INT_EN | GIER_OTG_INT_EN,
			fotg330->regs + FOTG330_GIER);
}

static int fotg330_otg_probe(struct platform_device *pdev)
{
	struct fotg330_otg *fotg330;
	struct usb_otg *otg;
	struct resource *res;
	int retval = 0;

	fotg330 = devm_kzalloc(&pdev->dev, sizeof(*fotg330), GFP_KERNEL);
	if (!fotg330) {
		dev_err(&pdev->dev, "failed to allocate memory!\n");
		return -ENOMEM;
	}

	otg = devm_kzalloc(&pdev->dev, sizeof(*otg), GFP_KERNEL);
	if (!otg)
		return -ENOMEM;

	platform_set_drvdata(pdev, fotg330);

	fotg330->qwork = create_singlethread_workqueue("fotg330_otg_queue");
	if (!fotg330->qwork) {
		dev_dbg(&pdev->dev, "cannot create workqueue for OTG\n");
		return -ENOMEM;
	}

	INIT_DELAYED_WORK(&fotg330->work, fotg330_otg_work);

	/* OTG common part */
	fotg330->pdev = pdev;
	fotg330->phy.dev = &pdev->dev;
	fotg330->phy.otg = otg;
	fotg330->phy.label = driver_name;

	otg->state = OTG_STATE_UNDEFINED;
	otg->usb_phy = &fotg330->phy;
	otg->set_host = fotg330_otg_set_host;
	otg->set_peripheral = fotg330_otg_set_peripheral;

	res = platform_get_resource(fotg330->pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "no I/O memory resource defined\n");
		retval = -ENODEV;
		goto err_destroy_workqueue;
	}

	fotg330->regs = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (fotg330->regs == NULL) {
		dev_err(&pdev->dev, "failed to map I/O memory\n");
		retval = -EFAULT;
		goto err_destroy_workqueue;
	}

	//initial HW setting
	fotg330_init(fotg330);

	res = platform_get_resource(fotg330->pdev, IORESOURCE_IRQ, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "no IRQ resource defined\n");
		retval = -ENODEV;
		goto err_destroy_workqueue;
	}

	fotg330->irq = res->start;
	if (devm_request_irq(&pdev->dev, fotg330->irq, fotg330_otg_irq, IRQF_SHARED,
			driver_name, fotg330)) {
		dev_err(&pdev->dev, "Request irq %d for FOTG330 failed\n",
			fotg330->irq);
		fotg330->irq = 0;
		retval = -ENODEV;
		goto err_destroy_workqueue;
	}

	retval = usb_add_phy(&fotg330->phy, USB_PHY_TYPE_USB3);
	if (retval < 0) {
		dev_err(&pdev->dev, "can't register transceiver, %d\n",
			retval);
		goto err_destroy_workqueue;
	}

	spin_lock_init(&fotg330->wq_lock);
	if (spin_trylock(&fotg330->wq_lock)) {
		fotg330_run_state_machine(fotg330, 2 * HZ);
		spin_unlock(&fotg330->wq_lock);
	}

	dev_info(&pdev->dev, "successful probe FOTG330\n");

	return 0;

	usb_remove_phy(&fotg330->phy);
err_destroy_workqueue:
	flush_workqueue(fotg330->qwork);
	destroy_workqueue(fotg330->qwork);

	return retval;
}

static int fotg330_otg_remove(struct platform_device *pdev)
{
	struct fotg330_otg *fotg330 = platform_get_drvdata(pdev);

	if (fotg330->qwork) {
		flush_workqueue(fotg330->qwork);
		destroy_workqueue(fotg330->qwork);
	}

	usb_remove_phy(&fotg330->phy);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id fotg330_otg_dt_ids[] = {
	{ .compatible = "faraday,fotg330" },
	{ }
};

MODULE_DEVICE_TABLE(of, fotg330_otg_dt_ids);
#endif

static struct platform_driver fotg330_otg_driver = {
	.probe	= fotg330_otg_probe,
	.remove	= fotg330_otg_remove,
	.driver	= {
		.name = driver_name,
		.of_match_table = of_match_ptr(fotg330_otg_dt_ids),
	},
};
module_platform_driver(fotg330_otg_driver);
