/*
 *  arch/arm/mach-faraday/include/mach/ftscu100.h
 *
 *  Faraday FTSCU100 System Control Unit
 *
 *  Copyright (C) 2021 Faraday Technology
 *  Copyright (C) 2021 Guo-Cheng Lee <gclee@faraday-tech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __FTSCU100_H
#define __FTSCU100_H

#define FTSCU100_OFFSET_BTUP_STS	0x000
#define FTSCU100_OFFSET_BTUP_CTRL	0x004
#define FTSCU100_OFFSET_CHIPID		0x010
#define FTSCU100_OFFSET_VERID	    0x014
#define FTSCU100_OFFSET_STRAP		0x018
#define FTSCU100_OFFSET_OSCCTL		0x01C
#define FTSCU100_OFFSET_PWR_MOD	    0x020
#define FTSCU100_OFFSET_INT_STS		0x024
#define FTSCU100_OFFSET_INT_EN		0x028
#define FTSCU100_OFFSET_PLLCTL		0x030
#define FTSCU100_OFFSET_PLL2_CTRL	0x040
#define FTSCU100_OFFSET_DLLCTL		0x044
#define FTSCU100_OFFSET_AHBCLKG		0x050
#define FTSCU100_OFFSET_APBCLKG		0x060
#define FTSCU100_OFFSET_EXTAPBPCLKG	0x064
#define FTSCU100_OFFSET_AXICLKG		0x080
#define FTSCU100_OFFSET_REBOOT_STS  0x124


/* Extendable registers: 0x300 ~ 0x37c */
#define FTSCU100_OFFSET_DCSR_0      0x300
#define FTSCU100_OFFSET_DCSR_1      0x304
#define FTSCU100_OFFSET_DCSR_2      0x308
#define FTSCU100_OFFSET_SW_RST      0x30c
#define FTSCU100_OFFSET_CLKEN_O0    0x320
#define FTSCU100_OFFSET_CLKEN_O1    0x324
#define FTSCU100_OFFSET_CLKENR      0x32C
#define FTSCU100_OFFSET_PINMUX_0    0x330
#define FTSCU100_OFFSET_USBCTL_0    0x340
#define FTSCU100_OFFSET_USBCTL_1    0x344
#define FTSCU100_OFFSET_DDR3CTL     0x348
#define FTSCU100_OFFSET_MISCS_0     0x34C
#define FTSCU100_OFFSET_CA9CTL_0    0x350
#define FTSCU100_OFFSET_CA9CTL_1    0x354
#define FTSCU100_OFFSET_CA9CTL_2    0x358
#define FTSCU100_OFFSET_CA9CTL_3    0x35C
#define FTSCU100_OFFSET_CA9CTL_4    0x360
#define FTSCU100_OFFSET_CA9CTL_5    0x364
#define FTSCU100_OFFSET_CA9CTL_6    0x368
#define FTSCU100_OFFSET_CA9CTL_7    0x36C
#define FTSCU100_OFFSET_CA9CTL_8    0x370
#define FTSCU100_OFFSET_CA9CTL_9    0x374
#define FTSCU100_OFFSET_CA9CTL_10   0x378
#define FTSCU100_OFFSET_CA9CTL_11   0x37C

#define FTSCU100_PLL0_NS(cr)	    ((readl(cr) >> 16) & 0x7)
#define FTSCU100_PLL0_PRE_NS(cr)	((readl(cr) >> 24) & 0xff)
#define FTSCU100_PLL1_NS(cr)	    ((readl(cr) >> 24) & 0x3f)

void ftscu100_init(void __iomem *base);
unsigned int ftscu100_readl(unsigned int offset);
void ftscu100_writel(unsigned int val, unsigned int offset);

#endif /* __FTSCU100_H */
