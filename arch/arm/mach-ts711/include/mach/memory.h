/*
 * arch/arm/mach-hgu10g/include/mach/memory.h
 *
 *  Copyright (C) 2016 Faraday Technology
 *  B.C. Chen <bcchen@faraday-tech.com>
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
 
#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H

#ifdef CONFIG_ARM_ACP

/*
 * ARM CA9MP ACP Bus address offset
 */
#define ARM_ACPBUS_OFFSET           UL(0x40000000)

#define virt_to_acpbus(x)           ((x) - PAGE_OFFSET + ARM_ACPBUS_OFFSET)
#define acpbus_to_virt(x)           ((x) - ARM_ACPBUS_OFFSET + PAGE_OFFSET)
#define is_acpbus_device(dev)       (dev && ((strncmp(dev_name(dev), "xhci-hcd", 8) == 0) || (strncmp(dev_name(dev), "dwc3", 4) == 0)))

#define __arch_pfn_to_dma(dev, pfn)                             \
	({ dma_addr_t __dma = __pfn_to_phys(pfn);                   \
		if (is_acpbus_device(dev))                              \
			__dma = __dma - PHYS_OFFSET + ARM_ACPBUS_OFFSET;    \
		__dma; })

#define __arch_dma_to_pfn(dev, addr)                            \
	({ dma_addr_t __dma = addr;                                 \
		if (is_acpbus_device(dev))                              \
			__dma = __dma + PHYS_OFFSET - ARM_ACPBUS_OFFSET;    \
		__phys_to_pfn(__dma); })

#define __arch_dma_to_virt(dev, addr)                           \
	({ (void *) (is_acpbus_device(dev) ? acpbus_to_virt(addr) : __phys_to_virt(addr)); })

#define __arch_virt_to_dma(dev, addr)                           \
	({ unsigned long __addr = (unsigned long)(addr);            \
		(dma_addr_t) (is_acpbus_device(dev) ? virt_to_acpbus(__addr) : __virt_to_phys(__addr)); })

#endif	//CONFIG_ARM_ACP

#endif	//__ASM_ARCH_MEMORY_H

