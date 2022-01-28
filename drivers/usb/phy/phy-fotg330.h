// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2015-2019 Faraday Technology Corp.
 */

#ifndef	__FOTG330_USB_CONTROLLER__
#define	__FOTG330_USB_CONTROLLER__

/* Host controller register */
#define FOTG330_XHCI_USBCMD	0x20
#define USBCMD_RUN		(1 << 0)

#define FOTG330_XHCI_USBSTS	0x24
#define USBSTS_HCHALT		(1 << 0)

/* OTG register */
#define FOTG330_OCSR		0x1800	/* OTG Control Status Register */
#define FOTG330_OISR		0x1804	/* OTG Interrupt Status Register */
#define FOTG330_OIER		0x1808	/* OTG Interrupt Enable Register */
#define FOTG330_GIER		0x180c	/* OTG Global Interrupt Enable Register */

/* Device register, from offset 0x2000 */
#define FOTG330_DEV_IGER1	(0x2000 + 0x424)
#define FOTG330_DEV_EFCS	(0x2000 + 0x328)
#define EFCS_VBUS_DEBOUNCE(x)	(((x) & 0x3FF) << 0)  //b[9:0]

/*
* OTG Control Status Register (offset = 1800H)
*/
#define OCSR_CID		(1 << 0)
#define OCSR_CROLE		(1 << 1)
#define OCSR_POLARITY		(1 << 2)

#define ID_A_TYPE		0
#define ID_B_TYPE		1

#define CROLE_Host		0
#define CROLE_Peripheral	1

/*
* OTG Interrupt Enable Register (offset = 1808H)
*/
#define OIER_IDCHG_EN		(1 << 0)
#define OIER_RLCHG_EN		(1 << 1)

/*
* Global Interrupt Enable Register (offset = 180cH)
*/
#define GIER_OTG_INT_EN		(1 << 0)
#define GIER_HOST_INT_EN	(1 << 1)
#define GIER_DEV_INT_EN		(1 << 2)



struct fotg330_otg {
	struct usb_phy phy;

	/* base address */
	void __iomem *regs;

	struct platform_device *pdev;
	int irq;
	u32 irq_en;

	struct delayed_work work;
	struct workqueue_struct *qwork;

	spinlock_t wq_lock;
};

#endif
