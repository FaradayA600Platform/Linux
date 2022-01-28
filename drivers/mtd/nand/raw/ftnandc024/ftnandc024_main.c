// SPDX-License-Identifier: GPL-2.0
/*
 * FTNANDC024 NAND driver
 *
 * Copyright (C) 2019-2021 Faraday Technology Corp.
 */
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/rawnand.h>
#include <linux/mtd/partitions.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/ktime.h>
#include <linux/scatterlist.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/mtd/bbm.h>

#include "ftnandc024.h"

static int startchn = 0;
module_param(startchn, int, 0644);

static struct ftnandc024_nandchip_attr nand_attr[] = {
	/*
	 * Manufacturer ID, spare size, ECC bits, ECC base shift,
	 * ECC for spare, Block Boundary, Protect Spare, legacy flash
	 * */
	{"Micron 29F16G08MAA",
		218, 8, 9, 4, 128, 1, LEGACY_FLASH},	/* 4K MLC */
	{"Samsung K9F4G08U0A",
		64, 2, 9, 4, 64, 1, LEGACY_FLASH},	/* 2K SLC */
	{"Hynix HY27US08561A",
		16, 3, 9, 1, 32, 1, LEGACY_FLASH},	/* 512B SLC */
	{"Toshiba TH58NVG5D2ETA20",
		376, 24, 10, 4, 128, 1, LEGACY_FLASH},	/* 8K MLC */
	{"Toshiba TH58NVG7D2GTA20",
		640, 40, 10, 4, 256, 1, LEGACY_FLASH},	/* 8K MLC */
	{"Samsung K9HDGD8X5M",
		512, 24, 10, 4, 128, 1, TOGGLE1},	/* 8K TOGGLE MLC */
	{"Micron 29F32G08CBABB",
		224, 8, 9, 4, 256, 1, ONFI2},		/* 4K ONFI MLC */
	{"Samsung K9LBG08U0M",
		128, 4, 9, 4, 128, 1, LEGACY_FLASH},	/* 4K MLC */
	{"Toshiba TC58NVG4T2ETA00",
		376, 24, 10, 4, 256, 1, LEGACY_FLASH},	/* 8K TLC */
	{"Toshiba TC58NVG6DCJTA00",
		1280, 40, 10, 40, 256, 1, LEGACY_FLASH},/* 16K MLC */
	{"Samsung K9GCGY8S0A",
		640, 40, 10, 24, 128, 1, TOGGLE2},	/* 16K MLC */
	{"Toshiba TH58TEG7DCJBA4C",
		1280, 40, 10, 40, 256, 1, TOGGLE2},	/* 16K MLC */
	{"Samsung K9ABGD8U0B",
		1024, 60, 10, 60, 256, 1, TOGGLE1},	/* 8K TLC */
	{"Winbond W29N01GV", 64, 3, 9, 3, 64, 1, LEGACY_FLASH},
	{"Toshiba TH58TFT0DDLBA8H",
		1280, 40, 10, 40, 256, 1, TOGGLE2},	/* 16K Toggle2 */
	{"Micron 29F128G08CBECB",
		1872, 60, 10, 60, 512, 1, ONFI3},	/* 16K ONFI-3.2 MLC */
	{"Spansion S34ML01G100TF100", 
		64, 3, 9, 3, 64, 1, LEGACY_FLASH},	/* 2K SLC */
};

/* Note: The unit of tWPST/tRPST/tWPRE/tRPRE field of ftnandc024_chip_timing is ns.
 *
 * tWH, tCH, tCLH, tALH, tCALH, tWP, tREH, tCR, tRSTO, tREAID,
 * tREA, tRP, tWB, tRB, tWHR, tWHR2, tRHW, tRR, tAR, tRC
 * tADL, tRHZ, tCCS, tCS, tCS2, tCLS, tCLR, tALS, tCALS, tCAL2, tCRES, tCDQSS, tDBS, tCWAW, tWPRE,
 * tRPRE, tWPST, tRPST, tWPSTH, tRPSTH, tDQSHZ, tDQSCK, tCAD, tDSL
 * tDSH, tDQSL, tDQSH, tDQSD, tCKWR, tWRCK, tCK, tCALS2, tDQSRE, tWPRE2, tRPRE2, tCEH
 */
#if defined (CONFIG_FTNANDC024_MICRON_29F16G08MAA)
static struct ftnandc024_chip_timing chip_timing = {
	7, 5, 5, 5, 0, 10, 10, 0, 0, 0,
	20, 12, 100, 0, 60, 0, 100, 20, 10, 0,
	70, 100, 70, 15, 0, 10, 10, 10, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif defined (CONFIG_FTNANDC024_TOSHIBA_TC58NVG4T2ETA00)
static struct ftnandc024_chip_timing chip_timing = {
	10, 5, 5, 5, 0, 12, 10, 10, 0, 0,
	20, 12, 100, 0, 200, 0, 30, 20, 10, 0,
	0, 60, 0, 8, 0, 30, 10, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif defined (CONFIG_FTNANDC024_SAMSUNG_K9F4G08U0A)
static struct ftnandc024_chip_timing chip_timing = {
	10, 5, 5, 5, 0, 12, 10, 0, 0, 0,
	20, 12, 100, 0, 60, 0, 100, 20, 10, 0,
	70, 100, 0, 20, 0, 12, 10, 12, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif defined (CONFIG_FTNANDC024_HYNIX_HY27US08561A)
static struct ftnandc024_chip_timing chip_timing = {
	15, 10, 10, 10, 0, 25, 15, 0, 0, 0,
	30, 25, 100, 100, 60, 0, 0, 20, 10, 0,
	0, 30, 0, 0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 100};
#elif defined (CONFIG_FTNANDC024_TOSHIBA_TH58NVG5D2ETA20)
static struct ftnandc024_chip_timing chip_timing = {
	10, 5, 5, 5, 0, 12, 10, 10, 0, 0,
	20, 12, 100, 0, 180, 0, 30, 20, 10, 0,
	0, 60, 0, 8, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif defined (CONFIG_FTNANDC024_TOSHIBA_TH58NVG7D2GTA20)
static struct ftnandc024_chip_timing chip_timing = {
	10, 5, 5, 5, 0, 12, 10, 10, 0, 0,
	20, 12, 100, 0, 200, 0, 30, 20, 10, 0,
	0, 60, 0, 8, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif defined (CONFIG_FTNANDC024_SAMSUNG_K9HDGD8X5M)
static struct ftnandc024_chip_timing chip_timing = {
	11, 5, 0, 0, 5, 11, 7, 10, 0, 0,
	0, 7, 100, 0, 120, 300, 0, 20, 10, 0,
	300, 0, 0, 20, 0, 0, 10, 0, 15, 0, 0, 100, 0, 300, 15,
	15, 7, 33, 5, 5, 0, 0, 0, 0,
	0, 7, 7, 0, 0, 0, 0, 0, 25, 0, 0, 0};
#elif defined (CONFIG_FTNANDC024_MICRON_29F32G08CBABB)
static struct ftnandc024_chip_timing chip_timing = {
	10, 5, 5, 5, 0, 15, 10, 0, 0, 0,
	20, 15, 100, 0, 60, 0, 100, 20, 10, 0,
	100, 100, 200, 25, 0, 10, 10, 10, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static struct ftnandc024_chip_timing sync_timing = {
	0, 5, 0, 0, 5, 0, 0, 0, 0, 0,
	0, 0, 100, 0, 60, 0, 100, 20, 0, 0,
	100, 0, 200, 25, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 20, 20, 25, 0,
	0, 0, 0, 20, 0, 20, 0, 0, 0, 0, 0, 0};
#elif defined (CONFIG_FTNANDC024_MICRON_29F128G08CBECB)
// Asyn. Mode 3, I/F Clock ~=33MHz.
static struct ftnandc024_chip_timing chip_timing = {
	10, 5, 5, 5, 0, 15, 10, 0, 0, 0,
	20, 15, 100, 0, 60, 0, 100, 20, 10, 0,
	100, 100, 0, 25, 0, 10, 10, 10, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20
};

// NV-DDR Mode 1, I/F Clock ~=33MHz
static struct ftnandc024_chip_timing sync_timing = {
	0, 5, 0, 0, 5, 0, 0, 0, 0, 0,
	0, 0, 100, 0, 80, 0, 100, 20, 0, 0,
	100, 0, 0, 25, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 20, 20, 25, 0,
	0, 0, 0, 18, 0, 20, 0, 0, 0, 0, 0, 20
};

// NV-DDR2 Mode 0, I/F Clock ~=33MHz
static struct ftnandc024_chip_timing ddr2_timing = {
	11, 5, 0, 0, 5, 11, 0, 10, 0, 0,
	0, 0, 100, 0, 80, 0, 100, 20, 10, 0,
	100, 0, 300, 20, 30, 0, 10, 0, 15, 0, 0, 30, 0, 0, 15,
	15, 7, 0, 15, 15, 0, 0, 0, 0,
	0, 0, 0, 18, 0, 0, 0, 25, 25, 25, 25, 20
};
#elif defined (CONFIG_FTNANDC024_SAMSUNG_K9LBG08U0M)
static struct ftnandc024_chip_timing chip_timing = {
	10, 5, 5, 5, 0, 12, 10, 20, 0, 0,
	20, 12, 100, 0, 60, 0, 100, 20, 10, 0,
	100, 100, 0, 20, 0, 12, 10, 12, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif defined (CONFIG_FTNANDC024_TOSHIBA_TC58NVG6DCJTA00)
static struct ftnandc024_chip_timing chip_timing = {
	11, 5, 5, 5, 0, 11, 7, 10, 0, 0,
	20, 10, 100, 0, 180, 300, 30, 20, 10, 0,
	300, 60, 0, 15, 0, 10, 10, 10, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif defined (CONFIG_FTNANDC024_SAMSUNG_K9GCGY8S0A)
static struct ftnandc024_chip_timing chip_timing = {
	11, 5, 0, 0, 5, 11, 0, 10, 0, 0,
	0, 0, 100, 0, 120, 300, 0, 20, 10, 0,
	300, 0, 0, 25, 0, 0, 10, 0, 15, 0, 10, 100, 0, 300, 15,
	15, 7, 0, 25, 25, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 25, 0, 0, 0};
#elif defined (CONFIG_FTNANDC024_SAMSUNG_K9ABGD8U0B)
static struct ftnandc024_chip_timing chip_timing = {
	11, 5, 0, 0, 5, 11, 7, 0, 0, 0,
	0, 0, 100, 0, 0, 0, 0, 20, 10, 0,
	300, 0, 0, 0, 0, 0, 0, 0, 15, 0, 100, 0, 0, 300, 15,
	15, 7, 0, 0, 25, 0, 0, 0, 0,
	0, 7, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
#elif defined (CONFIG_FTNANDC024_TOSHIBA_TH58TEG7DCJBA4C)
/* Note: The unit of tWPST/tRPST/tWPRE/tRPRE field of ftnandc024_chip_timing is ns.
 *
 * tWH, tCH, tCLH, tALH, tCALH, tWP, tREH, tCR, tRSTO, tREAID,
 * tREA, tRP, tWB, tRB, tWHR, tWHR2, tRHW, tRR, tAR, tRC
 * tADL, tRHZ, tCCS, tCS, tCS2, tCLS, tCLR, tALS, tCALS, tCAL2, tCRES, tCDQSS, tDBS, tCWAW, tWPRE,
 * tRPRE, tWPST, tRPST, tWPSTH, tRPSTH, tDQSHZ, tDQSCK, tCAD, tDSL
 * tDSH, tDQSL, tDQSH, tDQSD, tCKWR, tWRCK, tCK, tCALS2, tDQSRE, tWPRE2, tRPRE2, tCEH
 */
static struct ftnandc024_chip_timing chip_timing = {
	11, 5, 0, 0, 5, 11, 0, 10, 0, 0,
	0, 0, 100, 0, 120, 300, 0, 20, 10, 0,
	300, 0, 0, 20, 30, 0, 10, 0, 15, 0, 10, 100, 0, 300, 15,
	15, 7, 0, 25, 25, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 25, 25, 25, 25, 0};
#elif defined (CONFIG_FTNANDC024_TOSHIBA_TH58TFT0DDLBA8H)
static struct ftnandc024_chip_timing chip_timing = {
	11, 5, 0, 0, 5, 11, 0, 10, 0, 0,
	0, 0, 100, 0, 120, 300, 0, 5, 10, 0,
	300, 0, 0, 20, 30, 0, 10, 0, 15, 0, 10, 100, 0, 300, 15,
	15, 7, 0, 25, 25, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 25, 25, 25, 25, 0};
#elif defined (CONFIG_FTNANDC024_WINBOND_W29N01GV)
 static struct ftnandc024_chip_timing chip_timing = {
	10, 5, 5, 5, 0, 12, 10, 0, 0, 0,
	0, 12, 100, 0, 60, 0, 100, 20, 10, 25,
	70, 100, 0, 15, 0, 10, 10, 10, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif defined (CONFIG_FTNANDC024_SPANSION_S34ML01G100TF100)
 static struct ftnandc024_chip_timing chip_timing = {
	10, 5, 5, 5, 0, 12, 10, 10, 0, 0,
	20, 12, 100, 0, 60, 0, 100, 20, 10, 25,
	70, 100, 0, 20, 0, 10, 10, 10, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#else
static struct ftnandc024_chip_timing chip_timing = {
	10, 5, 5, 5, 0, 12, 10, 20, 0, 0,
	20, 12, 100, 0, 60, 0, 100, 20, 10, 0,
	100, 100, 0, 20, 0, 12, 10, 12, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#endif

#if defined (CONFIG_FTNANDC024_MICRON_29F16G08MAA) ||\
	defined (CONFIG_FTNANDC024_SAMSUNG_K9HDGD8X5M) ||\
	defined (CONFIG_FTNANDC024_MICRON_29F32G08CBABB) ||\
	defined (CONFIG_FTNANDC024_SAMSUNG_K9LBG08U0M) ||\
	defined (CONFIG_FTNANDC024_TOSHIBA_TH58NVG7D2GTA20)
static struct mtd_partition ftnandc024_partition_info[] = {
	{
	 .name = "DiskOnChip",
	 .offset = 128 * 1024 * 1024,
	 .size = 1536 * 1024 * 1024},
	{
	 .name = "Partition 1",
	 .offset = MTDPART_OFS_APPEND,
	 .size = 1536 * 1024 * 1024},
};
#elif defined (CONFIG_FTNANDC024_TOSHIBA_TC58NVG6DCJTA00) ||\
	defined (CONFIG_FTNANDC024_SAMSUNG_K9GCGY8S0A) ||\
	defined (CONFIG_FTNANDC024_TOSHIBA_TH58TEG7DCJBA4C) ||\
	defined (CONFIG_FTNANDC024_TOSHIBA_TH58TFT0DDLBA8H) ||\
	defined (CONFIG_FTNANDC024_MICRON_29F128G08CBECB)
static struct mtd_partition ftnandc024_partition_info[] = {
	{
	 .name = "Partition 1",
	 .offset = 128 * 1024 * 1024,
	 .size =  (uint64_t)((uint64_t)3584 * (uint64_t)1024 * (uint64_t)1024)},
	{
	 .name = "Partition 2",
	 .offset = MTDPART_OFS_APPEND,
	 .size =  (uint64_t)((uint64_t)3584 * (uint64_t)1024 * (uint64_t)1024)},
};
#elif defined (CONFIG_FTNANDC024_SAMSUNG_K9ABGD8U0B)
static struct mtd_partition ftnandc024_partition_info[] = {
	{
	 .name = "Partition 0",
	 .offset = 3 * 1024 * 1024,
	 .size = (long long)((long long)4089 *(long long)1024 * (long long)1024)},
};
#elif defined (CONFIG_FTNANDC024_TOSHIBA_TC58NVG4T2ETA00)
static struct mtd_partition ftnandc024_partition_info[] = {
	{
	 .name = "Partition 0",
	 .offset = 3 * 1024 * 1024,
	 .size = 2040 * 1024 * 1024},
};
#elif defined (CONFIG_FTNANDC024_TOSHIBA_TH58NVG5D2ETA20)
static struct mtd_partition ftnandc024_partition_info[] = {
	{
	 .name = "DiskOnChip",
	 .offset = 16 * 1024 * 1024,
	 .size = 896 * 1024 * 1024},
	{
	 .name = "Partition 1",
	 .offset = MTDPART_OFS_APPEND,
	 .size = 896 * 1024 * 1024},
};
#elif defined (CONFIG_FTNANDC024_HYNIX_HY27US08561A)
static struct mtd_partition ftnandc024_partition_info[] = {
	{
	 .name = "DiskOnChip",
	 .offset = 0,
	 .size = 16 * 1024 * 1024},
	{
	 .name = "Partition 1",
	 .offset = MTDPART_OFS_APPEND,
	 .size = 16 * 1024 * 1024},
};
#elif defined (CONFIG_FTNANDC024_SAMSUNG_K9F4G08U0A)
static struct mtd_partition ftnandc024_partition_info[] = {
	{
	 .name = "DiskOnChip",
	 .offset = 16 * 1024 * 1024,
	 .size = 192 * 1024 * 1024},
	{
	 .name = "Partition 1",
	 .offset = MTDPART_OFS_APPEND,
	 .size = 192 * 1024 * 1024},
};
#else
static struct mtd_partition ftnandc024_partition_info[] = {
	{
	 .name = "DiskOnChip",
	 .offset = 16 * 1024 * 1024,
	 .size = 64 * 1024 * 1024},
	{
	 .name = "Partition 1",
	 .offset = MTDPART_OFS_APPEND,
	 .size = 32 * 1024 * 1024},
};
#endif


static int ftnandc024_ooblayout_ecc(struct mtd_info *mtd, int section,
				struct mtd_oob_region *oobregion)
{
	struct nand_chip *chip = mtd_to_nand(mtd);

	if (section >= chip->ecc.steps)
		return -ERANGE;

	oobregion->offset = 0;
	oobregion->length = 1;

	return 0;
}

static int ftnandc024_ooblayout_free(struct mtd_info *mtd, int section,
				struct mtd_oob_region *oobregion)
{
	struct nand_chip *chip = mtd_to_nand(mtd);

	if (section >= chip->ecc.steps)
		return -ERANGE;

	oobregion->offset = 0;
	oobregion->length = mtd->oobsize;

	return 0;
}

static const struct mtd_ooblayout_ops ftnandc024_ooblayout_ops = {
	.ecc = ftnandc024_ooblayout_ecc,
	.free = ftnandc024_ooblayout_free,
};

static uint8_t ftnandc024_bbt_pattern[] = { 'B', 'b', 't', '0' };
static uint8_t ftnandc024_mirror_pattern[] = { '1', 't', 'b', 'B' };

static struct nand_bbt_descr ftnandc024_bbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
	    | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 0,
	.len = 4,
	.veroffs = 4,
	.maxblocks = 4,
	.pattern = ftnandc024_mirror_pattern
};

static struct nand_bbt_descr ftnandc024_bbt_main_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
	    | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 0,
	.len = 4,
	.veroffs = 4,
	.maxblocks = 4,
	.pattern = ftnandc024_bbt_pattern
};

static uint8_t ftnandc024_scan_ff_pattern[] = { 0xff, 0xff, 0xff, 0xff };

static struct nand_bbt_descr ftnandc024_largepage_flashbased = {
	.offs = 0,
	.len = 4,
	.pattern = ftnandc024_scan_ff_pattern
};

static void ftnandc024_set_warmup_cycle(struct nand_chip *chip,
			u8 wr_cyc, u8 rd_cyc)
{
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	int val;

	val = readl(data->io_base + MEM_ATTR_SET2);
	val &= ~(0xFF);
	val |= (((rd_cyc & 0x3) << 4) | (wr_cyc & 0x3));
	writel(val, data->io_base + MEM_ATTR_SET2);
}

/* The unit of Hclk is MHz, and the unit of Time is ns.
 * We desire to calculate N to satisfy N*(1/Hclk) > Time given Hclk and Time
 * ==> N > Time * Hclk
 * ==> N > Time * 10e(-9) * Hclk *10e(6)        --> take the order out
 * ==> N > Time * Hclk * 10e(-3)
 * ==> N > Time * Hclk / 1000
 * ==> N = (Time * Hclk + 999) / 1000
 */
static void ftnandc024_calc_timing(struct nand_chip *chip)
{
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	int tWH, tWP, tREH, tRES, tBSY, tBUF1;
	int tBUF2, tBUF3, tBUF4, tPRE, tRLAT, t1;
	int tPST, tPSTH, tWRCK;
	int i, toggle_offset = 0;
	struct ftnandc024_chip_timing *p;
	u32 CLK, FtCK, timing[4];

	CLK = FREQ_SETTING / 1000000;

	tWH = tWP = tREH = tRES =  0;
	tRLAT = tBSY = t1 = 0;
	tBUF4 = tBUF3 = tBUF2 = tBUF1 = 0;
	tPRE = tPST = tPSTH = tWRCK = 0;
#if defined (CONFIG_FTNANDC024_MICRON_29F32G08CBABB)
	if (data->flash_type == ONFI2)
		p = &sync_timing;
	else
#elif defined (CONFIG_FTNANDC024_MICRON_29F128G08CBECB)
	if (data->flash_type == ONFI3)
		p = &ddr2_timing;
	else if (data->flash_type == ONFI2)
		p = &sync_timing;
	else
#endif
		p = &chip_timing;

	if(data->flash_type == LEGACY_FLASH) {
		// tWH = max(tWH, tCH, tCLH, tALH)
		tWH = max_4(p->tWH, p->tCH, (int)p->tCLH, (int)p->tALH);
		tWH = (tWH * CLK) / 1000;
		// tWP = tWP
		tWP = (p->tWP * CLK) / 1000;
		// tREH = tREH
		tREH = (p->tREH * CLK) / 1000;
		// tRES = max(tREA, tRSTO, tREAID)
		tRES = max_3(p->tREA, p->tRSTO, (int)p->tREAID);
		tRES = (tRES * CLK) / 1000;
		// tRLAT < (tRES + tREH) + 2
		tRLAT = tRES + tREH;
		// t1 = max(tCS, tCLS, tALS) - tWP
		t1 = max_3(p->tCS, p->tCLS, (int)p->tALS) - p->tWP;
		if (t1 < 0)
			t1 = 0;
		else
			t1 = (t1 * CLK) / 1000;
		// tPSTH(EBI setup time) = max(tCS, tCLS, tALS)
		tPSTH = max_3(p->tCS, p->tCLS, (int)p->tALS);
		tPSTH = (tPSTH * CLK) / 1000;
		// tWRCK(EBI hold time) = max(tRHZ, tREH)
		tWRCK = max_2(p->tRHZ, p->tREH);
		tWRCK = (tWRCK * CLK) / 1000;
	}

	else if(data->flash_type == ONFI2) {
		// tWP = tCAD
		tWP = (p->tCAD * CLK) / 1000;

		// Fill this field with value N, FTck = mem_clk/2(N + 1)
		// Note:mem_clk is same as core_clk. Here, we'd like to
		// assign 30MHz to FTck.
		tRES = 0;
		FtCK = CLK / ( 2 * (tRES + 1));

		// Increase p->tCK by one, is for the fraction which
		// cannot store in the variable, Integer type.
		p->tCK = 1000 / FtCK + 1;
		#if 0
		// The vfp calculation isn't supported in kernel code.
		p->tWPRE = 1.5 * p->tCK;
		p->tWPST = 1.5 * p->tCK;
		p->tDQSL = 0.6 * p->tCK;
		p->tDQSH = 0.6 * p->tCK;
		#else
		p->tWPRE = 2 * p->tCK;
		p->tWPST = 2 * p->tCK;
		p->tDQSL = 1 * p->tCK;
		p->tDQSH = 1 * p->tCK;
		#endif

		p->tCKWR = (p->tDQSCK + p->tCK) / p->tCK;
		if(p->tDQSCK % p->tCK !=0)
			p->tCKWR += 1;

		t1 = (p->tCS * CLK) / 1000;

		tPRE = 2;	// Assign 2 due to p->tWPRE is 1.5*p->tCK
		tPST = 2;	// Assign 2 due to p->tWPST is 1.5*p->tCK
		tPSTH = ((p->tDQSHZ * FtCK) / 1000) + 1;
		tWRCK = (p->tWRCK * CLK) /1000;
	}

	else if(data->flash_type == ONFI3) {
		p->tRC = 1000 / CLK;
		p->tRPST = p->tDQSRE + (p->tRC >> 1);
		p->tRP = p->tREH = p->tDQSH = p->tDQSL = (p->tRC >> 1);

		tWH = max_2(p->tCH, p->tCALH);
		tWH = (tWH * CLK) / 1000;

		tWP = (p->tWP * CLK) / 1000;

		tRES = max_2(p->tRP, p->tREH);
		tRES = (tRES * CLK) / 1000;

		t1 = max_2((p->tCALS2 - p->tWP), p->tCWAW);
		t1 = (t1 * CLK) / 1000;

		tPRE = max_2(max_3(p->tWPRE, p->tWPRE2, p->tRPRE),
						max_3(p->tRPRE2, p->tCS, p->tCS2));
		tPRE = (tPRE * CLK) / 1000;
		tPRE+= 1;

		tPST = max_4(p->tWPST, p->tRPST, p->tCH, p->tCALH);
		tPST = (tPST * CLK) / 1000;
		tPST+= 1;

		tPSTH = max_3(p->tWPSTH, p->tRPSTH, p->tCEH);
		tPSTH = (tPSTH * CLK) / 1000;

		tWRCK = max_4(p->tDSL, p->tDSH, p->tDQSL, p->tDQSH);
		tWRCK = (tWRCK * CLK) / 1000;
	}

	else if(data->flash_type == TOGGLE1) {
		// tWH = max(tWH, tCH, tCALH)
		tWH = max_3(p->tWH, p->tCH, (int)p->tCALH);
		tWH = (tWH * CLK) / 1000;
		// tWP = tWP
		tWP = (p->tWP * CLK) / 1000;
		// tREH = tCR
		tREH = (p->tCR * CLK) / 1000;
		// tRES = max(tRP, tREH)
		tRES = max_2(p->tRP, p->tREH);
		tRES = (tRES * CLK) / 1000;
		// t1 = max(tCALS2-tWP, tCWAW)
		t1 = max_2((p->tCALS2 - p->tWP), p->tCWAW);
		t1 = (t1 * CLK) / 1000;
		// tPRE = max(tWPRE, 2*tRPRE, tRPRE+tDQSRE) + 1
		tPRE = max_3((int)p->tWPRE, (int)(p->tRPRE << 1),\
					 (p->tRPRE + p->tDQSRE));
		tPRE = (tPRE * CLK) / 1000;
		tPRE +=1;
		// tPST = max(tWPST, tRPST) + 1
		tPST = max_2(p->tWPST, p->tRPST);
		tPST = (tPST * CLK) / 1000;
		tPST +=1;
		// tPSTH = max(tWPSTH, tRPSTH)
		tPSTH = max_2(p->tWPSTH, p->tRPSTH);
		tPSTH = (tPSTH * CLK) / 1000;
		// tWRCK = max(tDSL, tDSH, tDQSL, tDQSH)
		tWRCK = max_4(p->tDSL, p->tDSH, (int)p->tDQSL, (int)p->tDQSH);
		tWRCK = (tWRCK * CLK) / 1000;
	}

	else if(data->flash_type == TOGGLE2) {
		p->tRC = 1000 / CLK;
		p->tRPST = p->tDQSRE + (p->tRC >> 1);
		p->tRP = p->tREH = p->tDQSH = p->tDQSL = (p->tRC >> 1);

		tWH = max_2(p->tCH, p->tCALH);
		tWH = (tWH * CLK) / 1000;

		tWP = (p->tWP * CLK) / 1000;

		tRES = max_2(p->tRP, p->tREH);
		tRES = (tRES * CLK) / 1000;

		t1 = max_2((p->tCALS2 - p->tWP), p->tCWAW);
		t1 = (t1 * CLK) / 1000;

		tPRE = max_2(max_3(p->tWPRE, p->tWPRE2, p->tRPRE),
						max_3(p->tRPRE2, p->tCS, p->tCS2));
		tPRE = (tPRE * CLK) / 1000;
		tPRE+= 1;

		tPST = max_2(p->tWPST, p->tRPST);
		tPST = (tPST * CLK) / 1000;
		tPST+= 1;

		tPSTH = max_2(p->tWPSTH, p->tRPSTH);
		tPSTH = (tPSTH * CLK) / 1000;

		tWRCK = max_4(p->tDSL, p->tDSH, p->tDQSL, p->tDQSH);
		tWRCK = (tWRCK * CLK) / 1000;
	}

	// tBSY = max(tWB, tRB), min value = 1
	tBSY = max_2(p->tWB, p->tRB);
	tBSY = (tBSY * CLK) / 1000;
	if(tBSY < 1)
		tBSY = 1;
	// tBUF1 = max(tADL, tCCS)
	tBUF1 = max_2(p->tADL, p->tCCS);
	tBUF1 = (tBUF1 * CLK) / 1000;
	// tBUF2 = max(tAR, tRR, tCLR, tCDQSS, tCRES, tCALS, tCALS2, tDBS)
	tBUF2 = max_2(max_4(p->tAR, p->tRR, (int)p->tCLR, (int)p->tCDQSS),
			max_4((int)p->tCRES, (int)p->tCALS, (int)p->tCALS2, (int)p->tDBS));
	tBUF2 = (tBUF2 * CLK) / 1000;
	// tBUF3 = max(tRHW, tRHZ, tDQSHZ)
	tBUF3 = max_3(p->tRHW, p->tRHZ, (int)p->tDQSHZ);
	tBUF3 = (tBUF3 * CLK) / 1000;
	// tBUF4 = max(tWHR, tWHR2)
	tBUF4 = max_2((int)p->tWHR, p->tWHR2);
	if(data->flash_type == ONFI3)
		tBUF4 = max_2(tBUF4, p->tCCS);
	tBUF4 = (tBUF4 * CLK) / 1000;

	// For FPGA, we use the looser AC timing
	if(data->flash_type == TOGGLE1 || data->flash_type == TOGGLE2) {

		toggle_offset = 3;
		tREH += toggle_offset;
		tRES += toggle_offset;
		tWH +=toggle_offset;
		tWP +=toggle_offset;
		t1  +=toggle_offset;
		tBSY+=toggle_offset;
		tBUF1+=toggle_offset;
		tBUF2+=toggle_offset;
		tBUF3+=toggle_offset;
		tBUF4+=toggle_offset;
		tWRCK+=toggle_offset;
		tPSTH+=toggle_offset;
		tPST+=toggle_offset;
		tPRE+=toggle_offset;
	}

	timing[0] = (tWH << 24) | (tWP << 16) | (tREH << 8) | tRES;
	timing[1] = (tRLAT << 16) | (tBSY << 8) | t1;
	timing[2] = (tBUF4 << 24) | (tBUF3 << 16) | (tBUF2 << 8) | tBUF1;
	timing[3] = (tPRE << 28) | (tPST << 24) | (tPSTH << 16) | tWRCK;

	for (i = 0;i < MAX_CHANNEL;i++) {
		writel(timing[0], data->io_base + FL_AC_TIMING0(i));
		writel(timing[1], data->io_base + FL_AC_TIMING1(i));
		writel(timing[2], data->io_base + FL_AC_TIMING2(i));
		writel(timing[3], data->io_base + FL_AC_TIMING3(i));

		/* A380: Illegal data latch occur at setting "rlat" field
		 * of ac timing register from 0 to 1.
		 * read command failed on A380 Linux
		 * Workaround: Set Software Reset(0x184) after
		 * "Trlat" field of AC Timing Register changing.
		 * Fixed in IP version 2.2.0
		 */
		if (tRLAT) {
			if (readl(data->io_base + REVISION_NUM) < 0x020200) {
				writel((1 << i), data->io_base + NANDC_SW_RESET);
				// Wait for the NANDC024 reset is complete
				while(readl(data->io_base + NANDC_SW_RESET) & (1 << i)) ;
			}
		}
	}

	DBGLEVEL2(ftnandc024_dbg("AC Timing 0:0x%08x\n", timing[0]));
	DBGLEVEL2(ftnandc024_dbg("AC Timing 1:0x%08x\n", timing[1]));
	DBGLEVEL2(ftnandc024_dbg("AC Timing 2:0x%08x\n", timing[2]));
	DBGLEVEL2(ftnandc024_dbg("AC Timing 3:0x%08x\n", timing[3]));
}

static void ftnandc024_onfi_set_feature(struct nand_chip *chip, int val)
{
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;

	/* val is sub-feature Parameter P1 (P2~P4 = 0)
	 * b[5:4] means Data interface: 0x0(SDR); 0x1(NV-DDR); 0x2(NV-DDR2)
	 * b[3:0] means Timing mode number
	 */
	writel(val, data->io_base + SPARE_SRAM + (data->cur_chan << data->spare_ch_offset));

	/* 0x1 is Timing mode feature address */
	cmd_f.row_cycle = ROW_ADDR_1CYCLE;
	cmd_f.col_cycle = COL_ADDR_1CYCLE;
	cmd_f.cq1 = 0x1;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = 0;
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(LEGACY_FLASH) |\
			CMD_START_CE(data->sel_chip) | CMD_BYTE_MODE | CMD_SPARE_NUM(4) |\
			CMD_INDEX(ONFI_FIXFLOW_SETFEATURE);

	ftnandc024_issue_cmd(chip, &cmd_f);

	ftnandc024_nand_wait(chip);

}

static u32 ftnandc024_onfi_get_feature(struct nand_chip *chip, int type)
{
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;
	u32 val;

	/* 0x1 is Timing mode feature address */
	cmd_f.row_cycle = ROW_ADDR_1CYCLE;
	cmd_f.col_cycle = COL_ADDR_1CYCLE;
	cmd_f.cq1 = 0x1;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = 0;
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(type) |\
			CMD_START_CE(data->sel_chip) | CMD_BYTE_MODE | CMD_SPARE_NUM(4) |\
			CMD_INDEX(ONFI_FIXFLOW_GETFEATURE);

	ftnandc024_issue_cmd(chip, &cmd_f);

	ftnandc024_nand_wait(chip);

	val = readl(data->io_base + SPARE_SRAM + (data->cur_chan << data->spare_ch_offset));

	return val;
}

static void ftnandc024_onfi_config_DDR2(struct nand_chip *chip, u8 wr_cyc, u8 rd_cyc)
{
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;
	u8 i;
	u8 param[4];

	//sub-feature Parameter P1/P2 (P3~P4 = 0)
	//P1:
	param[0] = 0;
	//P2: Warmup DQS cycles for Data Input and Data Output
	param[1] = ((wr_cyc & 0x3) << 4) | (rd_cyc & 0x3);
	param[2] = param[3] = 0;

	for (i = 0; i< 4; i ++) {
		writel(param[i], data->io_base + SPARE_SRAM + 
			(data->cur_chan << data->spare_ch_offset) + i);
	}

	/* 0x2 is NV-DDR2 Configuration feature address */
	cmd_f.row_cycle = ROW_ADDR_1CYCLE;
	cmd_f.col_cycle = COL_ADDR_1CYCLE;
	cmd_f.cq1 = 0x2;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = 0;
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(LEGACY_FLASH) |\
			CMD_START_CE(data->sel_chip) | CMD_BYTE_MODE | CMD_SPARE_NUM(4) |\
			CMD_INDEX(ONFI_FIXFLOW_SETFEATURE);

	ftnandc024_issue_cmd(chip, &cmd_f);

	ftnandc024_nand_wait(chip);

	// Set the controller
	ftnandc024_set_warmup_cycle(chip, wr_cyc, rd_cyc);
}

static int ftnandc024_onfi_sync(struct nand_chip *chip)
{
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	u32 val;
	int ret = -1;

	ftnandc024_nand_select_chip(chip, 0);
	val = ftnandc024_onfi_get_feature(chip, LEGACY_FLASH);
	printk("SDR feature for Ch %d, CE %d: 0x%x\n", data->cur_chan, data->sel_chip, val);

	//Check if the FTNANDC024 support DDR interface
	val = readl(data->io_base + FEATURE_1);
	if ((val & DDR_IF_EN) == 0)
		goto out;

	if (data->flash_type == ONFI2) {
		ftnandc024_onfi_set_feature(chip, 0x11);

		val = ftnandc024_onfi_get_feature(chip, ONFI2);
		printk("NV-DDR feature for Ch %d, CE %d: 0x%x\n", data->cur_chan, data->sel_chip, val);
		if (val != 0x1111) {
			goto out;
		}
	}
	else if (data->flash_type == ONFI3) {
		/* set NV-DDR2 Configuration feature address before Timing mode feature */
		ftnandc024_onfi_config_DDR2(chip, 0, 0);

		ftnandc024_onfi_set_feature(chip, 0x21);

		val = ftnandc024_onfi_get_feature(chip, ONFI3);
		printk("NV-DDR2 feature for Ch %d, CE %d: 0x%x\n", data->cur_chan, data->sel_chip, val);
		if (val != 0x2121) {
			// Reset the setting
			ftnandc024_set_warmup_cycle(chip, 0, 0);
			goto out;
		}
	}
	ret = 0;

out:
	return ret;
}

static void ftnandc024_read_raw_id(struct nand_chip *chip)
{
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;
	u8 id_size = 5;

#if defined(CONFIG_FTNANDC024_TOSHIBA_TC58NVG4T2ETA00)
	data->cur_chan = 1;
	data->sel_chip = 0;
#elif defined(CONFIG_FTNANDC024_TOSHIBA_TC58NVG6DCJTA00)
	data->cur_chan = 0;
	data->sel_chip = 3;
#elif defined(CONFIG_FTNANDC024_TOSHIBA_TH58TEG7DCJBA4C) ||\
	defined(CONFIG_FTNANDC024_SAMSUNG_K9ABGD8U0B)
	data->cur_chan = 0;
	data->sel_chip = 1;
#elif defined (CONFIG_FTNANDC024_WINBOND_W29N01GV)
	data->cur_chan = 0;
	data->sel_chip = 2;
#else
	data->cur_chan = 0;
	data->sel_chip = 0;
#endif

	// Set the flash to Legacy mode, in advance.
	if(data->flash_type == ONFI2 || data->flash_type == ONFI3) {
		ftnandc024_onfi_set_feature(chip, 0x00);
	}

	// Issue the RESET cmd
	cmd_f.cq1 = 0;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = 0;
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(LEGACY_FLASH) |\
			CMD_START_CE(data->sel_chip) | CMD_INDEX(FIXFLOW_RESET);

	ftnandc024_issue_cmd(chip, &cmd_f);

	ftnandc024_nand_wait(chip);


	// Issue the READID cmd
	cmd_f.row_cycle = ROW_ADDR_1CYCLE;
	cmd_f.col_cycle = COL_ADDR_1CYCLE;
	cmd_f.cq1 = 0;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = CMD_COUNT(1);
	cmd_f.cq4 = CMD_FLASH_TYPE(LEGACY_FLASH) | CMD_COMPLETE_EN |\
			CMD_INDEX(FIXFLOW_READID) | CMD_START_CE(data->sel_chip) |\
			CMD_BYTE_MODE | CMD_SPARE_NUM(id_size);

	ftnandc024_issue_cmd(chip, &cmd_f);

	ftnandc024_nand_wait(chip);

	memcpy(data->flash_raw_id, data->io_base + SPARE_SRAM + (data->cur_chan << data->spare_ch_offset) , id_size);

	DBGLEVEL2(ftnandc024_dbg("ID@(ch:%d, ce:%d):0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
					data->cur_chan, data->sel_chip, data->flash_raw_id[0],
					data->flash_raw_id[1], data->flash_raw_id[2],
					data->flash_raw_id[3], data->flash_raw_id[4]));
}

static void ftnandc024_calibrate_dqs_delay(struct nand_chip *chip)
{
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;
	int i, max_dqs_delay = 0;
	int id_size = 5;
	int id_size_ddr = (id_size << 1);
	u8 *p, *golden_p;
	u8 dqs_lower_bound, dqs_upper_bound, state;
	u32 val;

	dqs_lower_bound = dqs_upper_bound = 0;
	p = kmalloc(id_size_ddr, GFP_KERNEL);
	golden_p = kmalloc(id_size_ddr, GFP_KERNEL);

	if(data->flash_type == ONFI2 || data->flash_type == ONFI3) {
		/* Extent the data from SDR to DDR.
		   Ex. If "0xaa, 0xbb, 0xcc, 0xdd, 0xee" is in SDR,
		          "0xaa, 0xaa, 0xbb, 0xbb, 0xcc, 0xcc, 0xdd, 0xdd, 0xee, 0xee" is in DDR(ONFI).
		*/
		for(i = 0; i< id_size; i++) {
			*(golden_p + (i << 1) + 0) = *(data->flash_raw_id + i);
			*(golden_p + (i << 1) + 1) = *(data->flash_raw_id + i);
		}
		DBGLEVEL2(ftnandc024_dbg("Golden ID:0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
					*golden_p, *(golden_p+1), *(golden_p+2),
					*(golden_p+3), *(golden_p+4), *(golden_p+5)));
		max_dqs_delay = 20;
	}
	else if(data->flash_type == TOGGLE1 || data->flash_type == TOGGLE2) {
		/* Extent the data from SDR to DDR.
		   Ex. If "0xaa, 0xbb, 0xcc, 0xdd, 0xee" is in SDR,
		          "0xaa, 0xbb, 0xbb, 0xcc, 0xcc, 0xdd, 0xdd, 0xee, 0xee" is in DDR(TOGGLE).
		*/
		for(i = 0; i< id_size; i++) {
			*(golden_p + (i << 1) + 0) = *(data->flash_raw_id + i);
			*(golden_p + (i << 1) + 1) = *(data->flash_raw_id + i);
		}
		golden_p ++;

		DBGLEVEL2(ftnandc024_dbg("Golden ID:0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
					*golden_p, *(golden_p+1), *(golden_p+2),
					*(golden_p+3), *(golden_p+4), *(golden_p+5)));
		max_dqs_delay = 18;
	}
	else {
		printk("%s:Type:%d isn't allowed\n", __func__, data->flash_type);
		goto out;
	}


	state = 0;
	for(i = 0; i <= max_dqs_delay; i++) {
		// setting the dqs delay before READID.
		writel(i, data->io_base + DQS_DELAY);
		memset(p, 0, id_size_ddr);

		// Issuing the READID
		cmd_f.row_cycle = ROW_ADDR_1CYCLE;
		cmd_f.col_cycle = COL_ADDR_1CYCLE;
		cmd_f.cq1 = 0;
		cmd_f.cq2 = 0;
		cmd_f.cq3 = CMD_COUNT(1);
		cmd_f.cq4 = CMD_FLASH_TYPE(data->flash_type) | CMD_COMPLETE_EN |\
				CMD_INDEX(FIXFLOW_READID) | CMD_BYTE_MODE |\
				CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(id_size_ddr);

		ftnandc024_issue_cmd(chip, &cmd_f);

		ftnandc024_nand_wait(chip);


		if(data->flash_type == ONFI2 || data->flash_type == ONFI3) {
			memcpy(p, data->io_base + SPARE_SRAM + (data->cur_chan<< data->spare_ch_offset), id_size_ddr);
			if(state == 0 && memcmp(golden_p, p, id_size_ddr) == 0) {
				dqs_lower_bound = i;
				state = 1;
			}
			else if(state == 1 && memcmp(golden_p, p, id_size_ddr) != 0){
				dqs_upper_bound = i - 1;
				break;
			}
		}
		else if(data->flash_type == TOGGLE1 || data->flash_type == TOGGLE2) {
			memcpy(p, data->io_base + SPARE_SRAM + (data->cur_chan<< data->spare_ch_offset), id_size_ddr-1);

			if(state == 0 && memcmp(golden_p, p, (id_size_ddr - 1)) == 0) {
				dqs_lower_bound = i;
				state = 1;
			}
			else if(state == 1 && memcmp(golden_p, p, (id_size_ddr - 1)) != 0){
				dqs_upper_bound = (i - 1);
				break;
			}

		}
		DBGLEVEL2(ftnandc024_dbg("===============================================\n"));
		DBGLEVEL2(ftnandc024_dbg("ID       :0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
							*p, *(p+1), *(p+2), *(p+3),
							*(p+4), *(p+5), *(p+6), *(p+7),
							*(p+8) ));
		DBGLEVEL2(ftnandc024_dbg("Golden ID:0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
							*golden_p, *(golden_p+1), *(golden_p+2), *(golden_p+3),
							*(golden_p+4), *(golden_p+5),*(golden_p+6), *(golden_p+7),
							*(golden_p+8) ));
		DBGLEVEL2(ftnandc024_dbg("===============================================\n"));
	}
	// Prevent the dqs_upper_bound is zero when ID still accuracy on the max dqs delay
	if(i == max_dqs_delay + 1)
		dqs_upper_bound = max_dqs_delay;

	printk("Upper:%d & Lower:%d for DQS, then Middle:%d\n",
		dqs_upper_bound, dqs_lower_bound, ((dqs_upper_bound + dqs_lower_bound) >> 1));
	// Setting the middle dqs delay
	val = readl(data->io_base + DQS_DELAY);
	val &= ~0x1F;
	val |= (((dqs_lower_bound + dqs_upper_bound) >> 1) & 0x1F);
	writel(val, data->io_base + DQS_DELAY);
out:
	kfree(p);
	kfree(golden_p);

}

static void ftnandc024_calibrate_rlat(struct nand_chip *chip)
{
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;
	int i, max_rlat;
	int id_size = 5;
	u8 *p, *golden_p;
	u8 rlat_lower_bound, rlat_upper_bound, state;
	u32 ac_reg0, ac_reg1, val;

	rlat_lower_bound = rlat_upper_bound = 0;
	p = kmalloc(id_size, GFP_KERNEL);
	golden_p = kmalloc(id_size, GFP_KERNEL);

	if(data->flash_type == LEGACY_FLASH) {
		for(i = 0; i< id_size; i++) {
			*(golden_p + i) = *(data->flash_raw_id + i);
		}
	} else {
		printk("%s:Type:%d isn't allowed\n", __func__, data->flash_type);
		goto out;
	}

	ac_reg0 = readl(data->io_base + FL_AC_TIMING0(0));
	max_rlat = (ac_reg0 & 0x1F) + ((ac_reg0 >> 8) & 0xF);
	ac_reg1 = readl(data->io_base + FL_AC_TIMING1(0));
	state = 0;
	for(i = 0; i <= max_rlat; i++) {
		// setting the trlat delay before READID.
		val = (ac_reg1 & ~(0x3F<<16)) | (i<<16);
		writel(val, data->io_base + FL_AC_TIMING1(0));
		memset(p, 0, id_size);

		// Issuing the READID
		cmd_f.row_cycle = ROW_ADDR_1CYCLE;
		cmd_f.col_cycle = COL_ADDR_1CYCLE;
		cmd_f.cq1 = 0;
		cmd_f.cq2 = 0;
		cmd_f.cq3 = CMD_COUNT(1);
		cmd_f.cq4 = CMD_FLASH_TYPE(data->flash_type) | CMD_COMPLETE_EN |\
				CMD_INDEX(FIXFLOW_READID) | CMD_BYTE_MODE |\
				CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(id_size);

		ftnandc024_issue_cmd(chip, &cmd_f);

		ftnandc024_nand_wait(chip);

		memcpy(p, data->io_base + SPARE_SRAM + (data->cur_chan<< data->spare_ch_offset), id_size);
		if(state == 0 && memcmp(golden_p, p, id_size) == 0) {
			rlat_lower_bound = i;
			state = 1;
		}
		else if(state == 1 && memcmp(golden_p, p, id_size) != 0) {
			rlat_upper_bound = i - 1;
			break;
		}

		DBGLEVEL2(ftnandc024_dbg("===============================================\n"));
		DBGLEVEL2(ftnandc024_dbg("ID       :0x%x 0x%x 0x%x 0x%x 0x%x\n",
							*p, *(p+1), *(p+2), *(p+3), *(p+4)));
		DBGLEVEL2(ftnandc024_dbg("Golden ID:0x%x 0x%x 0x%x 0x%x 0x%x\n",
							*golden_p, *(golden_p+1), *(golden_p+2), *(golden_p+3), *(golden_p+4)));
		DBGLEVEL2(ftnandc024_dbg("===============================================\n"));
	}

	// Prevent the dqs_upper_bound is zero when ID still accuracy on the max dqs delay
	if(i == max_rlat + 1)
		rlat_upper_bound = max_rlat;

	DBGLEVEL2(ftnandc024_dbg("Upper:%d & Lower:%d for tRLAT, then Middle:%d\n",
		rlat_upper_bound, rlat_lower_bound, ((rlat_upper_bound + rlat_lower_bound) >> 1)));

	// Setting the middle tRLAT
	val = ac_reg1&~(0x3F<<16);
	val |= ((((rlat_upper_bound + rlat_lower_bound) >> 1) & 0x3F) << 16);
	writel(val, data->io_base + FL_AC_TIMING1(0));
out:
	kfree(p);
	kfree(golden_p);
}

static void ftnandc024_t2_get_feature(struct nand_chip *chip, u8 *buf)
{
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;
	u8 i;

	/* 0x2 is Timing mode feature address */
	cmd_f.row_cycle = ROW_ADDR_1CYCLE;
	cmd_f.cq1 = 0x2;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = 0;
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type) |\
			CMD_START_CE(data->sel_chip) | CMD_BYTE_MODE | CMD_SPARE_NUM(8) |\
			CMD_INDEX(ONFI_FIXFLOW_GETFEATURE);

	ftnandc024_issue_cmd(chip, &cmd_f);

	ftnandc024_nand_wait(chip);

	for (i = 0; i< 4; i++)
		*(buf + i) = readb(data->io_base + SPARE_SRAM +
				   (data->cur_chan << data->spare_ch_offset) + (i << 1));

	printk("T2 Get feature:0x%08x\n", *((int *)(buf)));

}

static void ftnandc024_t2_set_feature(struct nand_chip *chip, u8 *buf)
{
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;
	u8 i;

	for (i = 0; i< 4; i ++) {
		writel(*(buf + i), data->io_base + SPARE_SRAM +
			(data->cur_chan << data->spare_ch_offset) + (i << 1));
		writel(*(buf + i), data->io_base + SPARE_SRAM +
			(data->cur_chan << data->spare_ch_offset) + (i << 1) + 1);
	}

	/* 0x2 is Timing mode feature address */
	cmd_f.row_cycle = ROW_ADDR_1CYCLE;
	cmd_f.cq1 = 0x2;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = 0;
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type) |\
			CMD_START_CE(data->sel_chip) | CMD_BYTE_MODE | CMD_SPARE_NUM(8) |\
			CMD_INDEX(ONFI_FIXFLOW_SETFEATURE);

	ftnandc024_issue_cmd(chip, &cmd_f);

	ftnandc024_nand_wait(chip);

}


static int ftnandc024_t2_sync(struct nand_chip *chip, u8 wr_cyc, u8 rd_cyc)
{
	u8 param[4];

	// Reset the setting to make sure the accuracy of Get/Set feature
	ftnandc024_set_warmup_cycle(chip, 0, 0);

	ftnandc024_nand_select_chip(chip, 0);

	// Set feature
	param[0] = param[1] = param[2] = param[3] = 0;
	ftnandc024_t2_set_feature(chip, param);

	// Get feature
	ftnandc024_t2_get_feature(chip, param);

	// Set feature
	param[1] = ((wr_cyc & 0x3) << 4) | (rd_cyc & 0x3);
	ftnandc024_t2_set_feature(chip, param);

	// Get feature
	ftnandc024_t2_get_feature(chip, param);

	// Set the controller
	ftnandc024_set_warmup_cycle(chip, wr_cyc, rd_cyc);

	return 0;
}


static int ftnandc024_available_oob(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);

	int ret = 0;
	int consume_byte, eccbyte, eccbyte_spare;
	int available_spare;
	int sector_num = (mtd->writesize >> data->eccbasft);

	if (data->useecc < 0)
		goto out;
	if (data->protect_spare != 0)
		data->protect_spare = 1;
	else
		data->protect_spare = 0;

	eccbyte = (data->useecc * 14) / 8;
	if (((data->useecc * 14) % 8) != 0)
		eccbyte++;

	consume_byte = (eccbyte * sector_num);
	if (data->protect_spare == 1) {

		eccbyte_spare = (data->useecc_spare * 14) / 8;
		if (((data->useecc_spare * 14) % 8) != 0)
			eccbyte_spare++;
		consume_byte += eccbyte_spare;
	}
#if (CONFIG_FTNANDC024_VERSION >= 230)
	//consume byte +1 BI
	consume_byte += 1;
#endif
	available_spare = data->spare - consume_byte;

	DBGLEVEL2(ftnandc024_dbg(
		"mtd->erasesize:%d, mtd->writesize:%d, data->block_boundary:%d\n",
		mtd->erasesize, mtd->writesize, data->block_boundary));
	DBGLEVEL2(ftnandc024_dbg(
		"page num:%d, data->eccbasft:%d, protect_spare:%d, spare:%d Byte\n",
		mtd->erasesize/mtd->writesize,data->eccbasft, data->protect_spare,
		data->spare));
	DBGLEVEL2(ftnandc024_dbg(
		"consume_byte:%d, eccbyte:%d, eccbytes(spare):%d, useecc:%d bit\n",
		consume_byte, eccbyte, eccbyte_spare, data->useecc));

	/*----------------------------------------------------------
	 * YAFFS require 16 bytes OOB without ECC, 28 bytes with
	 * ECC enable.
	 * BBT require 5 bytes for Bad Block Table marker.
	 */
	if (available_spare >= 4) {
		if (available_spare >= data->max_spare) {
			ret = data->max_spare;
		}
		else {
			if (available_spare >= 64) {
				ret = 64;
			}
			else if (available_spare >= 32) {
				ret = 32;
			}
			else if (available_spare >= 16) {
				ret = 16;
			}
			else if (available_spare >= 8) {
				ret = 8;
			}
			else if (available_spare >= 4) {
				ret = 4;
			}
		}
		printk(KERN_INFO "Available OOB is %d byte, but we use %d bytes in page mode.\n", available_spare, ret);
	} else {
		printk(KERN_INFO "Not enough OOB, try to reduce ECC correction bits.\n");
		printk(KERN_INFO "(Currently ECC setting for Data:%d)\n", data->useecc);
		printk(KERN_INFO "(Currently ECC setting for Spare:%d)\n", data->useecc_spare);
	}
out:
	return ret;
}

static u8 ftnandc024_read_byte(struct nand_chip *chip)
{
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	u32 lv;
	u8 b = 0;

	switch (data->cur_cmd) {
	case NAND_CMD_READID:
		b = readb(data->io_base + SPARE_SRAM + (data->cur_chan << data->spare_ch_offset) + data->byte_ofs);
		data->byte_ofs += 1;
		if (data->byte_ofs == data->max_spare)
			data->byte_ofs = 0;
		break;
	case NAND_CMD_STATUS:
		lv = readl(data->io_base + READ_STATUS0);
		lv = lv >> (data->cur_chan * 8);
		b = (lv & 0xFF);
		break;
	}
	return b;
}


static void ftnandc024_nand_cmdfunc(struct nand_chip *chip, unsigned command,
				    int column, int page_addr)
{
//	struct mtd_info *mtd = nand_to_mtd(chip);
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;
	int real_pg, cmd_sts;
	u8 id_size = 5;

	#if defined(CONFIG_FTNANDC024_TOSHIBA_TC58NVG4T2ETA00) ||\
		defined(CONFIG_FTNANDC024_SAMSUNG_K9ABGD8U0B)
	int real_blk_nm, real_off;
	#endif

	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type);
	data->cur_cmd = command;
	if (page_addr != -1)
		data->page_addr = page_addr;
	if (column != -1)
		data->column = column;

	switch (command) {
	case NAND_CMD_READID:
		DBGLEVEL2(ftnandc024_dbg( "Read ID@(CH:%d, CE:%d)\n", data->cur_chan, data->sel_chip));
		data->byte_ofs = 0;
		// ID size is doubled when the mode is DDR.
		if(data->flash_type == TOGGLE1 || data->flash_type == TOGGLE2 ||
		   data->flash_type == ONFI2 || data->flash_type == ONFI3) {
			id_size = (id_size << 1);
		}

		cmd_f.row_cycle = ROW_ADDR_1CYCLE;
		cmd_f.col_cycle = COL_ADDR_1CYCLE;
		cmd_f.cq1 = 0;
		cmd_f.cq2 = 0;
		cmd_f.cq3 = CMD_COUNT(1);
		cmd_f.cq4 |= CMD_START_CE(data->sel_chip) | CMD_BYTE_MODE |\
				CMD_SPARE_NUM(id_size) | CMD_INDEX(FIXFLOW_READID);

		cmd_sts = ftnandc024_issue_cmd(chip, &cmd_f);
		if(!cmd_sts)
			ftnandc024_nand_wait(chip);
		else
			printk(KERN_ERR "Read ID err\n");

		break;
	case NAND_CMD_RESET:
		DBGLEVEL2(ftnandc024_dbg("Cmd Reset@(CH:%d, CE:%d)\n", data->cur_chan, data->sel_chip));

		cmd_f.cq1 = 0;
		cmd_f.cq2 = 0;
		cmd_f.cq3 = 0;
		cmd_f.cq4 |= CMD_START_CE(data->sel_chip);
		if (data->flash_type == ONFI2 || data->flash_type == ONFI3)
			cmd_f.cq4 |= CMD_INDEX(ONFI_FIXFLOW_SYNCRESET);
		else
			cmd_f.cq4 |= CMD_INDEX(FIXFLOW_RESET);

		cmd_sts = ftnandc024_issue_cmd(chip, &cmd_f);
		if(!cmd_sts)
			ftnandc024_nand_wait(chip);
		else
			printk(KERN_ERR "Reset Flash err\n");

		break;
	case NAND_CMD_STATUS:
		DBGLEVEL2(ftnandc024_dbg( "Read Status\n"));

		cmd_f.cq1 = 0;
		cmd_f.cq2 = 0;
		cmd_f.cq3 = CMD_COUNT(1);
		cmd_f.cq4 |= CMD_START_CE(data->sel_chip) | CMD_INDEX(FIXFLOW_READSTATUS);

		cmd_sts = ftnandc024_issue_cmd(chip, &cmd_f);
		if(!cmd_sts)
			ftnandc024_nand_wait(chip);
		else
			printk(KERN_ERR "Read Status err\n");

		break;
	case NAND_CMD_ERASE1:
		#if defined(CONFIG_FTNANDC024_TOSHIBA_TC58NVG4T2ETA00) ||\
			defined(CONFIG_FTNANDC024_SAMSUNG_K9ABGD8U0B)
		real_blk_nm = data->page_addr / (mtd->erasesize/mtd->writesize);
		real_off = data->page_addr % (mtd->erasesize/mtd->writesize);
		real_pg = (real_blk_nm * data->block_boundary) + real_off;
		#else
		real_pg = data->page_addr;
		#endif
		DBGLEVEL2(ftnandc024_dbg(
			"Erase Page: 0x%x, Real:0x%x\n", data->page_addr, real_pg));

		cmd_f.cq1 = real_pg;
		cmd_f.cq2 = 0;
		cmd_f.cq3 = CMD_COUNT(1);
		cmd_f.cq4 |= CMD_START_CE(data->sel_chip) | CMD_SCALE(1);

		if (data->large_page) {
			cmd_f.row_cycle = ROW_ADDR_3CYCLE;
			cmd_f.col_cycle = COL_ADDR_2CYCLE;
			cmd_f.cq4 |= CMD_INDEX(LARGE_FIXFLOW_ERASE);
		} else {
			cmd_f.row_cycle = ROW_ADDR_2CYCLE;
			cmd_f.col_cycle = COL_ADDR_1CYCLE;
			cmd_f.cq4 |= CMD_INDEX(SMALL_FIXFLOW_ERASE);
		}

		/* Someone may be curious the following snippet that
		* ftnandc024_issue_cmd doesn't be followed by
		* ftnandc024_nand_wait.
		* Waiting cmd complete will be call on the mtd upper layer via
		* the registered data->chip.waitfunc.
		*/
		cmd_sts = ftnandc024_issue_cmd(chip, &cmd_f);
		if(cmd_sts)
			printk(KERN_ERR "Erase block err\n");

		break;
	case NAND_CMD_ERASE2:
	case NAND_CMD_PAGEPROG:
	case NAND_CMD_SEQIN:
	default:
		DBGLEVEL2(ftnandc024_dbg( "Unimplemented command (cmd=%u)\n", command));
		break;
	}
}

void ftnandc024_nand_select_chip(struct nand_chip *chip, int cs)
{
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	int chn = 0;

	if (data->scan_state != 1) {
		while (cs != -1) {
			if (cs < data->valid_chip[chn]) {
				break;
			} else {
				cs = cs - data->valid_chip[chn];
				chn++;
			}
		}
		data->cur_chan = chn;
	}
#if defined(CONFIG_FTNANDC024_HYNIX_HY27US08561A) ||\
	defined(CONFIG_FTNANDC024_SAMSUNG_K9F4G08U0A)
	data->cur_chan = 0;
	data->sel_chip = 2;
#elif defined(CONFIG_FTNANDC024_TOSHIBA_TH58NVG7D2GTA20) ||\
	defined(CONFIG_FTNANDC024_TOSHIBA_TC58NVG4T2ETA00)
	data->cur_chan = 1;
	data->sel_chip = 0;
#elif defined(CONFIG_FTNANDC024_TOSHIBA_TC58NVG6DCJTA00)
	data->cur_chan = 0;
	data->sel_chip = 3;
#elif defined(CONFIG_FTNANDC024_SAMSUNG_K9GCGY8S0A) ||\
	defined(CONFIG_FTNANDC024_TOSHIBA_TH58TFT0DDLBA8H) ||\
	defined(CONFIG_FTNANDC024_MICRON_29F128G08CBECB)
	data->cur_chan = 0;
	data->sel_chip = 0;
#elif defined(CONFIG_FTNANDC024_TOSHIBA_TH58TEG7DCJBA4C) ||\
	defined(CONFIG_FTNANDC024_SAMSUNG_K9ABGD8U0B)
	data->cur_chan = 0;
	data->sel_chip = 1;
#elif defined (CONFIG_FTNANDC024_WINBOND_W29N01GV)
	data->cur_chan = 0;
	data->sel_chip = 2;
#else
	data->sel_chip = cs;
#endif
	DBGLEVEL2(ftnandc024_dbg("==>chan = %d, ce = %d\n", data->cur_chan, data->sel_chip));
}

static int ftnandc024_attach_chip(struct nand_chip *chip)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	struct nand_memory_organization *memorg;
	u32 val;
	int i;
	#ifdef CONFIG_FTNANDC024_USE_AHBDMA
	struct dma_chan *dma_chan;
	#endif

	memorg = nanddev_get_memorg(&chip->base);

	data->chip.bbt_td = &ftnandc024_bbt_main_descr;
	data->chip.bbt_md = &ftnandc024_bbt_mirror_descr;
	data->chip.badblock_pattern = &ftnandc024_largepage_flashbased;

	//usually, spare size is 1/32 page size
	if (data->spare < (mtd->writesize >> 5))
		data->spare = (mtd->writesize >> 5);

	val = readl(data->io_base + MEM_ATTR_SET);
	val &= ~(0x7 << 16);
	switch (mtd->writesize) {
	case 512:
		val |= PG_SZ_512;
		data->large_page = 0;
		break;
	case 2048:
		val |= PG_SZ_2K;
		data->large_page = 1;
		break;
	case 4096:
		val |= PG_SZ_4K;
		data->large_page = 1;
		break;
	case 8192:
		val |= PG_SZ_8K;
		data->large_page = 1;
		break;
	case 16384:
		val |= PG_SZ_16K;
		data->large_page = 1;
		break;
	}
	val &= ~(0x3FF << 2);
	val |= ((data->block_boundary - 1) << 2);
#if (CONFIG_FTNANDC024_VERSION >= 230)
	val &= ~(0x7 << 19);
	if(data->large_page)
		data->bi_bye = 1;
	else
		data->bi_bye = 6;

	val |= (data->bi_bye << 19);
#endif
	writel(val, data->io_base + MEM_ATTR_SET);

	val = readl(data->io_base + MEM_ATTR_SET2);
	val &= ~(0x3FF << 16);
	val |=  VALID_PAGE((mtd->erasesize / mtd->writesize - 1));
	writel(val, data->io_base + MEM_ATTR_SET2);

	i = ftnandc024_available_oob(mtd);
	if (likely(i >= 4)) {
		if (i > data->max_spare)
			memorg->oobsize = data->max_spare;
		else
			memorg->oobsize = i;
	} else
		return -ENXIO;

	DBGLEVEL1(ftnandc024_dbg("total oobsize: %d\n", memorg->oobsize));

	switch(memorg->oobsize){
	case 4:
	case 8:
	case 16:
	case 32:
	case 64:
	case 128:
		data->spare = memorg->oobsize;
		DBGLEVEL1(ftnandc024_dbg("oobsize(page mode): %02d\n", memorg->oobsize));
		break;
	default:
		memorg->oobsize = 4;
		data->spare = memorg->oobsize;
		DBGLEVEL1(
			ftnandc024_dbg("Warning: Unknown spare setting %d, use default oobsize(page mode): 4\n"
			, memorg->oobsize));
		break;
	}
	mtd->oobsize = memorg->oobsize;

	if (data->useecc > 0) {
		DBGLEVEL1(ftnandc024_dbg("ECC correction bits: %d\n", data->useecc));
		writel(0x01010101, data->io_base + ECC_THRES_BITREG1);
		writel(0x01010101, data->io_base + ECC_THRES_BITREG2);
		val = (data->useecc - 1) | ((data->useecc - 1) << 8) |
			((data->useecc - 1) << 16) | ((data->useecc - 1) << 24);
		writel(val, data->io_base + ECC_CORRECT_BITREG1);
		writel(val, data->io_base + ECC_CORRECT_BITREG2);

		val = readl(data->io_base + ECC_CONTROL);
		val &= ~ECC_BASE;
		if (data->eccbasft > 9)
			val |= ECC_BASE;
		val |= (ECC_EN(0xFF) | ECC_ERR_MASK(0xFF));
		writel(val, data->io_base + ECC_CONTROL);
		writel(ECC_INTR_THRES_HIT | ECC_INTR_CORRECT_FAIL, data->io_base + ECC_INTR_EN);
	} else {
		DBGLEVEL1(ftnandc024_dbg("ECC disabled\n"));
		writel(0, data->io_base + ECC_THRES_BITREG1);
		writel(0, data->io_base + ECC_THRES_BITREG2);
		writel(0, data->io_base + ECC_CORRECT_BITREG1);
		writel(0, data->io_base + ECC_CORRECT_BITREG2);

		val = readl(data->io_base + ECC_CONTROL);
		val &= ~ECC_BASE;
		val &= ~(ECC_EN(0xFF) | ECC_ERR_MASK(0xFF));
		val |= ECC_NO_PARITY;
		writel(val, data->io_base + ECC_CONTROL);
	}

	// Enable the Status Check Intr
	val = readl(data->io_base + INTR_ENABLE);
	val &= ~INTR_ENABLE_STS_CHECK_EN(0xff);
	val |= INTR_ENABLE_STS_CHECK_EN(0xff);
	writel(val, data->io_base + INTR_ENABLE);

	// Setting the ecc capability & threshold for spare
	writel(0x01010101, data->io_base + ECC_THRES_BIT_FOR_SPARE_REG1);
	writel(0x01010101, data->io_base + ECC_THRES_BIT_FOR_SPARE_REG2);
	val = (data->useecc_spare-1) | ((data->useecc_spare-1) << 8) |
		((data->useecc_spare-1) << 16) | ((data->useecc_spare-1) << 24);
	writel(val, data->io_base + ECC_CORRECT_BIT_FOR_SPARE_REG1);
	writel(val, data->io_base + ECC_CORRECT_BIT_FOR_SPARE_REG2);

	data->chip.ecc.engine_type = NAND_ECC_ENGINE_TYPE_ON_HOST;
	data->chip.ecc.size = mtd->writesize;
	data->chip.ecc.bytes = 0;
	data->chip.ecc.strength = 1;
	data->chip.ecc.read_page = ftnandc024_nand_read_page;
	data->chip.ecc.write_page = ftnandc024_nand_write_page_lowlevel;
	data->chip.ecc.read_oob = ftnandc024_nand_read_oob_std;
	data->chip.ecc.write_oob = ftnandc024_nand_write_oob_std;
	data->chip.ecc.read_page_raw = ftnandc024_nand_read_page;
	if (data->large_page) {
		data->read_page = ftnandc024_nand_read_page_lp;
		data->write_page = ftnandc024_nand_write_page_lp;
		data->read_oob = ftnandc024_nand_read_oob_lp;
		data->write_oob = ftnandc024_nand_write_oob_lp;
	} else {
		data->read_page = ftnandc024_nand_read_page_sp;
		data->write_page = ftnandc024_nand_write_page_sp;
		data->read_oob = ftnandc024_nand_read_oob_sp;
		data->write_oob = ftnandc024_nand_write_oob_sp;
	}
	mtd_set_ooblayout(mtd, &ftnandc024_ooblayout_ops);

#ifdef CONFIG_FTNANDC024_USE_AHBDMA
	data->dma_buf = dma_alloc_coherent(data->dev, mtd->writesize,
					   &data->dmaaddr, GFP_KERNEL);
	if (!data->dma_buf) {
		return -ENOMEM;
	}
	
	dma_chan = dma_request_slave_channel(data->dev, "tx-rx");
	if (!dma_chan) {
		dev_err(data->dev, "Failed to allocate DMA channel\n");
		dma_free_coherent(data->dev, mtd->writesize, data->dma_buf,
					  data->dmaaddr);
		return -EBUSY;
	}
	data->dma_chan = dma_chan;
	init_waitqueue_head(&data->dma_wait_queue);
	
	printk("Transfer: DMA\n");
#else
	printk("Transfer: PIO\n");
#endif

	return 0;
}

static const struct nand_controller_ops ftnandc024_controller_ops = {
	.attach_chip = ftnandc024_attach_chip,
};

/*
 * Probe for the NAND device.
 */
static int ftnandc024_nand_probe(struct platform_device *pdev)
{
//	struct clk *clk;
	struct ftnandc024_nand_data *data;
	struct mtd_info *mtd;
	struct nand_chip *chip;
	int ret, chipnum;
	uint64_t size, max_sz = -1;
	int i, sel, type;
	u32 val;

	struct mtd_partition *partitions = NULL;
	int num_partitions = 0;

	ret = chipnum = size = type = 0;
	/* Allocate memory for the device structure (and zero it) */

	/*
	 * Initialize the parameter
	 */
	data = ftnandc024_init_param();
	//data = kzalloc(sizeof(struct ftnandc024_nand_data), GFP_KERNEL);
	if (!data) {
		dev_err(&pdev->dev, "failed to allocate device structure.\n");
		ret = -ENOMEM;
		goto out;
	}

#if 0  //EVB specific, if not A369/A380 EVB with FPGA, maybe you should #if 0
	/* Setting the Ext-AHB in A369*/
	clk = clk_get(&pdev->dev, "extahb");
	if (IS_ERR(clk))
		printk("can't set ext-ahb clk!\n");
	else {
		clk_set_rate(clk, 33000000);
		clk_prepare_enable(clk);
		clk_put(clk);
	}
#endif

	data->io_base = ioremap(pdev->resource[0].start,
				pdev->resource[0].end - pdev->resource[0].start + 1);
	if (data->io_base == NULL) {
		dev_err(&pdev->dev, "ioremap failed for register.\n");
		ret = -EIO;
		goto out_free_data;
	}
	DBGLEVEL2(ftnandc024_dbg("data->io_base:0x%08x", (unsigned int)data->io_base));

#ifdef CONFIG_FTNANDC024_USE_AHBDMA
	data->chip.legacy.IO_ADDR_R = (void *)pdev->resource[1].start;
#else
	data->chip.legacy.IO_ADDR_R = ioremap(pdev->resource[1].start,
				pdev->resource[1].end - pdev->resource[1].start + 1);
	if (data->chip.legacy.IO_ADDR_R == NULL) {
		dev_err(&pdev->dev, "ioremap failed for data port.\n");
		ret = -EIO;
		goto out_no_ior;
	}
#endif

	// Reset the HW
	// Note: We can't use the function of ftnandc024_soft_reset to reset the hw
	//       because the private data field of ftnandc024_nand_data is null.
	writel(1, data->io_base + GLOBAL_RESET);
	while (readl(data->io_base + GLOBAL_RESET)) ;

#if defined(CONFIG_FTNANDC024_HYNIX_HY27US08561A) ||\
	defined(CONFIG_FTNANDC024_SAMSUNG_K9F4G08U0A) ||\
	defined(CONFIG_FTNANDC024_SPANSION_S34ML01G100TF100)
	/* We disable scramble function on SLC because SLC
	 * usually needs fewer ecc correction capability.
	 * The fewer ecc correction capability, the more
	 * probability of ecc misjudgement especially for
	 * paged pattern(0xff)
	 */
	// which is randomized by scrambler.
	#ifdef CONFIG_FTNANDC024_USE_DATA_INVERSE
	writel(BUSY_RDY_LOC(6) | CMD_STS_LOC(0) | CE_NUM(2) | DATA_INVERSE, data->io_base + GENERAL_SETTING);
	#else
	writel(BUSY_RDY_LOC(6) | CMD_STS_LOC(0) | CE_NUM(2), data->io_base + GENERAL_SETTING);
	#endif
#else
	#ifdef CONFIG_FTNANDC024_USE_DATA_INVERSE
	writel(BUSY_RDY_LOC(6) | CMD_STS_LOC(0) | CE_NUM(2) | DATA_INVERSE | DATA_SCRAMBLER, data->io_base + GENERAL_SETTING);
	#else
	writel(BUSY_RDY_LOC(6) | CMD_STS_LOC(0) | CE_NUM(2) | DATA_SCRAMBLER, data->io_base + GENERAL_SETTING);
	#endif
#endif

	//lichun@add, for FTNANDC024 v2.4
	data->max_spare = 32;
	data->spare_ch_offset = 5; //shift 5 means 0x20
	data->seed_val = 0;
#if 0 //enable at next version (v2.5)
	if (readl(data->io_base + REVISION_NUM) >= 0x020400) {
		printk(KERN_INFO "FTNANDC024 >= v2.4\n");

		//Check if the FTNANDC024 support up to 128 bytes spare number
		val = readl(data->io_base + FEATURE_1);
		if (val & MAX_SPARE_DATA_128BYTE) {
			data->max_spare = 128;
			data->spare_ch_offset = 7; //shift 7 means 0x80
		}

		//Support FW to program scramble seed
/*		val = readl(data->io_base + NANDC_EXT_CTRL);
		for(i = 0; i < MAX_CHANNEL; i++)
			val |= SEED_SEL(i);
		writel(val, data->io_base + NANDC_EXT_CTRL);
		data->seed_val = 0x2fa5; //random set, b[13:0]
*/
	}
#endif
	//~lichun@add

	val = readl(data->io_base + AHB_SLAVEPORT_SIZE);
	val &= ~0xFFF0FF;
	val |= AHB_SLAVE_SPACE_32KB;
	for(i = 0; i < MAX_CHANNEL; i++)
		val |= AHB_PREFETCH(i);
	val |= AHB_PRERETCH_LEN(128);
	writel(val, data->io_base + AHB_SLAVEPORT_SIZE);

#if defined (CONFIG_FTNANDC024_MICRON_29F16G08MAA)
	sel = 0;
#elif defined (CONFIG_FTNANDC024_SAMSUNG_K9F4G08U0A)
	sel = 1;
#elif defined (CONFIG_FTNANDC024_HYNIX_HY27US08561A)
	sel = 2;
#elif defined (CONFIG_FTNANDC024_TOSHIBA_TH58NVG5D2ETA20)
	sel = 3;
#elif defined (CONFIG_FTNANDC024_TOSHIBA_TH58NVG7D2GTA20)
	sel = 4;
#elif defined (CONFIG_FTNANDC024_SAMSUNG_K9HDGD8X5M)
	sel = 5;
#elif defined (CONFIG_FTNANDC024_MICRON_29F32G08CBABB)
	sel = 6;
#elif defined (CONFIG_FTNANDC024_SAMSUNG_K9LBG08U0M)
	sel = 7;
#elif defined (CONFIG_FTNANDC024_TOSHIBA_TC58NVG4T2ETA00)
	sel = 8;
#elif defined (CONFIG_FTNANDC024_TOSHIBA_TC58NVG6DCJTA00)
	sel = 9;
#elif defined (CONFIG_FTNANDC024_SAMSUNG_K9GCGY8S0A)
	sel = 10;
#elif defined (CONFIG_FTNANDC024_TOSHIBA_TH58TEG7DCJBA4C)
	sel = 11;
#elif defined (CONFIG_FTNANDC024_SAMSUNG_K9ABGD8U0B)
	sel = 12;
#elif defined (CONFIG_FTNANDC024_WINBOND_W29N01GV)
	sel = 13;
#elif defined (CONFIG_FTNANDC024_TOSHIBA_TH58TFT0DDLBA8H)
	sel = 14;
#elif defined (CONFIG_FTNANDC024_MICRON_29F128G08CBECB)
	sel = 15;
#elif defined (CONFIG_FTNANDC024_SPANSION_S34ML01G100TF100)
	sel = 16;
#else
	sel = -1;
#endif

	if (sel != -1) {
		printk(KERN_INFO "Use %s NAND chip...\n", nand_attr[sel].name);
		data->spare = nand_attr[sel].sparesize;
		data->useecc = nand_attr[sel].ecc;
		data->useecc_spare = nand_attr[sel].ecc_spare;
		data->eccbasft = nand_attr[sel].eccbaseshift;
		data->protect_spare = nand_attr[sel].crc;
		data->flash_type = nand_attr[sel].flash_type;

	} else {
		printk(KERN_INFO "Use Unknown type NAND chip...\n");
		goto out_no_ior;
	}

	nand_set_controller_data(&data->chip, data);
	mtd = nand_to_mtd(&data->chip);
	mtd->dev.parent = &pdev->dev;
	data->dev = &pdev->dev;
	data->chip.legacy.IO_ADDR_W = data->chip.legacy.IO_ADDR_R;
	data->chip.legacy.select_chip = ftnandc024_nand_select_chip;
	data->chip.legacy.cmdfunc = ftnandc024_nand_cmdfunc;
	data->chip.legacy.read_byte = ftnandc024_read_byte;
	data->chip.legacy.waitfunc = ftnandc024_nand_wait;
	data->chip.legacy.chip_delay = 0;
	data->chip.options = NAND_NO_SUBPAGE_WRITE;// | NAND_OWN_BUFFERS;
	data->chip.options |= NAND_BBM_FIRSTPAGE | NAND_BBM_SECONDPAGE;
	data->chip.bbt_options = NAND_BBT_USE_FLASH;
	data->block_boundary = nand_attr[sel].block_boundary;
	platform_set_drvdata(pdev, data);

	chip = &data->chip;
	// Set the default AC timing/Warmup cyc for ftnandc024.
	// The register of AC timing/Warmup  keeps the value
	// set before although the Global Reset is set.
	ftnandc024_set_default_timing(chip);
	ftnandc024_set_warmup_cycle(chip, 0, 0);

	// Disable the scan state for ftnandc024_nand_select_chip
	data->scan_state = 0;

	// Read the raw id to calibrate the DQS delay for Sync. latching(DDR)
	ftnandc024_read_raw_id(chip);
	if(data->flash_type == TOGGLE1 || data->flash_type == TOGGLE2) {
		ftnandc024_calc_timing(chip);
		ftnandc024_calibrate_dqs_delay(chip);
	}

	/*--------------------------------------------------------
	 * ONFI flash must work in Asynch mode for READ ID command.
	 * Switch it back to Legacy.
	 */
	if (data->flash_type == ONFI2 || data->flash_type == ONFI3) {
		type = data->flash_type;
		data->flash_type = LEGACY_FLASH;
	}

	// Enable the scan state for ftnandc024_nand_select_chip
	data->scan_state = 1;
	// Reset the flash delay set before.
	if(data->flash_type == TOGGLE2)
		ftnandc024_t2_sync(chip, 0, 0);

	data->chip.legacy.dummy_controller.ops = &ftnandc024_controller_ops;
	/* Scan to find existance of the device */
	for (i = startchn; i < MAX_CHANNEL; i++) {
		printk(KERN_INFO "Scan Channel %d...\n", i);
		data->cur_chan = i;
		if (!nand_scan(chip, MAX_CE)) {
			if (((max_sz - size) > mtd->size)
			    && ((chipnum + nanddev_ntargets(&chip->base)) <= NAND_MAX_CHIPS)) {
				data->valid_chip[i] = nanddev_ntargets(&chip->base);
				chipnum += nanddev_ntargets(&chip->base);
				size += (chipnum * nanddev_target_size(&chip->base));
			} else {
				printk(KERN_INFO "Can not accept more flash chips.\n");
				break;
			}
		}
	}
	// Disable the scan-state for ftnandc024_nand_select_chip
	data->scan_state = 0;

	if (chipnum == 0) {
		ret = -ENXIO;
		goto out_unset_drv;
	}

	mtd->size = size;

	/*----------------------------------------------------------
	 * ONFI synch mode means High Speed. If fails to change to
	 * Synch mode, then use flash as Async mode(Normal speed) and
	 * use LEGACY_LARGE fix flow.
	 */
	if (type == ONFI2 || type == ONFI3) {
		data->flash_type = type;
		if (ftnandc024_onfi_sync(chip) == 0) {
			ftnandc024_calc_timing(chip);
			ftnandc024_calibrate_dqs_delay(chip);
		}
		else{
			data->flash_type = LEGACY_FLASH;
		}
	}

	// Toggle & ONFI flash has set the proper timing before READ ID.
	// We don't do that twice.
	if(data->flash_type == LEGACY_FLASH) {
		ftnandc024_calc_timing(chip);
		ftnandc024_calibrate_rlat(chip);
	}
	else if(data->flash_type == TOGGLE2) {
#if defined(CONFIG_FTNANDC024_SAMSUNG_K9GCGY8S0A)
		ftnandc024_t2_sync(chip, 0, 2);
#elif defined(CONFIG_FTNANDC024_TOSHIBA_TH58TEG7DCJBA4C) ||\
	defined(CONFIG_FTNANDC024_TOSHIBA_TH58TFT0DDLBA8H)
		ftnandc024_t2_sync(chip, 3, 3);
#endif
	}

	if (num_partitions <= 0) {
		partitions = ftnandc024_partition_info;
		num_partitions = ARRAY_SIZE(ftnandc024_partition_info);
	}
	ret = mtd_device_register(mtd, partitions, num_partitions);

	if (!ret)
		return ret;

	nand_cleanup(chip);
out_unset_drv:
	platform_set_drvdata(pdev, NULL);
out_no_ior:
#ifndef CONFIG_FTNANDC024_USE_AHBDMA
	iounmap(data->chip.legacy.IO_ADDR_R);
#endif
	iounmap(data->io_base);
out_free_data:
	kfree(data);
out:
	return ret;
}

/*
 * Remove a NAND device.
 */
static int ftnandc024_nand_remove(struct platform_device *pdev)
{
	struct ftnandc024_nand_data *data = platform_get_drvdata(pdev);
	struct nand_chip *chip = &data->chip;
	int ret;

	iounmap(data->chip.legacy.IO_ADDR_R);

	ret = mtd_device_unregister(nand_to_mtd(chip));
	WARN_ON(ret);
	nand_cleanup(chip);

	iounmap(data->io_base);
	kfree(data);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id ftnandc024_dt_ids[] = {
	{ .compatible = "faraday,ftnandc024" },
	{ }
};

MODULE_DEVICE_TABLE(of, ftnandc024_dt_ids);
#endif

static struct platform_driver ftnandc024_nand_driver __refdata = {
	.probe	= ftnandc024_nand_probe,
	.remove	= ftnandc024_nand_remove,
	.driver	= {
		.name = "ftnandc024_nand",
		.of_match_table = of_match_ptr(ftnandc024_dt_ids),
	},
};

module_platform_driver(ftnandc024_nand_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Faraday Technology Corporation");
MODULE_DESCRIPTION("FTNANDC024 NAND controller driver");
MODULE_ALIAS("platform:ftnandc024_nand");

