/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Debugging macro include header
 *
 * Copyright (C) 2005 Faraday Technology
 * Moved from arch/arm/mach-faraday/include/mach/debug-macro.S
 */

#include <linux/linkage.h>
#include <mach/hardware.h>
#include <mach/serial.h>

		.macro	addruart, rp, rv, tmp
		ldr	\rp, =DEBUG_LL_FTUART010_PA_BASE	@ physical base address of UART
		ldr	\rv, =DEBUG_LL_FTUART010_VA_BASE	@ virtual base address of UART
		.endm

		.macro	senduart, rd, rx
		strb	\rd, [\rx, #SERIAL_THR]
		.endm

		.macro	waituartcts, rd, rx
		.endm

		.macro	waituarttxrdy,rd,rx
1001:		ldrb	\rd, [\rx, #SERIAL_LSR]	@ LSR
		tst	\rd, #SERIAL_LSR_THRE	@ test empty
		beq	1001b
		.endm

		.macro	busyuart, rd, rx
		mov	\rd, #0x100
1010:		subs	\rd, \rd, #1
		bne	1010b
		.endm
