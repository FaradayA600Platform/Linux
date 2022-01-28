/*
 *  arch/arm/mach-ts711/include/mach/hardware.h
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

#ifndef __MACH_BOARD_TS711_H
#define __MACH_BOARD_TS711_H

#ifdef CONFIG_MACH_TS711

/*
 * Size of Memory Aperture
 */
#define PLAT_APB_MEMSIZE        SZ_64M
#define PLAT_AHB_MEMSIZE        SZ_32M
#define PLAT_FTSPI020_MEMSIZE   SZ_32M

/*
 * Base addresses
 */
/* Reserved Aperture */
#define PLAT_RSVD_0_BASE    0x00100000  // 480MB
#define PLAT_RSVD_1_BASE    0x44000000  // 192MB
#define PLAT_RSVD_2_BASE    0x60100000  // 1MB
#define PLAT_RSVD_3_BASE    0x60600000  // 531MB
#define PLAT_RSVD_4_BASE    0x88000000  // ~2GB
#define PLAT_RSVD_5_BASE    0xF4002000  // 996KB
#define PLAT_RSVD_6_BASE    0xF4100000  // ~2GB

#define PLAT_APB_VA_BASE            PLAT_RSVD_6_BASE
#define PLAT_AHB_VA_BASE            (PLAT_APB_VA_BASE + PLAT_APB_MEMSIZE)
#define PLAT_FTSPI020_VA_BASE       (PLAT_AHB_VA_BASE + PLAT_AHB_MEMSIZE)

 
#define PLAT_FTSPI020_PA_BASE           0x40000000
#define PLAT_FTSMC030_PA_BASE           0x50000000
#define PLAT_FTEMC030_GNSS_PA_BASE      0x60400000

/* APB Aperture */
#define PLAT_APB_BASE                   0x80000000

/* APB 0 Memory Aperture */
#define PLAT_FTX2P030_U0_PA_BASE        0x80000000
#define PLAT_FTTMR010_PA_BASE           0x80200000
#define PLAT_FTTMR010_1_PA_BASE         0x80300000
#define PLAT_SYSC_PA_BASE               0x80400000
#define PLAT_FTIIC010_PA_BASE           0x80500000
#define PLAT_GNSS_TOP_PA_BASE           0x80600000
#define PLAT_FTCSC010_ADCC_PA_BASE      0x80700000
#define PLAT_CAN_AMBA_APB_PA_BASE       0x80800000
#define PLAT_CAN_AMBA_APB_1_PA_BASE     0x80900000
#define PLAT_FTUART010_PA_BASE          0x80A00000
#define PLAT_FTUART010_1_PA_BASE        0x80B00000
#define PLAT_FTUART010_2_PA_BASE        0x80C00000
#define PLAT_FTUART010_3_PA_BASE        0x80D00000
#define PLAT_FTDMAC030_U1_PA_BASE       0x80E00000
#define PLAT_FTDMAC030_U0_PA_BASE       0x80F00000
#define PLAT_FTDMAC030_U0_1_PA_BASE     0x81000000
#define PLAT_FTGPIO010_PA_BASE          0x81100000
/* end of APB 0 Memory Aperture */

/* APB 1 Memory Aperture */
#define PLAT_FTX2P030_U1_PA_BASE            0x82000000
#define PLAT_FTSSP010_PA_BASE               0x82100000
#define PLAT_FTSSP010_1_PA_BASE             0x82200000
#define PLAT_FTSSP010_2_PA_BASE             0x82300000
#define PLAT_FTSSP010_3_PA_BASE             0x82400000
#define PLAT_FTSSP010_4_PA_BASE             0x82500000
#define PLAT_FTSSP010_5_PA_BASE             0x82600000
#define PLAT_INTERNAL_FTEMC030_1_PA_BASE    0x82800000
#define PLAT_GNSSRAM_FTEMC030_3_PA_BASE     0x82900000
#define PLAT_EXTMEM_FTSMC030_PA_BASE        0x82A00000
#define PLAT_FTDDR3030_PA_BASE              0x82B00000
#define PLAT_FTAXIC030_PA_BASE              0x82C00000
#define PLAT_FTX2X030_PA_BASE               0x82D00000
#define PLAT_FTX2X030_1_PA_BASE             0x82E00000
#define PLAT_FTX2H030_PA_BASE               0x82F00000
#define PLAT_FTH2X030_PA_BASE               0x83000000
/* end of APB 1 Memory Aperture */

#define PLAT_APB_PHY_TO_VIRT(phy)   (PLAT_APB_VA_BASE + (phy - PLAT_APB_BASE))

#define PLAT_TIMER_PA_BASE          PLAT_FTTMR010_PA_BASE
#define PLAT_TIMER_VA_BASE          PLAT_APB_PHY_TO_VIRT(PLAT_FTTMR010_PA_BASE)

#define PLAT_SCU_PA_BASE            PLAT_SYSC_PA_BASE
#define PLAT_SCU_VA_BASE            PLAT_APB_PHY_TO_VIRT(PLAT_SYSC_PA_BASE)

#define PLAT_IIC_PA_BASE            PLAT_FTIIC010_PA_BASE
#define PLAT_IIC_VA_BASE            PLAT_APB_PHY_TO_VIRT(PLAT_FTIIC010_PA_BASE)

#define PLAT_UART_PA_BASE		    PLAT_FTUART010_PA_BASE
#define PLAT_UART_VA_BASE		    PLAT_APB_PHY_TO_VIRT(PLAT_FTUART010_PA_BASE)
#define PLAT_UART_1_PA_BASE		    PLAT_FTUART010_1_PA_BASE
#define PLAT_UART_1_VA_BASE		    PLAT_APB_PHY_TO_VIRT(PLAT_FTUART010_1_PA_BASE)
#define DEBUG_LL_FTUART010_PA_BASE  PLAT_UART_PA_BASE
#define DEBUG_LL_FTUART010_VA_BASE  PLAT_UART_VA_BASE

#define PLAT_DMA_PA_BASE            PLAT_FTDMAC030_U0_PA_BASE
#define PLAT_DMA_VA_BASE            PLAT_APB_PHY_TO_VIRT(PLAT_FTDMAC030_U0_PA_BASE)

/* end of APB Aperture */

/* AHB Aperture */
#define PLAT_AHB_BASE                   0x84000000

#define PLAT_FTAHBC020S_PA_BASE         0x84000000
#define PLAT_FTSDC021_PA_BASE           0x84100000
#define PLAT_FTSDC021_1_PA_BASE         0x84200000
#define PLAT_FOTG210_PA_BASE            0x84300000
#define PLAT_FOTG210_1_PA_BASE          0x84400000
#define PLAT_FTGMAC100_S_PA_BASE        0x84500000

#define PLAT_AHB_PHY_TO_VIRT(phy)   (PLAT_AHB_VA_BASE + (phy - PLAT_AHB_BASE))

#define PLAT_SDC_PA_BASE            PLAT_FTSDC021_PA_BASE
#define PLAT_SDC_VA_BASE            PLAT_AHB_PHY_TO_VIRT(PLAT_FTSDC021_PA_BASE)

#define PLAT_OTG210_PA_BASE         PLAT_FOTG210_PA_BASE
#define PLAT_OTG210_VA_BASE         PLAT_AHB_PHY_TO_VIRT(PLAT_FOTG210_PA_BASE)
#define PLAT_OTG210_1_PA_BASE       PLAT_FOTG210_1_PA_BASE
#define PLAT_OTG210_1_VA_BASE       PLAT_AHB_PHY_TO_VIRT(PLAT_FOTG210_1_PA_BASE)
/* end of AHB Aperture */

#ifdef CONFIG_SMP
#define PLAT_SMP_MAGIC              0x12345678
#define PLAT_SMP_PA_BASE
#define PLAT_SMP_VA_BASE
#endif

#ifdef CONFIG_CPU_CA9
#define PLAT_CPU_PERIPH_PA_BASE     0xF4000000
#define PLAT_CPU_PERIPH_VA_BASE     0xF4000000

#define PLAT_GIC_CPU_PA_BASE        (PLAT_CPU_PERIPH_PA_BASE + 0x100)
#define PLAT_GIC_CPU_VA_BASE        (PLAT_CPU_PERIPH_VA_BASE + 0x100)
#define PLAT_GTIMER_PA_BASE         (PLAT_CPU_PERIPH_PA_BASE + 0x200)
#define PLAT_GTIMER_VA_BASE         (PLAT_CPU_PERIPH_VA_BASE + 0x200)
#define PLAT_TWD_PA_BASE            (PLAT_CPU_PERIPH_PA_BASE + 0x600)
#define PLAT_TWD_VA_BASE            (PLAT_CPU_PERIPH_VA_BASE + 0x600)
#define PLAT_GIC_DIST_PA_BASE       (PLAT_CPU_PERIPH_PA_BASE + 0x1000)
#define PLAT_GIC_DIST_VA_BASE       (PLAT_CPU_PERIPH_VA_BASE + 0x1000)
#endif

#ifdef CONFIG_CACHE_L2X0
#define PLAT_PL310_PA_BASE      0xF5000000
#define PLAT_PL310_VA_BASE      0xF9F00000
#endif

/*
 * The "Main CLK" Oscillator on the board which is used by the PLL to generate
 * CPU/AHB/APB clocks.
 */
#define MAIN_CLK							33000000

#endif	/* CONFIG_MACH_TS711 */

#endif	/* __MACH_BOARD_TS711_H */
