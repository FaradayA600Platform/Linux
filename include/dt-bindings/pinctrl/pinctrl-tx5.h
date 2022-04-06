/*
 * Provide constants for TX5 pinctrl bindings
 *
 * (C) Copyright 2021 Faraday Technology
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
#ifndef __DT_BINDINGS_PINCTRL_TX5_H__
#define __DT_BINDINGS_PINCTRL_TX5_H__

#define TX5_MUX_CAN_FD_2            0
#define TX5_MUX_DAC_TOP             1
#define TX5_MUX_DAC_TOP_1           2
#define TX5_MUX_DAC_TOP_2           3
#define TX5_MUX_DAC_TOP_3           4
#define TX5_MUX_FTGPIO010           5
#define TX5_MUX_FTIIC010_2          6
#define TX5_MUX_FTIIC010_3          7
#define TX5_MUX_FTPWMTMR010         8
#define TX5_MUX_FTSDC021            9
#define TX5_MUX_FTSDC021_1          10
#define TX5_MUX_FTSSP010_2          11
#define TX5_MUX_FTSSP010_3          12
#define TX5_MUX_FTUART010           13
#define TX5_MUX_FTUART010_1         14
#define TX5_MUX_FTUART010_2         15
#define TX5_MUX_FTUART010_3         16
#define TX5_MUX_FTUSART010          17
#define TX5_MUX_FTUSART010_1        18
#define TX5_MUX_GLUE_TOP            19
#define TX5_MUX_MOTOR_TOP           20
#define TX5_MUX_NA                  21
#define TX5_MUX_SBS_GMAC            22
#define TX5_MUX_SBS_GMAC_1          23
#define TX5_MUX_SEC_SUBSYS          24

#endif	/* __DT_BINDINGS_PINCTRL_TX5_H__ */
