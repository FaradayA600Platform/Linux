// SPDX-License-Identifier: GPL-2.0
/*
 * FUSB300 UDC (USB gadget)
 *
 * Copyright (C) 2010-2021 Faraday Technology Corp.
 *
 * Author: Faraday CTD/SD Dept.
 */
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

#include "fusb300_udc.h"

#define IDMA_MODE	//IDMA mode or PIO mode
//#define POLLING_PRD	//polling method or IRQ method to wait idma complete

MODULE_DESCRIPTION("FUSB300 USB gadget driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Faraday Technology Corporation");
MODULE_ALIAS("platform:fusb300_udc");

static const char udc_name[] = "fusb300_udc";
static const char * const fusb300_ep_name[] = {
	"ep0", "ep1", "ep2", "ep3", "ep4", "ep5", "ep6", "ep7", "ep8", "ep9",
	"ep10", "ep11", "ep12", "ep13", "ep14", "ep15"
};


/* only use for value = 1 */
static void fusb300_enable_bit(struct fusb300 *fusb300, u32 offset,
			       u32 value)
{
	u32 reg = ioread32(fusb300->reg + offset);

	reg |= value;
	iowrite32(reg, fusb300->reg + offset);
}

static void fusb300_disable_bit(struct fusb300 *fusb300, u32 offset,
				u32 value)
{
	u32 reg = ioread32(fusb300->reg + offset);

	reg &= ~value;
	iowrite32(reg, fusb300->reg + offset);
}

static int enable_fifo_int(struct fusb300_ep *ep)
{
	struct fusb300 *fusb300 = ep->fusb300;

	if (ep->epnum) {
		fusb300_enable_bit(fusb300, FUSB300_OFFSET_IGER0,
			FUSB300_IGER0_EEPn_FIFO_INT(ep->epnum));
	} else {
		pr_err("can't enable_fifo_int ep0\n");
		return -EINVAL;
	}

	return 0;
}

static int disable_fifo_int(struct fusb300_ep *ep)
{
	struct fusb300 *fusb300 = ep->fusb300;

	if (ep->epnum) {
		fusb300_disable_bit(fusb300, FUSB300_OFFSET_IGER0,
			FUSB300_IGER0_EEPn_FIFO_INT(ep->epnum));
	} else {
		pr_err("can't disable_fifo_int ep0\n");
		return -EINVAL;
	}

	return 0;
}

static void fusb300_set_cxdone(struct fusb300 *fusb300)
{
	fusb300_enable_bit(fusb300, FUSB300_OFFSET_CSR,
			   FUSB300_CSR_DONE);
}

static void fusb300_done(struct fusb300_ep *ep, struct fusb300_request *req,
		 int status)
__releases(ep->fusb300->lock)
__acquires(ep->fusb300->lock)
{
	list_del_init(&req->queue);

	/* don't modify queue heads during completion callback */
	if (ep->fusb300->gadget.speed == USB_SPEED_UNKNOWN)
		req->req.status = -ESHUTDOWN;
	else
		req->req.status = status;

	spin_unlock(&ep->fusb300->lock);
	if (req->req.complete)
		usb_gadget_giveback_request(&ep->ep, &req->req);
	spin_lock(&ep->fusb300->lock);

	if (ep->epnum) {
		disable_fifo_int(ep);
		if (!list_empty(&ep->queue))
			enable_fifo_int(ep);
	} else
		fusb300_set_cxdone(ep->fusb300);
}


static int fusb300_ep_release(struct fusb300_ep *ep)
{
	if (!ep->epnum)
		return 0;
	ep->epnum = 0;
	ep->stall = 0;
	ep->wedged = 0;
	return 0;
}

static void fusb300_set_eptype(struct fusb300_ep *ep)
{
	struct fusb300 *fusb300 = ep->fusb300;
	u32 reg;

	reg = ioread32(fusb300->reg + FUSB300_OFFSET_EPSET1(ep->epnum));
	reg &= ~FUSB300_EPSET1_TYPE_MSK;
	reg |= FUSB300_EPSET1_TYPE(ep->type);
	iowrite32(reg, fusb300->reg + FUSB300_OFFSET_EPSET1(ep->epnum));
}

static void fusb300_set_epdir(struct fusb300_ep *ep)
{
	struct fusb300 *fusb300 = ep->fusb300;
	u32 reg;

	reg = ioread32(fusb300->reg + FUSB300_OFFSET_EPSET1(ep->epnum));
	reg &= ~FUSB300_EPSET1_DIR_MSK;
	if (ep->dir_in)
		reg |= FUSB300_EPSET1_DIRIN;
	iowrite32(reg, fusb300->reg + FUSB300_OFFSET_EPSET1(ep->epnum));
}

static void fusb300_set_epmps(struct fusb300_ep *ep)
{
	struct fusb300 *fusb300 = ep->fusb300;
	u32 reg;

	reg= ioread32(fusb300->reg + FUSB300_OFFSET_EPSET2(ep->epnum));
	reg &= ~FUSB300_EPSET2_MPS_MSK;
	reg |= FUSB300_EPSET2_MPS(ep->ep.maxpacket);
	iowrite32(reg, fusb300->reg + FUSB300_OFFSET_EPSET2(ep->epnum));
}

/* For SS ISO IN */
static void fusb300_set_ISO_IN_pktnum(struct fusb300_ep *ep)
{
	struct fusb300 *fusb300 = ep->fusb300;
	u32 reg;

	reg = ioread32(fusb300->reg + FUSB300_OFFSET_EPSET1(ep->epnum));
	reg &= ~FUSB300_EPSET1_ISO_IN_PKTNUM(0x3F);
	reg |= FUSB300_EPSET1_ISO_IN_PKTNUM(ep->bw_num);
	iowrite32(reg, fusb300->reg + FUSB300_OFFSET_EPSET1(ep->epnum));
}

static void fusb300_set_bwnum(struct fusb300_ep *ep)
{
	struct fusb300 *fusb300 = ep->fusb300;
	u32 reg;

	reg = ioread32(fusb300->reg + FUSB300_OFFSET_EPSET1(ep->epnum));
	reg &= ~FUSB300_EPSET1_BWNUM(0x3);
	reg |= FUSB300_EPSET1_BWNUM(ep->bw_num);
	iowrite32(reg, fusb300->reg + FUSB300_OFFSET_EPSET1(ep->epnum));
}

static void fusb300_set_fifo_entry(struct fusb300_ep *ep)
{
	struct fusb300 *fusb300 = ep->fusb300;
	u32 reg;

	reg = ioread32(fusb300->reg + FUSB300_OFFSET_EPSET1(ep->epnum));
	reg &= ~FUSB300_EPSET1_FIFOENTRY_MSK;
	reg |= FUSB300_EPSET1_FIFOENTRY(ep->fifo_entry_num);
	iowrite32(reg, fusb300->reg + FUSB300_OFFSET_EPSET1(ep->epnum));
}

static void fusb300_set_start_entry(struct fusb300_ep *ep)
{
	struct fusb300 *fusb300 = ep->fusb300;
	u32 reg;
	u8 tmp;

	reg = ioread32(fusb300->reg + FUSB300_OFFSET_EPSET1(ep->epnum));
	reg &= ~FUSB300_EPSET1_START_ENTRY_MSK;
	reg |= FUSB300_EPSET1_START_ENTRY(fusb300->start_entry);
	iowrite32(reg, fusb300->reg + FUSB300_OFFSET_EPSET1(ep->epnum));

	fusb300->start_entry += ep->fifo_entry_num;

	reg = ioread32(fusb300->reg + FUSB300_OFFSET_FEATURE2);
	if (fusb300->start_entry > FUSB300_TOTAL_ENTRY(reg)) {
		pr_err("WARN!! fifo entry %d (%d) is over the maximum number %d!\n",
			fusb300->start_entry, ep->fifo_entry_num, FUSB300_TOTAL_ENTRY(reg));
		tmp = fusb300->start_entry - FUSB300_TOTAL_ENTRY(reg);
		fusb300->start_entry -= ep->fifo_entry_num;
		//change
		ep->fifo_entry_num -= tmp;
		if (ep->fifo_entry_num == 0) {
			pr_err("ERR!! Please modify the fifo entry number!\n");
			while(1);
		}
		pr_err("Modify ep%d fifo entry to %d\n", ep->epnum, ep->fifo_entry_num);
		fusb300->start_entry += ep->fifo_entry_num;
	}
}

/* set fusb300_set_start_entry first before fusb300_set_epaddrofs */
static void fusb300_set_epaddrofs(struct fusb300_ep *ep)
{
	struct fusb300 *fusb300 = ep->fusb300;
	u32 reg;

	reg = ioread32(fusb300->reg + FUSB300_OFFSET_EPSET2(ep->epnum));
	reg &= ~FUSB300_EPSET2_ADDROFS_MSK;
	reg |= FUSB300_EPSET2_ADDROFS(fusb300->addrofs);
	iowrite32(reg, fusb300->reg + FUSB300_OFFSET_EPSET2(ep->epnum));
	fusb300->addrofs += ((ep->ep.maxpacket + 7) / 8 * ep->fifo_entry_num);
}

static void fusb300_set_ep_active(struct fusb300_ep *ep)
{
	struct fusb300 *fusb300 = ep->fusb300;
	u32 reg;

	reg = ioread32(fusb300->reg + FUSB300_OFFSET_EPSET1(ep->epnum));
	reg |= FUSB300_EPSET1_ACTEN;
	iowrite32(reg, fusb300->reg + FUSB300_OFFSET_EPSET1(ep->epnum));
}

static void fusb300_set_max_interval(struct fusb300 *fusb300)
{
	u32 reg = ioread32(fusb300->reg + FUSB300_OFFSET_SSCR0);

	reg &= ~FUSB300_SSCR0_MAX_INTERVAL(0x7);
	reg |= FUSB300_SSCR0_MAX_INTERVAL(fusb300->max_interval);
	iowrite32(reg, fusb300->reg + FUSB300_OFFSET_SSCR0);
}

static int config_ep(struct fusb300_ep *ep)
{
	struct fusb300 *fusb300 = ep->fusb300;

	/* transfer type */
	fusb300_set_eptype(ep);

	/* direction */
	fusb300_set_epdir(ep);

	/* service interval */
	if (fusb300->gadget.speed == USB_SPEED_SUPER) {
		if ((ep->type == USB_ENDPOINT_XFER_ISOC) && (ep->dir_in))
			fusb300_set_ISO_IN_pktnum(ep);

		if (fusb300->max_interval)
			fusb300_set_max_interval(fusb300);
	}
	else {
		fusb300_set_bwnum(ep);
	}

	/* fifo entry */
	fusb300_set_start_entry(ep);
	fusb300_set_fifo_entry(ep);

	/* max packet size */
	fusb300_set_epmps(ep);

	/* ep fifo address offset, if the first ep to set, set as 0 */
	fusb300_set_epaddrofs(ep);

	/* active */
	fusb300_set_ep_active(ep);

	fusb300->ep[ep->epnum] = ep;

	return 0;
}


static int fusb300_enable(struct usb_ep *_ep,
			  const struct usb_endpoint_descriptor *desc)
{
	struct fusb300_ep *ep;

	ep = container_of(_ep, struct fusb300_ep, ep);

	ep->ep.desc = desc;
	ep->epnum = usb_endpoint_num(desc);
	ep->type = usb_endpoint_type(desc);
	ep->dir_in = usb_endpoint_dir_in(desc);
	ep->ep.maxpacket = usb_endpoint_maxp(desc);

	if (ep->fusb300->reenum) {
		ep->fusb300->max_interval = 0;
		ep->fusb300->start_entry = 0;
		ep->fusb300->addrofs = 0;
		ep->fusb300->reenum = 0;
	}

	/* service interval */
	if (ep->fusb300->gadget.speed == USB_SPEED_SUPER) {
		if ((ep->type == USB_ENDPOINT_XFER_INT) ||
		    (ep->type == USB_ENDPOINT_XFER_ISOC)) {
			if (ep->fusb300->max_interval < desc->bInterval)
				ep->fusb300->max_interval = desc->bInterval;

			if (ep->type == USB_ENDPOINT_XFER_ISOC) {
				ep->bw_num = (ep->ep.comp_desc->bMaxBurst + 1) *
					USB_SS_MULT(ep->ep.comp_desc->bmAttributes);
			}
		}
	}
	else {
		ep->bw_num = usb_endpoint_maxp_mult(desc);
	}

	/* fifo entry: take care the total fifo entry size */
	if (ep->fusb300->gadget.speed == USB_SPEED_SUPER) {
		//set to the burst number
		ep->fifo_entry_num = (ep->ep.comp_desc->bMaxBurst + 1);
		//ep->fifo_entry_num = FUSB300_FIFO_ENTRY_NUM;
	}
	else {
		//For the HS/FS mode, set triple FIFOs to improve performance
		ep->fifo_entry_num = FUSB300_FIFO_ENTRY_TRIPLE;
	}

	return config_ep(ep);
}

static int fusb300_disable(struct usb_ep *_ep)
{
	struct fusb300_ep *ep;
	struct fusb300_request *req;
	unsigned long flags;

	ep = container_of(_ep, struct fusb300_ep, ep);

	BUG_ON(!ep);

	while (!list_empty(&ep->queue)) {
		req = list_entry(ep->queue.next, struct fusb300_request, queue);
		spin_lock_irqsave(&ep->fusb300->lock, flags);
		fusb300_done(ep, req, -ECONNRESET);
		spin_unlock_irqrestore(&ep->fusb300->lock, flags);
	}

	ep->ep.desc = NULL;
	return fusb300_ep_release(ep);
}

static struct usb_request *fusb300_alloc_request(struct usb_ep *_ep,
						gfp_t gfp_flags)
{
	struct fusb300_request *req;

	req = kzalloc(sizeof(struct fusb300_request), gfp_flags);
	if (!req)
		return NULL;
	INIT_LIST_HEAD(&req->queue);

	return &req->req;
}

static void fusb300_free_request(struct usb_ep *_ep, struct usb_request *_req)
{
	struct fusb300_request *req;

	req = container_of(_req, struct fusb300_request, req);
	kfree(req);
}

static void fusb300_set_cxlen(struct fusb300 *fusb300, u32 length)
{
	u32 reg;

	reg = ioread32(fusb300->reg + FUSB300_OFFSET_CSR);
	reg &= ~FUSB300_CSR_LEN_MSK;
	reg |= FUSB300_CSR_LEN(length);
	iowrite32(reg, fusb300->reg + FUSB300_OFFSET_CSR);
}

/* write data to cx fifo */
static void fusb300_wrcxf(struct fusb300_ep *ep,
		   struct fusb300_request *req)
{
	int i = 0;
	u8 *tmp;
	u32 data;
	struct fusb300 *fusb300 = ep->fusb300;
	u32 length = req->req.length - req->req.actual;
	u32 ep0_mps = ep->ep.maxpacket;

	tmp = req->req.buf + req->req.actual;

	if (length > ep0_mps) {
		for (i = (ep0_mps >> 2); i > 0; i--) {
			data = *tmp | *(tmp + 1) << 8 | *(tmp + 2) << 16 |
				*(tmp + 3) << 24;
			iowrite32(data, fusb300->reg + FUSB300_OFFSET_CXPORT);
			tmp += 4;
		}
		req->req.actual += ep0_mps;
	} else { /* length is less than max packet size */
		for (i = length >> 2; i > 0; i--) {
			data = *tmp | *(tmp + 1) << 8 | *(tmp + 2) << 16 |
				*(tmp + 3) << 24;
			printk(KERN_DEBUG "    0x%x\n", data);
			iowrite32(data, fusb300->reg + FUSB300_OFFSET_CXPORT);
			tmp = tmp + 4;
		}
		switch (length % 4) {
		case 1:
			data = *tmp;
			printk(KERN_DEBUG "    0x%x\n", data);
			iowrite32(data, fusb300->reg + FUSB300_OFFSET_CXPORT);
			break;
		case 2:
			data = *tmp | *(tmp + 1) << 8;
			printk(KERN_DEBUG "    0x%x\n", data);
			iowrite32(data, fusb300->reg + FUSB300_OFFSET_CXPORT);
			break;
		case 3:
			data = *tmp | *(tmp + 1) << 8 | *(tmp + 2) << 16;
			printk(KERN_DEBUG "    0x%x\n", data);
			iowrite32(data, fusb300->reg + FUSB300_OFFSET_CXPORT);
			break;
		default:
			break;
		}
		req->req.actual += length;
	}
}

/* read data from cx fifo */
static void fusb300_rdcxf(struct fusb300 *fusb300,
		   u8 *buffer, u32 length)
{
	int i = 0;
	u8 *tmp;
	u32 data;

	tmp = buffer;

	for (i = (length >> 2); i > 0; i--) {
		data = ioread32(fusb300->reg + FUSB300_OFFSET_CXPORT);
		printk(KERN_DEBUG "    0x%x\n", data);
		*tmp = data & 0xFF;
		*(tmp + 1) = (data >> 8) & 0xFF;
		*(tmp + 2) = (data >> 16) & 0xFF;
		*(tmp + 3) = (data >> 24) & 0xFF;
		tmp = tmp + 4;
	}

	switch (length % 4) {
	case 1:
		data = ioread32(fusb300->reg + FUSB300_OFFSET_CXPORT);
		printk(KERN_DEBUG "    0x%x\n", data);
		*tmp = data & 0xFF;
		break;
	case 2:
		data = ioread32(fusb300->reg + FUSB300_OFFSET_CXPORT);
		printk(KERN_DEBUG "    0x%x\n", data);
		*tmp = data & 0xFF;
		*(tmp + 1) = (data >> 8) & 0xFF;
		break;
	case 3:
		data = ioread32(fusb300->reg + FUSB300_OFFSET_CXPORT);
		printk(KERN_DEBUG "    0x%x\n", data);
		*tmp = data & 0xFF;
		*(tmp + 1) = (data >> 8) & 0xFF;
		*(tmp + 2) = (data >> 16) & 0xFF;
		break;
	default:
		break;
	}
}

static void fusb300_set_epnstall(struct fusb300 *fusb300, u8 ep)
{
	fusb300_enable_bit(fusb300, FUSB300_OFFSET_EPSET0(ep),
		FUSB300_EPSET0_STL);
}

static void fusb300_clear_epnstall(struct fusb300 *fusb300, u8 ep)
{
	u32 reg = ioread32(fusb300->reg + FUSB300_OFFSET_EPSET0(ep));

	if (reg & FUSB300_EPSET0_STL) {
		printk(KERN_DEBUG "EP%d stall... Clear!!\n", ep);
		reg |= FUSB300_EPSET0_STL_CLR;
		iowrite32(reg, fusb300->reg + FUSB300_OFFSET_EPSET0(ep));
	}
}

static void ep0_queue(struct fusb300_ep *ep, struct fusb300_request *req)
{
	if (ep->dir_in) { /* if IN */
		if (req->req.length) {
			fusb300_set_cxlen(ep->fusb300, req->req.length);
			fusb300_wrcxf(ep, req);
		} else
			printk(KERN_DEBUG "%s : req->req.length = 0x%x\n",
				__func__, req->req.length);
		if ((req->req.length == req->req.actual) ||
		    (req->req.actual < ep->ep.maxpacket))
			fusb300_done(ep, req, 0);
	} else { /* OUT */
		if (!req->req.length)
			fusb300_done(ep, req, 0);
		else {
			/* For set_feature(TEST_PACKET) */
			if (ep->fusb300->ep0_length == 53) {
				fusb300_set_cxlen(ep->fusb300, req->req.length);
				fusb300_wrcxf(ep, req);
				ep->fusb300->ep0_length = 0;
			}
			else
			fusb300_enable_bit(ep->fusb300, FUSB300_OFFSET_IGER1,
				FUSB300_IGER1_CX_OUT_INT);
		}
	}
}

static int fusb300_queue(struct usb_ep *_ep, struct usb_request *_req,
			 gfp_t gfp_flags)
{
	struct fusb300_ep *ep;
	struct fusb300_request *req;
	unsigned long flags;
	int request  = 0;

	ep = container_of(_ep, struct fusb300_ep, ep);
	req = container_of(_req, struct fusb300_request, req);

	if (ep->fusb300->gadget.speed == USB_SPEED_UNKNOWN)
		return -ESHUTDOWN;

	spin_lock_irqsave(&ep->fusb300->lock, flags);

	if (list_empty(&ep->queue))
		request = 1;

	list_add_tail(&req->queue, &ep->queue);

	req->req.actual = 0;
	req->req.status = -EINPROGRESS;

	if (!ep->epnum) /* ep0 */
		ep0_queue(ep, req);
	else if (request && !ep->stall)
		enable_fifo_int(ep);

	spin_unlock_irqrestore(&ep->fusb300->lock, flags);

	return 0;
}

static int fusb300_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
	struct fusb300_ep *ep;
	struct fusb300_request *req;
	unsigned long flags;

	ep = container_of(_ep, struct fusb300_ep, ep);
	req = container_of(_req, struct fusb300_request, req);

	spin_lock_irqsave(&ep->fusb300->lock, flags);
	if (!list_empty(&ep->queue))
		fusb300_done(ep, req, -ECONNRESET);
	spin_unlock_irqrestore(&ep->fusb300->lock, flags);

	return 0;
}

static int fusb300_set_halt_and_wedge(struct usb_ep *_ep, int value, int wedge)
{
	struct fusb300_ep *ep;
	struct fusb300 *fusb300;
	unsigned long flags;
	int ret = 0, i = 0;
	u32 reg;

	ep = container_of(_ep, struct fusb300_ep, ep);

	fusb300 = ep->fusb300;

	spin_lock_irqsave(&ep->fusb300->lock, flags);

	if (!list_empty(&ep->queue)) {
		ret = -EAGAIN;
		goto out;
	}

	if (value) {
		//lichun@add
		reg = ioread32(fusb300->reg + FUSB300_OFFSET_EPSET1(ep->epnum));
		if (reg & FUSB300_EPSET1_DIRIN) {
			//for IN EP, check if SYNC_FIFO is empty before set EP_stall
			do {
				reg = ioread32(fusb300->reg + FUSB300_OFFSET_IGR1);
				reg &= FUSB300_IGR1_SYNF0_EMPTY_INT;
				if (i)
					printk(KERN_INFO "sync fifo is not empty!\n");
				i++;
			} while (!reg);
		}
#ifdef IDMA_MODE
		else {
			//for OUT EP, check if IDMA is complete before set EP_stall
			do {
				reg = ioread32(fusb300->reg + FUSB300_OFFSET_EPPRDRDY);
				reg &= FUSB300_EPPRDR_EP_PRD_RDY(ep->epnum);
				if (i)
					printk(KERN_INFO "ep prd_ready is not 0!\n");
				i++;
			} while (reg);
		}
#endif
		//~lichun@add

		fusb300_set_epnstall(fusb300, ep->epnum);
		ep->stall = 1;
		if (wedge)
			ep->wedged = 1;
	} else {
		fusb300_clear_epnstall(fusb300, ep->epnum);
		ep->stall = 0;
		ep->wedged = 0;
	}
out:
	spin_unlock_irqrestore(&ep->fusb300->lock, flags);
	return ret;
}

static int fusb300_set_halt(struct usb_ep *_ep, int value)
{
	return fusb300_set_halt_and_wedge(_ep, value, 0);
}

static int fusb300_set_wedge(struct usb_ep *_ep)
{
	return fusb300_set_halt_and_wedge(_ep, 1, 1);
}

static void fusb300_fifo_flush(struct usb_ep *_ep)
{
}

static const struct usb_ep_ops fusb300_ep_ops = {
	.enable		= fusb300_enable,
	.disable	= fusb300_disable,

	.alloc_request	= fusb300_alloc_request,
	.free_request	= fusb300_free_request,

	.queue		= fusb300_queue,
	.dequeue	= fusb300_dequeue,

	.set_halt	= fusb300_set_halt,
	.fifo_flush	= fusb300_fifo_flush,
	.set_wedge	= fusb300_set_wedge,
};

/*****************************************************************************/
/* Write 1 to clear */
static void fusb300_clear_int(struct fusb300 *fusb300, u32 offset,
		       u32 value)
{
	iowrite32(value, fusb300->reg + offset);
}

static void fusb300_reset(struct fusb300 *fusb300)
{
	fusb300->u1en = 0;
	fusb300->u2en = 0;
}

static void fusb300_set_cxstall(struct fusb300 *fusb300)
{
	fusb300_enable_bit(fusb300, FUSB300_OFFSET_CSR,
			   FUSB300_CSR_STL);
}

static u8 fusb300_get_cxstall(struct fusb300 *fusb300)
{
	u8 value = 0;
	u32 reg = ioread32(fusb300->reg + FUSB300_OFFSET_CSR);

	if (reg & FUSB300_CSR_STL)
		value = 1;

	return value;
}

static u8 fusb300_get_epnstall(struct fusb300 *fusb300, u8 ep)
{
	u8 value = 0;
	u32 reg = ioread32(fusb300->reg + FUSB300_OFFSET_EPSET0(ep));

	if (reg & FUSB300_EPSET0_STL)
		value = 1;

	return value;
}

static u8 fusb300_get_config(struct fusb300 *fusb300)
{
	u8 value = 0;
	u32 reg = ioread32(fusb300->reg + FUSB300_OFFSET_DAR);

	if (reg & FUSB300_DAR_SETCONFG)
		value = 1;

	return value;
}

static void fusb300_set_u2_timeout(struct fusb300 *fusb300,
				   u32 time)
{
	u32 reg;

	reg = ioread32(fusb300->reg + FUSB300_OFFSET_SSCR2);
	reg &= ~0xff;
	reg |= FUSB300_SSCR2_U2TIMEOUT(time);

	iowrite32(reg, fusb300->reg + FUSB300_OFFSET_SSCR2);
}

static void fusb300_set_u1_timeout(struct fusb300 *fusb300,
				   u32 time)
{
	u32 reg;

	reg = ioread32(fusb300->reg + FUSB300_OFFSET_SSCR2);
	reg &= ~(0xff << 8);
	reg |= FUSB300_SSCR2_U1TIMEOUT(time);

	iowrite32(reg, fusb300->reg + FUSB300_OFFSET_SSCR2);
}

static int fusb300_is_rmwkup(struct fusb300 *fusb300)
{
	u32 value = ioread32(fusb300->reg + FUSB300_OFFSET_HSCR);

	return value & FUSB300_HSCR_CAP_RMWKUP ? 1 : 0;
}

static void fusb300_set_rmwkup(struct fusb300 *fusb300)
{
	u32 value = ioread32(fusb300->reg + FUSB300_OFFSET_HSCR);

	value |= FUSB300_HSCR_CAP_RMWKUP;
	iowrite32(value, fusb300->reg + FUSB300_OFFSET_HSCR);
}

static void fusb300_clear_rmwkup(struct fusb300 *fusb300)
{
	u32 value = ioread32(fusb300->reg + FUSB300_OFFSET_HSCR);

	value &= ~FUSB300_HSCR_CAP_RMWKUP;
	iowrite32(value, fusb300->reg + FUSB300_OFFSET_HSCR);
}

static void request_error(struct fusb300 *fusb300)
{
	fusb300_set_cxstall(fusb300);
	printk(KERN_DEBUG "request error!!\n");
}

static void get_status(struct fusb300 *fusb300, struct usb_ctrlrequest *ctrl)
__releases(fusb300->lock)
__acquires(fusb300->lock)
{
	u8 ep;
	u16 status = 0;
	u16 w_index = le16_to_cpu(ctrl->wIndex);

	switch (ctrl->bRequestType & USB_RECIP_MASK) {
	case USB_RECIP_DEVICE:
		status = 1 << USB_DEVICE_SELF_POWERED;
		if (fusb300->gadget.speed == USB_SPEED_SUPER)
			status |= (fusb300->u2en << 3 | fusb300->u1en << 2);
		else {  //non-SS device
			if (fusb300_is_rmwkup(fusb300))
				status |= 1 << USB_DEVICE_REMOTE_WAKEUP;
		}
		break;
	case USB_RECIP_INTERFACE:
		status = 0;
		break;
	case USB_RECIP_ENDPOINT:
		ep = w_index & USB_ENDPOINT_NUMBER_MASK;
		if (ep) {
			if (fusb300_get_epnstall(fusb300, ep))
				status = 1 << USB_ENDPOINT_HALT;
		} else {
			if (fusb300_get_cxstall(fusb300))
				status = 0;
		}
		break;

	default:
		request_error(fusb300);
		return;		/* exit */
	}

	fusb300->ep0_data = cpu_to_le16(status);
	fusb300->ep0_req->buf = &fusb300->ep0_data;
	fusb300->ep0_req->length = 2;

	spin_unlock(&fusb300->lock);
	fusb300_queue(fusb300->gadget.ep0, fusb300->ep0_req, GFP_KERNEL);
	spin_lock(&fusb300->lock);
}

void gen_tst_packet(u8 *tst_packet)
{
	u8 *tp = tst_packet;

	int i;
	for (i = 0; i < 9; i++)/*JKJKJKJK x 9*/
		*tp++ = 0x00;

	for (i = 0; i < 8; i++) /* 8*AA */
		*tp++ = 0xAA;

	for (i = 0; i < 8; i++) /* 8*EE */
		*tp++ = 0xEE;

	*tp++ = 0xFE;

	for (i = 0; i < 11; i++) /* 11*FF */
		*tp++ = 0xFF;

	*tp++ = 0x7F;
	*tp++ = 0xBF;
	*tp++ = 0xDF;
	*tp++ = 0xEF;
	*tp++ = 0xF7;
	*tp++ = 0xFB;
	*tp++ = 0xFD;
	*tp++ = 0xFC;
	*tp++ = 0x7E;
	*tp++ = 0xBF;
	*tp++ = 0xDF;
	*tp++ = 0xEF;
	*tp++ = 0xF7;
	*tp++ = 0xFB;
	*tp++ = 0xFD;
	*tp++ = 0x7E;
}

static void set_feature(struct fusb300 *fusb300, struct usb_ctrlrequest *ctrl)
__releases(fusb300->lock)
__acquires(fusb300->lock)
{
	u16 w_index = le16_to_cpu(ctrl->wIndex);
	u16 w_value = le16_to_cpu(ctrl->wValue);
	u8 tst_packet[53];

	struct fusb300_ep *ep = 
		fusb300->ep[w_index & USB_ENDPOINT_NUMBER_MASK];

	switch (ctrl->bRequestType & USB_RECIP_MASK) {
	case USB_RECIP_DEVICE:
		if (fusb300->gadget.speed == USB_SPEED_SUPER) {
			if (!fusb300_get_config(fusb300)) {
				request_error(fusb300);
				return;
			}

			switch(w_value) {
			case USB_DEVICE_U1_ENABLE:
				fusb300_set_u1_timeout(fusb300, 0x1);
				fusb300_enable_bit(fusb300, FUSB300_OFFSET_SSCR0,
						FUSB300_SSCR0_U1_FUN_EN);
				fusb300->u1en = 1;
				break;
			case USB_DEVICE_U2_ENABLE:
				fusb300_set_u2_timeout(fusb300, 0x1);
				fusb300_enable_bit(fusb300, FUSB300_OFFSET_SSCR0,
						FUSB300_SSCR0_U2_FUN_EN);
				fusb300->u2en = 1;
				break;
			case USB_DEVICE_LTM_ENABLE:
				break;
			default:
				request_error(fusb300);
				return;
			}
		}
		else { //non-SS device
			switch(w_value) {
			case USB_DEVICE_REMOTE_WAKEUP:
				fusb300_set_rmwkup(fusb300);
				break;
			case USB_DEVICE_TEST_MODE:
				switch (w_index >> 8) {
				case USB_TEST_J:
					fusb300_enable_bit(fusb300, FUSB300_OFFSET_HSPTM,
							FUSB300_HSPTM_TSTJSTA);
					break;
				case USB_TEST_K:
					fusb300_enable_bit(fusb300, FUSB300_OFFSET_HSPTM,
							FUSB300_HSPTM_TSTKSTA);
					break;
				case USB_TEST_SE0_NAK:
					fusb300_enable_bit(fusb300, FUSB300_OFFSET_HSPTM,
							FUSB300_HSPTM_TSTSET0NAK);
					break;
				case USB_TEST_PACKET:
					fusb300_enable_bit(fusb300, FUSB300_OFFSET_HSPTM,
							FUSB300_HSPTM_TSTPKT);
					fusb300_set_cxdone(fusb300);

					gen_tst_packet(tst_packet);
					while ((ioread32(fusb300->reg + FUSB300_OFFSET_CSR))
						& FUSB300_CSR_DONE) ;

					fusb300->ep0_req->buf = tst_packet;
					fusb300->ep0_req->length = 53;
					fusb300->ep0_length = 53;

					spin_unlock(&fusb300->lock);
					fusb300_queue(fusb300->gadget.ep0,
							fusb300->ep0_req, GFP_KERNEL);
					spin_lock(&fusb300->lock);
					return; /* didn't need to do set_cxdone again */

				case USB_TEST_FORCE_ENABLE:
					break;
				default:
					request_error(fusb300);
					return;
				}
				break;

			default:
				request_error(fusb300);
				return;
			}
		}
		fusb300_set_cxdone(fusb300);
		break;

	case USB_RECIP_INTERFACE:
		fusb300_set_cxdone(fusb300);
		break;
	case USB_RECIP_ENDPOINT:
		if (w_value == USB_ENDPOINT_HALT) {
			if (w_index & USB_ENDPOINT_NUMBER_MASK) {
				fusb300_set_epnstall(fusb300, ep->epnum);
				ep->stall = 1;
			}
			else
				fusb300_set_cxstall(fusb300);

			fusb300_set_cxdone(fusb300);
		}
		else
			request_error(fusb300);
		break;
	default:
		request_error(fusb300);
		break;
	}
}

static void fusb300_clear_seqnum(struct fusb300 *fusb300, u8 ep)
{
	fusb300_enable_bit(fusb300, FUSB300_OFFSET_EPSET0(ep),
			    FUSB300_EPSET0_CLRSEQNUM);
}

static void clear_feature(struct fusb300 *fusb300, struct usb_ctrlrequest *ctrl)
{
	u16 w_index = le16_to_cpu(ctrl->wIndex);
	u16 w_value = le16_to_cpu(ctrl->wValue);

	struct fusb300_ep *ep =
		fusb300->ep[w_index & USB_ENDPOINT_NUMBER_MASK];

	switch (ctrl->bRequestType & USB_RECIP_MASK) {
	case USB_RECIP_DEVICE:
		if (fusb300->gadget.speed == USB_SPEED_SUPER) {
			switch(w_value) {
			case USB_DEVICE_U1_ENABLE:
				fusb300_disable_bit(fusb300, FUSB300_OFFSET_SSCR0,
							FUSB300_SSCR0_U1_FUN_EN);
				fusb300->u1en = 0;
				break;
			case USB_DEVICE_U2_ENABLE:
				fusb300_disable_bit(fusb300, FUSB300_OFFSET_SSCR0,
							FUSB300_SSCR0_U2_FUN_EN);
				fusb300->u2en = 0;
				break;
			case USB_DEVICE_LTM_ENABLE:
				break;
			default:
				request_error(fusb300);
				return;
			}
		}
		else {  //non-SS device
			switch(w_value) {
			case USB_DEVICE_REMOTE_WAKEUP:
				fusb300_clear_rmwkup(fusb300);
				break;
			default:
				request_error(fusb300);
				return;
			}
		}
		fusb300_set_cxdone(fusb300);
		break;

	case USB_RECIP_INTERFACE:
		fusb300_set_cxdone(fusb300);
		break;
	case USB_RECIP_ENDPOINT:
		if (w_value == USB_ENDPOINT_HALT) {
			if (w_index & USB_ENDPOINT_NUMBER_MASK) {
				fusb300_clear_seqnum(fusb300, ep->epnum);
				if (ep->wedged) {
					fusb300_set_cxdone(fusb300);
					return;
				}
				if (ep->stall) {
					ep->stall = 0;
					fusb300_clear_epnstall(fusb300, ep->epnum);
				}
				if (!list_empty(&ep->queue))
					enable_fifo_int(ep);
			}
			fusb300_set_cxdone(fusb300);
		}
		else {
			request_error(fusb300);
			return;
		}
		break;
	default:
		request_error(fusb300);
		break;
	}
}

static void fusb300_set_dev_addr(struct fusb300 *fusb300, u16 addr)
{
	u32 reg = ioread32(fusb300->reg + FUSB300_OFFSET_DAR);

	reg &= ~FUSB300_DAR_DRVADDR_MSK;
	reg |= FUSB300_DAR_DRVADDR(addr);

	iowrite32(reg, fusb300->reg + FUSB300_OFFSET_DAR);
}

static void set_address(struct fusb300 *fusb300, struct usb_ctrlrequest *ctrl)
{
	u16 w_value = le16_to_cpu(ctrl->wValue);

	if (w_value >= 0x0100)
		request_error(fusb300);
	else {
		fusb300_set_dev_addr(fusb300, w_value);
		fusb300_set_cxdone(fusb300);
	}
}

static int set_configuration(struct fusb300 *fusb300, struct usb_ctrlrequest *ctrl)
{
	u16 w_value = le16_to_cpu(ctrl->wValue);
	u8 i;
	int ret = 1;

	if (!(w_value & 0xff)) {
		fusb300_disable_bit(fusb300, FUSB300_OFFSET_DAR,
				FUSB300_DAR_SETCONFG);
		fusb300->u1en = 0;
		fusb300->u2en = 0;
	}
	else {
		if (w_value == 1) {
			fusb300_enable_bit(fusb300, FUSB300_OFFSET_DAR,
					FUSB300_DAR_SETCONFG);
			/* clear sequence number */
			for (i = 1; i < FUSB300_MAX_NUM_EP; i++)
				fusb300_clear_seqnum(fusb300, i);

			fusb300->reenum = 1;
		}
		else {	//For CV (<= 2.1.12.1) test: TD_9.13 set_configure_2, keep previous configure_num
			//fusb300_set_cxdone(fusb300);
			request_error(fusb300);
			ret = 0;
		}
	}
	return ret;
}

static void set_interface(struct fusb300 *fusb300, struct usb_ctrlrequest *ctrl)
{
	u8 i;

	/* clear sequence number */
	for (i = 1; i < FUSB300_MAX_NUM_EP; i++)
		fusb300_clear_seqnum(fusb300, i);

	fusb300->reenum = 1;
}

static void set_sel(struct fusb300 *fusb300, struct usb_ctrlrequest *ctrl)
__releases(fusb300->lock)
__acquires(fusb300->lock)
{
	u8 cx_data[6];

	fusb300->ep0_req->buf = cx_data;
	fusb300->ep0_req->length = 6;

	//wait CXout for data stage
	spin_unlock(&fusb300->lock);
	fusb300_queue(fusb300->gadget.ep0, fusb300->ep0_req, GFP_KERNEL);
	spin_lock(&fusb300->lock);
}

static void set_iso_delay(struct fusb300 *fusb300, struct usb_ctrlrequest *ctrl)
{
	if (ctrl->wValue >= 65535)
		request_error(fusb300);
	else {
		fusb300_set_cxdone(fusb300);
	}
}

static int setup_packet(struct fusb300 *fusb300, struct usb_ctrlrequest *ctrl)
{
	u8 *p = (u8 *)ctrl;
	u8 ret = 0;

	fusb300_rdcxf(fusb300, p, 8);
	fusb300->ep[0]->dir_in = ctrl->bRequestType & USB_DIR_IN;

	if (fusb300->gadget.speed == USB_SPEED_UNKNOWN) {
		printk("ERROR!! no link!! dev_mode = 0\n");
		return ret;
	}

	/* check request */
	if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD) {
		switch (ctrl->bRequest) {
		case USB_REQ_GET_STATUS:
			get_status(fusb300, ctrl);
			break;
		case USB_REQ_CLEAR_FEATURE:
			clear_feature(fusb300, ctrl);
			break;
		case USB_REQ_SET_FEATURE:
			set_feature(fusb300, ctrl);
			break;
		case USB_REQ_SET_ADDRESS:
			set_address(fusb300, ctrl);
			break;
		case USB_REQ_SET_CONFIGURATION:
			if (set_configuration(fusb300, ctrl))
				ret = 1; //let class driver to do something
			break;
		case USB_REQ_SET_INTERFACE:
			set_interface(fusb300, ctrl);
			ret = 1;
			break;
		case USB_REQ_SET_SEL:
			set_sel(fusb300, ctrl);
			break;
		case USB_REQ_SET_ISOCH_DELAY:
			set_iso_delay(fusb300, ctrl);
			break;
		default:
			ret = 1;
			break;
		}
	} else
		ret = 1;

	return ret;
}

#ifdef IDMA_MODE
static void fusb300_fill_idma_prdtbl(struct fusb300_ep *ep, dma_addr_t d,
		u32 len)
{
	u32 value;
	u32 reg;

	/* wait SW owner */
	do {
		reg = ioread32(ep->fusb300->reg +
			FUSB300_OFFSET_EPPRD_W0(ep->epnum));
		reg &= FUSB300_EPPRD0_H;
	} while (reg);

	iowrite32(d, ep->fusb300->reg + FUSB300_OFFSET_EPPRD_W1(ep->epnum));

	value = FUSB300_EPPRD0_BTC(len) | FUSB300_EPPRD0_H |
		FUSB300_EPPRD0_F | FUSB300_EPPRD0_L | FUSB300_EPPRD0_I;
	iowrite32(value, ep->fusb300->reg + FUSB300_OFFSET_EPPRD_W0(ep->epnum));

	iowrite32(0x0, ep->fusb300->reg + FUSB300_OFFSET_EPPRD_W2(ep->epnum));

	fusb300_enable_bit(ep->fusb300, FUSB300_OFFSET_EPPRDRDY,
		FUSB300_EPPRDR_EP_PRD_RDY(ep->epnum));
}

#ifdef POLLING_PRD
static void fusb300_wait_idma_finished(struct fusb300_ep *ep)
{
	u32 reg;
	u32 prd_rdy_bit;
	u8 polling_prd_rdy = 0;

	do {
		reg = ioread32(ep->fusb300->reg + FUSB300_OFFSET_IGR1);
		if ((reg & FUSB300_IGR1_VBUS_CHG_INT) ||
		    (reg & FUSB300_IGR1_WARM_RST_INT) ||
		    (reg & FUSB300_IGR1_HOT_RST_INT) ||
		    (reg & FUSB300_IGR1_USBRST_INT)
		) {
			polling_prd_rdy = 1;
			goto IDMA_DONE;
		}
		reg = ioread32(ep->fusb300->reg + FUSB300_OFFSET_IGR0);
		reg &= FUSB300_IGR0_EPn_PRD_INT(ep->epnum);
	} while (!reg);

	/* Write 1 clear */
	fusb300_clear_int(ep->fusb300, FUSB300_OFFSET_IGR0,
		FUSB300_IGR0_EPn_PRD_INT(ep->epnum));

IDMA_DONE:
	/* must confirm prd_rdy_bit = 0 */
	if (polling_prd_rdy) {
		do {
			prd_rdy_bit = ioread32(ep->fusb300->reg + FUSB300_OFFSET_EPPRDRDY);
			prd_rdy_bit &= FUSB300_EPPRDR_EP_PRD_RDY(ep->epnum);
		} while(prd_rdy_bit);

		polling_prd_rdy = 0;
	}

	/* Mask the PRD interrupt */
	fusb300_disable_bit(ep->fusb300, FUSB300_OFFSET_IGER0,
			FUSB300_IGER0_EEPn_PRD_INT(ep->epnum));
}
#endif

static void fusb300_irq_prd_done(struct fusb300_ep *ep, int abort_idma)
{
	struct fusb300_request *req;
	u32 value, rem_len;

	/* Mask the PRD interrupt */
	fusb300_disable_bit(ep->fusb300, FUSB300_OFFSET_IGER0,
			FUSB300_IGER0_EEPn_PRD_INT(ep->epnum));

	if (abort_idma) {
		printk(KERN_INFO "fusb300_irq_prd_done: ep %d abort_idma\n", ep->epnum);
		/* reset ep fifo */
		fusb300_enable_bit(ep->fusb300, FUSB300_OFFSET_EPFFR(ep->epnum),
				FUSB300_FFR_RST);

		//reset IDMA and polling
		fusb300_enable_bit(ep->fusb300, FUSB300_OFFSET_DMAAPR,
				FUSB300_DMAAPR_DMA_RESET);
		while((ioread32(ep->fusb300->reg + FUSB300_OFFSET_DMAAPR)
			& FUSB300_DMAAPR_DMA_RESET) == 1) ;

		//polling prd_rdy_bit
		do {
			value = ioread32(ep->fusb300->reg + FUSB300_OFFSET_EPPRDRDY);
			value &= FUSB300_EPPRDR_EP_PRD_RDY(ep->epnum);
		} while(value);
	}

	req = ep->irq_current_task->req;

	/* update actual transfer length */
	value = ioread32(ep->fusb300->reg + FUSB300_OFFSET_EPPRD_W0(ep->epnum));
	rem_len = FUSB300_EPPRD0_BTC(value);
	req->req.actual += (ep->irq_current_task->transfer_length - rem_len);

	dma_unmap_single(ep->fusb300->gadget.dev.parent,
			 ep->irq_current_task->dma_addr,
			 ep->irq_current_task->transfer_length,
			 ep->dir_in ? DMA_TO_DEVICE :
					DMA_FROM_DEVICE);

	fusb300_done(ep, req, 0);
}

static void fusb300_set_idma(struct fusb300_ep *ep,
			struct fusb300_request *req,
			u32 len)
{
	dma_addr_t d;
#ifdef POLLING_PRD
	u32 reg, rem_len;
#endif

	d = dma_map_single(ep->fusb300->gadget.dev.parent,
			req->req.buf, len,
			ep->dir_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

	if (dma_mapping_error(ep->fusb300->gadget.dev.parent, d)) {
		printk(KERN_ERR "dma_mapping_error\n");
		return;
	}

	ep->irq_current_task->req = req;
	ep->irq_current_task->transfer_length = len;
	ep->irq_current_task->dma_addr = d;

	fusb300_enable_bit(ep->fusb300, FUSB300_OFFSET_IGER0,
		FUSB300_IGER0_EEPn_PRD_INT(ep->epnum));

	fusb300_fill_idma_prdtbl(ep, d, len);

#ifdef POLLING_PRD
	/* check idma is done */
	fusb300_wait_idma_finished(ep);

	/* update actual transfer length */
	reg = ioread32(ep->fusb300->reg + FUSB300_OFFSET_EPPRD_W0(ep->epnum));
	rem_len = FUSB300_EPPRD0_BTC(reg);
	req->req.actual += (len - rem_len);

	dma_unmap_single(ep->fusb300->gadget.dev.parent,
			 d, len, ep->dir_in ? DMA_TO_DEVICE :
				 DMA_FROM_DEVICE);
#endif
}

/* polling method or wait for interrupt (wfi) method
 * if wfi, fusb300_irq_prd_done() will be called when idma finishs EPn PRD
 */
static void in_ep_fifo_handler_idma(struct fusb300_ep *ep)
{
	struct fusb300_request *req = list_entry(ep->queue.next,
					struct fusb300_request, queue);
#ifdef POLLING_PRD
	if (req->req.length) {
		fusb300_set_idma(ep, req, req->req.length);
	}
	fusb300_done(ep, req, 0);
#else
	if (req->req.length) {
		disable_fifo_int(ep);
		fusb300_set_idma(ep, req, req->req.length);
	}
	else
		fusb300_done(ep, req, 0);
#endif
}

static void out_ep_fifo_handler_idma(struct fusb300_ep *ep)
{
	struct fusb300_request *req = list_entry(ep->queue.next,
						 struct fusb300_request, queue);

#ifdef POLLING_PRD
	fusb300_set_idma(ep, req, req->req.length);
	/* finish out transfer */
	fusb300_done(ep, req, 0);
#else
	disable_fifo_int(ep);
	fusb300_set_idma(ep, req, req->req.length);
#endif
}

#else	//PIO_mode, not IDMA_mode

static void fusb300_set_ep_bycnt(struct fusb300_ep *ep,
			struct fusb300_request *req)
{
	u32 reg;

	reg = ioread32(ep->fusb300->reg + FUSB300_OFFSET_EPFFR(ep->epnum));
	reg &= ~FUSB300_FFR_BYCNT;
	reg |= (req->req.length & FUSB300_FFR_BYCNT);
	iowrite32(reg, ep->fusb300->reg + FUSB300_OFFSET_EPFFR(ep->epnum));
}

static void fusb300_wrfifo(struct fusb300_ep *ep,
			struct fusb300_request *req)
{
	int i = 0;
	u8 *tmp;
	u32 data, reg;
	struct fusb300 *fusb300 = ep->fusb300;

	do {
		reg = ioread32(fusb300->reg + FUSB300_OFFSET_IGR1);
		reg &= FUSB300_IGR1_SYNF0_EMPTY_INT;
		if (i)
			printk(KERN_INFO "sync fifo is not empty!\n");
		i++;
	} while (!reg);

	tmp = req->req.buf;
	req->req.actual = req->req.length;

	for (i = (req->req.length >> 2); i > 0; i--) {
		data = *tmp | *(tmp + 1) << 8 | *(tmp + 2) << 16 |
			*(tmp + 3) << 24;
		iowrite32(data, fusb300->reg + FUSB300_OFFSET_EPPORT(ep->epnum));
		tmp = tmp + 4;
	}

	switch (req->req.length % 4) {
	case 1:
		data = *tmp;
		iowrite32(data, fusb300->reg + FUSB300_OFFSET_EPPORT(ep->epnum));
		break;
	case 2:
		data = *tmp | *(tmp + 1) << 8;
		iowrite32(data, fusb300->reg + FUSB300_OFFSET_EPPORT(ep->epnum));
		break;
	case 3:
		data = *tmp | *(tmp + 1) << 8 | *(tmp + 2) << 16;
		iowrite32(data, fusb300->reg + FUSB300_OFFSET_EPPORT(ep->epnum));
		break;
	default:
		break;
	}
}

static void fusb300_rdfifo(struct fusb300_ep *ep,
			  struct fusb300_request *req,
			  u32 length)
{
	int i = 0;
	u8 *tmp;
	u32 data, reg;
	struct fusb300 *fusb300 = ep->fusb300;

	do {
		reg = ioread32(fusb300->reg + FUSB300_OFFSET_IGR1);
		reg &= FUSB300_IGR1_SYNF0_EMPTY_INT;
		if (i)
			printk(KERN_INFO "sync fifo is not empty!\n");
		i++;
	} while (!reg);

	tmp = req->req.buf + req->req.actual;
	req->req.actual += length;

	if (req->req.actual > req->req.length)
		printk(KERN_DEBUG "req->req.actual > req->req.length\n");

	for (i = (length >> 2); i > 0; i--) {
		data = ioread32(fusb300->reg +
			FUSB300_OFFSET_EPPORT(ep->epnum));
		*tmp = data & 0xFF;
		*(tmp + 1) = (data >> 8) & 0xFF;
		*(tmp + 2) = (data >> 16) & 0xFF;
		*(tmp + 3) = (data >> 24) & 0xFF;
		tmp = tmp + 4;
	}

	switch (length % 4) {
	case 1:
		data = ioread32(fusb300->reg +
			FUSB300_OFFSET_EPPORT(ep->epnum));
		*tmp = data & 0xFF;
		break;
	case 2:
		data = ioread32(fusb300->reg +
			FUSB300_OFFSET_EPPORT(ep->epnum));
		*tmp = data & 0xFF;
		*(tmp + 1) = (data >> 8) & 0xFF;
		break;
	case 3:
		data = ioread32(fusb300->reg +
			FUSB300_OFFSET_EPPORT(ep->epnum));
		*tmp = data & 0xFF;
		*(tmp + 1) = (data >> 8) & 0xFF;
		*(tmp + 2) = (data >> 16) & 0xFF;
		break;
	default:
		break;
	}
}

static void in_ep_fifo_handler(struct fusb300_ep *ep)
{
	struct fusb300_request *req = list_entry(ep->queue.next,
					struct fusb300_request, queue);

	if (req->req.length) {
		fusb300_set_ep_bycnt(ep, req);
		fusb300_wrfifo(ep, req);
	}
	fusb300_done(ep, req, 0);
}

static void out_ep_fifo_handler(struct fusb300_ep *ep)
{
	struct fusb300_request *req = list_entry(ep->queue.next,
						 struct fusb300_request, queue);
	u32 reg = ioread32(ep->fusb300->reg + FUSB300_OFFSET_EPFFR(ep->epnum));
	u32 length = reg & FUSB300_FFR_BYCNT;

	fusb300_rdfifo(ep, req, length);

	/* finish out transfer */
	if ((req->req.length == req->req.actual) || (length < ep->ep.maxpacket))
		fusb300_done(ep, req, 0);
}
#endif

static void check_device_mode(struct fusb300 *fusb300)
{
	u32 reg = ioread32(fusb300->reg + FUSB300_OFFSET_GCR);

	switch (reg & FUSB300_GCR_DEVEN_MSK) {
	case FUSB300_GCR_DEVEN_SS:
		fusb300->gadget.speed = USB_SPEED_SUPER;
		fusb300->gadget.ep0->maxpacket = SS_CTL_MAX_PACKET_SIZE; // 2^9
		break;
	case FUSB300_GCR_DEVEN_HS:
		fusb300->gadget.speed = USB_SPEED_HIGH;
		fusb300->gadget.ep0->maxpacket = HS_CTL_MAX_PACKET_SIZE;
		break;
	case FUSB300_GCR_DEVEN_FS:
		fusb300->gadget.speed = USB_SPEED_FULL;
		fusb300->gadget.ep0->maxpacket = HS_CTL_MAX_PACKET_SIZE;
		break;
	default:
		fusb300->gadget.speed = USB_SPEED_UNKNOWN;
		break;
	}
	printk(KERN_INFO "dev_mode = %d\n", (reg & FUSB300_GCR_DEVEN_MSK));
}


static void fusb300_ep0out(struct fusb300 *fusb300)
{
	struct fusb300_ep *ep = fusb300->ep[0];
	u32 reg = ioread32(fusb300->reg + FUSB300_OFFSET_CSR);
	u32 length = (reg & FUSB300_CSR_LEN_MSK) >> 8;

	if (!list_empty(&ep->queue)) {
		struct fusb300_request *req;

		req = list_first_entry(&ep->queue,
			struct fusb300_request, queue);
		if (length) {
			fusb300_rdcxf(fusb300, req->req.buf + req->req.actual,
				length);
			req->req.actual += length;
		}
		if (req->req.length == req->req.actual) {
			fusb300_done(ep, req, 0);
			//mask CX_OUT_INT until ep0_queue()
			fusb300_disable_bit(fusb300, FUSB300_OFFSET_IGER1,
				FUSB300_IGER1_CX_OUT_INT);
		}
	} else
		pr_err("%s : empty queue\n", __func__);
}

static void fusb300_ep0in(struct fusb300 *fusb300)
{
	struct fusb300_request *req;
	struct fusb300_ep *ep = fusb300->ep[0];

	if ((!list_empty(&ep->queue)) && (ep->dir_in)) {
		req = list_entry(ep->queue.next,
				struct fusb300_request, queue);
		if ((req->req.length - req->req.actual) > 0)
			fusb300_wrcxf(ep, req);
		if (req->req.length == req->req.actual)
			fusb300_done(ep, req, 0);
	} else
		fusb300_set_cxdone(fusb300);
}

static void fusb300_grp2_handler(struct fusb300 *fusb300)
{
}

static void fusb300_grp3_handler(struct fusb300 *fusb300)
{
}

static void fusb300_grp4_handler(struct fusb300 *fusb300)
{
	u32 int_grp4 = ioread32(fusb300->reg + FUSB300_OFFSET_IGR4);
	u32 int_grp4_en = ioread32(fusb300->reg + FUSB300_OFFSET_IGER4);
	int i;
	u32 reg;

	int_grp4 &= int_grp4_en;

	for (i = 1; i < FUSB300_MAX_NUM_EP; i++) {
		if (int_grp4 & FUSB300_IGR4_EPn_RX0_INT(i)) {
			fusb300_clear_int(fusb300, FUSB300_OFFSET_IGR4,
					FUSB300_IGR4_EPn_RX0_INT(i)); //W1C
			printk(KERN_INFO"fusb300_grp4_handler: EP%d RX 0 byte\n", i);

#ifdef IDMA_MODE
			// TODO: SW do: If device receive 0 byte from Host, make idma finish (HW version < v1.9.0)
			reg = ioread32(fusb300->reg + FUSB300_OFFSET_EPPRD_W0(i));
			if (reg & FUSB300_EPPRD0_H)
				fusb300_irq_prd_done(fusb300->ep[i], 1);
#endif
		}
	}
}

static void fusb300_grp5_handler(struct fusb300 *fusb300)
{
}

static irqreturn_t fusb300_irq(int irq, void *_fusb300)
{
	struct fusb300 *fusb300 = _fusb300;
	u32 int_grp1 = ioread32(fusb300->reg + FUSB300_OFFSET_IGR1);
	u32 int_grp1_en = ioread32(fusb300->reg + FUSB300_OFFSET_IGER1);
	u32 int_grp0 = ioread32(fusb300->reg + FUSB300_OFFSET_IGR0);
	u32 int_grp0_en = ioread32(fusb300->reg + FUSB300_OFFSET_IGER0);
	struct usb_ctrlrequest ctrl;
	u8 in;
	u32 reg;
	int i;

	spin_lock(&fusb300->lock);

	int_grp1 &= int_grp1_en;
	int_grp0 &= int_grp0_en;

	if (int_grp1 & FUSB300_IGR1_WARM_RST_INT) {
		fusb300_clear_int(fusb300, FUSB300_OFFSET_IGR1,
				  FUSB300_IGR1_WARM_RST_INT);
		printk(KERN_INFO"fusb300_warmreset\n");
		fusb300_reset(fusb300);
	}

	if (int_grp1 & FUSB300_IGR1_HOT_RST_INT) {
		fusb300_clear_int(fusb300, FUSB300_OFFSET_IGR1,
				  FUSB300_IGR1_HOT_RST_INT);
		printk(KERN_INFO"fusb300_hotreset\n");
		fusb300_reset(fusb300);
	}

	if (int_grp1 & FUSB300_IGR1_USBRST_INT) {
		fusb300_clear_int(fusb300, FUSB300_OFFSET_IGR1,
				  FUSB300_IGR1_USBRST_INT);
		printk(KERN_INFO"fusb300_reset\n");
		fusb300_reset(fusb300);
	}

	if (int_grp1 & FUSB300_IGR1_LINK_IN_TEST_MODE_INT) {
		fusb300_disable_bit(fusb300, FUSB300_OFFSET_IGER1,
				FUSB300_IGER1_LINK_IN_TEST_MODE_INT);
		printk(KERN_INFO"for electrical\n");
	}

	/* COMABT_INT has a highest priority */
	if (int_grp1 & FUSB300_IGR1_CX_COMABT_INT) {
		fusb300_clear_int(fusb300, FUSB300_OFFSET_IGR1,
				  FUSB300_IGR1_CX_COMABT_INT);
		printk(KERN_INFO"fusb300_ep0abt\n");
	}

	if (int_grp1 & FUSB300_IGR1_VBUS_CHG_INT) {
		fusb300_clear_int(fusb300, FUSB300_OFFSET_IGR1,
				  FUSB300_IGR1_VBUS_CHG_INT);
		printk(KERN_INFO"fusb300_vbus_change\n");
		reg = ioread32(fusb300->reg + FUSB300_OFFSET_GCR);
		reg &= FUSB300_GCR_VBUS_STATUS;
		printk(KERN_INFO"FUSB300_OFFSET_GCR : 0x%x\n", reg);
	}

	if (int_grp1 & FUSB300_IGR1_U3_EXIT_FAIL_INT) {
		fusb300_clear_int(fusb300, FUSB300_OFFSET_IGR1,
				  FUSB300_IGR1_U3_EXIT_FAIL_INT);
	}

	if (int_grp1 & FUSB300_IGR1_U2_EXIT_FAIL_INT) {
		fusb300_clear_int(fusb300, FUSB300_OFFSET_IGR1,
				  FUSB300_IGR1_U2_EXIT_FAIL_INT);
	}

	if (int_grp1 & FUSB300_IGR1_U1_EXIT_FAIL_INT) {
		fusb300_clear_int(fusb300, FUSB300_OFFSET_IGR1,
				  FUSB300_IGR1_U1_EXIT_FAIL_INT);
	}

	if (int_grp1 & FUSB300_IGR1_U2_ENTRY_FAIL_INT) {
		fusb300_clear_int(fusb300, FUSB300_OFFSET_IGR1,
				  FUSB300_IGR1_U2_ENTRY_FAIL_INT);
	}

	if (int_grp1 & FUSB300_IGR1_U1_ENTRY_FAIL_INT) {
		fusb300_clear_int(fusb300, FUSB300_OFFSET_IGR1,
				  FUSB300_IGR1_U1_ENTRY_FAIL_INT);
	}

	if (int_grp1 & FUSB300_IGR1_U3_EXIT_INT) {
		fusb300_clear_int(fusb300, FUSB300_OFFSET_IGR1,
				  FUSB300_IGR1_U3_EXIT_INT);
		printk(KERN_INFO "FUSB300_IGR1_U3_EXIT_INT\n");
	}

	if (int_grp1 & FUSB300_IGR1_U2_EXIT_INT) {
		fusb300_clear_int(fusb300, FUSB300_OFFSET_IGR1,
				  FUSB300_IGR1_U2_EXIT_INT);
//		printk(KERN_INFO "FUSB300_IGR1_U2_EXIT_INT\n");
	}

	if (int_grp1 & FUSB300_IGR1_U1_EXIT_INT) {
		fusb300_clear_int(fusb300, FUSB300_OFFSET_IGR1,
				  FUSB300_IGR1_U1_EXIT_INT);
//		printk(KERN_INFO "FUSB300_IGR1_U1_EXIT_INT\n");
	}

	if (int_grp1 & FUSB300_IGR1_U3_ENTRY_INT) {
		fusb300_clear_int(fusb300, FUSB300_OFFSET_IGR1,
				  FUSB300_IGR1_U3_ENTRY_INT);
		fusb300_enable_bit(fusb300, FUSB300_OFFSET_SSCR1,
				   FUSB300_SSCR1_GO_U3_DONE);
		printk(KERN_INFO "FUSB300_IGR1_U3_ENTRY_INT\n");
	}

	if (int_grp1 & FUSB300_IGR1_U2_ENTRY_INT) {
		fusb300_clear_int(fusb300, FUSB300_OFFSET_IGR1,
				  FUSB300_IGR1_U2_ENTRY_INT);
//		printk(KERN_INFO "FUSB300_IGR1_U2_ENTRY_INT\n");
	}

	if (int_grp1 & FUSB300_IGR1_U1_ENTRY_INT) {
		fusb300_clear_int(fusb300, FUSB300_OFFSET_IGR1,
				  FUSB300_IGR1_U1_ENTRY_INT);
//		printk(KERN_INFO "FUSB300_IGR1_U1_ENTRY_INT\n");
	}

	if (int_grp1 & FUSB300_IGR1_RESM_INT) {
		fusb300_clear_int(fusb300, FUSB300_OFFSET_IGR1,
				  FUSB300_IGR1_RESM_INT);
		printk(KERN_INFO "fusb300_resume\n");
	}

	if (int_grp1 & FUSB300_IGR1_SUSP_INT) {
		fusb300_clear_int(fusb300, FUSB300_OFFSET_IGR1,
				  FUSB300_IGR1_SUSP_INT);
		printk(KERN_INFO "fusb300_suspend\n");
		fusb300_enable_bit(fusb300, FUSB300_OFFSET_HSCR,
			FUSB300_HSCR_HS_GOSUSP);

		if (fusb300_is_rmwkup(fusb300)) {
			mdelay(1500); //USB-CV: wait > 1s
			fusb300_enable_bit(fusb300, FUSB300_OFFSET_HSCR,
				FUSB300_HSCR_HS_GORMWKU);
		}
	}

	if (int_grp1 & FUSB300_IGR1_HS_LPM_INT) {
		fusb300_clear_int(fusb300, FUSB300_OFFSET_IGR1,
				  FUSB300_IGR1_HS_LPM_INT);
		printk(KERN_INFO "fusb300_HS_LPM_INT\n");
	}

	if (int_grp1 & FUSB300_IGR1_DEV_MODE_CHG_INT) {
		fusb300_clear_int(fusb300, FUSB300_OFFSET_IGR1,
				  FUSB300_IGR1_DEV_MODE_CHG_INT);
		check_device_mode(fusb300);
	}

	if (int_grp1 & FUSB300_IGR1_CX_COMFAIL_INT) {
		fusb300_set_cxstall(fusb300);
		printk(KERN_INFO "fusb300_ep0fail\n");
	}

	if (int_grp1 & FUSB300_IGR1_CX_SETUP_INT) {
		printk(KERN_INFO "fusb300_ep0setup\n");
		if (setup_packet(fusb300, &ctrl)) {
			spin_unlock(&fusb300->lock);
			if (!fusb300->driver)
				fusb300_set_cxstall(fusb300);
			else {
				if (fusb300->driver->setup(&fusb300->gadget, &ctrl) < 0)
					fusb300_set_cxstall(fusb300);
			}
			spin_lock(&fusb300->lock);
		}
	}

	if (int_grp1 & FUSB300_IGR1_CX_CMDEND_INT)
		printk(KERN_INFO "fusb300_cmdend\n");


	if (int_grp1 & FUSB300_IGR1_CX_OUT_INT) {
		printk(KERN_INFO "fusb300_cxout\n");
		fusb300_ep0out(fusb300);
	}

	if (int_grp1 & FUSB300_IGR1_CX_IN_INT) {
		printk(KERN_INFO "fusb300_cxin\n");
		fusb300_ep0in(fusb300);
	}

	if (int_grp0) {
		for (i = 1; i < FUSB300_MAX_NUM_EP; i++) {
#if defined (IDMA_MODE) && !defined (POLLING_PRD)
			// DMA finishs EPn PRD
			if (int_grp0 & FUSB300_IGR0_EPn_PRD_INT(i)) {
				/* Write 1 clear */
				fusb300_clear_int(fusb300, FUSB300_OFFSET_IGR0,
						FUSB300_IGR0_EPn_PRD_INT(i));

				fusb300_irq_prd_done(fusb300->ep[i], 0);
			}
			else
#endif
			{
				if (int_grp0 & FUSB300_IGR0_EPn_FIFO_INT(i)) {
					reg = ioread32(fusb300->reg +
						FUSB300_OFFSET_EPSET1(i));
					in = (reg & FUSB300_EPSET1_DIRIN) ? 1 : 0;
#ifdef IDMA_MODE
					if (in)
						in_ep_fifo_handler_idma(fusb300->ep[i]);
					else
						out_ep_fifo_handler_idma(fusb300->ep[i]);
#else
					if (in)
						in_ep_fifo_handler(fusb300->ep[i]);
					else
						out_ep_fifo_handler(fusb300->ep[i]);
#endif
				}
			}
		}
	}

	if (int_grp1 & FUSB300_IGR1_INTGRP2)
		fusb300_grp2_handler(fusb300);

	if (int_grp1 & FUSB300_IGR1_INTGRP3)
		fusb300_grp3_handler(fusb300);

	if (int_grp1 & FUSB300_IGR1_INTGRP4)
		fusb300_grp4_handler(fusb300);

	if (int_grp1 & FUSB300_IGR1_INTGRP5)
		fusb300_grp5_handler(fusb300);

	spin_unlock(&fusb300->lock);

	return IRQ_HANDLED;
}

/* set vbus debounce: 0x3ff (vbus gating), < 0x3ff (has vbus)*/
static void fusb300_set_vbus(struct fusb300 *fusb300,
				u16 deb_time)
{
	u32 reg;

	reg = ioread32(fusb300->reg + FUSB300_OFFSET_EFCS);
	reg &= ~(0x3ff << 0);
	reg |= FUSB300_EFCS_VBUS_DEBOUNCE(deb_time);

	iowrite32(reg, fusb300->reg + FUSB300_OFFSET_EFCS);
}

static void init_controller(struct fusb300 *fusb300)
{
	/* enable high-speed LPM */
	fusb300_enable_bit(fusb300, FUSB300_OFFSET_HSCR, FUSB300_HSCR_HS_LPM_PERMIT);

	/*set u1 u2 timer*/
	fusb300_set_u2_timeout(fusb300, 0xff);
	fusb300_set_u1_timeout(fusb300, 0xff);

	/* disable all grp1 interrupt */
	iowrite32(0x0, fusb300->reg + FUSB300_OFFSET_IGER1);

	/*set vbus gating */
	fusb300_set_vbus(fusb300, 0x3ff);
}
/*------------------------------------------------------------------------*/

static int fusb300_udc_start(struct usb_gadget *g,
		struct usb_gadget_driver *driver)
{
	struct fusb300 *fusb300 = to_fusb300(g);
	unsigned long	flags;
#ifdef CONFIG_USB_FOTG330_OTG
	u32 reg;
#endif

	spin_lock_irqsave(&fusb300->lock, flags);

	/* hook up the driver */
	driver->driver.bus = NULL;
	fusb300->driver = driver;

#ifdef CONFIG_USB_FOTG330_OTG
	reg = ioread32(fusb300->reg + FUSB300_OTG_CSR);
	/* if B-peripheral, disable host */
	if (reg & OTG_CSR_CROLE) {
		printk("  OCSR: device role\n");
		reg = ioread32(fusb300->reg + FUSB330_XHCI_USBCMD);
		if (reg & XHCI_USBCMD_RUN) {
			reg &= ~XHCI_USBCMD_RUN;
			iowrite32(reg, fusb300->reg + FUSB330_XHCI_USBCMD);
		}
		/* set device/otg global interrupt */
		iowrite32(OTG_GIER_DEV_INT_EN | OTG_GIER_OTG_INT_EN,
			fusb300->reg + FUSB300_OTG_GIER);

		/* enable all grp1 interrupt, except for CX_CMDEND_INT and CX_OUT_INT */
		iowrite32(0xcfffff9f, fusb300->reg + FUSB300_OFFSET_IGER1);

		/*release vbus gating */
		fusb300_set_vbus(fusb300, 0x50);
	}
	/* host mode */
	else {
		printk("  OCSR: host role\n");
		/* set host/otg global interrupt */
		iowrite32(OTG_GIER_HOST_INT_EN | OTG_GIER_OTG_INT_EN,
			fusb300->reg + FUSB300_OTG_GIER);

		/* disable all grp1 interrupt (and vbus keeps gating) */
		iowrite32(0x0, fusb300->reg + FUSB300_OFFSET_IGER1);
	}
#else
	/* enable all grp1 interrupt, except for CX_CMDEND_INT and CX_OUT_INT */
	iowrite32(0xcfffff9f, fusb300->reg + FUSB300_OFFSET_IGER1);

	/*release vbus gating */
	fusb300_set_vbus(fusb300, 0x50);
#endif

	spin_unlock_irqrestore(&fusb300->lock, flags);
	return 0;
}

static int fusb300_udc_stop(struct usb_gadget *g)
{
	struct fusb300 *fusb300 = to_fusb300(g);
	unsigned long	flags;

	spin_lock_irqsave(&fusb300->lock, flags);

	init_controller(fusb300);
	fusb300->driver = NULL;

	spin_unlock_irqrestore(&fusb300->lock, flags);

	return 0;
}

static int fusb300_udc_pullup(struct usb_gadget *g, int is_on)
{
	return 0;
}

static const struct usb_gadget_ops fusb300_gadget_ops = {
	.pullup		= fusb300_udc_pullup,
	.udc_start	= fusb300_udc_start,
	.udc_stop	= fusb300_udc_stop,
};
/*--------------------------------------------------------------------------*/

static int fusb300_remove(struct platform_device *pdev)
{
	struct fusb300 *fusb300 = platform_get_drvdata(pdev);
	int i;

	usb_del_gadget_udc(&fusb300->gadget);
	iounmap(fusb300->reg);
	free_irq(platform_get_irq(pdev, 0), fusb300);

	fusb300_free_request(&fusb300->ep[0]->ep, fusb300->ep0_req);
	for (i = 0; i < FUSB300_MAX_NUM_EP; i++)
		kfree(fusb300->ep[i]);
	kfree(fusb300);

	return 0;
}

static int fusb300_probe(struct platform_device *pdev)
{
	struct resource *res, *ires;
	void __iomem *reg = NULL;
	struct fusb300 *fusb300 = NULL;
	struct fusb300_ep *_ep[FUSB300_MAX_NUM_EP];
	int ret = 0;
	int i;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		ret = -ENODEV;
		pr_err("platform_get_resource error.\n");
		goto clean_up;
	}

	ires = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!ires) {
		ret = -ENODEV;
		dev_err(&pdev->dev,
			"platform_get_resource IORESOURCE_IRQ error.\n");
		goto clean_up;
	}

	reg = ioremap(res->start, resource_size(res));
	if (reg == NULL) {
		ret = -ENOMEM;
		pr_err("ioremap error.\n");
		goto clean_up;
	}

	/* initialize udc */
	fusb300 = kzalloc(sizeof(struct fusb300), GFP_KERNEL);
	if (fusb300 == NULL) {
		ret = -ENOMEM;
		goto clean_up;
	}

	for (i = 0; i < FUSB300_MAX_NUM_EP; i++) {
		_ep[i] = kzalloc(sizeof(struct fusb300_ep), GFP_KERNEL);
		if (_ep[i] == NULL) {
			ret = -ENOMEM;
			goto clean_up;
		}
		fusb300->ep[i] = _ep[i];
	}

	spin_lock_init(&fusb300->lock);

	platform_set_drvdata(pdev, fusb300);

	fusb300->gadget.ops = &fusb300_gadget_ops;

	fusb300->gadget.max_speed = USB_SPEED_SUPER;
	fusb300->gadget.name = udc_name;
	fusb300->reg = reg;

	ret = request_irq(ires->start, fusb300_irq, IRQF_SHARED,
			  udc_name, fusb300);
	if (ret < 0) {
		pr_err("request_irq error (%d)\n", ret);
		goto clean_up;
	}

	INIT_LIST_HEAD(&fusb300->gadget.ep_list);

	for (i = 0; i < FUSB300_MAX_NUM_EP ; i++) {
		struct fusb300_ep *ep = fusb300->ep[i];

		if (i != 0) {
			INIT_LIST_HEAD(&fusb300->ep[i]->ep.ep_list);
			list_add_tail(&fusb300->ep[i]->ep.ep_list,
				     &fusb300->gadget.ep_list);
		}
		ep->fusb300 = fusb300;
		INIT_LIST_HEAD(&ep->queue);
		ep->ep.name = fusb300_ep_name[i];
		ep->ep.ops = &fusb300_ep_ops;
		usb_ep_set_maxpacket_limit(&ep->ep, SS_BULK_MAX_PACKET_SIZE);

		if (i == 0) {
			ep->ep.caps.type_control = true;
		} else {
			ep->ep.caps.type_iso = true;
			ep->ep.caps.type_bulk = true;
			ep->ep.caps.type_int = true;
		}

		ep->ep.caps.dir_in = true;
		ep->ep.caps.dir_out = true;
	}
	usb_ep_set_maxpacket_limit(&fusb300->ep[0]->ep, SS_CTL_MAX_PACKET_SIZE);
	fusb300->ep[0]->epnum = 0;
	fusb300->gadget.ep0 = &fusb300->ep[0]->ep;
	INIT_LIST_HEAD(&fusb300->gadget.ep0->ep_list);

	fusb300->ep0_req = fusb300_alloc_request(&fusb300->ep[0]->ep,
				GFP_KERNEL);
	if (fusb300->ep0_req == NULL) {
		ret = -ENOMEM;
		goto clean_up3;
	}

	init_controller(fusb300);
	ret = usb_add_gadget_udc(&pdev->dev, &fusb300->gadget);
	if (ret)
		goto err_add_udc;

	for (i = 1; i < FUSB300_MAX_NUM_EP; i++) {
		struct fusb300_ep *ep = fusb300->ep[i];

		ep->irq_current_task = kzalloc(sizeof(struct fusb300_irq_task), GFP_KERNEL);
		if (ep->irq_current_task == NULL) {
			pr_err("irq_current_task kzalloc error.\n");
			goto err_add_udc;
		}

		ep->irq_current_task->req = NULL;
	}

	dev_info(&pdev->dev, "%s probe\n", udc_name);

	return 0;

err_add_udc:
	fusb300_free_request(&fusb300->ep[0]->ep, fusb300->ep0_req);

clean_up3:
	free_irq(ires->start, fusb300);

clean_up:
	if (fusb300) {
		if (fusb300->ep0_req)
			fusb300_free_request(&fusb300->ep[0]->ep,
				fusb300->ep0_req);
		for (i = 0; i < FUSB300_MAX_NUM_EP; i++)
			kfree(fusb300->ep[i]);
		kfree(fusb300);
	}
	if (reg)
		iounmap(reg);

	return ret;
}

#ifdef CONFIG_OF
static const struct of_device_id fusb300_udc_dt_ids[] = {
	{ .compatible = "faraday,fusb300" },
	{ }
};

MODULE_DEVICE_TABLE(of, fusb300_udc_dt_ids);
#endif

static struct platform_driver fusb300_driver = {
	.remove =	fusb300_remove,
	.driver		= {
		.name =	(char *) udc_name,
		.of_match_table = of_match_ptr(fusb300_udc_dt_ids),
	},
};

module_platform_driver_probe(fusb300_driver, fusb300_probe);
