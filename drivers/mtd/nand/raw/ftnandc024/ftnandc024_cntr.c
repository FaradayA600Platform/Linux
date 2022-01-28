// SPDX-License-Identifier: GPL-2.0
/*
 * FTNANDC024 NAND R/W API
 *
 * Copyright (C) 2019-2021 Faraday Technology Corp.
 */
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
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

void ftnandc024_regdump(struct nand_chip *chip)
{
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	volatile u32 val;
	u32 i;
	printk("===================================================\n");
	printk("0x0000: ");
	for(i = 0; i< 0x50; i=i+4){
		if(i != 0 && (i%0x10)==0){
			printk("\n");
			printk("0x%04x: ", i);
		}
		val = readl(data->io_base + i);
		printk("0x%08x ", val);
	}
	for(i = 0x100; i< 0x1B0; i=i+4){
		if(i != 0 && (i%0x10)==0){
			printk("\n");
			printk("0x%04x: ", i);
		}
		val = readl(data->io_base + i);
		printk("0x%08x ", val);
	}
	for(i = 0x200; i< 0x530; i=i+4){
		if(i != 0 && (i%0x10)==0){
			printk("\n");
			printk("0x%04x: ", i);
		}
		val = readl(data->io_base + i);
		printk("0x%08x ", val);
	}
	printk("\n===================================================\n");
}

static inline void ftnandc024_set_row_col_addr(
		struct ftnandc024_nand_data *data, int row, int col)
{
	int val;

	val = readl(data->io_base + MEM_ATTR_SET);
	val &= ~(0x7 << 12);
	val |= (ATTR_ROW_CYCLE(row) | ATTR_COL_CYCLE(col));

	writel(val, data->io_base + MEM_ATTR_SET);
}

static int ftnandc024_nand_check_cmdq(struct nand_chip *chip)
{
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	unsigned long timeo = jiffies;
	u32 status;
	int ret;

	ret = -EIO;
	timeo += HZ;
	while (time_before(jiffies, timeo)) {
		status = readl(data->io_base + CMDQUEUE_STATUS);
		if ((status & CMDQUEUE_STATUS_FULL(data->cur_chan)) == 0) {
			ret = 0;
			break;
		}
		cond_resched();
	}
	if(ret != 0)
		printk("check cmdq timeout");
	return ret;
}

static void ftnandc024_soft_reset(struct nand_chip *chip)
{
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);

	writel((1 << data->cur_chan), data->io_base + NANDC_SW_RESET);
	// Wait for the NANDC024 software reset is complete
	while(readl(data->io_base + NANDC_SW_RESET) & (1 << data->cur_chan)) ;
}

void ftnandc024_abort(struct nand_chip *chip)
{
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;

	// Abort the operation

	// Step1. Flush Command queue & Poll whether Command queue is ready
	writel((1 << data->cur_chan), data->io_base + CMDQUEUE_FLUSH);
	// Wait until flash is ready!!
	while (!(readl(data->io_base + DEV_BUSY) & (1 << data->cur_chan))) ;

	// Step2. Reset Nandc & Poll whether the "Reset of NANDC" returns to 0
	ftnandc024_soft_reset(chip);

	// Step3. Reset the BMC region
	writel(1 << (data->cur_chan), data->io_base + BMC_REGION_SW_RESET);

	// Step4. Reset the AHB data slave port 0
	if (readl(data->io_base + FEATURE_1) & AHB_SLAVE_MODE_ASYNC(0)) {
		writel(1 << 0, data->io_base + AHB_SLAVE_RESET);
		while(readl(data->io_base + AHB_SLAVE_RESET) & (1 << 0)) ;
	}

	// Step5. Issue the Reset cmd to flash
	cmd_f.cq1 = 0;
	cmd_f.cq2 = 0;
	cmd_f.cq3 = 0;
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type) |\
	            CMD_START_CE(data->sel_chip);
	if (data->flash_type == ONFI2 || data->flash_type == ONFI3)
		cmd_f.cq4 |= CMD_INDEX(ONFI_FIXFLOW_SYNCRESET);
	else
		cmd_f.cq4 |= CMD_INDEX(FIXFLOW_RESET);

	ftnandc024_issue_cmd(chip, &cmd_f);

	ftnandc024_nand_wait(chip);
}

int ftnandc024_issue_cmd(struct nand_chip *chip, struct cmd_feature *cmd_f)
{
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	int status;

	status = ftnandc024_nand_check_cmdq(chip);
	if (status == 0) {
		ftnandc024_set_row_col_addr(data, cmd_f->row_cycle, cmd_f->col_cycle);

		writel(cmd_f->cq1, data->io_base + CMDQUEUE1(data->cur_chan));
		writel(cmd_f->cq2, data->io_base + CMDQUEUE2(data->cur_chan));
		writel(cmd_f->cq3, data->io_base + CMDQUEUE3(data->cur_chan));
		writel(cmd_f->cq4, data->io_base + CMDQUEUE4(data->cur_chan)); // Issue cmd
	}
	return status;
}

void ftnandc024_set_default_timing(struct nand_chip *chip)
{
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	int i;
	u32 timing[4];

	timing[0] = 0x0f1f0f1f;
	timing[1] = 0x00007f7f;
	timing[2] = 0x7f7f7f7f;
	timing[3] = 0xff1f001f;

	for (i = 0;i < MAX_CHANNEL;i++) {
		writel(timing[0], data->io_base + FL_AC_TIMING0(i));
		writel(timing[1], data->io_base + FL_AC_TIMING1(i));
		writel(timing[2], data->io_base + FL_AC_TIMING2(i));
		writel(timing[3], data->io_base + FL_AC_TIMING3(i));
	}
}

int ftnandc024_nand_wait(struct nand_chip *chip)
{
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);

	unsigned long timeo;
	int ret;
	volatile u32 intr_sts, ecc_intr_sts;
	volatile u8 cmd_comp_sts, sts_fail_sts;
	volatile u8 ecc_sts_for_data;
	volatile u8 ecc_sts_for_spare;

	data->cmd_status = CMD_SUCCESS;
	ret = NAND_STATUS_FAIL;
	timeo = jiffies;
	timeo += 5 * HZ;

	//No command
	if (readl(data->io_base + CMDQUEUE_STATUS) & CMDQUEUE_STATUS_EMPTY(data->cur_chan)) {
		ret = NAND_STATUS_READY;
		goto out;
	}

	do {
		intr_sts = readl(data->io_base + INTR_STATUS);
		cmd_comp_sts = ((intr_sts & 0xFF0000) >> 16);

		if (likely(cmd_comp_sts & (1 << (data->cur_chan)))) {
			// Clear the intr status when the cmd complete occurs.
			writel(intr_sts, data->io_base + INTR_STATUS);

			ret = NAND_STATUS_READY;
			sts_fail_sts = (intr_sts & 0xFF);

			if (sts_fail_sts & (1 << (data->cur_chan))) {
				printk(KERN_ERR "STATUS FAIL@(pg_addr:0x%x)\n", data->page_addr);
				data->cmd_status |= CMD_STATUS_FAIL;
				ret = CMD_STATUS_FAIL;
				ftnandc024_abort(chip);
			}

			if (data->read_state) {
				ecc_intr_sts = readl(data->io_base + ECC_INTR_STATUS);
				// Clear the ECC intr status
				writel(ecc_intr_sts, data->io_base + ECC_INTR_STATUS);
				// ECC failed on data
				ecc_sts_for_data = (ecc_intr_sts & 0xFF);
				if (ecc_sts_for_data & (1 << data->cur_chan)) {
					data->cmd_status |= CMD_ECC_FAIL_ON_DATA;
					ret = NAND_STATUS_FAIL;
				}

				ecc_sts_for_spare = ((ecc_intr_sts & 0xFF0000) >> 16);
				// ECC failed on spare
				if (ecc_sts_for_spare & (1 << data->cur_chan)) {
					data->cmd_status |= CMD_ECC_FAIL_ON_SPARE;
					ret = NAND_STATUS_FAIL;
				}
			}
			goto out;
		}
		cond_resched();
	} while (time_before(jiffies, timeo));

	DBGLEVEL1(ftnandc024_dbg("nand wait time out\n"));
	ftnandc024_regdump(chip);
out:
	return ret;
}

void ftnandc024_fill_prog_code(struct nand_chip *chip, int location,
		int cmd_index) {
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	writeb((cmd_index & 0xff), data->io_base + PROGRAMMABLE_OPCODE+location);
}

void ftnandc024_fill_prog_flow(struct nand_chip *chip, int *program_flow_buf,
		int buf_len) {
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	u8 *p = (u8 *)program_flow_buf;

	int i;
	for(i = 0; i < buf_len; i++) {
		writeb( *(p+i), data->io_base + PROGRAMMABLE_FLOW_CONTROL + i);
	}
}

int byte_rd(struct nand_chip *chip, int real_pg, int col, int len,
				u_char *spare_buf)
{
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	struct cmd_feature cmd_f = {0};
	int status, i, tmp_col, tmp_len, cmd_len, ret;
	u_char *buf;

	ret = 0;
	tmp_col = col;
	tmp_len = len;

	if(data->flash_type == TOGGLE1 || data->flash_type == TOGGLE2 ||
		data->flash_type == ONFI2 || data->flash_type == ONFI3) {
		if(col & 0x1) {
			tmp_col --;

			if (tmp_len & 0x1)
				tmp_len ++;
			else
				tmp_len += 2;
		}
		else if(tmp_len & 0x1) {
			tmp_len ++;
		}
	}

	buf = (u_char *)vmalloc(tmp_len);

	for(i = 0; i < tmp_len; i += data->max_spare) {
		if(tmp_len - i >= data->max_spare)
			cmd_len = data->max_spare;
		else
			cmd_len = tmp_len - i;

		cmd_f.row_cycle = ROW_ADDR_3CYCLE;
		cmd_f.col_cycle = COL_ADDR_2CYCLE;
		cmd_f.cq1 = real_pg | SCR_SEED_VAL1(data->seed_val);
		cmd_f.cq2 = CMD_EX_SPARE_NUM(cmd_len) | SCR_SEED_VAL2(data->seed_val);
		cmd_f.cq3 = CMD_COUNT(1) | tmp_col;
		cmd_f.cq4 = CMD_COMPLETE_EN | CMD_BYTE_MODE |\
				CMD_FLASH_TYPE(data->flash_type) |\
				CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(cmd_len) |\
				CMD_INDEX(LARGE_FIXFLOW_BYTEREAD);

		status = ftnandc024_issue_cmd(chip, &cmd_f);
		if(status < 0) {
			ret = 1;
			break;
		}
		ftnandc024_nand_wait(chip);
		memcpy(buf + i, data->io_base + SPARE_SRAM + (data->cur_chan << data->spare_ch_offset), cmd_len);

		tmp_col += cmd_len;
	}

	if(data->flash_type == TOGGLE1 || data->flash_type == TOGGLE2 ||
		data->flash_type == ONFI2 || data->flash_type == ONFI3) {
		if(col & 0x1)
			memcpy(spare_buf, buf + 1, len);
		else
			memcpy(spare_buf, buf, len);
	}
	else
		memcpy(spare_buf, buf, len);

	vfree(buf);
	return ret;
}

int rd_pg_w_oob(struct nand_chip *chip, int real_pg,
			uint8_t *data_buf, u8 *spare_buf)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;
	int status;
	#ifndef CONFIG_FTNANDC024_USE_AHBDMA
	int i;
	u32 *lbuf;
	#endif

	cmd_f.row_cycle = ROW_ADDR_3CYCLE;
	cmd_f.col_cycle = COL_ADDR_2CYCLE;
	cmd_f.cq1 = real_pg | SCR_SEED_VAL1(data->seed_val);
	cmd_f.cq2 = CMD_EX_SPARE_NUM(data->spare) | SCR_SEED_VAL2(data->seed_val);
	cmd_f.cq3 = CMD_COUNT(mtd->writesize >> data->eccbasft) |
			(data->column & 0xFF);
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type) |\
			CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(data->spare) |\
			CMD_INDEX(LARGE_FIXFLOW_PAGEREAD_W_SPARE);
	#ifdef CONFIG_FTNANDC024_USE_AHBDMA
	cmd_f.cq4 |= CMD_DMA_HANDSHAKE_EN;
	#endif

	status = ftnandc024_issue_cmd(chip, &cmd_f);
	if(status < 0)
		return 1;

	#ifdef CONFIG_FTNANDC024_USE_AHBDMA
	if(ftnandc024_nand_dma_rd(chip, data_buf)) {
		ftnandc024_abort(chip);
		return 1;
	}
	#else
	lbuf = (u32 *) data_buf;
	for (i = 0; i < mtd->writesize; i += 4)
		*lbuf++ = *(volatile unsigned *)(chip->legacy.IO_ADDR_R);
	#endif

	ftnandc024_nand_wait(chip);
	memcpy(spare_buf, data->io_base + SPARE_SRAM + (data->cur_chan << data->spare_ch_offset), mtd->oobsize); 

	return 0;
}

int rd_pg_w_oob_sp(struct nand_chip *chip, int real_pg,
			uint8_t *data_buf, u8 *spare_buf)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;
	int status;
	#ifndef CONFIG_FTNANDC024_USE_AHBDMA
	u32 *lbuf;
	int i;
	#endif

	cmd_f.row_cycle = ROW_ADDR_2CYCLE;
	cmd_f.col_cycle = COL_ADDR_1CYCLE;
	cmd_f.cq1 = real_pg | SCR_SEED_VAL1(data->seed_val);
	cmd_f.cq2 = SCR_SEED_VAL2(data->seed_val);
	cmd_f.cq3 = CMD_COUNT(1) | (data->column & 0xFF);
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type) |\
			CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(data->spare) |\
			CMD_INDEX(SMALL_FIXFLOW_PAGEREAD);
	#ifdef CONFIG_FTNANDC024_USE_AHBDMA
	cmd_f.cq4 |= CMD_DMA_HANDSHAKE_EN;
	#endif

	status = ftnandc024_issue_cmd(chip, &cmd_f);
	if (status < 0)
		return 1;

	#ifdef CONFIG_FTNANDC024_USE_AHBDMA
	if (ftnandc024_nand_dma_rd(chip, data_buf)) {
		ftnandc024_abort(chip);
		return 1;
	}
	#else
	lbuf = (u32 *)data_buf;
	for (i = 0; i < mtd->writesize; i += 4)
		*lbuf++ = *(volatile unsigned *)(chip->legacy.IO_ADDR_R);
	#endif

	ftnandc024_nand_wait(chip);

	memcpy(spare_buf, data->io_base + SPARE_SRAM + (data->cur_chan << data->spare_ch_offset), mtd->oobsize);

	return 0;
}

int rd_oob(struct nand_chip *chip, int real_pg, u8 *spare_buf)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;
	int status;

	cmd_f.row_cycle = ROW_ADDR_3CYCLE;
	cmd_f.col_cycle = COL_ADDR_2CYCLE;
	cmd_f.cq1 = real_pg | SCR_SEED_VAL1(data->seed_val);
	cmd_f.cq2 = CMD_EX_SPARE_NUM(data->spare) | SCR_SEED_VAL2(data->seed_val);
	cmd_f.cq3 = CMD_COUNT(1);
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type) |\
			CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(data->spare) |\
			CMD_INDEX(LARGE_FIXFLOW_READOOB);

	status = ftnandc024_issue_cmd(chip, &cmd_f);
	if (status < 0)
		return 1;

	ftnandc024_nand_wait(chip);
	memcpy(spare_buf, data->io_base + SPARE_SRAM + (data->cur_chan << data->spare_ch_offset), mtd->oobsize);
	return 0;
}

static int rd_oob_sp(struct nand_chip *chip, int real_pg, u8 *spare_buf)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;
	int status;

	cmd_f.row_cycle = ROW_ADDR_2CYCLE;
	cmd_f.col_cycle = COL_ADDR_1CYCLE;
	cmd_f.cq1 = real_pg | SCR_SEED_VAL1(data->seed_val);
	cmd_f.cq2 = SCR_SEED_VAL2(data->seed_val);
	cmd_f.cq3 = CMD_COUNT(1);
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type) |\
			CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(data->spare) |\
			CMD_INDEX(SMALL_FIXFLOW_READOOB);

	status = ftnandc024_issue_cmd(chip, &cmd_f);
	if (status < 0)
		return 1;

	ftnandc024_nand_wait(chip);

	memcpy(spare_buf, data->io_base + SPARE_SRAM + (data->cur_chan << data->spare_ch_offset), mtd->oobsize);

	return 0;
}

int rd_pg(struct nand_chip *chip, int real_pg, uint8_t *data_buf)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;
	int status;
	#ifndef CONFIG_FTNANDC024_USE_AHBDMA
	u32 *lbuf;
	int i;
	#endif

	cmd_f.row_cycle = ROW_ADDR_3CYCLE;
	cmd_f.col_cycle = COL_ADDR_2CYCLE;
	cmd_f.cq1 = real_pg | SCR_SEED_VAL1(data->seed_val);
	cmd_f.cq2 = CMD_EX_SPARE_NUM(data->spare) | SCR_SEED_VAL2(data->seed_val);
	cmd_f.cq3 = CMD_COUNT(mtd->writesize >> data->eccbasft) | (data->column & 0xFF);
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type) |\
			CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(data->spare) |\
			CMD_INDEX(LARGE_FIXFLOW_PAGEREAD);
	#ifdef CONFIG_FTNANDC024_USE_AHBDMA
	cmd_f.cq4 |= CMD_DMA_HANDSHAKE_EN;
	#endif

	status = ftnandc024_issue_cmd(chip, &cmd_f);
	if(status < 0)
		return 1;

	#ifdef CONFIG_FTNANDC024_USE_AHBDMA
	if(ftnandc024_nand_dma_rd(chip, data_buf)) {
		ftnandc024_abort(chip);
		return 1;
	}
	#else
	lbuf = (u32 *) data_buf;
	for (i = 0; i < mtd->writesize; i += 4)
		*lbuf++ = *(volatile unsigned *)(chip->legacy.IO_ADDR_R);
	#endif

	ftnandc024_nand_wait(chip);

	return 0;
}

int rd_pg_sp(struct nand_chip *chip, int real_pg, uint8_t *data_buf)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;
	int progflow_buf[3];
	int status;
	#ifndef CONFIG_FTNANDC024_USE_AHBDMA
	int i;
	u32 *lbuf;
	#endif

	cmd_f.row_cycle = ROW_ADDR_2CYCLE;
	cmd_f.col_cycle = COL_ADDR_1CYCLE;
	cmd_f.cq1 = real_pg | SCR_SEED_VAL1(data->seed_val);
	cmd_f.cq2 = SCR_SEED_VAL2(data->seed_val);
	cmd_f.cq3 = CMD_COUNT(1) | (data->column & 0xFF);
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type) |\
			CMD_START_CE(data->sel_chip);
	#ifdef CONFIG_FTNANDC024_USE_AHBDMA
	cmd_f.cq4 |= CMD_DMA_HANDSHAKE_EN;
	#endif

	progflow_buf[0] = 0x66414200;
	progflow_buf[1] = 0x66626561;
	progflow_buf[2] = 0x000067C8;
	ftnandc024_fill_prog_flow(chip, progflow_buf, 10);
	cmd_f.cq4 |= CMD_PROM_FLOW | CMD_INDEX(0x0);

	status = ftnandc024_issue_cmd(chip, &cmd_f);
	if (status < 0)
		return 1;

	#ifdef CONFIG_FTNANDC024_USE_AHBDMA
	if (ftnandc024_nand_dma_rd(chip, data_buf)) {
		ftnandc024_abort(chip);
		return 1;
	}
	#else
	lbuf = (u32 *)data_buf;
	for (i = 0; i < mtd->writesize; i += 4)
		*lbuf++ = *(volatile unsigned *)(chip->legacy.IO_ADDR_R);
	#endif

	ftnandc024_nand_wait(chip);

	return 0;
}

int ftnandc024_check_bad_spare(struct nand_chip *chip, int pg)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	int spare_phy_start, spare_phy_len, eccbyte;
	int errbit_num, i, j, ret;
	int sec_num = (mtd->writesize >> data->eccbasft);
	u_char *spare_buf;
	int ecc_corr_bit_spare, chan;

	eccbyte = (data->useecc * 14) / 8;
	if (((data->useecc * 14) % 8) != 0)
		eccbyte++;

	// The amount of data-payload in each (sector+sector_parity) or
	// (spare + spare_parity) on Toggle/ONFI mode must be even.
	if(data->flash_type == TOGGLE1 || data->flash_type == TOGGLE2 ||
		data->flash_type == ONFI2 || data->flash_type == ONFI3) {
		if (eccbyte & 0x1)
			eccbyte ++;
	}
#if (CONFIG_FTNANDC024_VERSION >= 230)
	spare_phy_start = mtd->writesize + (eccbyte * sec_num) + 1;
#else
	spare_phy_start = mtd->writesize + (eccbyte * sec_num);
#endif

	eccbyte = (data->useecc_spare * 14) / 8;
	if (((data->useecc_spare * 14) % 8) != 0)
		eccbyte++;

	if(data->flash_type == TOGGLE1 || data->flash_type == TOGGLE2 ||
		data->flash_type == ONFI2 || data->flash_type == ONFI3) {
		if (eccbyte & 0x1)
			eccbyte ++;
	}
	spare_phy_len = data->spare + eccbyte;
	spare_buf = vmalloc(spare_phy_len);

	ret = 0;
	errbit_num = 0;

	if(!byte_rd(chip, pg, spare_phy_start, spare_phy_len, spare_buf)) {

		for(i = 0; i < spare_phy_len; i++) {
			if(*(spare_buf + i) != 0xFF) {
				for(j = 0; j < 8; j ++) {
					if((*(spare_buf + i) & (0x1 << j)) == 0)
						errbit_num ++;
				}
			}
		}
		if (errbit_num != 0) {
			if (data->cur_chan < 4) {
				chan = (data->cur_chan << 3);
				ecc_corr_bit_spare = (readl(data->io_base + ECC_CORRECT_BIT_FOR_SPARE_REG1) >> chan) & 0x7F;
			}
			else {
				chan = (data->cur_chan - 4) << 3;
				ecc_corr_bit_spare = (readl(data->io_base + ECC_CORRECT_BIT_FOR_SPARE_REG2) >> chan) & 0x7F;
			}
			printk("spare_phy_len = %d, errbit_num = %d\n", spare_phy_len, errbit_num);

			if(errbit_num > ecc_corr_bit_spare + 1)
				ret = 1;
		}
	}
	else
		ret = 1;

	vfree(spare_buf);
	return ret;

}


int ftnandc024_nand_read_page(struct nand_chip *chip,
		uint8_t * buf, int oob_required, int page)
{
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);

	data->page_addr = page;

	return data->read_page(chip, buf);
}

int ftnandc024_nand_write_page_lowlevel(
		struct nand_chip *chip, const uint8_t *buf,
		int oob_required, int page)
{
//	struct mtd_info *mtd = nand_to_mtd(chip);
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	int status = 0;

//	DBGLEVEL2(ftnandc024_dbg ("w_2: ch = %d, ce = %d, page = 0x%x, size = %d, data->column = %d\n", 
//				data->cur_chan, data->sel_chip,  page, mtd->writesize, data->column));
	data->page_addr = page;
	status = data->write_page(chip, buf);
	if (status < 0)
		return status;

	// Returning the any value isn't allowed, except 0, -EBADMSG, or -EUCLEAN
	return 0;
}

int ftnandc024_nand_read_oob_std(struct nand_chip *chip, int page)
{
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);

	data->page_addr = page;

	return data->read_oob(chip, chip->oob_poi);
}

int ftnandc024_nand_write_oob_std(struct nand_chip *chip, int page)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);

	DBGLEVEL2(ftnandc024_dbg("write oob only to page = 0x%x\n", page));
	data->page_addr = page;

	return data->write_oob(chip, chip->oob_poi, mtd->oobsize);
}

int ftnandc024_nand_read_page_lp(struct nand_chip *chip, uint8_t *buf)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	int status = 0, chk_data_0xFF, chk_spare_0xFF;
	int i, ecc_original_setting, generic_original_setting, val;
	int real_pg;
	u8  data_empty, spare_empty;
	u32 *lbuf;

	#if defined(CONFIG_FTNANDC024_TOSHIBA_TC58NVG4T2ETA00) ||\
		defined(CONFIG_FTNANDC024_SAMSUNG_K9ABGD8U0B)
	int real_blk_nm, real_off;
	real_blk_nm = data->page_addr / (mtd->erasesize/mtd->writesize);
	real_off = data->page_addr % (mtd->erasesize/mtd->writesize);
	real_pg = (real_blk_nm * data->block_boundary) + real_off;
	#else
	real_pg = data->page_addr;
	#endif

	DBGLEVEL2(ftnandc024_dbg
		("r: ch = %d, ce = %d, page = 0x%x, real = 0x%x, size = %d, data->column = %d\n",
		data->cur_chan, data->sel_chip, data->page_addr, real_pg, mtd->writesize, data->column));

	data->read_state = 1;
retry:
	chk_data_0xFF = chk_spare_0xFF = 0;
	data_empty = spare_empty = 0;

	if(!rd_pg_w_oob(chip, real_pg, buf, chip->oob_poi)) {

		if (data->cmd_status &
			(CMD_ECC_FAIL_ON_DATA | CMD_ECC_FAIL_ON_SPARE)) {
			// Store the original setting
			ecc_original_setting = readl(data->io_base + ECC_CONTROL);
			generic_original_setting = readl(data->io_base + GENERAL_SETTING);
			// Disable the ECC engine & HW-Scramble, temporarily.
			val = readl(data->io_base + ECC_CONTROL);
			val = val & ~(ECC_EN(0xFF));
			writel(val, data->io_base + ECC_CONTROL);
			val = readl(data->io_base + GENERAL_SETTING);
			val &= ~DATA_SCRAMBLER;
			writel(val, data->io_base + GENERAL_SETTING);

			if(data->cmd_status==(CMD_ECC_FAIL_ON_DATA|CMD_ECC_FAIL_ON_SPARE))
			{
				if(!rd_pg_w_oob(chip, real_pg, buf, chip->oob_poi)) {
					chk_data_0xFF = chk_spare_0xFF = 1;
					data_empty = spare_empty = 1;
				}
			}
			else if(data->cmd_status == CMD_ECC_FAIL_ON_DATA) {
				if(!rd_pg(chip, real_pg, buf)) {
					chk_data_0xFF = 1;
					data_empty = 1;
				}
			}
			else if(data->cmd_status == CMD_ECC_FAIL_ON_SPARE) {
				if(!rd_oob(chip, real_pg, chip->oob_poi)) {
					chk_spare_0xFF = 1;
					spare_empty = 1;
				}
			}

			// Restore the ecc original setting & generic original setting.
			writel(ecc_original_setting, data->io_base + ECC_CONTROL);
			writel(generic_original_setting, data->io_base + GENERAL_SETTING);

			if(chk_data_0xFF == 1) {

				lbuf = (int *)buf;
				for (i = 0; i < (mtd->writesize >> 2); i++) {
					if (*(lbuf + i) != 0xFFFFFFFF) {
						printk(KERN_ERR "ECC err @ page0x%x real:0x%x\n",
							data->page_addr, real_pg);
						data_empty = 0;
						break;
					}
				}
				if(data_empty == 1)
					DBGLEVEL2(ftnandc024_dbg("Data Real 0xFF\n"));
			}

			if(chk_spare_0xFF == 1) {
				//lichun@add, If BI_byte test
				if (readl(data->io_base + MEM_ATTR_SET) & BI_BYTE_MASK) {
					for (i = 0; i < mtd->oobsize; i++) {
						if (*(chip->oob_poi + i) != 0xFF) {
							printk(KERN_ERR "ECC err for spare(Read page) @");
							printk(KERN_ERR	"ch:%d ce:%d page0x%x real:0x%x\n",
								data->cur_chan, data->sel_chip, data->page_addr, real_pg);
							spare_empty = 0;
							break;
						}
					}
				}
				else {
				//~lichun
				if(ftnandc024_check_bad_spare(chip, data->page_addr)) {
					printk(KERN_ERR "ECC err for spare(Read page) @");
					printk(KERN_ERR	"ch:%d ce:%d page0x%x real:0x%x\n",
						data->cur_chan, data->sel_chip, data->page_addr, real_pg);
					spare_empty = 0;
				}
				}

				if(spare_empty == 1)
					DBGLEVEL2(ftnandc024_dbg("Spare Real 0xFF\n"));
			}

			if( (chk_data_0xFF == 1 && data_empty == 0) ||
				(chk_spare_0xFF == 1 && spare_empty == 0) ) {

				if(data->set_param != NULL) {
					if(data->set_param(chip) == 1)
						goto retry;
				}

				mtd->ecc_stats.failed++;
				status = -1;
			}
		}
	}
	data->read_state = 0;

	if(data->terminate != NULL)
		data->terminate(chip);

	// Returning the any value isn't allowed, except 0, -EBADMSG, or -EUCLEAN
	return 0;
}


int ftnandc024_nand_write_page_lp(struct nand_chip *chip, const uint8_t * buf)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;
	u8 *p, w_wo_spare = 1;
	#ifndef CONFIG_FTNANDC024_USE_AHBDMA
	u32 *lbuf;
	#endif
	int real_pg;
	int i, status = 0;

	#if defined(CONFIG_FTNANDC024_TOSHIBA_TC58NVG4T2ETA00) ||\
		defined(CONFIG_FTNANDC024_SAMSUNG_K9ABGD8U0B)
	int real_blk_nm, real_off;
	real_blk_nm = data->page_addr / (mtd->erasesize/mtd->writesize);
	real_off = data->page_addr % (mtd->erasesize/mtd->writesize);
	real_pg = (real_blk_nm * data->block_boundary) + real_off;
	#else
	real_pg = data->page_addr;
	#endif

	DBGLEVEL2(ftnandc024_dbg (
		"w: ch = %d, ce = %d, page = 0x%x, real page:0x%x size = %d, data->column = %d\n",
		data->cur_chan, data->sel_chip,  data->page_addr, real_pg, mtd->writesize, data->column));

	p = chip->oob_poi;
	for(i = 0; i < mtd->oobsize; i++) {
		if( *( p + i) != 0xFF) {
			w_wo_spare = 0;
			break;
		}
	}

	cmd_f.row_cycle = ROW_ADDR_3CYCLE;
	cmd_f.col_cycle = COL_ADDR_2CYCLE;
	cmd_f.cq1 = real_pg | SCR_SEED_VAL1(data->seed_val);
	cmd_f.cq2 = CMD_EX_SPARE_NUM(data->spare) | SCR_SEED_VAL2(data->seed_val);
	cmd_f.cq3 = CMD_COUNT(mtd->writesize >> data->eccbasft) |
			(data->column & 0xFF);
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type) | \
			CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(data->spare);
	#ifdef CONFIG_FTNANDC024_USE_AHBDMA
	cmd_f.cq4 |= CMD_DMA_HANDSHAKE_EN;
	#endif

	if(w_wo_spare == 0) {
		memcpy((data->io_base + SPARE_SRAM + (data->cur_chan << data->spare_ch_offset)), p, mtd->oobsize);
		cmd_f.cq4 |= CMD_INDEX(LARGE_PAGEWRITE_W_SPARE);
	}
	else {
		cmd_f.cq4 |= CMD_INDEX(LARGE_PAGEWRITE);
	}

	status = ftnandc024_issue_cmd(chip, &cmd_f);
	if (status < 0)
		goto out;

	#ifdef CONFIG_FTNANDC024_USE_AHBDMA
	if (ftnandc024_nand_dma_wr(chip, buf)) {
		ftnandc024_abort(chip);
		status = -EIO;
		goto out;
	}
	#else
	lbuf = (u32 *) buf;
	for (i = 0; i < mtd->writesize; i += 4)
		*(volatile unsigned *)(chip->legacy.IO_ADDR_R) = *lbuf++;
	#endif

	if (ftnandc024_nand_wait(chip) == NAND_STATUS_FAIL) {
		status = -EIO;
		printk("FAILED\n");
	}
out:
	return status;
}

int ftnandc024_nand_read_oob_lp(struct nand_chip *chip, u8 *buf)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	int status = 0, i, ecc_original_setting, generic_original_setting, val;
	int real_pg, empty;

	#if defined(CONFIG_FTNANDC024_TOSHIBA_TC58NVG4T2ETA00) ||\
		defined(CONFIG_FTNANDC024_SAMSUNG_K9ABGD8U0B)
	int real_blk_nm, real_off;
	real_blk_nm = data->page_addr / (mtd->erasesize/mtd->writesize);
	real_off = data->page_addr % (mtd->erasesize/mtd->writesize);
	real_pg = (real_blk_nm * data->block_boundary) + real_off;
	#else
	real_pg = data->page_addr;
	#endif

	DBGLEVEL2(ftnandc024_dbg(
		"read_oob: ch = %d, ce = %d, page = 0x%x, real: 0x%x, size = %d\n",
		data->cur_chan, data->sel_chip, data->page_addr, real_pg, mtd->writesize));

	data->read_state = 1;
	if(!rd_oob(chip, real_pg, buf)) {
		if(data->cmd_status & CMD_ECC_FAIL_ON_SPARE) {
			// Store the original setting
			ecc_original_setting = readl(data->io_base + ECC_CONTROL);
			generic_original_setting = readl(data->io_base + GENERAL_SETTING);
			// Disable the ECC engine & HW-Scramble, temporarily.
			val = readl(data->io_base + ECC_CONTROL);
			val = val & ~(ECC_EN(0xFF));
			writel(val, data->io_base + ECC_CONTROL);
			val = readl(data->io_base + GENERAL_SETTING);
			val &= ~DATA_SCRAMBLER;
			writel(val, data->io_base + GENERAL_SETTING);

			if(!rd_oob(chip, real_pg, buf)) {
				empty = 1;
				for (i = 0; i < mtd->oobsize; i++) {
					if (*(buf + i) != 0xFF) {
						printk(KERN_ERR "ECC err for spare(Read oob) @");
						printk(KERN_ERR	"ch:%d ce:%d page0x%x real:0x%x\n",
							data->cur_chan, data->sel_chip, data->page_addr, real_pg);
						mtd->ecc_stats.failed++;
						status = -1;
						empty = 0;
						break;
					}
				}
				if (empty == 1)
					DBGLEVEL2(ftnandc024_dbg("Spare real 0xFF"));
			}
			// Restore the ecc original setting & generic original setting.
			writel(ecc_original_setting, data->io_base + ECC_CONTROL);
			writel(generic_original_setting, data->io_base + GENERAL_SETTING);
		}
	}
	data->read_state = 0;

	// Returning the any value isn't allowed, except 0, -EBADMSG, or -EUCLEAN
	return 0;
}

int ftnandc024_nand_write_oob_lp(struct nand_chip *chip, u8 *buf, int len)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;
	int status = 0, real_pg;

	#if defined(CONFIG_FTNANDC024_TOSHIBA_TC58NVG4T2ETA00) ||\
		defined(CONFIG_FTNANDC024_SAMSUNG_K9ABGD8U0B)
	int real_blk_nm, real_off;
	real_blk_nm = data->page_addr / (mtd->erasesize/mtd->writesize);
	real_off = data->page_addr % (mtd->erasesize/mtd->writesize);
	real_pg = (real_blk_nm * data->block_boundary) + real_off;
	#else
	real_pg = data->page_addr;
	#endif

	DBGLEVEL2(ftnandc024_dbg(
		"write_oob: ch = %d, ce = %d, page = 0x%x, real page:0x%x, sz = %d, oobsz = %d\n",
		data->cur_chan, data->sel_chip, data->page_addr, real_pg, mtd->writesize, mtd->oobsize));

	memcpy(data->io_base + SPARE_SRAM + (data->cur_chan << data->spare_ch_offset), buf, mtd->oobsize);

	cmd_f.row_cycle = ROW_ADDR_3CYCLE;
	cmd_f.col_cycle = COL_ADDR_2CYCLE;
	cmd_f.cq1 = real_pg | SCR_SEED_VAL1(data->seed_val);
	cmd_f.cq2 = CMD_EX_SPARE_NUM(data->spare) | SCR_SEED_VAL2(data->seed_val);
	cmd_f.cq3 = CMD_COUNT(1);
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type) |\
			CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(data->spare) |\
			CMD_INDEX(LARGE_FIXFLOW_WRITEOOB);

	status = ftnandc024_issue_cmd(chip, &cmd_f);
	if (status < 0)
		goto out;

	if (ftnandc024_nand_wait(chip) == NAND_STATUS_FAIL) {
		status = -EIO;
	}
out:
	// Returning the any value isn't allowed, except 0, -EBADMSG, or -EUCLEAN
	return status;
}

int ftnandc024_nand_read_page_sp(struct nand_chip *chip, uint8_t *buf)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	int status = 0;
	int i, ecc_original_setting, val;
	int chk_data_0xFF, chk_spare_0xFF, empty;
	u8 *p;
	u32 *lbuf;

	DBGLEVEL2(ftnandc024_dbg("smallr: ch = %d, ce = %d, page = 0x%x, size = %d, data->column = %d\n",
			data->cur_chan, data->sel_chip, data->page_addr, mtd->writesize, data->column));

	data->read_state = 1;
	chk_data_0xFF = chk_spare_0xFF = 0;

	if(!rd_pg_w_oob_sp(chip, data->page_addr, buf, chip->oob_poi)) {
		if(data->cmd_status & (CMD_ECC_FAIL_ON_DATA | CMD_ECC_FAIL_ON_SPARE)) {
			// Store the original setting
			ecc_original_setting = readl(data->io_base + ECC_CONTROL);
			// Disable the ECC engine & HW-Scramble, temporarily.
			val = readl(data->io_base + ECC_CONTROL);
			val = val & ~(ECC_EN(0xFF));
			writel(val, data->io_base + ECC_CONTROL);

			if(data->cmd_status == (CMD_ECC_FAIL_ON_DATA | CMD_ECC_FAIL_ON_SPARE)) {
				if(!rd_pg_w_oob_sp(chip, data->page_addr, buf, chip->oob_poi)) {
					chk_data_0xFF = chk_spare_0xFF = 1;
				}
			}
			else if(data->cmd_status == CMD_ECC_FAIL_ON_DATA) {
				if(!rd_pg_sp(chip, data->page_addr, buf)) {
					chk_data_0xFF = 1;
				}
			}
			else if(data->cmd_status == CMD_ECC_FAIL_ON_SPARE) {
				if(!rd_oob_sp(chip, data->page_addr, chip->oob_poi)) {
					chk_spare_0xFF = 1;
				}
			}
			// Restore the ecc original setting & generic original setting.
			writel(ecc_original_setting, data->io_base + ECC_CONTROL);

			if(chk_data_0xFF == 1) {
				lbuf = (u32 *)buf;
				empty = 1;
				for (i = 0; i < (mtd->writesize >> 2); i++) {
					if (*(lbuf + i) != 0xFFFFFFFF) {
						printk(KERN_ERR "ECC err @ page0x%x\n", data->page_addr); 
						ftnandc024_regdump(chip);
						mtd->ecc_stats.failed++;
						status = -1;
						empty = 0;
						break;
					}
				}
				if (empty == 1)
					DBGLEVEL2(ftnandc024_dbg("Data Real 0xFF\n"));
			}
			if(chk_spare_0xFF == 1) {
				p = chip->oob_poi;
				empty = 1;
				for (i = 0; i < mtd->oobsize; i++) {
					if (*(p + i) != 0xFF) {
						printk(KERN_ERR"ECC err for spare(Read page) @ page0x%x\n", data->page_addr);
						mtd->ecc_stats.failed++;
						status = -1;
						empty = 0;
						break;
					}
				}
				if (empty == 1)
					DBGLEVEL2(ftnandc024_dbg("Spare Real 0xFF\n"));
			}
		}
	}
	data->read_state = 0;

	// Returning the any value isn't allowed, except 0, -EBADMSG, or -EUCLEAN
	return 0;
}

int ftnandc024_nand_write_page_sp(struct nand_chip *chip, const uint8_t * buf)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;
	int i;
	int status = 0;
	u8 *p, w_wo_spare = 1;
	int progflow_buf[3];
	#ifndef CONFIG_FTNANDC024_USE_AHBDMA
	u32 *lbuf;
	#endif

	DBGLEVEL2(ftnandc024_dbg ("smallw: ch = %d, ce = %d, page = 0x%x, size = %d, data->column = %d\n", 
				data->cur_chan, data->sel_chip,  data->page_addr, mtd->writesize, data->column));

	p = chip->oob_poi;
	for(i = 0; i < mtd->oobsize; i++) {
		if( *( p + i) != 0xFF) {
			w_wo_spare = 0;
			break;
		}
	}

	cmd_f.row_cycle = ROW_ADDR_2CYCLE;
	cmd_f.col_cycle = COL_ADDR_1CYCLE;
	cmd_f.cq1 = data->page_addr | SCR_SEED_VAL1(data->seed_val);
	cmd_f.cq2 = SCR_SEED_VAL2(data->seed_val);
	cmd_f.cq3 = CMD_COUNT(1) | (data->column / mtd->writesize);
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type) |\
			CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(data->spare);
	#ifdef CONFIG_FTNANDC024_USE_AHBDMA
	cmd_f.cq4 |= CMD_DMA_HANDSHAKE_EN;
	#endif

	if(w_wo_spare == 0) {
		memcpy((data->io_base + SPARE_SRAM + (data->cur_chan << data->spare_ch_offset)), chip->oob_poi, mtd->oobsize);
		cmd_f.cq4 |= CMD_INDEX(SMALL_FIXFLOW_PAGEWRITE);
	}
	else {
		progflow_buf[0] = 0x41421D00;
		progflow_buf[1] = 0x66086460;
		progflow_buf[2] = 0x000067C7;
		ftnandc024_fill_prog_flow(chip, progflow_buf, 10);
		cmd_f.cq4 |= CMD_PROM_FLOW | CMD_INDEX(0x0);
	}

	status = ftnandc024_issue_cmd(chip, &cmd_f);
	if (status < 0)
		goto out;

	#ifdef CONFIG_FTNANDC024_USE_AHBDMA
	if (ftnandc024_nand_dma_wr(chip, buf)) {
		ftnandc024_abort(chip);
		status = -EIO;
		goto out;
	}
	#else
	lbuf = (u32 *) buf;
	for (i = 0; i < mtd->writesize; i += 4)
		*(volatile unsigned *)(chip->legacy.IO_ADDR_R) = *lbuf++;
	#endif

	if (ftnandc024_nand_wait(chip) == NAND_STATUS_FAIL) {
		status = -EIO;
		printk("FAILED\n");
	}
out:
	// Returning the any value isn't allowed, except 0, -EBADMSG, or -EUCLEAN
	return status;
}

int ftnandc024_nand_read_oob_sp(struct nand_chip *chip, u8 *buf)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	int i, status = 0;
	int val, ecc_original_setting, empty;

	DBGLEVEL2(ftnandc024_dbg("smallread_oob: ch = %d, ce = %d, page = 0x%x, size = %d\n",
				data->cur_chan, data->sel_chip, data->page_addr, mtd->writesize));

	data->read_state = 1;
	if(!rd_oob_sp(chip, data->page_addr, buf)) {
		if(data->cmd_status & CMD_ECC_FAIL_ON_SPARE) {
			// Store the original setting
			ecc_original_setting = readl(data->io_base + ECC_CONTROL);
			// Disable the ECC engine & HW-Scramble, temporarily.
			val = readl(data->io_base + ECC_CONTROL);
			val = val & ~(ECC_EN(0xFF));
			writel(val, data->io_base + ECC_CONTROL);

			if(!rd_oob_sp(chip, data->page_addr, buf)) {
				empty = 1;
				for (i = 0; i < mtd->oobsize; i++) {
					if (*(buf + i) != 0xFF) {
						printk(KERN_ERR "ECC err for spare(Read oob) @ page0x%x\n",
								data->page_addr);
						mtd->ecc_stats.failed++;
						status = -1;
						empty = 0;
						break;
					}
				}
				if(empty == 1)
					DBGLEVEL2(ftnandc024_dbg("Spare real 0xFF"));
			}
			// Restore the ecc original setting & generic original setting.
			writel(ecc_original_setting, data->io_base + ECC_CONTROL);
		}
	}
	data->read_state = 0;

	// Returning the any value isn't allowed, except 0, -EBADMSG, or -EUCLEAN
	return 0;
}

int ftnandc024_nand_write_oob_sp(struct nand_chip *chip, u8 *buf, int len)
{
//	struct mtd_info *mtd = nand_to_mtd(chip);
	struct ftnandc024_nand_data *data = nand_get_controller_data(chip);
	struct cmd_feature cmd_f;
	int status = 0;

//	DBGLEVEL2(ftnandc024_dbg("smallwrite_oob: ch = %d, ce = %d, page = 0x%x, size = %d\n",
//				data->cur_chan, data->sel_chip, data->page_addr, mtd->writesize));

	memcpy(data->io_base + SPARE_SRAM + (data->cur_chan << data->spare_ch_offset), buf, len);

	cmd_f.row_cycle = ROW_ADDR_2CYCLE;
	cmd_f.col_cycle = COL_ADDR_1CYCLE;
	cmd_f.cq1 = data->page_addr | SCR_SEED_VAL1(data->seed_val);
	cmd_f.cq2 = SCR_SEED_VAL2(data->seed_val);
	cmd_f.cq3 = CMD_COUNT(1);
	cmd_f.cq4 = CMD_COMPLETE_EN | CMD_FLASH_TYPE(data->flash_type) |\
			CMD_START_CE(data->sel_chip) | CMD_SPARE_NUM(data->spare) |\
			CMD_INDEX(SMALL_FIXFLOW_WRITEOOB);

	status = ftnandc024_issue_cmd(chip, &cmd_f);
	if (status < 0)
		goto out;

	if (ftnandc024_nand_wait(chip) == NAND_STATUS_FAIL) {
		status = -EIO;
	}
out:
	// Returning the any value isn't allowed, except 0, -EBADMSG, or -EUCLEAN
	return status;
}

