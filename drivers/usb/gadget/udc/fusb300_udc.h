// SPDX-License-Identifier: GPL-2.0
/*
 * FUSB300 UDC (USB gadget)
 *
 * Copyright (C) 2010-2021 Faraday Technology Corp.
 *
 * Author: Faraday CTD/SD Dept.
 */


#ifndef __FUSB300_UDC_H__
#define __FUSB300_UDC_H__

#include <linux/kthread.h>

#ifdef CONFIG_USB_FOTG330_OTG
/* Host controller register */
#define FUSB330_XHCI_USBCMD		0x20
#define XHCI_USBCMD_RUN			(1 << 0)

/* OTG register */
#define FUSB300_OTG_CSR			0x1800	/* OTG Control Status Register */
#define FUSB300_OTG_GIER		0x180c	/* OTG Global Interrupt Enable Register */

#define OTG_CSR_CID			(1 << 0)
#define OTG_CSR_CROLE			(1 << 1)

#define OTG_GIER_OTG_INT_EN		(1 << 0)
#define OTG_GIER_HOST_INT_EN		(1 << 1)
#define OTG_GIER_DEV_INT_EN		(1 << 2)
#endif

/*----------------------------------------------------------------------*/
#define FOTG330_DEV_BASE		0x2000

#define FUSB300_OFFSET_GCR		(FOTG330_DEV_BASE + 0x00)
#define FUSB300_OFFSET_GTM		(FOTG330_DEV_BASE + 0x04)
#define FUSB300_OFFSET_DAR		(FOTG330_DEV_BASE + 0x08)
#define FUSB300_OFFSET_CSR		(FOTG330_DEV_BASE + 0x0C)
#define FUSB300_OFFSET_CXPORT		(FOTG330_DEV_BASE + 0x10)
#define FUSB300_OFFSET_EPSET0(n)	(FOTG330_DEV_BASE + 0x20 + ((n) - 1) * 0x30)
#define FUSB300_OFFSET_EPSET1(n)	(FOTG330_DEV_BASE + 0x24 + ((n) - 1) * 0x30)
#define FUSB300_OFFSET_EPSET2(n)	(FOTG330_DEV_BASE + 0x28 + ((n) - 1) * 0x30)
#define FUSB300_OFFSET_EPFFR(n)		(FOTG330_DEV_BASE + 0x2c + ((n) - 1) * 0x30)
#define FUSB300_OFFSET_EPSTRID(n)	(FOTG330_DEV_BASE + 0x40 + ((n) - 1) * 0x30)
#define FUSB300_OFFSET_HSPTM		(FOTG330_DEV_BASE + 0x300)
#define FUSB300_OFFSET_HSCR		(FOTG330_DEV_BASE + 0x304)
#define FUSB300_OFFSET_SSCR0		(FOTG330_DEV_BASE + 0x308)
#define FUSB300_OFFSET_SSCR1		(FOTG330_DEV_BASE + 0x30C)
#define FUSB300_OFFSET_SSCR2		(FOTG330_DEV_BASE + 0x310)
#define FUSB300_OFFSET_DEVNOTF		(FOTG330_DEV_BASE + 0x314)
#define FUSB300_OFFSET_DNC1		(FOTG330_DEV_BASE + 0x318)
#define FUSB300_OFFSET_CS		(FOTG330_DEV_BASE + 0x31C)
#define FUSB300_OFFSET_SOF		(FOTG330_DEV_BASE + 0x324)
#define FUSB300_OFFSET_EFCS		(FOTG330_DEV_BASE + 0x328)
#define FUSB300_OFFSET_LTMCFG		(FOTG330_DEV_BASE + 0x32C)
#define FUSB300_OFFSET_IGR0		(FOTG330_DEV_BASE + 0x400)
#define FUSB300_OFFSET_IGR1		(FOTG330_DEV_BASE + 0x404)
#define FUSB300_OFFSET_IGR2		(FOTG330_DEV_BASE + 0x408)
#define FUSB300_OFFSET_IGR3		(FOTG330_DEV_BASE + 0x40C)
#define FUSB300_OFFSET_IGR4		(FOTG330_DEV_BASE + 0x410)
#define FUSB300_OFFSET_IGR5		(FOTG330_DEV_BASE + 0x414)
#define FUSB300_OFFSET_IGER0		(FOTG330_DEV_BASE + 0x420)
#define FUSB300_OFFSET_IGER1		(FOTG330_DEV_BASE + 0x424)
#define FUSB300_OFFSET_IGER2		(FOTG330_DEV_BASE + 0x428)
#define FUSB300_OFFSET_IGER3		(FOTG330_DEV_BASE + 0x42C)
#define FUSB300_OFFSET_IGER4		(FOTG330_DEV_BASE + 0x430)
#define FUSB300_OFFSET_IGER5		(FOTG330_DEV_BASE + 0x434)
#define FUSB300_OFFSET_DMAHMER		(FOTG330_DEV_BASE + 0x500)
#define FUSB300_OFFSET_EPPRDRDY		(FOTG330_DEV_BASE + 0x504)
#define FUSB300_OFFSET_DMAEPMR		(FOTG330_DEV_BASE + 0x508)
#define FUSB300_OFFSET_DMAENR		(FOTG330_DEV_BASE + 0x50C)
#define FUSB300_OFFSET_DMAAPR		(FOTG330_DEV_BASE + 0x510)
#define FUSB300_OFFSET_AHBCR		(FOTG330_DEV_BASE + 0x514)
#define FUSB300_OFFSET_EPPRD_W0(n)	(FOTG330_DEV_BASE + 0x520 + ((n) - 1) * 0x10)
#define FUSB300_OFFSET_EPPRD_W1(n)	(FOTG330_DEV_BASE + 0x524 + ((n) - 1) * 0x10)
#define FUSB300_OFFSET_EPPRD_W2(n)	(FOTG330_DEV_BASE + 0x528 + ((n) - 1) * 0x10)
#define FUSB300_OFFSET_EPRD_PTR(n)	(FOTG330_DEV_BASE + 0x52C + ((n) - 1) * 0x10)
#define FUSB300_OFFSET_REVISION		(FOTG330_DEV_BASE + 0x610)
#define FUSB300_OFFSET_FEATURE1		(FOTG330_DEV_BASE + 0x614)
#define FUSB300_OFFSET_FEATURE2		(FOTG330_DEV_BASE + 0x618)
#define FUSB300_OFFSET_BUFDBG_START	(FOTG330_DEV_BASE + 0x800)
#define FUSB300_OFFSET_BUFDBG_END	(FOTG330_DEV_BASE + 0xBFC)
#define FUSB300_OFFSET_EPPORT(n)	(FOTG330_DEV_BASE + 0x1010 + ((n) - 1) * 0x10)

/*
 * *	Global Control Register (offset = 000H)
 * */
#define FUSB300_GCR_SS_DIS		(1 << 10)
#define FUSB300_GCR_SF_RST		(1 << 8)
#define FUSB300_GCR_VBUS_STATUS		(1 << 7)
#define FUSB300_GCR_USB3_LPMEN		(1 << 6)
#define FUSB300_GCR_SYNC_FIFO1_CLR	(1 << 5)
#define FUSB300_GCR_SYNC_FIFO0_CLR	(1 << 4)
#define FUSB300_GCR_FIFOCLR		(1 << 3)
#define FUSB300_GCR_GLINTEN		(1 << 2)
#define FUSB300_GCR_DEVEN_FS		0x3
#define FUSB300_GCR_DEVEN_HS		0x2
#define FUSB300_GCR_DEVEN_SS		0x1
#define FUSB300_GCR_DEVDIS		0x0
#define FUSB300_GCR_DEVEN_MSK		0x3


/*
 * *Global Test Mode (offset = 004H)
 * */
#define FUSB300_GTM_TST_FORCE_FS	(1 << 19)
#define FUSB300_GTM_TST_FORCE_HS	(1 << 18)
#define FUSB300_GTM_TST_FORCE_SS	(1 << 17)
#define FUSB300_GTM_TST_DIS_SOFGEN	(1 << 16)
#define FUSB300_GTM_TST_CUR_EP_ENTRY(n)	(((n) & 0xF) << 12)
#define FUSB300_GTM_TST_EP_ENTRY(n)	(((n) & 0xF) << 8)
#define FUSB300_GTM_TST_EP_NUM(n)	(((n) & 0xF) << 4)
#define FUSB300_GTM_TST_FIFO_DEG	(1 << 1)
#define FUSB300_GTM_TSTMODE		(1 << 0)

/*
 * * Device Address Register (offset = 008H)
 * */
#define FUSB300_DAR_SETCONFG	(1 << 7)
#define FUSB300_DAR_DRVADDR(x)	((x) & 0x7F)
#define FUSB300_DAR_DRVADDR_MSK	0x7F

/*
 * *Control Transfer Configuration and Status Register
 * (CX_Config_Status, offset = 00CH)
 * */
#define FUSB300_CSR_LEN(x)	(((x) & 0xFFFF) << 8)
#define FUSB300_CSR_LEN_MSK	(0xFFFF << 8)
#define FUSB300_CSR_EMP		(1 << 4)
#define FUSB300_CSR_FUL		(1 << 3)
#define FUSB300_CSR_CLR		(1 << 2)
#define FUSB300_CSR_STL		(1 << 1)
#define FUSB300_CSR_DONE	(1 << 0)

/*
 * * EPn Setting 0 (EPn_SET0, offset = 020H+(n-1)*30H, n=1~15 )
 * */
#define FUSB300_EPSET0_STL_CLR		(1 << 3)
#define FUSB300_EPSET0_CLRSEQNUM	(1 << 2)
#define FUSB300_EPSET0_STL		(1 << 0)

/*
 * * EPn Setting 1 (EPn_SET1, offset = 024H+(n-1)*30H, n=1~15)
 * */
#define FUSB300_EPSET1_START_ENTRY(x)	(((x) & 0xFF) << 24)
#define FUSB300_EPSET1_START_ENTRY_MSK	(0xFF << 24)
#define FUSB300_EPSET1_FIFOENTRY(x)	(((x) & 0x1F) << 12)
#define FUSB300_EPSET1_FIFOENTRY_MSK	(0x1f << 12)
#define FUSB300_EPSET1_ISO_IN_PKTNUM(x)	(((x) & 0x3F) << 6)
#define FUSB300_EPSET1_BWNUM(x)		(((x) & 0x3) << 4)
#define FUSB300_EPSET1_TYPEISO		(1 << 2)
#define FUSB300_EPSET1_TYPEBLK		(2 << 2)
#define FUSB300_EPSET1_TYPEINT		(3 << 2)
#define FUSB300_EPSET1_TYPE(x)		(((x) & 0x3) << 2)
#define FUSB300_EPSET1_TYPE_MSK		(0x3 << 2)
#define FUSB300_EPSET1_DIROUT		(0 << 1)
#define FUSB300_EPSET1_DIRIN		(1 << 1)
#define FUSB300_EPSET1_DIR(x)		(((x) & 0x1) << 1)
#define FUSB300_EPSET1_DIR_MSK		((0x1) << 1)
#define FUSB300_EPSET1_ACTDIS		0
#define FUSB300_EPSET1_ACTEN		1

/*
 * *EPn Setting 2 (EPn_SET2, offset = 028H+(n-1)*30H, n=1~15)
 * */
#define FUSB300_EPSET2_ADDROFS(x)	(((x) & 0x7FFF) << 16)
#define FUSB300_EPSET2_ADDROFS_MSK	(0x7fff << 16)
#define FUSB300_EPSET2_MPS(x)		((x) & 0x7FF)
#define FUSB300_EPSET2_MPS_MSK		0x7FF

/*
 * * EPn FIFO Register (offset = 2cH+(n-1)*30H)
 * */
#define FUSB300_FFR_RST		(1 << 31)
#define FUSB300_FF_FUL		(1 << 30)
#define FUSB300_FF_EMPTY	(1 << 29)
#define FUSB300_EP_TX0BYTE	(1 << 28)
#define FUSB300_FFR_BYCNT	0x1FFFF

/*
 * *EPn Stream ID (EPn_STR_ID, offset = 040H+(n-1)*30H, n=1~15)
 * */
#define FUSB300_STRID_STREN	(1 << 16)
#define FUSB300_STRID_STRID(x)	((x) & 0xFFFF)

/*
 * *HS PHY Test Mode (offset = 300H)
 * */
//#define FUSB300_HSPTM_TSTPKDONE		(1 << 4)
#define FUSB300_HSPTM_TSTPKT		(1 << 3)
#define FUSB300_HSPTM_TSTSET0NAK	(1 << 2)
#define FUSB300_HSPTM_TSTKSTA		(1 << 1)
#define FUSB300_HSPTM_TSTJSTA		(1 << 0)

/*
 * *HS Control Register (offset = 304H)
 * */
#define FUSB300_HSCR_HS_LPM_BESL_MIN(x)	(((x) & 0xf) << 13)
#define FUSB300_HSCR_HS_LPM_PERMIT	(1 << 8)
#define FUSB300_HSCR_HS_LPM_RMWKUP	(1 << 7)
#define FUSB300_HSCR_CAP_LPM_RMWKUP	(1 << 6)
#define FUSB300_HSCR_HS_GOSUSP		(1 << 5)
#define FUSB300_HSCR_HS_GORMWKU		(1 << 4)
#define FUSB300_HSCR_CAP_RMWKUP		(1 << 3)
#define FUSB300_HSCR_IDLECNT_0MS	0
#define FUSB300_HSCR_IDLECNT_1MS	1
#define FUSB300_HSCR_IDLECNT_2MS	2
#define FUSB300_HSCR_IDLECNT_3MS	3
#define FUSB300_HSCR_IDLECNT_4MS	4
#define FUSB300_HSCR_IDLECNT_5MS	5
#define FUSB300_HSCR_IDLECNT_6MS	6
#define FUSB300_HSCR_IDLECNT_7MS	7

/*
 * * SS Controller Register 0 (offset = 308H)
 * */
#define FUSB300_SSCR0_MAX_INTERVAL(x)	(((x) & 0x7) << 4)
#define FUSB300_SSCR0_U2_FUN_EN		(1 << 1)
#define FUSB300_SSCR0_U1_FUN_EN		(1 << 0)

/*
 * * SS Controller Register 1 (offset = 30CH)
 * */
#define FUSB300_SSCR1_GO_U3_DONE	(1 << 8)
#define FUSB300_SSCR1_TXDEEMPH_LEVEL	(1 << 7)
#define FUSB300_SSCR1_DIS_SCRMB		(1 << 6)
#define FUSB300_SSCR1_FORCE_RECOVERY	(1 << 5)
#define FUSB300_SSCR1_U3_WAKEUP_EN	(1 << 4)
#define FUSB300_SSCR1_U2_EXIT_EN	(1 << 3)
#define FUSB300_SSCR1_U1_EXIT_EN	(1 << 2)
#define FUSB300_SSCR1_U2_ENTRY_EN	(1 << 1)
#define FUSB300_SSCR1_U1_ENTRY_EN	(1 << 0)

/*
 * *SS Controller Register 2  (offset = 310H)
 * */
#define FUSB300_SSCR2_SS_TX_SWING		(1 << 25)
#define FUSB300_SSCR2_FORCE_LINKPM_ACCEPT	(1 << 24)
#define FUSB300_SSCR2_U2_INACT_TIMEOUT(x)	(((x) & 0xFF) << 16)
#define FUSB300_SSCR2_U1TIMEOUT(x)		(((x) & 0xFF) << 8)
#define FUSB300_SSCR2_U2TIMEOUT(x)		((x) & 0xFF)

/*
 * *SS Device Notification Control (DEV_NOTF, offset = 314H)
 * */
#define FUSB300_DEVNOTF_CONTEXT0(x)		(((x) & 0xFFFFFF) << 8)
#define FUSB300_DEVNOTF_TYPE_DIS		0
#define FUSB300_DEVNOTF_TYPE_FUNCWAKE		1
#define FUSB300_DEVNOTF_TYPE_LTM		2
#define FUSB300_DEVNOTF_TYPE_BUSINT_ADJMSG	3

/*
 * *BFM Arbiter Priority Register (BFM_ARB offset = 31CH)
 * */
#define FUSB300_BFMARB_ARB_M1	(1 << 3)
#define FUSB300_BFMARB_ARB_M0	(1 << 2)
#define FUSB300_BFMARB_ARB_S1	(1 << 1)
#define FUSB300_BFMARB_ARB_S0	1

/*
 * *Vendor Specific IO Control Register (offset = 320H)
 * */
#define FUSB300_VSIC_VCTLOAD_N	(1 << 8)
#define FUSB300_VSIC_VCTL(x)	((x) & 0x3F)

/*
 * *SOF Mask Timer (offset = 324H)
 * */
#define FUSB300_SOF_MASK_TIMER_HS	0x044c
#define FUSB300_SOF_MASK_TIMER_FS	0x2710

/*
 * *Error Flag and Control Status (offset = 328H)
 * */
#define FUSB300_EFCS_PM_STATE_U3	(3 << 21)
#define FUSB300_EFCS_PM_STATE_U2	(2 << 21)
#define FUSB300_EFCS_PM_STATE_U1	(1 << 21)
#define FUSB300_EFCS_PM_STATE_U0	(0 << 21)
#define FUSB300_EFCS_VBUS_DEBOUNCE(x)		(((x) & 0x3FF) << 0)	//b[9:0]

/*
 * *LTM CFG (offset = 32CH)
 * */
#define FUSB300_LTMCFG_LTM_EN			1
#define FUSB300_LTMCFG_MINBELT_LV_OFFSET	8
#define FUSB300_LTMCFG_MINBELT_LS_OFFSET	18
#define FUSB300_LTMCFG_MAXBELT_LV_OFFSET	20
#define FUSB300_LTMCFG_MAXBELT_LS_OFFSET	30
#define LTMCFG_MULTBY_1K			1
#define LTMCFG_MULTBY_32K			2
#define LTMCFG_MULTBY_1M			3

/*
 * *Interrupt Group 0 Register (offset = 400H)
 * */
#define FUSB300_IGR0_EP15_PRD_INT	(1 << 31)
#define FUSB300_IGR0_EP14_PRD_INT	(1 << 30)
#define FUSB300_IGR0_EP13_PRD_INT	(1 << 29)
#define FUSB300_IGR0_EP12_PRD_INT	(1 << 28)
#define FUSB300_IGR0_EP11_PRD_INT	(1 << 27)
#define FUSB300_IGR0_EP10_PRD_INT	(1 << 26)
#define FUSB300_IGR0_EP9_PRD_INT	(1 << 25)
#define FUSB300_IGR0_EP8_PRD_INT	(1 << 24)
#define FUSB300_IGR0_EP7_PRD_INT	(1 << 23)
#define FUSB300_IGR0_EP6_PRD_INT	(1 << 22)
#define FUSB300_IGR0_EP5_PRD_INT	(1 << 21)
#define FUSB300_IGR0_EP4_PRD_INT	(1 << 20)
#define FUSB300_IGR0_EP3_PRD_INT	(1 << 19)
#define FUSB300_IGR0_EP2_PRD_INT	(1 << 18)
#define FUSB300_IGR0_EP1_PRD_INT	(1 << 17)
#define FUSB300_IGR0_EPn_PRD_INT(n)	(1 << ((n) + 16))

#define FUSB300_IGR0_EP15_FIFO_INT	(1 << 15)
#define FUSB300_IGR0_EP14_FIFO_INT	(1 << 14)
#define FUSB300_IGR0_EP13_FIFO_INT	(1 << 13)
#define FUSB300_IGR0_EP12_FIFO_INT	(1 << 12)
#define FUSB300_IGR0_EP11_FIFO_INT	(1 << 11)
#define FUSB300_IGR0_EP10_FIFO_INT	(1 << 10)
#define FUSB300_IGR0_EP9_FIFO_INT	(1 << 9)
#define FUSB300_IGR0_EP8_FIFO_INT	(1 << 8)
#define FUSB300_IGR0_EP7_FIFO_INT	(1 << 7)
#define FUSB300_IGR0_EP6_FIFO_INT	(1 << 6)
#define FUSB300_IGR0_EP5_FIFO_INT	(1 << 5)
#define FUSB300_IGR0_EP4_FIFO_INT	(1 << 4)
#define FUSB300_IGR0_EP3_FIFO_INT	(1 << 3)
#define FUSB300_IGR0_EP2_FIFO_INT	(1 << 2)
#define FUSB300_IGR0_EP1_FIFO_INT	(1 << 1)
#define FUSB300_IGR0_EPn_FIFO_INT(n)	(1 << (n))

/*
 * *Interrupt Group 1 Register (offset = 404H)
 * */
#define FUSB300_IGR1_INTGRP5		(1 << 31)
#define FUSB300_IGR1_VBUS_CHG_INT	(1 << 30)
#define FUSB300_IGR1_SYNF1_EMPTY_INT	(1 << 29)
#define FUSB300_IGR1_SYNF0_EMPTY_INT	(1 << 28)
#define FUSB300_IGR1_U3_EXIT_FAIL_INT	(1 << 27)
#define FUSB300_IGR1_U2_EXIT_FAIL_INT	(1 << 26)
#define FUSB300_IGR1_U1_EXIT_FAIL_INT	(1 << 25)
#define FUSB300_IGR1_U2_ENTRY_FAIL_INT	(1 << 24)
#define FUSB300_IGR1_U1_ENTRY_FAIL_INT	(1 << 23)
#define FUSB300_IGR1_U3_EXIT_INT	(1 << 22)
#define FUSB300_IGR1_U2_EXIT_INT	(1 << 21)
#define FUSB300_IGR1_U1_EXIT_INT	(1 << 20)
#define FUSB300_IGR1_U3_ENTRY_INT	(1 << 19)
#define FUSB300_IGR1_U2_ENTRY_INT	(1 << 18)
#define FUSB300_IGR1_U1_ENTRY_INT	(1 << 17)
#define FUSB300_IGR1_HOT_RST_INT	(1 << 16)
#define FUSB300_IGR1_WARM_RST_INT	(1 << 15)
#define FUSB300_IGR1_RESM_INT		(1 << 14)
#define FUSB300_IGR1_SUSP_INT		(1 << 13)
#define FUSB300_IGR1_HS_LPM_INT		(1 << 12)
#define FUSB300_IGR1_USBRST_INT		(1 << 11)
#define FUSB300_IGR1_LINK_IN_TEST_MODE_INT	(1 << 10)
#define FUSB300_IGR1_DEV_MODE_CHG_INT	(1 << 9)
#define FUSB300_IGR1_CX_COMABT_INT	(1 << 8)
#define FUSB300_IGR1_CX_COMFAIL_INT	(1 << 7)
#define FUSB300_IGR1_CX_CMDEND_INT	(1 << 6)
#define FUSB300_IGR1_CX_OUT_INT		(1 << 5)
#define FUSB300_IGR1_CX_IN_INT		(1 << 4)
#define FUSB300_IGR1_CX_SETUP_INT	(1 << 3)
#define FUSB300_IGR1_INTGRP4		(1 << 2)
#define FUSB300_IGR1_INTGRP3		(1 << 1)
#define FUSB300_IGR1_INTGRP2		(1 << 0)

/*
 * *Interrupt Group 2 Register (offset = 408H)
 * */
#define FUSB300_IGR2_EPn_STR_ACCEPT_INT(n)	(1 << (5 * (n) - 1))
#define FUSB300_IGR2_EPn_STR_RESUME_INT(n)	(1 << (5 * (n) - 2))
#define FUSB300_IGR2_EPn_STR_REQ_INT(n)		(1 << (5 * (n) - 3))
#define FUSB300_IGR2_EPn_STR_NOTRDY_INT(n)	(1 << (5 * (n) - 4))
#define FUSB300_IGR2_EPn_STR_PRIME_INT(n)	(1 << (5 * (n) - 5))

/*
 * *Interrupt Group 3 Register (offset = 40CH)
 * */
#define FUSB300_IGR3_EPn_STR_ACCEPT_INT(n)	(1 << (5 * ((n) - 6) - 1))
#define FUSB300_IGR3_EPn_STR_RESUME_INT(n)	(1 << (5 * ((n) - 6) - 2))
#define FUSB300_IGR3_EPn_STR_REQ_INT(n)		(1 << (5 * ((n) - 6) - 3))
#define FUSB300_IGR3_EPn_STR_NOTRDY_INT(n)	(1 << (5 * ((n) - 6) - 4))
#define FUSB300_IGR3_EPn_STR_PRIME_INT(n)	(1 << (5 * ((n) - 6) - 5))

/*
 * *Interrupt Group 4 Register (offset = 410H)
 * */
#define FUSB300_IGR4_EPn_RX0_INT(n)		(1 << ((n) + 16))
#define FUSB300_IGR4_EPn_STR_ACCEPT_INT(n)	(1 << (5 * ((n) - 12) - 1))
#define FUSB300_IGR4_EPn_STR_RESUME_INT(n)	(1 << (5 * ((n) - 12) - 2))
#define FUSB300_IGR4_EPn_STR_REQ_INT(n)		(1 << (5 * ((n) - 12) - 3))
#define FUSB300_IGR4_EPn_STR_NOTRDY_INT(n)	(1 << (5 * ((n) - 12) - 4))
#define FUSB300_IGR4_EPn_STR_PRIME_INT(n)	(1 << (5 * ((n) - 12) - 5))

/*
 * *Interrupt Group 5 Register (offset = 414H)
 * */
#define FUSB300_IGR5_EPn_STL_INT(n)		(1 << (n))
#define FUSB300_IGR5_EPn_INACK_INT(n)		(1 << ((n) + 16))

/*
 * *Interrupt Enable Group 0 Register (offset = 420H)
 * */
#define FUSB300_IGER0_EEP15_PRD_INT	(1 << 31)
#define FUSB300_IGER0_EEP14_PRD_INT	(1 << 30)
#define FUSB300_IGER0_EEP13_PRD_INT	(1 << 29)
#define FUSB300_IGER0_EEP12_PRD_INT	(1 << 28)
#define FUSB300_IGER0_EEP11_PRD_INT	(1 << 27)
#define FUSB300_IGER0_EEP10_PRD_INT	(1 << 26)
#define FUSB300_IGER0_EEP9_PRD_INT	(1 << 25)
#define FUSB300_IGER0_EP8_PRD_INT	(1 << 24)
#define FUSB300_IGER0_EEP7_PRD_INT	(1 << 23)
#define FUSB300_IGER0_EEP6_PRD_INT	(1 << 22)
#define FUSB300_IGER0_EEP5_PRD_INT	(1 << 21)
#define FUSB300_IGER0_EEP4_PRD_INT	(1 << 20)
#define FUSB300_IGER0_EEP3_PRD_INT	(1 << 19)
#define FUSB300_IGER0_EEP2_PRD_INT	(1 << 18)
#define FUSB300_IGER0_EEP1_PRD_INT	(1 << 17)
#define FUSB300_IGER0_EEPn_PRD_INT(n)	(1 << ((n) + 16))

#define FUSB300_IGER0_EEP15_FIFO_INT	(1 << 15)
#define FUSB300_IGER0_EEP14_FIFO_INT	(1 << 14)
#define FUSB300_IGER0_EEP13_FIFO_INT	(1 << 13)
#define FUSB300_IGER0_EEP12_FIFO_INT	(1 << 12)
#define FUSB300_IGER0_EEP11_FIFO_INT	(1 << 11)
#define FUSB300_IGER0_EEP10_FIFO_INT	(1 << 10)
#define FUSB300_IGER0_EEP9_FIFO_INT	(1 << 9)
#define FUSB300_IGER0_EEP8_FIFO_INT	(1 << 8)
#define FUSB300_IGER0_EEP7_FIFO_INT	(1 << 7)
#define FUSB300_IGER0_EEP6_FIFO_INT	(1 << 6)
#define FUSB300_IGER0_EEP5_FIFO_INT	(1 << 5)
#define FUSB300_IGER0_EEP4_FIFO_INT	(1 << 4)
#define FUSB300_IGER0_EEP3_FIFO_INT	(1 << 3)
#define FUSB300_IGER0_EEP2_FIFO_INT	(1 << 2)
#define FUSB300_IGER0_EEP1_FIFO_INT	(1 << 1)
#define FUSB300_IGER0_EEPn_FIFO_INT(n)	(1 << (n))

/*
 * *Interrupt Enable Group 1 Register (offset = 424H)
 * */
#define FUSB300_IGER1_EINT_GRP5		(1 << 31)
#define FUSB300_IGER1_VBUS_CHG_INT	(1 << 30)
#define FUSB300_IGER1_SYNF1_EMPTY_INT	(1 << 29)
#define FUSB300_IGER1_SYNF0_EMPTY_INT	(1 << 28)
#define FUSB300_IGER1_U3_EXIT_FAIL_INT	(1 << 27)
#define FUSB300_IGER1_U2_EXIT_FAIL_INT	(1 << 26)
#define FUSB300_IGER1_U1_EXIT_FAIL_INT	(1 << 25)
#define FUSB300_IGER1_U2_ENTRY_FAIL_INT	(1 << 24)
#define FUSB300_IGER1_U1_ENTRY_FAIL_INT	(1 << 23)
#define FUSB300_IGER1_U3_EXIT_INT	(1 << 22)
#define FUSB300_IGER1_U2_EXIT_INT	(1 << 21)
#define FUSB300_IGER1_U1_EXIT_INT	(1 << 20)
#define FUSB300_IGER1_U3_ENTRY_INT	(1 << 19)
#define FUSB300_IGER1_U2_ENTRY_INT	(1 << 18)
#define FUSB300_IGER1_U1_ENTRY_INT	(1 << 17)
#define FUSB300_IGER1_HOT_RST_INT	(1 << 16)
#define FUSB300_IGER1_WARM_RST_INT	(1 << 15)
#define FUSB300_IGER1_RESM_INT		(1 << 14)
#define FUSB300_IGER1_SUSP_INT		(1 << 13)
#define FUSB300_IGER1_LPM_INT		(1 << 12)
#define FUSB300_IGER1_HS_RST_INT	(1 << 11)
#define FUSB300_IGER1_LINK_IN_TEST_MODE_INT	(1 << 10)
#define FUSB300_IGER1_DEV_MODE_CHG_INT	(1 << 9)
#define FUSB300_IGER1_CX_COMABT_INT	(1 << 8)
#define FUSB300_IGER1_CX_COMFAIL_INT	(1 << 7)
#define FUSB300_IGER1_CX_CMDEND_INT	(1 << 6)
#define FUSB300_IGER1_CX_OUT_INT	(1 << 5)
#define FUSB300_IGER1_CX_IN_INT		(1 << 4)
#define FUSB300_IGER1_CX_SETUP_INT	(1 << 3)
#define FUSB300_IGER1_INTGRP4		(1 << 2)
#define FUSB300_IGER1_INTGRP3		(1 << 1)
#define FUSB300_IGER1_INTGRP2		(1 << 0)

/*
 * *Interrupt Enable Group 2 Register (offset = 428H)
 * */
#define FUSB300_IGER2_EEPn_STR_ACCEPT_INT(n)	(1 << (5 * (n) - 1))
#define FUSB300_IGER2_EEPn_STR_RESUME_INT(n)	(1 << (5 * (n) - 2))
#define FUSB300_IGER2_EEPn_STR_REQ_INT(n)	(1 << (5 * (n) - 3))
#define FUSB300_IGER2_EEPn_STR_NOTRDY_INT(n)	(1 << (5 * (n) - 4))
#define FUSB300_IGER2_EEPn_STR_PRIME_INT(n)	(1 << (5 * (n) - 5))

/*
 * *Interrupt Enable Group 3 Register (offset = 42CH)
 * */
#define FUSB300_IGER3_EEPn_STR_ACCEPT_INT(n)	(1 << (5 * ((n) - 6) - 1))
#define FUSB300_IGER3_EEPn_STR_RESUME_INT(n)	(1 << (5 * ((n) - 6) - 2))
#define FUSB300_IGER3_EEPn_STR_REQ_INT(n)	(1 << (5 * ((n) - 6) - 3))
#define FUSB300_IGER3_EEPn_STR_NOTRDY_INT(n)	(1 << (5 * ((n) - 6) - 4))
#define FUSB300_IGER3_EEPn_STR_PRIME_INT(n)	(1 << (5 * ((n) - 6) - 5))

/*
 * *Interrupt Enable Group 4 Register (offset = 430H)
 * */
#define FUSB300_IGER4_EEPn_RX0_INT(n)		(1 << ((n) + 16))
#define FUSB300_IGER4_EEPn_STR_ACCEPT_INT(n)	(1 << (5 * ((n) - 12) - 1))
#define FUSB300_IGER4_EEPn_STR_RESUME_INT(n)	(1 << (5 * ((n) - 12) - 2))
#define FUSB300_IGER4_EEPn_STR_REQ_INT(n)	(1 << (5 * ((n) - 12) - 3))
#define FUSB300_IGER4_EEPn_STR_NOTRDY_INT(n)	(1 << (5 * ((n) - 12) - 4))
#define FUSB300_IGER4_EEPn_STR_PRIME_INT(n)	(1 << (5 * ((n) - 12) - 5))

/*
 * *Interrupt Enable Group 5 Register (offset = 434H)
 * */
#define FUSB300_IGER5_EEPn_STL_INT(n)		(1 << (n))
#define FUSB300_IGER5_EEPn_INACK_INT(n)		(1 << ((n) + 16))


/* EP PRD Ready (EP_PRD_RDY, offset = 504H) */
#define FUSB300_EPPRDR_EP15_PRD_RDY		(1 << 15)
#define FUSB300_EPPRDR_EP14_PRD_RDY		(1 << 14)
#define FUSB300_EPPRDR_EP13_PRD_RDY		(1 << 13)
#define FUSB300_EPPRDR_EP12_PRD_RDY		(1 << 12)
#define FUSB300_EPPRDR_EP11_PRD_RDY		(1 << 11)
#define FUSB300_EPPRDR_EP10_PRD_RDY		(1 << 10)
#define FUSB300_EPPRDR_EP9_PRD_RDY		(1 << 9)
#define FUSB300_EPPRDR_EP8_PRD_RDY		(1 << 8)
#define FUSB300_EPPRDR_EP7_PRD_RDY		(1 << 7)
#define FUSB300_EPPRDR_EP6_PRD_RDY		(1 << 6)
#define FUSB300_EPPRDR_EP5_PRD_RDY		(1 << 5)
#define FUSB300_EPPRDR_EP4_PRD_RDY		(1 << 4)
#define FUSB300_EPPRDR_EP3_PRD_RDY		(1 << 3)
#define FUSB300_EPPRDR_EP2_PRD_RDY		(1 << 2)
#define FUSB300_EPPRDR_EP1_PRD_RDY		(1 << 1)
#define FUSB300_EPPRDR_EP_PRD_RDY(n)		(1 << (n))

/* DMA Arbiter Priority Register (offset = 510H) */
#define FUSB300_DMAAPR_DMA_RESET		(1 << 31)

/* AHB Bus Control Register (offset = 514H) */
#define FUSB300_AHBBCR_S1_SPLIT_ON		(1 << 17)
#define FUSB300_AHBBCR_S0_SPLIT_ON		(1 << 16)
#define FUSB300_AHBBCR_S1_1entry		(0 << 12)
#define FUSB300_AHBBCR_S1_4entry		(3 << 12)
#define FUSB300_AHBBCR_S1_8entry		(5 << 12)
#define FUSB300_AHBBCR_S1_16entry		(7 << 12)
#define FUSB300_AHBBCR_S0_1entry		(0 << 8)
#define FUSB300_AHBBCR_S0_4entry		(3 << 8)
#define FUSB300_AHBBCR_S0_8entry		(5 << 8)
#define FUSB300_AHBBCR_S0_16entry		(7 << 8)
#define FUSB300_AHBBCR_M1_BURST_SINGLE		(0 << 4)
#define FUSB300_AHBBCR_M1_BURST_INCR		(1 << 4)
#define FUSB300_AHBBCR_M1_BURST_INCR4		(3 << 4)
#define FUSB300_AHBBCR_M1_BURST_INCR8		(5 << 4)
#define FUSB300_AHBBCR_M1_BURST_INCR16		(7 << 4)
#define FUSB300_AHBBCR_M0_BURST_SINGLE		0
#define FUSB300_AHBBCR_M0_BURST_INCR		1
#define FUSB300_AHBBCR_M0_BURST_INCR4		3
#define FUSB300_AHBBCR_M0_BURST_INCR8		5
#define FUSB300_AHBBCR_M0_BURST_INCR16		7

/* WORD 0 Data Structure of PRD Table */
#define FUSB300_EPPRD0_M			(1 << 30)
#define FUSB300_EPPRD0_O			(1 << 29)
/* The finished prd */
#define FUSB300_EPPRD0_F			(1 << 28)
#define FUSB300_EPPRD0_I			(1 << 27)
#define FUSB300_EPPRD0_A			(1 << 26)
/* To decide HW point to first prd at next time */
#define FUSB300_EPPRD0_L			(1 << 25)
#define FUSB300_EPPRD0_H			(1 << 24)
#define FUSB300_EPPRD0_BTC(n)			((n) & 0xFFFFFF)

/* Internal DMA related */
#define MAX_PRD_TFR_LEN			0xFFFFFF  /* 16MB-1 */

/* FEATURE2 (offset = 618H) */
#define FUSB300_TOTAL_ENTRY(value)	((value) >> 4 & 0xFF)

/*----------------------------------------------------------------------*/
#define FUSB300_MAX_NUM_EP		(8 + 1)	//(IN+OUT) + EP0

#define SS_CTL_MAX_PACKET_SIZE		0x200
#define SS_BULK_MAX_PACKET_SIZE		0x400
#define SS_INT_MAX_PACKET_SIZE		0x400
#define SS_ISO_MAX_PACKET_SIZE		0x400

#define HS_BULK_MAX_PACKET_SIZE		0x200
#define HS_CTL_MAX_PACKET_SIZE		0x40
#define HS_INT_MAX_PACKET_SIZE		0x400
#define HS_ISO_MAX_PACKET_SIZE		0x400

#define FUSB300_FIFO_ENTRY_NUM		4  //Take care the total fifo entry number
// Block toggle number define
#define FUSB300_FIFO_ENTRY_SINGLE		1
#define FUSB300_FIFO_ENTRY_DOUBLE		2
#define FUSB300_FIFO_ENTRY_TRIPLE		3

struct fusb300_request {

	struct usb_request	req;
	struct list_head	queue;
};

struct fusb300_irq_task {
	dma_addr_t		dma_addr;
	u32			transfer_length;
	struct fusb300_request	*req;
};

struct fusb300_ep {
	struct usb_ep		ep;
	struct fusb300		*fusb300;

	struct list_head	queue;
	unsigned		stall:1;
	unsigned		wedged:1;
	unsigned		use_dma:1;

	unsigned char		epnum;
	unsigned char		type;
	unsigned char		dir_in;
	unsigned char		fifo_entry_num;
	unsigned char		bw_num; /* for SS ISO ep and HS ISO/INT ep */
	struct fusb300_irq_task		*irq_current_task;
};

struct fusb300 {
	spinlock_t		lock;
	void __iomem		*reg;

	struct usb_gadget		gadget;
	struct usb_gadget_driver	*driver;

	struct fusb300_ep	*ep[FUSB300_MAX_NUM_EP];

	struct usb_request	*ep0_req;	/* for internal request */
	__le16			ep0_data;
	u32			ep0_length;	/* for internal request */

	u8		max_interval;
	u8		start_entry;	/* next start fifo entry */
	u16		addrofs;	/* next fifo address offset */
	u8		reenum;         /* if re-enumeration ep */
	u8		u1en, u2en;	/* keep the U1 and U2 enable state */
};

#define to_fusb300(g)		(container_of((g), struct fusb300, gadget))

#endif
