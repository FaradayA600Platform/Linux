/*
 * Faraday Clock Framework
 *
 * (C) Copyright 2021-2022 Faraday Technology
 * Bo-Cun Chen <bcchen@faraday-tech.com>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __DT_BINDINGS_CLOCK_A500_H__
#define __DT_BINDINGS_CLOCK_A500_H__

#define FARADAY_NR_CLKS          128

/* Fixed Rate */
#define A500_FIXED_30M           1

/* PLL */
#define A500_PLL0                11
#define A500_PLL1                12
#define A500_PLL2                13
#define A500_PLL3                14

/* Fixed Factor */
#define A500_CA53_0_CLK          21
#define A500_CA53_1_CLK          22
#define A500_SDCLK1X             23
#define A500_USB3_CORECLK        24

/* Divider */
#define A500_CS_ACLK             31
#define A500_CA53_1_ACLK         32
#define A500_CA53_0_ACLK         33
#define A500_SYS_2_CLK           34
#define A500_SYS_1_CLK           35
#define A500_SYS_0_CLK           36
#define A500_PLL1_DIV            37
#define A500_PLL0_DIV            38
#define A500_X2P_PCLK            39
#define A500_H2P_PCLK            40
#define A500_SYS_HCLK            41
#define A500_PERI_1_CLK          42
#define A500_PERI_0_CLK          43
#define A500_AXIC_1_ACLK         44
#define A500_AXIC_0_ACLK         45
#define A500_TRACE_CLK           46
#define A500_GIC_ACLK            47
#define A500_GMAC0_CLK2P5M       48
#define A500_GMAC1_CLK25M        49
#define A500_GMAC0_CLK25M        50
#define A500_GMAC1_CLK125M       51
#define A500_GMAC0_CLK125M       52
#define A500_USB3_CLK60M         53
#define A500_USB3_LP_CLK         54
#define A500_USB3_CLK30M         55
#define A500_GMAC1_CLK2P5M       56
#define A500_TDC_XCLK            57
#define A500_EXT_AHB_CLK         58
#define A500_SPI_CLK             59
#define A500_ADC_MCLK            60
#define A500_SDC_CLK             61
#define A500_EAHB_ACLK           62
#define A500_PCIE_LP_CLK         63
#define A500_LCD_SCACLK          64
#define A500_LCD_PIXCLK          65

/* MUX */
#define A500_OTG_CK_SEL_MUX      71
#define A500_AXIC1_BYPASS_MUX    72
#define A500_AXIC1_MUX           73
#define A500_EAHB_MUX            74
#define A500_AXIC0_BYPASS_MUX    75
#define A500_AXIC0_MUX           76
#define A500_CS_GIC_MUX          77
#define A500_PCIE_LP_CLK_MUX     78
#define A500_SDC_MUX             79
#define A500_EXT_AHB_MUX         80
#define A500_SPI_MUX             81
#define A500_ADC_MUX             82
#define A500_USB3_MUX            83
#define A500_GMAC_MUX            84

/* GATED */
#define A500_PCIE1_ACLK_G        91
#define A500_PCIE0_ACLK_G        92
#define A500_USB3_ACLK_G         93
#define A500_GMAC1_ACLK_G        94
#define A500_GMAC0_ACLK_G        95
#define A500_LCDC_ACLK_G         96
#define A500_EAHB_ACLK_G         97
#define A500_DDR_ACLK_G          98
#define A500_DMAC_HCLK_G         99
#define A500_AES_HCLK_G         100
#define A500_SDC_HCLK_G         101
#define A500_EAHB_HCLK_G        102
#define A500_MISC_PCLK_G        103
#define A500_RSA_PCLK_G         104
#define A500_SHA_PCLK_G         105
#define A500_TRNG_PCLK_G        106
#define A500_ADC_PCLK_G         107
#define A500_GMAC1_PCLK_G       108
#define A500_GMAC0_PCLK_G       109
#define A500_LCDC_PCLK_G        110
#define A500_PCIE1_PCLK_G       111
#define A500_PCIE0_PCLK_G       112
#define A500_DDR_PCLK_G         113
#define A500_PCIE_LP_CLK_G      114
#define A500_LCD_SCACLK_G       115
#define A500_LCD_PIXCLK_G       116
#define A500_SDC_CLK_G          117
#define A500_TDC_XCLK_G         118
#define A500_EXT_AHB_CLK_G      119
#define A500_SPI_CLK_G          120
#define A500_ADC_MCLK_G         121
#define A500_USB3_G             122
#define A500_GMAC1_G            123
#define A500_GMAC0_G            124

#endif	/* __DT_BINDINGS_CLOCK_A500_H__ */
