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

#ifndef __DT_BINDINGS_CLOCK_TX5_H__
#define __DT_BINDINGS_CLOCK_TX5_H__

#define FARADAY_NR_CLKS              196

/* Fixed Rate */
#define TX5_FIXED_25M                  1
#define TX5_FIXED_40M                  2
#define TX5_FIXED_48M                  3
#define TX5_FIXED_100M                 4
#define TX5_FIXED_32768                5

/* PLL */
#define TX5_PLL_CPU                   11
#define TX5_PLL_LVDS                  12
#define TX5_PLL_BUS                   13
#define TX5_PLL_GMAC                  14
#define TX5_PLL_DDR4                  15

/* Fixed Factor */
#define TX5_FIXED_48M_DIV4            21
#define TX5_FIXED_48M_DIV48           22
#define TX5_PLL_CPU_DIV2              23
#define TX5_PLL_CPU_DIV3              24
#define TX5_PLL_CPU_DIV6              25
#define TX5_PLL_BUS_DIV2              26
#define TX5_PLL_BUS_DIV4              27
#define TX5_PLL_BUS_DIV8              28
#define TX5_PLL_BUS_DIV16             29
#define TX5_PLL_BUS_DIV10             30
#define TX5_PLL_GMAC_DIV5             31
#define TX5_PLL_GMAC_DIV12            32
#define TX5_PLL_GMAC_DIV600           33
#define TX5_PLL_GMAC_DIV25            34
#define TX5_CPU_FCLK                  35
#define TX5_CPU_ACLK                  36
#define TX5_CPU_PCLK                  37
#define TX5_CPU_TRACECLK              38
#define TX5_GPU_CLK2X                 39
#define TX5_LVDS_PIXEL_CLK            40
#define TX5_AXI_CLK                   41
#define TX5_AHB_CLK                   42
#define TX5_APB_CLK                   43
#define TX5_SD0_SDCLK2X               44
#define TX5_SD0_SDCLK1X               45
#define TX5_SD1_SDCLK2X               46
#define TX5_SD1_SDCLK1X               47
#define TX5_SPICLK                    48
#define TX5_SSP0_SSPCLK               49
#define TX5_SSP1_SSPCLK               50
#define TX5_SSP2_SSPCLK               51
#define TX5_SSP3_SSPCLK               52
#define TX5_ADC_MCLK                  53
#define TX5_PWMTMR0_EXTCLK0           54
#define TX5_PWMTMR0_EXTCLK1           55
#define TX5_PWMTMR0_EXTCLK2           56
#define TX5_PWMTMR0_EXTCLK3           57
#define TX5_PWMTMR0_EXTCLK4           58
#define TX5_PWMTMR0_EXTCLK5           59
#define TX5_PWMTMR0_EXTCLK6           60
#define TX5_PWMTMR0_EXTCLK7           61
#define TX5_PWMTMR1_EXTCLK0           62
#define TX5_PWMTMR1_EXTCLK1           63
#define TX5_PWMTMR1_EXTCLK2           64
#define TX5_PWMTMR1_EXTCLK3           65
#define TX5_PWMTMR1_EXTCLK4           66
#define TX5_PWMTMR1_EXTCLK5           67
#define TX5_PWMTMR1_EXTCLK6           68
#define TX5_PWMTMR1_EXTCLK7           69
#define TX5_TMR_EXTCLK0               70
#define TX5_TMR_EXTCLK1               71
#define TX5_TMR_EXTCLK2               72
#define TX5_TMR_EXTCLK3               73
#define TX5_WDT0_EXTCLK               74
#define TX5_WDT1_EXTCLK               75
#define TX5_GMAC0_ACLK                76
#define TX5_GMAC0_CLK2P5M             77
#define TX5_GMAC0_CLK25M              78
#define TX5_GMAC0_CLK125M             79
#define TX5_GMAC0_PTP                 80
#define TX5_GMAC1_ACLK                81
#define TX5_GMAC1_CLK2P5M             82
#define TX5_GMAC1_CLK25M              83
#define TX5_GMAC1_CLK125M             84
#define TX5_GMAC1_PTP                 85
#define TX5_GPU_ACLK                  86
#define TX5_USB3_ACLK                 87
#define TX5_USB3_LPCLK                88
#define TX5_USB3_CLK60M               89
#define TX5_DDR_CLK                   90
#define TX5_UART0_UCLK                91
#define TX5_UART1_UCLK                92
#define TX5_UART2_UCLK                93
#define TX5_UART3_UCLK                94
#define TX5_USART0_UCLK               95
#define TX5_USART1_UCLK               96
#define TX5_TDC_XCLK                  97
#define TX5_PCIE_REFCLK               98

/* Divider */
#define TX5_CANFD_DEGLI_CLK_DIV      100
#define TX5_CANFD_EXT_CLK_DIV        101
#define TX5_LVDS_TV_CLK_DIV          102
#define TX5_LVDS_LC_SCALER_CLK_DIV   103
#define TX5_EXTCLK_DIV               104

/* MUX */
#define TX5_CPU_FCLK_MUX0            110
#define TX5_CPU_ACLK_MUX0            111
#define TX5_BUSCLK_MUX0              112
#define TX5_BUSCLK_MUX1              113
#define TX5_BUSCLK_MUX2              114

/* GATED */
#define TX5_GPU_HCLK_EN              120
#define TX5_CAN0_HCLK_EN             121
#define TX5_CAN1_HCLK_EN             122
#define TX5_CAN2_HCLK_EN             123
#define TX5_NIC_HCLK_EN              124
#define TX5_SD0_HCLK_EN              125
#define TX5_SD1_HCLK_EN              126
#define TX5_SEC_HCLK_EN              127
#define TX5_ADC_MCLK_EN              128
#define TX5_TDC_XCLK_EN              129
#define TX5_CAN0_OSC_EN              130
#define TX5_CAN1_OSC_EN              131
#define TX5_CAN2_OSC_EN              132
#define TX5_CANFD_EXT_CLK_EN         133
#define TX5_CANFD_DEGLI_CLK_EN       134
#define TX5_GMAC0_CLK125M_EN         135
#define TX5_GMAC0_CLK25M_EN          136
#define TX5_GMAC0_CLK2P5M_EN         137
#define TX5_GMAC0_PTP_EN             138
#define TX5_GMAC1_CLK125M_EN         139
#define TX5_GMAC1_CLK25M_EN          140
#define TX5_GMAC1_CLK2P5M_EN         141
#define TX5_GMAC1_PTP_EN             142
#define TX5_GPU_CLK2X_EN             143
#define TX5_GPU_ACLK_EN              144
#define TX5_LVDS_LC_CLK_EN           145
#define TX5_LVDS_TV_CLK_EN           146
#define TX5_LVDS_LC_SCALER_CLK_EN    147
#define TX5_DDR_CLK_EN               148
#define TX5_USB3_CLK60M_EN           149
#define TX5_USB3_LPCLK_EN            150
#define TX5_USB3_ACLK_EN             151
#define TX5_PWMTMR0_EXTCLK_EN        152
#define TX5_PWMTMR1_EXTCLK_EN        153
#define TX5_TMR_EXTCLKEN             154
#define TX5_WDT_EXTCLK_EN            155
#define TX5_SD0_SDCLK1X_EN           156
#define TX5_SD1_SDCLK1X_EN           157
#define TX5_SPICLK_EN                158
#define TX5_SEC_RSA_CLK_EN           159
#define TX5_SSP0_SSPCLK_EN           160
#define TX5_SSP1_SSPCLK_EN           161
#define TX5_SSP2_SSPCLK_EN           162
#define TX5_SSP3_SSPCLK_EN           163
#define TX5_UART0_UCLK_EN            164
#define TX5_UART1_UCLK_EN            165
#define TX5_UART2_UCLK_EN            166
#define TX5_UART3_UCLK_EN            167
#define TX5_USART0_UCLK_EN           168
#define TX5_USART1_UCLK_EN           169
#define TX5_PCIE_AUXCLK_EN           170
#define TX5_MOTOR_ENDAT_SYSCLK_EN    171
#define TX5_MOTOR_EPWM_SYSCLK_EN     172
#define TX5_MOTOR_EQEP_SYSCLK_EN     173
#define TX5_MOTOR_EXADC_SYS40MCLK_EN 174

#endif	/* __DT_BINDINGS_CLOCK_TX5_H__ */
