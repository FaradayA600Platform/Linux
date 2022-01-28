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

#ifndef __DT_BINDINGS_CLOCK_A600_H__
#define __DT_BINDINGS_CLOCK_A600_H__

#define FARADAY_NR_CLKS          196

/* Fixed Rate */
#define A600_FIXED_12M            1
#define A600_FIXED_12288K         2

/* PLL */
#define A600_PLL0                11
#define A600_PLL1                12
#define A600_PLL2                13
#define A600_PLL3                14

/* Fixed Factor */
#define A600_PLL3_DIV2           21
#define A600_PLL3_DIV4           22
#define A600_CA53_CLK            23
#define A600_CA53_ACLK           24
#define A600_CS_CLK              25
#define A600_AXI_CLK             26
#define A600_AXI_CLK_DIV2        27
#define A600_AHB_CLK             28
#define A600_APB_CLK             29
#define A600_SD0_SDCLK1X         30
#define A600_SD1_SDCLK1X         31
#define A600_ADC_CLK             32
#define A600_TDC_CLK             33
#define A600_WDT_EXTCLK          34
#define A600_LOWFREQ_CLK         35

/* Divider */
#define A600_TRACE_CLK           41
#define A600_GIC_ACLK            42
#define A600_LCD_SCACLK          43
#define A600_LCD_PIXCLK          44
#define A600_ADC_CLK_DIV         45
#define A600_GMAC0_CLK2P5M       46
#define A600_GMAC0_CLK25M        47
#define A600_GMAC0_CLK125M       48
#define A600_HB_CLK              49
#define A600_TDC_CLK_DIV         50
#define A600_GMAC1_CLK2P5M       51
#define A600_GMAC1_CLK25M        52
#define A600_GMAC1_CLK125M       53

/* MUX */
#define A600_PLL0_BYPASS_MUX     71
#define A600_PLL1_BYPASS_MUX     72
#define A600_DDR_CLK_SEL_MUX     73

/* GATED(BUS) */
#define A600_GMAC1_ACLK_EN       81
#define A600_GMAC0_ACLK_EN       82
#define A600_USB2_1_HCLK_EN      83
#define A600_USB2_0_HCLK_EN      84
#define A600_SD1_HCLK_EN         85
#define A600_SD0_HCLK_EN         86
#define A600_WDT1_PCLK_EN        87
#define A600_WDT0_PCLK_EN        88
#define A600_PCIE1_PCLK_EN       89
#define A600_PCIE0_PCLK_EN       90
#define A600_DMAC1_PCLK_EN       91
#define A600_DMAC0_PCLK_EN       92
#define A600_DDR_PCLK_EN         93
#define A600_I2C3_PCLK_EN        94
#define A600_I2C2_PCLK_EN        95
#define A600_I2C1_PCLK_EN        96
#define A600_I2C0_PCLK_EN        97
#define A600_UART3_PCLK_EN       97
#define A600_UART2_PCLK_EN       99
#define A600_UART1_PCLK_EN      100
#define A600_UART0_PCLK_EN      101
#define A600_PWM1_PCLK_EN       102
#define A600_SPI1_PCLK_EN       103
#define A600_SPI0_PCLK_EN       104
#define A600_I2S_PCLK_EN        105
#define A600_H265_PCLK_EN       106
#define A600_LCDC_PCLK_EN       107
#define A600_GMAC1_PCLK_EN      108
#define A600_GMAC0_PCLK_EN      109
#define A600_GPIO_PCLK_EN       110
#define A600_ADC_PCLK_EN        111
#define A600_TDC_PCLK_EN        112
#define A600_PWM0_PCLK_EN       113
#define A600_HB_ACLK_EN         114
#define A600_PCIE1_ACLK_EN      115
#define A600_PCIE0_ACLK_EN      116
#define A600_DDR_ACLK_EN        117
#define A600_H265_ACLK_EN       118
#define A600_ID_MAPPER_ACLK_EN  119
#define A600_LCDC_ACLK_EN       120
#define A600_DMAC1_ACLK_EN      121
#define A600_DMAC0_ACLK_EN      122
#define A600_TRACE_CK_EN        123
#define A600_GIC_CK_EN          124
#define A600_PWM1_CK8_EN        125
#define A600_PWM1_CK7_EN        126
#define A600_PWM1_CK6_EN        127
#define A600_PWM1_CK5_EN        128
#define A600_PWM1_CK4_EN        129
#define A600_PWM1_CK3_EN        130
#define A600_PWM1_CK2_EN        131
#define A600_PWM1_CK1_EN        132
#define A600_EFUSE_CK_EN        133
#define A600_SPICLK_CK_EN       134

/* GATED(IP) */
#define A600_UART3_CK_EN        141
#define A600_UART2_CK_EN        142
#define A600_UART1_CK_EN        143
#define A600_UART0_CK_EN        144
#define A600_PWM0_CK8_EN        145
#define A600_PWM0_CK7_EN        146
#define A600_PWM0_CK6_EN        147
#define A600_PWM0_CK5_EN        148
#define A600_PWM0_CK4_EN        149
#define A600_PWM0_CK3_EN        150
#define A600_PWM0_CK2_EN        151
#define A600_PWM0_CK1_EN        152
#define A600_SPI1_SSPCLK_CK_EN  153
#define A600_SPI0_SSPCLK_CK_EN  154
#define A600_I2S_SSPCLK_CK_EN   155
#define A600_H265_CK_EN         156
#define A600_LC_CK_EN           157
#define A600_LC_SCALER_CK_EN    158
#define A600_SD0_CK_EN          159
#define A600_SD1_CK_EN          160
#define A600_GMAC1_CK_EN        161
#define A600_GMAC0_CK_EN        162
#define A600_ADC_CK_EN          163
#define A600_TDC_CK_EN          164
#define A600_HB_CK_EN           165

#endif	/* __DT_BINDINGS_CLOCK_A600_H__ */
