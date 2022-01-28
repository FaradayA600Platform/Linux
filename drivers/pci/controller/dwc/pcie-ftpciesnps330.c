/*
 * Faraday FTPCIESNPS330 PCIE Controller
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

#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/pci.h>
#include <linux/platform_device.h>

#include "pcie-ftpciesnps330.h"
#include "pcie-designware.h"

#define DRIVER_VERSION                  "2-Nov-2021"

#define IATU_REGION0                    0
#define IATU_REGION1                    1

#define IATU_TYPE_MEM                   0x0
#define IATU_TYPE_IO                    0x2
#define IATU_TYPE_CFG0                  0x4
#define IATU_TYPE_CFG1                  0x5

#define IATU_BASE                       0x300000

#define IATU_OUTBOUND_REGION_CR1(x)     (IATU_BASE + (x)*0x200 + 0x000)
#define IATU_OUTBOUND_REGION_CR2(x)     (IATU_BASE + (x)*0x200 + 0x004)
#define IATU_OUTBOUND_LOWER_BASE(x)     (IATU_BASE + (x)*0x200 + 0x008)
#define IATU_OUTBOUND_UPPER_BASE(x)     (IATU_BASE + (x)*0x200 + 0x00C)
#define IATU_OUTBOUND_LIMIT(x)          (IATU_BASE + (x)*0x200 + 0x010)
#define IATU_OUTBOUND_LOWER_TARGET(x)   (IATU_BASE + (x)*0x200 + 0x014)
#define IATU_OUTBOUND_UPPER_TARGET(x)   (IATU_BASE + (x)*0x200 + 0x018)
#define IATU_OUTBOUND_REGION_CR3(x)     (IATU_BASE + (x)*0x200 + 0x01C)
#define IATU_OUTBOUND_UPPER_LIMIT(x)    (IATU_BASE + (x)*0x200 + 0x020)

#define IATU_INBOUND_REGION_CR1(x)      (IATU_BASE + (x)*0x200 + 0x100)
#define IATU_INBOUND_REGION_CR2(x)      (IATU_BASE + (x)*0x200 + 0x104)
#define IATU_INBOUND_LOWER_BASE(x)      (IATU_BASE + (x)*0x200 + 0x108)
#define IATU_INBOUND_UPPER_BASE(x)      (IATU_BASE + (x)*0x200 + 0x10C)
#define IATU_INBOUND_LIMIT(x)           (IATU_BASE + (x)*0x200 + 0x110)
#define IATU_INBOUND_LOWER_TARGET(x)    (IATU_BASE + (x)*0x200 + 0x114)
#define IATU_INBOUND_UPPER_TARGET(x)    (IATU_BASE + (x)*0x200 + 0x118)
#define IATU_INBOUND_REGION_CR3(x)      (IATU_BASE + (x)*0x200 + 0x11C)
#define IATU_INBOUND_UPPER_LIMIT(x)     (IATU_BASE + (x)*0x200 + 0x120)

#define to_ftpciesnps330_pcie(x)        container_of(x, struct ftpciesnps330_pcie, pci)

struct ftpciesnps330_pcie {
	struct dw_pcie      pci;
	void __iomem        *ctl_base;
	void __iomem        *phy_base;
};

static void ftpciesnps330_pcie_prog_outbound_atu(struct dw_pcie *pci, int index, int type,
			u64 cpu_addr, u64 pci_addr, u32 size)
{	
	writel(lower_32_bits(cpu_addr),        pci->dbi_base + IATU_OUTBOUND_LOWER_BASE(index));
	writel(upper_32_bits(cpu_addr),        pci->dbi_base + IATU_OUTBOUND_UPPER_BASE(index));
	writel(lower_32_bits(cpu_addr) + size, pci->dbi_base + IATU_OUTBOUND_LIMIT(index));
	writel(lower_32_bits(pci_addr),        pci->dbi_base + IATU_OUTBOUND_LOWER_TARGET(index));
	writel(upper_32_bits(pci_addr),        pci->dbi_base + IATU_OUTBOUND_UPPER_TARGET(index));
	writel(type,                           pci->dbi_base + IATU_OUTBOUND_REGION_CR1(index));
	writel(0x80000000,                     pci->dbi_base + IATU_OUTBOUND_REGION_CR2(index));
}
		
static void ftpciesnps330_pcie_prog_inbound_atu(struct dw_pcie *pci, int index,int type, 
		u64 cpu_addr, u64 pci_addr, u32 size)
{
	writel(lower_32_bits(cpu_addr),        pci->dbi_base + IATU_INBOUND_LOWER_BASE(index));
	writel(upper_32_bits(cpu_addr),        pci->dbi_base + IATU_INBOUND_UPPER_BASE(index));
	writel(lower_32_bits(cpu_addr) + size, pci->dbi_base + IATU_INBOUND_LIMIT(index));
	writel(lower_32_bits(pci_addr),        pci->dbi_base + IATU_INBOUND_LOWER_TARGET(index));
	writel(upper_32_bits(pci_addr),        pci->dbi_base + IATU_INBOUND_UPPER_TARGET(index));
	writel(type,                           pci->dbi_base + IATU_INBOUND_REGION_CR1(index));
	writel(0x80000000,                     pci->dbi_base + IATU_INBOUND_REGION_CR2(index));
}

static int ftpciesnps330_pcie_rd_conf(struct pci_bus *bus,
			unsigned int devfn, int where, int size, u32 *val)
{
	struct pcie_port *pp = bus->sysdata;
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	int ret, type;
	u32 busdev, cfg_size;
	u64 cpu_addr;
	void __iomem *va_cfg_base;

	busdev = PCIE_ATU_BUS(bus->number) | PCIE_ATU_DEV(PCI_SLOT(devfn)) |
		 PCIE_ATU_FUNC(PCI_FUNC(devfn));

	type = PCIE_ATU_TYPE_CFG0;
	cpu_addr = pp->cfg0_base;
	cfg_size = pp->cfg0_size;
	va_cfg_base = pp->va_cfg0_base;

	ftpciesnps330_pcie_prog_outbound_atu(pci, IATU_REGION0,
				  type, cpu_addr,
				  busdev, cfg_size);

	ret = dw_pcie_read(va_cfg_base + where, size, val);

	return ret;
}

static int ftpciesnps330_pcie_wr_conf(struct pci_bus *bus,
			unsigned int devfn, int where, int size, u32 val)
{
	struct pcie_port *pp = bus->sysdata;
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	int ret, type;
	u32 busdev, cfg_size;
	u64 cpu_addr;
	void __iomem *va_cfg_base;

	busdev = PCIE_ATU_BUS(bus->number) | PCIE_ATU_DEV(PCI_SLOT(devfn)) |
		 PCIE_ATU_FUNC(PCI_FUNC(devfn));

	type = PCIE_ATU_TYPE_CFG0;

	cpu_addr = pp->cfg0_base;
	cfg_size = pp->cfg0_size;
	va_cfg_base = pp->va_cfg0_base;

	ftpciesnps330_pcie_prog_outbound_atu(pci, IATU_REGION0,
				  type, cpu_addr,
				  busdev, cfg_size);

	ret = dw_pcie_write(pp->va_cfg0_base + where, size, val);

	return ret;
}

static struct pci_ops ftpciesnps330_pci_ops = {
	.read = ftpciesnps330_pcie_rd_conf,
	.write = ftpciesnps330_pcie_wr_conf,
};

static int ftpciesnps330_pcie_link_up(struct dw_pcie *pci)
{
	struct ftpciesnps330_pcie *hw  = to_ftpciesnps330_pcie(pci);
	u32 val;

	val = (readl(hw->ctl_base + DMC_STATUS) >> 4) & 0x3f;

	return (val == PORT_LOGIC_LTSSM_STATE_L0);
}

static void ftpciesnps330_pcie_phy_init(struct pcie_port *pp)
{
	struct dw_pcie *pci  = to_dw_pcie_from_pp(pp);
	struct ftpciesnps330_pcie *hw = to_ftpciesnps330_pcie(pci);
	u32 val;

#ifdef CONFIG_A500_PLATFORM
	// Programming PCIE_PHY0
	// CMM
	// Set 5G PLL AAC CUR bit[7:2]
	val = readl(hw->phy_base + 0x0014) & 0xFFFFFF03;
	val |= 0x00000023;
	writel(val, hw->phy_base + 0x0014);

	// Set 8G PLL AAC CUR bit[23:18]
	val = readl(hw->phy_base + 0x0014) & 0xFF03FFFFF;
	val |= 0x00100000;
	writel(val, hw->phy_base + 0x0014);

	// 5G PLL LP DIV control, bit[24:20] 
	val = readl(hw->phy_base + 0x081C) & 0xE00FFFFF;
	val |= 0x01900000;
	writel(val, hw->phy_base + 0x081C);

	// 5G PLL LP DIV select from REG,
	val = readl(hw->phy_base + 0x0824) & 0xFEFFFFFF;
	val |= 0x01000000;
	writel(val, hw->phy_base + 0x0824);

	// LDO ref voltage from BG and Chopper enabled 
	val = readl(hw->phy_base + 0x0804) & 0xFF9FFFFF;
	val |= 0x00600000;
	writel(val, hw->phy_base + 0x0804);

	// 5G PLL POST_DIV [5:0]=6d'32 0x0808[25:16] = 10'h20
	val = readl(hw->phy_base + 0x0808) & 0xFC00FFFF;
	val |= 0x00200000;
	writel(val, hw->phy_base + 0x0808);

	// 8G PLL POST_DIV [5:0]=6d'32 0x0810[9:0] = 10'h20
	val = readl(hw->phy_base + 0x0810) & 0xFFFFFC00;
	val |= 0x00000020;
	writel(val, hw->phy_base + 0x0810);

	// TXRX_L0
	// - Offset 16'h8850[4] =  1'h1   - RX_CDR_DIV64_SEL
	val = readl(hw->phy_base + 0x4850) & 0xFFFFFFEF;
	val |= 0x00000010;
	writel(val, hw->phy_base + 0x4850);

	// SET TX_FFE_POST1 to 4'h6
	val = readl(hw->phy_base + 0x4820) & 0xFFFFFFF0;
	val |= 0x00000006;
	writel(val, hw->phy_base + 0x4820);

	// Trial setting added on 2018/10/08
	// Shall be added to TXRX_L1 later
	// Set DLPF TC initial value
	// Offset 16'h80F8[17:8] =  9'd511
	val = readl(hw->phy_base + 0x40F8) & 0xFFFC00FF;
	val |= 0x0001FF00;
	writel(val, hw->phy_base + 0x40F8);

	// Trial setting added on 2018/10/08
	// Shall be added to TXRX_L1 later
	// Set DLPF FC initial value
	// Offset 16'h80F8[6:0] =  7'd63
	val = readl(hw->phy_base + 0x40F8) & 0xFFFFFF80;
	val |= 0x0000003F;
	writel(val, hw->phy_base + 0x40F8);

	// Start of new setting for RX_CLK debug
	// 0x40F3: bit[2:0] = 'h1
	// 0x40F2: bit[7:0] = 'h0
	// 0x40F1: bit[1:0] = 'h0
	// 0x40F0: bit[7:0] = 'h80
	val = readl(hw->phy_base + 0x40F0) & 0xF800FC00;
	val |= 0x01000080;
	writel(val, hw->phy_base + 0x40F0);

	// 0x40F6: bit[0] = 'h1
	// 0x40F5: bit[7:0] = 'h0
	// 0x40F4: bit[5:0] = 'h30
	val = readl(hw->phy_base + 0x40F4) & 0xFFFE00C0;
	val |= 0x00010030;
	writel(val, hw->phy_base + 0x40F4);

	// 0x4856: bit[1:0] = 'h2
	// 0x4855: bit[7:6] = 'h2
	val = readl(hw->phy_base + 0x4854) & 0xFFFC3FFF;
	val |= 0x00028000;
	writel(val, hw->phy_base + 0x4854);

	// 0x40E3: bit[7] = 'h0
	val = readl(hw->phy_base + 0x40E0) & 0x7FFFFFFF;
	val |= 0x00000000;
	writel(val, hw->phy_base + 0x40E0);

	// 0x40FA: bit[1:0] = 'h2
	// 0x40F9: bit[7:0] = 'h0
	// 0x40F8: bit[6:0] = 'h40
	val = readl(hw->phy_base + 0x40F8) & 0xFFFC0080;
	val |= 0x00020040;
	writel(val, hw->phy_base + 0x40F8);

	// 0x40D2: bit[1:0] = 'h2
	// 0x40D1: bit[7:1] = 'h0
	val = readl(hw->phy_base + 0x40D0) & 0xFFFC01FF;
	val |= 0x00020000;
	writel(val, hw->phy_base + 0x40D0);

	// End of new setting for RX_CLK debug
	// Start of new setting added in 10/9 morning
	// OFC_DATA setting start
	// 0x4028: bit[0] = 'h0
	val = readl(hw->phy_base + 0x4028) & 0xFFFFFFFE;
	val |= 0x00000000;
	writel(val, hw->phy_base + 0x4028);

	// OFC_DATA setting cont.
	// 0x402C: bit[13] = 'h0
	// 0x402C: bit[5] = 'h0
	val = readl(hw->phy_base + 0x402C) & 0xFFFFDFDF;
	val |= 0x00000000;
	writel(val, hw->phy_base + 0x402C);

	// OFC_EDGE setting
	// 0x4020: bit[0] = 'h0
	val = readl(hw->phy_base + 0x4020) & 0xFFFFFFFE;
	val |= 0x00000000;
	writel(val, hw->phy_base + 0x4020);

	// OFC_EOM setting
	// 0x4010: bit[0] = 'h0
	val = readl(hw->phy_base + 0x4010) & 0xFFFFFFFE;
	val |= 0x00000000;
	writel(val, hw->phy_base + 0x4010);

	// OFC_ERR setting
	// 0x4018: bit[0] = 'h0
	val = readl(hw->phy_base + 0x4018) & 0xFFFFFFFE;
	val |= 0x00000000;
	writel(val, hw->phy_base + 0x4018);

	// AGC setting
	// 0x40A0: bit[30] = 'h1
	// 0x40A0: bit[19] = 'h1
	val = readl(hw->phy_base + 0x40A0) & 0xBFF7FFFF;
	val |= 0x40080000;
	writel(val, hw->phy_base + 0x40A0);

	// CTLE setting
	// 0x40AC: bit[23:22] = 2'b11
	val = readl(hw->phy_base + 0x40AC) & 0xFF3FFFFF;
	val |= 0x00C00000;
	writel(val, hw->phy_base + 0x40AC);

	// DFE setting
	// 0x40A4: bit[26] = 1'b1
	// 0x40A4: bit[25] = 1'b1
	// 0x40A4: bit[24] = 1'b1
	// 0x40A4: bit[23] = 1'b1
	// 0x40A4: bit[22] = 1'b1
	val = readl(hw->phy_base + 0x40A4) & 0xF83FFFFF;
	val |= 0x07C00000;
	writel(val, hw->phy_base + 0x40A4);

	// IIR setting
	// 0x40A4: bit[27] = 1'b1
	val = readl(hw->phy_base + 0x40A4) & 0xF7FFFFFF;
	val |= 0x08000000;
	writel(val, hw->phy_base + 0x40A4);

	// End of new setting added in 10/9 morning
	// Start of new setting added in 10/9 afternoon
	// REFP and AGC
	// 0x40A0: bit[9:4] = 'd48 ; REFP
	// 0x40A0: bit[3:0] = 'd10 ; AGC
	val = readl(hw->phy_base + 0x40A0) & 0xFFFFFC00;
	val |= 0x0000030A;
	writel(val, hw->phy_base + 0x40A0);

	// CTLE
	// 0x40A8: bit[27:25] = 'd4;
	// 0x40A8: bit[24:22] = 'd4;
	val = readl(hw->phy_base + 0x40A8) & 0xF03FFFFF;
	val |= 0x09000000;
	writel(val, hw->phy_base + 0x40A8);

	// PK and VGA
	// 0x402C: bit[12:8] = 'd0
	// 0x402C: bit[4:0] = 'd0
	val = readl(hw->phy_base + 0x402C) & 0xFFFFE0E0;
	val |= 0x00000000;
	writel(val, hw->phy_base + 0x402C);
	//End of new setting added in 10/9 afternoon

	// --------------------------------------------------------------------------- ;
	// - CMM Reg0x0000, bit[5:4] = 2'b01
	val = readl(hw->phy_base + 0x0000) & 0xFFFFFFCF;
	val |= 0x00000010;
	writel(val, hw->phy_base + 0x0000);

	// --------------------------------------------------------------------------- ;
	// TXRX_L1
	// - Offset 16'h8850[4] =  1'h1   - RX_CDR_DIV64_SEL
	val = readl(hw->phy_base + 0x4850) & 0xFFFFFFEF;
	val |= 0x00000010;
	writel(val, hw->phy_base + 0x4850);

	// SET TX_FFE_POST1 to 4'h6
	val = readl(hw->phy_base + 0x4820) & 0xFFFFFFF0;
	val |= 0x00000006;
	writel(val, hw->phy_base + 0x4820);

	// Trial setting added on 2018/10/08
	// Shall be added to TXRX_L1 later
	// Set DLPF TC initial value
	// Offset 16'h80F8[17:8] =  9'd511
	val = readl(hw->phy_base + 0x40F8) & 0xFFFC00FF;
	val |= 0x0001FF00;
	writel(val, hw->phy_base + 0x40F8);

	// Trial setting added on 2018/10/08
	// Shall be added to TXRX_L1 later
	// Set DLPF FC initial value
	// Offset 16'h80F8[6:0] =  7'd63
	val = readl(hw->phy_base + 0x40F8) & 0xFFFFFF80;
	val |= 0x0000003F;
	writel(val, hw->phy_base + 0x40F8);

	// Start of new setting for RX_CLK debug
	// 0x40F3: bit[2:0] = 'h1
	// 0x40F2: bit[7:0] = 'h0
	// 0x40F1: bit[1:0] = 'h0
	// 0x40F0: bit[7:0] = 'h80
	val = readl(hw->phy_base + 0x40F0) & 0xF800FC00;
	val |= 0x01000080;
	writel(val, hw->phy_base + 0x40F0);

	// 0x40F6: bit[0] = 'h1
	// 0x40F5: bit[7:0] = 'h0
	// 0x40F4: bit[5:0] = 'h30
	val = readl(hw->phy_base + 0x40F4) & 0xFFFE00C0;
	val |= 0x00010030;
	writel(val, hw->phy_base + 0x40F4);

	// 0x4856: bit[1:0] = 'h2
	// 0x4855: bit[7:6] = 'h2
	val = readl(hw->phy_base + 0x4854) & 0xFFFC3FFF;
	val |= 0x00028000;
	writel(val, hw->phy_base + 0x4854);

	// 0x40E3: bit[7] = 'h0
	val = readl(hw->phy_base + 0x40E0) & 0x7FFFFFFF;
	val |= 0x00000000;
	writel(val, hw->phy_base + 0x40E0);

	// 0x40FA: bit[1:0] = 'h2
	// 0x40F9: bit[7:0] = 'h0
	// 0x40F8: bit[6:0] = 'h40
	val = readl(hw->phy_base + 0x40F8) & 0xFFFC0080;
	val |= 0x00020040;
	writel(val, hw->phy_base + 0x40F8);

	// 0x40D2: bit[1:0] = 'h2
	// 0x40D1: bit[7:1] = 'h0
	val = readl(hw->phy_base + 0x40D0) & 0xFFFC01FF;
	val |= 0x00020000;
	writel(val, hw->phy_base + 0x40D0);
	// End of new setting for RX_CLK debug

	// Start of new setting added in 10/9 morning
	// OFC_DATA setting start
	// 0x4028: bit[0] = 'h1
	val = readl(hw->phy_base + 0x4028) & 0xFFFFFFFE;
	val |= 0x00000001;
	writel(val, hw->phy_base + 0x4028);

	// OFC_DATA setting cont.
	// 0x402C: bit[13] = 'h1
	// 0x402C: bit[5] = 'h1
	val = readl(hw->phy_base + 0x402C) & 0xFFFFDFDF;
	val |= 0x00002020;
	writel(val, hw->phy_base + 0x402C);

	// OFC_EDGE setting
	// 0x4020: bit[0] = 'h1
	val = readl(hw->phy_base + 0x4020) & 0xFFFFFFFE;
	val |= 0x00000001;
	writel(val, hw->phy_base + 0x4020);

	// OFC_EOM setting
	// 0x4010: bit[0] = 'h1
	val = readl(hw->phy_base + 0x4010) & 0xFFFFFFFE;
	val |= 0x00000001;
	writel(val, hw->phy_base + 0x4010);

	// OFC_ERR setting
	// 0x4018: bit[0] = 'h1
	val = readl(hw->phy_base + 0x4018) & 0xFFFFFFFE;
	val |= 0x00000001;
	writel(val, hw->phy_base + 0x4018);

	// AGC setting
	// 0x40A0: bit[30] = 'h1
	// 0x40A0: bit[19] = 'h1
	val = readl(hw->phy_base + 0x40A0) & 0xBFF7FFFF;
	val |= 0x40080000;
	writel(val, hw->phy_base + 0x40A0);

	// CTLE setting
	// 0x40AC: bit[23:22] = 2'b11
	val = readl(hw->phy_base + 0x40AC) & 0xFF3FFFFF;
	val |= 0x00C00000;
	writel(val, hw->phy_base + 0x40AC);

	// DFE setting
	// 0x40A4: bit[26] = 1'b1
	// 0x40A4: bit[25] = 1'b1
	// 0x40A4: bit[24] = 1'b1
	// 0x40A4: bit[23] = 1'b1
	// 0x40A4: bit[22] = 1'b1
	val = readl(hw->phy_base + 0x40A4) & 0xF83FFFFF;
	val |= 0x07C00000;
	writel(val, hw->phy_base + 0x40A4);

	// IIR setting
	// 0x40A4: bit[27] = 1'b1
	val = readl(hw->phy_base + 0x40A4) & 0xF7FFFFFF;
	val |= 0x08000000;
	writel(val, hw->phy_base + 0x40A4);

	// End of new setting added in 10/9 morning

	// Start of new setting added in 10/9 afternoon
	// REFP and AGC
	// 0x40A0: bit[9:4] = 'd48 ; REFP
	// 0x40A0: bit[3:0] = 'd10 ; AGC
	val = readl(hw->phy_base + 0x40A0) & 0xFFFFFC00;
	val |= 0x0000030A;
	writel(val, hw->phy_base + 0x40A0);

	// CTLE
	// 0x40A8: bit[27:25] = 'd4;
	// 0x40A8: bit[24:22] = 'd4;
	val = readl(hw->phy_base + 0x40A8) & 0xF03FFFFF;
	val |= 0x09000000;
	writel(val, hw->phy_base + 0x40A8);

	// PK and VGA
	// 0x402C: bit[12:8] = 'd0
	// 0x402C: bit[4:0] = 'd0
	val = readl(hw->phy_base + 0x402C) & 0xFFFFE0E0;
	val |= 0x00000000;
	writel(val, hw->phy_base + 0x402C);
	//End of new setting added in 10/9 afternoon

	// --------------------------------------------------------------------------- ;
	// DPMA FPGA
	// - Offset 16h'8000[0] = 1'b0         - CMM_PD_BIAS_GEN
	// - Offset 16h'8000[4] = 1'b1         - ICMM_RSTN_BIAS_GEN
	val = readl(hw->phy_base + 0x8000) & 0xFFFFFFEE;
	val |= 0x00000010;
	writel(val, hw->phy_base + 0x8000);

	// --------------------------------------------------------------------------- ;
	// PCS
	// Prog L0 Control Register
	// - Read offset 16'hC110
	// - Set bit[10,8,6] of read value
	// - Write new value to offset 16'hC110
	val = readl(hw->phy_base + 0xC110);
	val |= 0x00000540;
	writel(val, hw->phy_base + 0xC110);

	// --------------------------------------------------------------------------- ;
	// Prog L1 Control Register
	// - Read offset 16'hC120
	// - Set bit[10,8,6] of read value
	// - Write new value to offset 16'hC120
	val = readl(hw->phy_base + 0xC120);
	val |= 0x00000540;
	writel(val, hw->phy_base + 0xC120);

#elif CONFIG_A600_PLATFORM

	// workaround: reduce tx_rxdet_time
	val = readl(hw->phy_base + 0x409C) & ~0x000003FF;
	val |= 0x000000B2;
	writel(val, hw->phy_base + 0x409C);
#endif
}

 int ftpciesnps330_pcie_host_init(struct pcie_port *pp)
{
	struct dw_pcie *pci  = to_dw_pcie_from_pp(pp);
	struct ftpciesnps330_pcie *hw = to_ftpciesnps330_pcie(pci);
	struct resource_entry *tmp, *entry = NULL;

#ifdef CONFIG_A500_PLATFORM
	void __iomem *scu_va;
	u32 val;

	scu_va = ioremap(0x13000000, 0x10000);

	pp->bridge->child_ops= &ftpciesnps330_pci_ops;
	
	// set PCIE's mode to RC (PCIE0, PCIE1)
	val = 0x00000008;
	writel(val, scu_va + 0x8358);
	writel(val, scu_va + 0x8364);

	// modify pci_lp_cntp6 to 0x27
	val = readl(scu_va + 0x8114) & 0xFF03FFFF;
	val |= (0x27 << 18);
	writel(val, scu_va + 0x8114);

	// enable PCIE's aclk (PCIE0, PCIE1)
	val = readl(scu_va + 0x8118);
	val |= (0x1 << 7);	//PCIE1
	val |= (0x1 << 6);	//PCIE0
	writel(val, scu_va + 0x8118);

	// enable PCIE's pclk (PCIE0, PCIE1)
	val = readl(scu_va + 0x8124);
	val |= (0x1 << 2);	//PCIE1
	val |= (0x1 << 1);	//PCIE0
	writel(val, scu_va + 0x8124);

	// enable PCIE's lp_clk (PCIE0, PCIE1)
	val = readl(scu_va + 0x8128);
	val |= (0x1 << 10);
	writel(val, scu_va + 0x8128);

	// software reset release
	val = readl(scu_va + 0x8130);
	val |= (0x1 << 15);	//PCIE1
	val |= (0x1 << 14);	//PCIE0
	writel(val, scu_va + 0x8130);

	// setup pcie phy
	ftpciesnps330_pcie_phy_init(pp);

	//-- set ICAL_START -----------------------------////
	val = 0x00000009;
	writel(val, scu_va + 0x8358);	// set ICAL_START_PCIE0 = 1 
	writel(val, scu_va + 0x8364);	// set ICAL_START_PCIE1 = 1 

	// Start of new setting added in 10/16 afternoon
	// Shall be added after Caibraion
	// PCIEPHY0 
	// TXRX Reg0x00A8 bit[27:22] = 6'h3F
	val = readl(hw->phy_base + 0x40A8) & 0xF03FFFFF;
	val |= 0x0FC00000;
	writel(val, hw->phy_base + 0x40A8);
	// End of new setting added in 10/16 afternoon

	iounmap(scu_va);

#elif CONFIG_A600_PLATFORM

	// assert app_hold_phy_rst
	writel(0x00040002, hw->ctl_base + MISC1);

	// set PCIE's mode to RC
	writel(0x00000004, hw->ctl_base + MISC4);

	// set app_ltssm_enable
	writel(0x00000001, hw->ctl_base + MISC5);

	// wait sram init done
	while ((readl(hw->ctl_base + MISC4) & 0x80000000) != 0x80000000)
	{};

	// setup pcie phy
	ftpciesnps330_pcie_phy_init(pp);

	// set fw_update_done
	writel(0x00000024, hw->ctl_base + MISC4);

	// wait reset release
	while ((readl(hw->ctl_base + MISC4) & 0x10000000) != 0x10000000)
	{};

	// release app_hold_phy_rst
	writel(0x00040000, hw->ctl_base + MISC1);
#endif

	// setup root complex
	dw_pcie_setup_rc(pp);

	// wait to link up
	dw_pcie_wait_for_link(pci);

	resource_list_for_each_entry(tmp, &pp->bridge->windows)
		if (resource_type(tmp->res) == IORESOURCE_MEM){
			entry = tmp;
		}

	// setup memory mapping
	ftpciesnps330_pcie_prog_outbound_atu(pci, IATU_REGION1,
				  PCIE_ATU_TYPE_MEM,  (u64)entry->res->start + 0x1000000,				//original : 0x00000000, pp->mem_base + 0x01000000,
				 (u64)entry->res->start + 0x1000000, resource_size(entry->res)-0x1000000);

	ftpciesnps330_pcie_prog_inbound_atu(pci, IATU_REGION0,
				  PCIE_ATU_TYPE_MEM, PHYS_OFFSET,
				  PHYS_OFFSET, SZ_1G);

	// setup msi
	if (IS_ENABLED(CONFIG_PCI_MSI))
		dw_pcie_msi_init(pp);

	return 0;
}

static const struct dw_pcie_host_ops ftpciesnps330_pcie_host_ops = {	
	.host_init =		ftpciesnps330_pcie_host_init,
};


static const struct dw_pcie_ops ftpciesnps330_dw_pcie_ops = {
	.link_up        = ftpciesnps330_pcie_link_up,
};

static irqreturn_t ftpciesnps330_pcie_irq(int irq, void *arg)
{
	struct pcie_port *pp = arg;
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	struct ftpciesnps330_pcie *hw = to_ftpciesnps330_pcie(pci);
	u32 intr_sts, intr_msk;

	intr_sts = readl(hw->ctl_base + INTR_STATUS);
	intr_msk = readl(hw->ctl_base + INTR_MASK);

	intr_sts &= ~intr_msk;

	if (intr_sts & INTR_STATUS_ERR) {
		if (intr_sts & INTR_STATUS_QOVERFLOW_VC0) {
			if (printk_ratelimit())
				dev_err(pci->dev, "P/NP/CPL Receive Queues 0 Detected\n");
		}

		if (intr_sts & INTR_STATUS_QOVERFLOW_VC1) {
			if (printk_ratelimit())
				dev_err(pci->dev, "P/NP/CPL Receive Queues 1 Detected\n");
		}

		if (intr_sts & INTR_STATUS_ERR_INTERNAL) {
			if (printk_ratelimit())
				dev_err(pci->dev, "Internal Error Detected\n");
		}

		if (intr_sts & INTR_STATUS_ERR_MSI) {
			if (printk_ratelimit())
				dev_err(pci->dev, "MSI Error Detected\n");
		}

		if (intr_sts & INTR_STATUS_ERR_SYS) {
			if (printk_ratelimit())
				dev_err(pci->dev, "System Error Detected\n");
		}

		if (intr_sts & INTR_STATUS_ERR_CORRECTABLE) {
			if (printk_ratelimit())
				dev_err(pci->dev, "Correctable Error Detected\n");
		}

		if (intr_sts & INTR_STATUS_ERR_NONFATAL) {
			if (printk_ratelimit())
				dev_err(pci->dev, "Nonfatal Error Detected\n");
		}

		if (intr_sts & INTR_STATUS_ERR_FATAL) {
			if (printk_ratelimit())
				dev_err(pci->dev, "Fatal Error Detected\n");
		}
	}

	writel(intr_sts, hw->ctl_base + INTR_STATUS);

	if (intr_sts & INTR_STATUS_MSI)
		return dw_handle_msi_irq(&pci->pp);

	return IRQ_HANDLED;
}

static int __init ftpciesnps330_pcie_probe(struct platform_device *pdev)
{
	struct ftpciesnps330_pcie *ftpciesnps330_hw;
	struct dw_pcie *pci;
	struct pcie_port *pp;
	struct resource *ctl_base;
	struct resource *dbi_base;
	struct resource *phy_base;
	struct clk *clk;
	struct reset_control *rstc;
	int ret;

	ftpciesnps330_hw = devm_kzalloc(&pdev->dev, sizeof(*ftpciesnps330_hw),
	                  GFP_KERNEL);
	if (!ftpciesnps330_hw)
		return -ENOMEM;

	pci = &ftpciesnps330_hw->pci;

	pci->dev = &pdev->dev;
	pci->ops = &ftpciesnps330_dw_pcie_ops;

	clk = devm_clk_get(&pdev->dev, "aclk");
	if (!IS_ERR(clk)) {
		clk_prepare_enable(clk);
	}

	clk = devm_clk_get(&pdev->dev, "pclk");
	if (!IS_ERR(clk)) {
		clk_prepare_enable(clk);
	}

	rstc = devm_reset_control_get(&pdev->dev, "rstn");
	if (!IS_ERR_OR_NULL(rstc)) {
		reset_control_deassert(rstc);
	}

	ctl_base = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ftpciesnps330_hw->ctl_base = devm_ioremap_resource(&pdev->dev, ctl_base);
	if (IS_ERR(ftpciesnps330_hw->ctl_base)) {
		ret = PTR_ERR(ftpciesnps330_hw->ctl_base);
		goto map_failed;
	}

	phy_base = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	ftpciesnps330_hw->phy_base = devm_ioremap_resource(&pdev->dev, phy_base);
	if (IS_ERR(ftpciesnps330_hw->phy_base)) {
		ret = PTR_ERR(ftpciesnps330_hw->phy_base);
		goto map_failed;
	}

	dbi_base = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	pci->dbi_base = devm_ioremap_resource(&pdev->dev, dbi_base);
	if (IS_ERR(pci->dbi_base)) {
		ret = PTR_ERR(pci->dbi_base);
		goto map_failed;
	}

	pp = &pci->pp;

	pp->irq = platform_get_irq(pdev, 0);
	if (!pp->irq) {
		dev_err(&pdev->dev, "failed to get irq\n");
		return -ENODEV;
	}

	ret = devm_request_irq(&pdev->dev, pp->irq, ftpciesnps330_pcie_irq,
	                       IRQF_SHARED, "ftpciesnps330-pcie", pp);
	if (ret) {
		dev_err(&pdev->dev, "failed to request irq\n");
		return ret;
	}

	pp->ops = &ftpciesnps330_pcie_host_ops;	

	platform_set_drvdata(pdev, ftpciesnps330_hw);
	
	ret = dw_pcie_host_init(pp);
	if (ret) {
		dev_err(&pdev->dev, "failed to initialize host\n");
		return ret;
	}

	dev_info(&pdev->dev, "version %s\n", DRIVER_VERSION);

	return 0;

map_failed:
	devm_kfree(&pdev->dev, ftpciesnps330_hw);
	dev_err(&pdev->dev, "Faraday FTPCIESNPS330 probe failed\n");

	return ret;
}

static int __exit ftpciesnps330_pcie_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id ftpciesnps330_pcie_of_match[] = {
	{ .compatible = "faraday,ftpciesnps330", },
	{},
};
MODULE_DEVICE_TABLE(of, ftpciesnps330_pcie_of_match);

static struct platform_driver ftpciesnps330_pcie_driver = {
	.remove		= __exit_p(ftpciesnps330_pcie_remove),
	.driver = {
		.name	= "ftpciesnps330",
		.of_match_table = ftpciesnps330_pcie_of_match,
	},
};

/* Exynos PCIe driver does not allow module unload */

static int __init ftpciesnps330_pcie_init(void)
{
	return platform_driver_probe(&ftpciesnps330_pcie_driver, ftpciesnps330_pcie_probe);
}
subsys_initcall(ftpciesnps330_pcie_init);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("B.C. Chen <bcchen@faraday-tech.com>");
MODULE_DESCRIPTION("Faraday FTPCIESNPS330 PCIE Controller");
