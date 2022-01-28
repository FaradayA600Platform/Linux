/*
 *  arch/arm/mach-ts711/include/mach/irqs.h
 *
 *  Copyright (C) 2015 Faraday Technology
 *  Copyright (C) 2017 Faraday Linux Automation Tool
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

#ifndef __MACH_IRQS_TS711_H
#define __MACH_IRQS_TS711_H

#ifdef CONFIG_MACH_TS711

/*
 * Interrupt numbers of Hierarchical Architecture
 */
/* GIC DIST in FMP626 */
#define IRQ_GLOBALTIMER	27
#define IRQ_LOCALTIMER		29
#define IRQ_LOCALWDOG		30
#define IRQ_LEGACY			31

#ifdef CONFIG_CPU_CA9
#define PLATFORM_START_IRQ	32	/* this is determined by irqs supported by GIC */
#else
#define PLATFORM_START_IRQ	0
#endif

/* SPI */
#define PLATFORM_CA9D554PFC0002HH0LG_IRQ		(PLATFORM_START_IRQ + 0)
#define PLATFORM_FTAXIC030_IRQ		(PLATFORM_START_IRQ + 1)
#define PLATFORM_FTX2P030_U0_IRQ		(PLATFORM_START_IRQ + 2)
#define PLATFORM_FTX2P030_U1_IRQ		(PLATFORM_START_IRQ + 3)
#define PLATFORM_FTDMAC030_U0_IRQ		(PLATFORM_START_IRQ + 4)
#define PLATFORM_FTDMAC030_U0_TC_IRQ		(PLATFORM_START_IRQ + 5)
#define PLATFORM_FTDMAC030_U0_ERR_IRQ		(PLATFORM_START_IRQ + 6)
#define PLATFORM_FTDMAC030_U0_TC7_IRQ		(PLATFORM_START_IRQ + 7)
#define PLATFORM_FTDMAC030_U0_TC6_IRQ		(PLATFORM_START_IRQ + 8)
#define PLATFORM_FTDMAC030_U0_TC5_IRQ		(PLATFORM_START_IRQ + 9)
#define PLATFORM_FTDMAC030_U0_TC4_IRQ		(PLATFORM_START_IRQ + 10)
#define PLATFORM_FTDMAC030_U0_TC3_IRQ		(PLATFORM_START_IRQ + 11)
#define PLATFORM_FTDMAC030_U0_TC2_IRQ		(PLATFORM_START_IRQ + 12)
#define PLATFORM_FTDMAC030_U0_TC1_IRQ		(PLATFORM_START_IRQ + 13)
#define PLATFORM_FTDMAC030_U0_TC0_IRQ		(PLATFORM_START_IRQ + 14)
#define PLATFORM_FTDMAC030_U0_1_IRQ		(PLATFORM_START_IRQ + 15)
#define PLATFORM_FTDMAC030_U0_1_TC_IRQ		(PLATFORM_START_IRQ + 16)
#define PLATFORM_FTDMAC030_U0_1_ERR_IRQ		(PLATFORM_START_IRQ + 17)
#define PLATFORM_FTDMAC030_U0_1_TC7_IRQ		(PLATFORM_START_IRQ + 18)
#define PLATFORM_FTDMAC030_U0_1_TC6_IRQ		(PLATFORM_START_IRQ + 19)
#define PLATFORM_FTDMAC030_U0_1_TC5_IRQ		(PLATFORM_START_IRQ + 20)
#define PLATFORM_FTDMAC030_U0_1_TC4_IRQ		(PLATFORM_START_IRQ + 21)
#define PLATFORM_FTDMAC030_U0_1_TC3_IRQ		(PLATFORM_START_IRQ + 22)
#define PLATFORM_FTDMAC030_U0_1_TC2_IRQ		(PLATFORM_START_IRQ + 23)
#define PLATFORM_FTDMAC030_U0_1_TC1_IRQ		(PLATFORM_START_IRQ + 24)
#define PLATFORM_FTDMAC030_U0_1_TC0_IRQ		(PLATFORM_START_IRQ + 25)
#define PLATFORM_FTDDR3030_IRQ		(PLATFORM_START_IRQ + 26)
#define PLATFORM_FTSPI020_IRQ		(PLATFORM_START_IRQ + 27)
#define PLATFORM_FTGPIO010_IRQ		(PLATFORM_START_IRQ + 28)
#define PLATFORM_FTIIC010_IRQ		(PLATFORM_START_IRQ + 29)
#define PLATFORM_FTCSC010_ADCC_IRQ		(PLATFORM_START_IRQ + 30)
#define PLATFORM_FTTMR010_IRQ		(PLATFORM_START_IRQ + 31)
#define PLATFORM_FTTMR010_T1_IRQ		(PLATFORM_START_IRQ + 32)
#define PLATFORM_FTTMR010_T2_IRQ		(PLATFORM_START_IRQ + 33)
#define PLATFORM_FTTMR010_T3_IRQ		(PLATFORM_START_IRQ + 34)
#define PLATFORM_FTTMR010_1_IRQ		(PLATFORM_START_IRQ + 35)
#define PLATFORM_FTTMR010_1_T1_IRQ		(PLATFORM_START_IRQ + 36)
#define PLATFORM_FTTMR010_1_T2_IRQ		(PLATFORM_START_IRQ + 37)
#define PLATFORM_FTTMR010_1_T3_IRQ		(PLATFORM_START_IRQ + 38)
#define PLATFORM_FOTG210_CHIP_IRQ		(PLATFORM_START_IRQ + 41)
#define PLATFORM_FOTG210_CHIP_1_IRQ		(PLATFORM_START_IRQ + 42)
#define PLATFORM_FTSDC021_IRQ		(PLATFORM_START_IRQ + 43)
#define PLATFORM_FTSDC021_1_IRQ		(PLATFORM_START_IRQ + 44)
#define PLATFORM_FTGMAC100_S_IRQ		(PLATFORM_START_IRQ + 45)
#define PLATFORM_FTAHBC020S_IRQ		(PLATFORM_START_IRQ + 46)
#define PLATFORM_FTH2X030_IRQ		(PLATFORM_START_IRQ + 47)
#define PLATFORM_GNSS_TOP_CPU_INT_IRQ		(PLATFORM_START_IRQ + 48)
#define PLATFORM_GNSS_TOP_CPU_INTN_IRQ		(PLATFORM_START_IRQ + 49)
#define PLATFORM_CAN_CORE_IRQ		(PLATFORM_START_IRQ + 50)
#define PLATFORM_CAN_CORE_1_IRQ		(PLATFORM_START_IRQ + 51)
#define PLATFORM_FTUART010_IRQ		(PLATFORM_START_IRQ + 53)
#define PLATFORM_FTUART010_1_IRQ		(PLATFORM_START_IRQ + 54)
#define PLATFORM_FTUART010_2_IRQ		(PLATFORM_START_IRQ + 55)
#define PLATFORM_FTUART010_3_IRQ		(PLATFORM_START_IRQ + 56)
#define PLATFORM_FTSSP010_IRQ		(PLATFORM_START_IRQ + 57)
#define PLATFORM_FTSSP010_1_IRQ		(PLATFORM_START_IRQ + 58)
#define PLATFORM_FTSSP010_2_IRQ		(PLATFORM_START_IRQ + 59)
#define PLATFORM_FTSSP010_3_IRQ		(PLATFORM_START_IRQ + 60)
#define PLATFORM_FTSSP010_4_IRQ		(PLATFORM_START_IRQ + 61)
#define PLATFORM_FTSSP010_5_IRQ		(PLATFORM_START_IRQ + 62)
#define PLATFORM_FTDMAC030_U1_IRQ		(PLATFORM_START_IRQ + 63)
#define PLATFORM_FTDMAC030_U1_TC_IRQ		(PLATFORM_START_IRQ + 64)
#define PLATFORM_FTDMAC030_U1_ERR_IRQ		(PLATFORM_START_IRQ + 65)
#define PLATFORM_FTDMAC030_U1_TC7_IRQ		(PLATFORM_START_IRQ + 66)
#define PLATFORM_FTDMAC030_U1_TC6_IRQ		(PLATFORM_START_IRQ + 67)
#define PLATFORM_FTDMAC030_U1_TC5_IRQ		(PLATFORM_START_IRQ + 68)
#define PLATFORM_FTDMAC030_U1_TC4_IRQ		(PLATFORM_START_IRQ + 69)
#define PLATFORM_FTDMAC030_U1_TC3_IRQ		(PLATFORM_START_IRQ + 70)
#define PLATFORM_FTDMAC030_U1_TC2_IRQ		(PLATFORM_START_IRQ + 71)
#define PLATFORM_FTDMAC030_U1_TC1_IRQ		(PLATFORM_START_IRQ + 72)
#define PLATFORM_FTDMAC030_U1_TC0_IRQ		(PLATFORM_START_IRQ + 73)

#define NR_IRQS						(160)

/*
 * Hardware handshake number for FTAPBB020
 */
#define FTAPBB020_APB_DMAHS_UART0TX  1
#define FTAPBB020_APB_DMAHS_UART0RX  1
#define FTAPBB020_APB_DMAHS_UART1TX  2
#define FTAPBB020_APB_DMAHS_UART1RX  2
#define FTAPBB020_APB_DMAHS_UART2TX  3
#define FTAPBB020_APB_DMAHS_UART2RX  3
#define FTAPBB020_APB_DMAHS_UART3TX  4
#define FTAPBB020_APB_DMAHS_UART3RX  5
#define FTAPBB020_APB_DMAHS_IRDA     6
#define FTAPBB020_APB_DMAHS_SSP0TX   7
#define FTAPBB020_APB_DMAHS_SSP0RX   8
#define FTAPBB020_APB_DMAHS_SSP1TX   9
#define FTAPBB020_APB_DMAHS_SSP1RX   10
#define FTAPBB020_APB_DMAHS_TSC      11
#define FTAPBB020_APB_DMAHS_TMR1     12
#define FTAPBB020_APB_DMAHS_TMR2     13
#define FTAPBB020_APB_DMAHS_TMR5     14

#endif	/* CONFIG_MACH_TS711 */

#endif	/* __MACH_IRQS_TS711_H */