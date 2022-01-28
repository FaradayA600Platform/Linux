/*
 *  arch/arm/mach-wideband001/platform_dt.c
 *
 *  Copyright (C) 2021 Faraday Technology
 *  Guo-Cheng Lee <gclee@faraday-tech.com>
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

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/i2c.h>
//#include <linux/platform_data/at24.h>
#ifdef CONFIG_CPU_HAS_GIC
#include <linux/irqchip/arm-gic.h>
#endif
#include <linux/mtd/partitions.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/serial_8250.h>
#include <linux/pinctrl/machine.h>
#include <linux/usb/phy.h>

#include <linux/platform_data/clk-faraday.h>

//#include <asm/clkdev.h>
#include <asm/setup.h>
#include <asm/smp_twd.h>
#include <asm/memory.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/irq.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>
#ifdef CONFIG_LOCAL_TIMERS
#include <asm/localtimer.h>
#endif
#ifdef CONFIG_CACHE_L2X0
#include <asm/hardware/cache-l2x0.h>
#endif

#include <mach/hardware.h>
//#include <mach/serdes_vsemi.h>

#include <plat/core.h>
#include <plat/serdes.h>
#include <plat/faraday.h>
#include <plat/ftintc030.h>

/* CPU Match ID for FTINTC030 */
#define EXTCPU_MATCH_ID	0x000

extern void __init global_timer_register(void);

struct of_dev_auxdata plat_auxdata_lookup[] __initdata = {
	OF_DEV_AUXDATA("of_serial",             PLAT_FTUART010_PA_BASE,    "serial0",           NULL),
	OF_DEV_AUXDATA("of_serial",             PLAT_FTUART010_1_PA_BASE,  "serial1",           NULL),
	OF_DEV_AUXDATA("faraday,ftgpio010",     PLAT_FTGPIO010_PA_BASE,    "ftgpio010",         NULL),
	OF_DEV_AUXDATA("faraday,ftspi020",      PLAT_FTSPI020_PA_BASE,     "ftspi020",          NULL),
	OF_DEV_AUXDATA("faraday,ftsdc021-sdhci",PLAT_FTSDC021_PA_BASE,     "ftsdc021",          NULL),	
	OF_DEV_AUXDATA("faraday,fotg210",       PLAT_FOTG210_PA_BASE,      "fotg210.0",         NULL),
	OF_DEV_AUXDATA("faraday,fotg210",       PLAT_FOTG210_1_PA_BASE,    "fotg210.1",         NULL),
	OF_DEV_AUXDATA("faraday,fti2c010",      PLAT_FTIIC010_PA_BASE,     "fti2c010",          NULL),
	{}
};

/*
 * I2C devices
 */
#ifdef CONFIG_I2C_BOARDINFO

static struct at24_platform_data at24c16 = {
	.byte_len  = SZ_16K ,
	.page_size = 16,
};

static struct i2c_board_info __initdata i2c_devices[] = {
	{
		I2C_BOARD_INFO("at24", 0x50),   /* eeprom */
		.platform_data = &at24c16,
	},
};
#endif  /* #ifdef CONFIG_I2C_BOARDINFO */

/******************************************************************************
 * platform dependent functions
 *****************************************************************************/

static struct map_desc plat_io_desc[] __initdata = {
	{
		.virtual    = PLAT_SCU_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_SCU_PA_BASE),
		.length     = SZ_8K,
		.type       = MT_DEVICE,
	},
#ifdef CONFIG_CPU_CA9
	{	/* GIC DIST */
		.virtual    = PLAT_GIC_DIST_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_GIC_DIST_PA_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{	/* CPU PERIPHERAL */
		.virtual    = PLAT_CPU_PERIPH_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_CPU_PERIPH_PA_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
#endif
#ifdef CONFIG_CACHE_L2X0
	{
		.virtual    = PLAT_PL310_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_PL310_PA_BASE),
		.length     = SZ_64K,
		.type       = MT_DEVICE,
	},
#endif
	{
		.virtual    = PLAT_UART_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_UART_PA_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_UART_1_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_UART_1_PA_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_TIMER_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_TIMER_PA_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
#ifdef CONFIG_SMP
	{
		.virtual    = PLAT_SMP_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_SMP_PA_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
#endif
	{
		.virtual    = PLAT_DMA_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_DMA_PA_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_SDC_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_SDC_PA_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_FTSPI020_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_FTSPI020_PA_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_IIC_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_IIC_PA_BASE),
		.length     = SZ_4K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_OTG210_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_OTG210_PA_BASE),
		.length     = SZ_64K,
		.type       = MT_DEVICE,
	},
	{
		.virtual    = PLAT_OTG210_1_VA_BASE,
		.pfn        = __phys_to_pfn(PLAT_OTG210_1_PA_BASE),
		.length     = SZ_64K,
		.type       = MT_DEVICE,
	},
};

static void __init platform_map_io(void)
{
	iotable_init((struct map_desc *)plat_io_desc, ARRAY_SIZE(plat_io_desc));
}

static void __init platform_init(void)
{
#ifdef CONFIG_I2C_BOARDINFO
	i2c_register_board_info(0, i2c_devices, ARRAY_SIZE(i2c_devices));
#endif

#ifdef CONFIG_SPI_FTSSP010
	spi_register_board_info(spi_devices, ARRAY_SIZE(spi_devices));
#endif

	of_platform_populate(NULL, of_default_bus_match_table, plat_auxdata_lookup, NULL);
}

static void platform_restart(enum reboot_mode rm, const char *cmd)
{
	void __iomem *base = (void __iomem *)(PLAT_CPU_PERIPH_VA_BASE + 0x600);

	/* reset watchdog */
	__raw_writel(0x12345678, base + 0x34);
	__raw_writel(0x87654321, base + 0x34);
	__raw_writel(0, base + 0x28);
	__raw_writel(1, base + 0x2c);
	__raw_writel(1, base + 0x30);

	/* activate watchdog reset */
	__raw_writel(1, base + 0x20);
	__raw_writel(9, base + 0x28);
}

static int __init platform_late_init(void)
{
#ifdef CONFIG_CACHE_L2X0
#if 1
	__raw_writel(0x1|(0x1<<4)|(0x1<<8), (void __iomem *)PLAT_PL310_VA_BASE+0x108); // tag RAM latency 1T l2_clk
	__raw_writel(0x2|(0x2<<4)|(0x2<<8), (void __iomem *)PLAT_PL310_VA_BASE+0x10c); // data RAM latency 2T l2_clk
#else
	__raw_writel(0x2|(0x2<<4)|(0x2<<8), (void __iomem *)PLAT_PL310_VA_BASE+0x108); // tag RAM latency 2T l2_clk
	__raw_writel(0x3|(0x3<<4)|(0x3<<8), (void __iomem *)PLAT_PL310_VA_BASE+0x10c); // data RAM latency 3T l2_clk
#endif
	/* 512KB (64KB/way), 8-way associativity, evmon/parity/share enabled
	 * Bits:  .... ...0 0111 0110 0000 .... .... .... */
	l2x0_init((void __iomem *)PLAT_PL310_VA_BASE, 0x30660000, 0xce000fff);
#endif
	return 0;
}
late_initcall(platform_late_init);

static const char *faraday_dt_match[] __initconst = {
	"arm,faraday-soc",
	NULL,
};

DT_MACHINE_START(FARADAY, "TS711")
	.atag_offset  = 0x100,
	.dt_compat    = faraday_dt_match,
	.smp          = smp_ops(faraday_smp_ops),
	.map_io       = platform_map_io,
	.init_machine = platform_init,
	.restart      = platform_restart,
MACHINE_END
