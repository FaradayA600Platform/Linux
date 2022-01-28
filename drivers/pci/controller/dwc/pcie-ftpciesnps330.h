/* Faraday FTPCIESNPS330 PCIE Controller
 *
 * (C) Copyright 2018-2019 Faraday Technology
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

#ifndef __FTPCIESNPS330_H
#define __FTPCIESNPS330_H

#define SYS_CTRL_STATUS             0x0000
#define EM_SIGNAL                   0x0004
#define DMC_STATUS                  0x0008
#define INTR_STATUS                 0x0020
#define INTR_STATUS_MSI             (1 << 26)
#define INTR_STATUS_ERR_FATAL       (1 << 15)
#define INTR_STATUS_ERR_NONFATAL    (1 << 14)
#define INTR_STATUS_ERR_CORRECTABLE (1 << 13)
#define INTR_STATUS_ERR_SYS         (1 << 4)
#define INTR_STATUS_ERR_MSI         (1 << 3)
#define INTR_STATUS_ERR_INTERNAL    (1 << 2)
#define INTR_STATUS_QOVERFLOW_VC1   (1 << 1)
#define INTR_STATUS_QOVERFLOW_VC0   (1 << 0)
#define INTR_STATUS_ERR             (INTR_STATUS_ERR_FATAL | INTR_STATUS_ERR_NONFATAL | \
                                     INTR_STATUS_ERR_CORRECTABLE | INTR_STATUS_ERR_SYS | \
                                     INTR_STATUS_ERR_MSI | INTR_STATUS_ERR_INTERNAL | \
                                     INTR_STATUS_QOVERFLOW_VC1 | INTR_STATUS_QOVERFLOW_VC0)
#define INTR_MASK                   0x0024
#define MSG_NUM_ID                  0x0030
#define RADM_MSG_PAYLOAD0	        0x0034
#define RADM_MSG_PAYLOAD1	        0x0038
#define MSI_CTRL_IO                 0x003c
#define MSR_AWMISC_INFO0            0x0080
#define MSR_AWMISC_INFO1            0x0084
#define MSR_AWMISC_INFO_DMA         0x0088
#define MSR_ARMISC_INFO0            0x0090
#define MSR_ARMISC_INFO1            0x0094
#define MSR_ARMISC_INFO_DMA         0x0098
#define SLV_AWMISC_INFO             0x00a0
#define SLV_AWMISC_INFO_ATU_BYPASS  0x00a4
#define SLV_BMISC_INFO              0x00a8
#define SLV_ARMISC_INFO             0x00b0
#define SLV_ARMISC_INFO_ATU_BYPASS  0x00b4
#define SLV_RMISC_INFO              0x00b8
#define CFG_BAR0_START_LOWER        0x0100
#define CFG_BAR0_START_UPPER        0x0104
#define CFG_BAR1_START              0x0108
#define CFG_BAR0_LIMIT_LOWER        0x0110
#define CFG_BAR0_LIMIT_UPPER        0x0114
#define CFG_BAR1_LIMIT              0x0118
#define CFG_BAR2_START_LOWER        0x0120
#define CFG_BAR2_START_UPPER        0x0124
#define CFG_BAR3_START              0x0128
#define CFG_BAR2_LIMIT_LOWER        0x0130
#define CFG_BAR2_LIMIT_UPPER        0x0134
#define CFG_BAR3_LIMIT              0x0138
#define CFG_BAR4_START_LOWER        0x0140
#define CFG_BAR4_START_UPPER        0x0144
#define CFG_BAR5_START              0x0148
#define CFG_BAR4_LIMIT_LOWER        0x0150
#define CFG_BAR4_LIMIT_UPPER        0x0154
#define CFG_BAR5_LIMIT              0x0158
#define CFG_EXP_ROM_START           0x0160
#define CFG_EXP_ROM_LIMIT           0x0164
#define CFG_INFO                    0x0170
#define IBREQ_CPL_TMO_INFO0         0x0190
#define IBREQ_CPL_TMO_INFO1         0x0194
#define OBREQ_CPL_TMO_INFO0         0x0198
#define CXPL_DEBUG_INFO0            0x01a0
#define CXPL_DEBUG_INFO1            0x01a4
#define CXPL_DEBUG_INFO_EI          0x01ac
#define DIAG_CTRL_BUS               0x0200
#define DIAG_STATUS_BUS0            0x0204
#define DIAG_STATUS_BUS1            0x0208
#define DIAG_STATUS_BUS2            0x020c
#define DIAG_STATUS_BUS3            0x0210
#define DIAG_STATUS_BUS4            0x0214
#define DIAG_STATUS_BUS5            0x0218
#define DIAG_STATUS_BUS6            0x021c
#define DIAG_STATUS_BUS7            0x0220
#define DIAG_STATUS_BUS8            0x0224
#define DIAG_STATUS_BUS9            0x0228
#define DIAG_STATUS_BUS10           0x022c
#define DIAG_STATUS_BUS11           0x0230
#define DIAG_STATUS_BUS12           0x0234
#define DIAG_STATUS_BUS13           0x0238
#define DIAG_STATUS_BUS14           0x023c
#define DIAG_STATUS_BUS15           0x0240
#define MSTR_ARMISC                 0x0250
#define MSTR_AWMISC_INFO_HDR_34DW_0 0x0254
#define MSTR_AWMISC_INFO_HDR_34DW_1 0x0258
#define SLV_AWMISC_INFO_HDR_34DW_0  0x025c
#define SLV_AWMISC_INFO_HDR_34DW_1  0x0260
#define SIDEBAND                    0x0264
#define SLV_AWMISC_INFO_P_TAG       0x0268
#define DIAG_STATUS_BUS16           0x0280
#define DIAG_STATUS_BUS17           0x0284
#define DIAG_STATUS_BUS18           0x0288
#define DIAG_STATUS_BUS19           0x028c
#define DSP_AXI_MSI_INTR_VECTOR     0x03a0
#define CFG_MSI_PENDING             0x03a4
#define OTHER_SOURCE_SELECT         0x03a8
#define MISC1                       0x03ac
#define MSI_X_1                     0x03b0
#define MSI_X_2                     0x03b4
#define MSI_X_3                     0x03b8
#define MSI_X_4                     0x03bc
#define MSI_X_5                     0x03c0
#define MSI_X_6                     0x03c4
#define MISC2                       0x03c8
#define MISC3                       0x03cc
#define MISC4                       0x03d0
#define MISC5                       0x03d4

#endif