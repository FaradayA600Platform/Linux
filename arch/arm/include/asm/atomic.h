/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *  arch/arm/include/asm/atomic.h
 *
 *  Copyright (C) 1996 Russell King.
 *  Copyright (C) 2002 Deep Blue Solutions Ltd.
 */
#ifndef __ASM_ARM_ATOMIC_H
#define __ASM_ARM_ATOMIC_H

#include <linux/compiler.h>
#include <linux/prefetch.h>
#include <linux/types.h>
#include <linux/irqflags.h>
#include <asm/barrier.h>
#include <asm/cmpxchg.h>

#ifdef __KERNEL__

/*
 * On ARM, ordinary assignment (str instruction) doesn't clear the local
 * strex/ldrex monitor on some implementations. The reason we can use it for
 * atomic_set() is the clrex or dummy strex done on every exception return.
 */
#define atomic_read(v)	READ_ONCE((v)->counter)
#define atomic_set(v,i)	WRITE_ONCE(((v)->counter), (i))

#if __LINUX_ARM_ARCH__ >= 6

/*
 * ARMv6 UP and SMP safe atomic ops.  We use load exclusive and
 * store exclusive to ensure that these are atomic.  We may loop
 * to ensure that the update happens.
 */

#define ATOMIC_OP(op, c_op, asm_op)					\
static inline void atomic_##op(int i, atomic_t *v)			\
{									\
	unsigned long tmp;						\
	int result;							\
									\
	prefetchw(&v->counter);						\
	__asm__ __volatile__("@ atomic_" #op "\n"			\
"1:	ldrex	%0, [%3]\n"						\
"	" #asm_op "	%0, %0, %4\n"					\
"	strex	%1, %0, [%3]\n"						\
"	teq	%1, #0\n"						\
"	bne	1b"							\
	: "=&r" (result), "=&r" (tmp), "+Qo" (v->counter)		\
	: "r" (&v->counter), "Ir" (i)					\
	: "cc");							\
}									\

#define ATOMIC_OP_RETURN(op, c_op, asm_op)				\
static inline int atomic_##op##_return_relaxed(int i, atomic_t *v)	\
{									\
	unsigned long tmp;						\
	int result;							\
									\
	prefetchw(&v->counter);						\
									\
	__asm__ __volatile__("@ atomic_" #op "_return\n"		\
"1:	ldrex	%0, [%3]\n"						\
"	" #asm_op "	%0, %0, %4\n"					\
"	strex	%1, %0, [%3]\n"						\
"	teq	%1, #0\n"						\
"	bne	1b"							\
	: "=&r" (result), "=&r" (tmp), "+Qo" (v->counter)		\
	: "r" (&v->counter), "Ir" (i)					\
	: "cc");							\
									\
	return result;							\
}

#define ATOMIC_FETCH_OP(op, c_op, asm_op)				\
static inline int atomic_fetch_##op##_relaxed(int i, atomic_t *v)	\
{									\
	unsigned long tmp;						\
	int result, val;						\
									\
	prefetchw(&v->counter);						\
									\
	__asm__ __volatile__("@ atomic_fetch_" #op "\n"			\
"1:	ldrex	%0, [%4]\n"						\
"	" #asm_op "	%1, %0, %5\n"					\
"	strex	%2, %1, [%4]\n"						\
"	teq	%2, #0\n"						\
"	bne	1b"							\
	: "=&r" (result), "=&r" (val), "=&r" (tmp), "+Qo" (v->counter)	\
	: "r" (&v->counter), "Ir" (i)					\
	: "cc");							\
									\
	return result;							\
}

#define atomic_add_return_relaxed	atomic_add_return_relaxed
#define atomic_sub_return_relaxed	atomic_sub_return_relaxed
#define atomic_fetch_add_relaxed	atomic_fetch_add_relaxed
#define atomic_fetch_sub_relaxed	atomic_fetch_sub_relaxed

#define atomic_fetch_and_relaxed	atomic_fetch_and_relaxed
#define atomic_fetch_andnot_relaxed	atomic_fetch_andnot_relaxed
#define atomic_fetch_or_relaxed		atomic_fetch_or_relaxed
#define atomic_fetch_xor_relaxed	atomic_fetch_xor_relaxed

static inline int atomic_cmpxchg_relaxed(atomic_t *ptr, int old, int new)
{
	int oldval;
	unsigned long res;

	prefetchw(&ptr->counter);

	do {
		__asm__ __volatile__("@ atomic_cmpxchg\n"
		"ldrex	%1, [%3]\n"
		"mov	%0, #0\n"
		"teq	%1, %4\n"
		"strexeq %0, %5, [%3]\n"
		    : "=&r" (res), "=&r" (oldval), "+Qo" (ptr->counter)
		    : "r" (&ptr->counter), "Ir" (old), "r" (new)
		    : "cc");
	} while (res);

	return oldval;
}
#define atomic_cmpxchg_relaxed		atomic_cmpxchg_relaxed

static inline int atomic_fetch_add_unless(atomic_t *v, int a, int u)
{
	int oldval, newval;
	unsigned long tmp;

	smp_mb();
	prefetchw(&v->counter);

	__asm__ __volatile__ ("@ atomic_add_unless\n"
"1:	ldrex	%0, [%4]\n"
"	teq	%0, %5\n"
"	beq	2f\n"
"	add	%1, %0, %6\n"
"	strex	%2, %1, [%4]\n"
"	teq	%2, #0\n"
"	bne	1b\n"
"2:"
	: "=&r" (oldval), "=&r" (newval), "=&r" (tmp), "+Qo" (v->counter)
	: "r" (&v->counter), "r" (u), "r" (a)
	: "cc");

	if (oldval != u)
		smp_mb();

	return oldval;
}
#define atomic_fetch_add_unless		atomic_fetch_add_unless

#elif defined(CONFIG_CPU_FMP626)

/*
 * Faraday FMP626 UP and SMP safe atomic ops.  We use load exclusive and
 * store exclusive to ensure that these are atomic.  We may loop
 * to ensure that the update happens.  Writing to 'v->counter'
 * without using the following operations WILL break the atomic
 * nature of these ops.
 */
static inline void atomic_add(int i, atomic_t *v)
{
	unsigned long tmp;
	int result;

	__asm__ __volatile__("@ atomic_add\n"
"1:	ldc	p13, c0, [%3], {2}	@ set address for ldrex\n"
"	mrc	p13, 0, %0, c0, c0, 0	@ ldrex\n"
"	add	%0, %0, %4\n"
"	mcr	p13, 0, %0, c0, c0, 0	@ data for strex\n"
"	stc	p13, c0, [%3], {2}	@ strex to address\n"
"	mrc	p13, 0, %1, c0, c0, 2	@ strex status\n"
"	teq	%1, #0\n"
"	bne	1b"
	: "=&r" (result), "=&r" (tmp), "+Qo" (v->counter)
	: "r" (&v->counter), "Ir" (i)
	: "cc");
}

static inline int atomic_add_return(int i, atomic_t *v)
{
	unsigned long tmp;
	int result;

	smp_mb();

	__asm__ __volatile__("@ atomic_add_return\n"
"1:	ldc	p13, c0, [%3], {2}	@ set address for ldrex\n"
"	mrc	p13, 0, %0, c0, c0, 0	@ ldrex\n"
"	add	%0, %0, %4\n"
"	mcr	p13, 0, %0, c0, c0, 0	@ data for strex\n"
"	stc	p13, c0, [%3], {2}	@ strex to address\n"
"	mrc	p13, 0, %1, c0, c0, 2	@ strex status\n"
"	teq	%1, #0\n"
"	bne	1b"
	: "=&r" (result), "=&r" (tmp), "+Qo" (v->counter)
	: "r" (&v->counter), "Ir" (i)
	: "cc");

	smp_mb();

	return result;
}

static inline void atomic_sub(int i, atomic_t *v)
{
	unsigned long tmp;
	int result;

	__asm__ __volatile__("@ atomic_sub\n"
"1:	ldc	p13, c0, [%3], {2}	@ set address for ldrex\n"
"	mrc	p13, 0, %0, c0, c0, 0	@ ldrex\n"
"	sub	%0, %0, %4\n"
"	mcr	p13, 0, %0, c0, c0, 0	@ data for strex\n"
"	stc	p13, c0, [%3], {2}	@ strex to address\n"
"	mrc	p13, 0, %1, c0, c0, 2	@ strex status\n"
"	teq	%1, #0\n"
"	bne	1b"
	: "=&r" (result), "=&r" (tmp), "+Qo" (v->counter)
	: "r" (&v->counter), "Ir" (i)
	: "cc");
}

static inline int atomic_sub_return(int i, atomic_t *v)
{
	unsigned long tmp;
	int result;

	smp_mb();

	__asm__ __volatile__("@ atomic_sub_return\n"
"1:	ldc	p13, c0, [%3], {2}	@ set address for ldrex\n"
"	mrc	p13, 0, %0, c0, c0, 0	@ ldrex\n"
"	sub	%0, %0, %4\n"
"	mcr	p13, 0, %0, c0, c0, 0	@ data for strex\n"
"	stc	p13, c0, [%3], {2}	@ strex to address\n"
"	mrc	p13, 0, %1, c0, c0, 2	@ strex status\n"
"	teq	%1, #0\n"
"	bne	1b"
	: "=&r" (result), "=&r" (tmp), "+Qo" (v->counter)
	: "r" (&v->counter), "Ir" (i)
	: "cc");

	smp_mb();

	return result;
}

static inline int atomic_cmpxchg(atomic_t *ptr, int old, int new)
{
	unsigned long oldval, res;

	smp_mb();

	do {
		__asm__ __volatile__("@ atomic_cmpxchg\n"
		"ldc	p13, c0, [%3], {2}	@ set address for ldrex\n"
		"mrc	p13, 0, %1, c0, c0, 0	@ ldrex\n"
		"mov	%0, #0\n"
		"teq	%1, %4\n"
		"mcreq	p13, 0, %5, c0, c0, 0	@ data for strex\n"
		"stceq	p13, c0, [%3], {2}	@ strex to address\n"
		"mrceq	p13, 0, %0, c0, c0, 2	@ strex status\n"
		    : "=&r" (res), "=&r" (oldval), "+Qo" (ptr->counter)
		    : "r" (&ptr->counter), "Ir" (old), "r" (new)
		    : "cc");
	} while (res);

	smp_mb();

	return oldval;
}

static inline void atomic_clear_mask(unsigned long mask, unsigned long *addr)
{
	unsigned long tmp, tmp2;

	__asm__ __volatile__("@ atomic_clear_mask\n"
"1:	ldc	p13, c0, [%3], {2}	@ set address for ldrex\n"
"	mrc	p13, 0, %0, c0, c0, 0	@ ldrex\n"
"	bic	%0, %0, %4\n"
"	mcr	p13, 0, %0, c0, c0, 0	@ data for strex\n"
"	stc	p13, c0, [%3], {2}	@ strex to address\n"
"	mrc	p13, 0, %1, c0, c0, 2	@ strex status\n"
"	teq	%1, #0\n"
"	bne	1b"
	: "=&r" (tmp), "=&r" (tmp2), "+Qo" (*addr)
	: "r" (addr), "Ir" (mask)
	: "cc");
}

#else /* ARM_ARCH_6 */

#ifdef CONFIG_SMP
#error SMP not supported on pre-ARMv6 CPUs
#endif

#define ATOMIC_OP(op, c_op, asm_op)					\
static inline void atomic_##op(int i, atomic_t *v)			\
{									\
	unsigned long flags;						\
									\
	raw_local_irq_save(flags);					\
	v->counter c_op i;						\
	raw_local_irq_restore(flags);					\
}									\

#define ATOMIC_OP_RETURN(op, c_op, asm_op)				\
static inline int atomic_##op##_return(int i, atomic_t *v)		\
{									\
	unsigned long flags;						\
	int val;							\
									\
	raw_local_irq_save(flags);					\
	v->counter c_op i;						\
	val = v->counter;						\
	raw_local_irq_restore(flags);					\
									\
	return val;							\
}

#define ATOMIC_FETCH_OP(op, c_op, asm_op)				\
static inline int atomic_fetch_##op(int i, atomic_t *v)			\
{									\
	unsigned long flags;						\
	int val;							\
									\
	raw_local_irq_save(flags);					\
	val = v->counter;						\
	v->counter c_op i;						\
	raw_local_irq_restore(flags);					\
									\
	return val;							\
}

static inline int atomic_cmpxchg(atomic_t *v, int old, int new)
{
	int ret;
	unsigned long flags;

	raw_local_irq_save(flags);
	ret = v->counter;
	if (likely(ret == old))
		v->counter = new;
	raw_local_irq_restore(flags);

	return ret;
}

#define atomic_fetch_andnot		atomic_fetch_andnot

#endif /* __LINUX_ARM_ARCH__ */

#define ATOMIC_OPS(op, c_op, asm_op)					\
	ATOMIC_OP(op, c_op, asm_op)					\
	ATOMIC_OP_RETURN(op, c_op, asm_op)				\
	ATOMIC_FETCH_OP(op, c_op, asm_op)

ATOMIC_OPS(add, +=, add)
ATOMIC_OPS(sub, -=, sub)

#define atomic_andnot atomic_andnot

#undef ATOMIC_OPS
#define ATOMIC_OPS(op, c_op, asm_op)					\
	ATOMIC_OP(op, c_op, asm_op)					\
	ATOMIC_FETCH_OP(op, c_op, asm_op)

ATOMIC_OPS(and, &=, and)
ATOMIC_OPS(andnot, &= ~, bic)
ATOMIC_OPS(or,  |=, orr)
ATOMIC_OPS(xor, ^=, eor)

#undef ATOMIC_OPS
#undef ATOMIC_FETCH_OP
#undef ATOMIC_OP_RETURN
#undef ATOMIC_OP

#define atomic_xchg(v, new) (xchg(&((v)->counter), new))

#ifndef CONFIG_GENERIC_ATOMIC64
typedef struct {
	s64 counter;
} atomic64_t;

#define ATOMIC64_INIT(i) { (i) }

#ifndef CONFIG_CPU_FMP626
#ifdef CONFIG_ARM_LPAE
static inline s64 atomic64_read(const atomic64_t *v)
{
	s64 result;

	__asm__ __volatile__("@ atomic64_read\n"
"	ldrd	%0, %H0, [%1]"
	: "=&r" (result)
	: "r" (&v->counter), "Qo" (v->counter)
	);

	return result;
}

static inline void atomic64_set(atomic64_t *v, s64 i)
{
	__asm__ __volatile__("@ atomic64_set\n"
"	strd	%2, %H2, [%1]"
	: "=Qo" (v->counter)
	: "r" (&v->counter), "r" (i)
	);
}
#else
static inline s64 atomic64_read(const atomic64_t *v)
{
	s64 result;

	__asm__ __volatile__("@ atomic64_read\n"
"	ldrexd	%0, %H0, [%1]"
	: "=&r" (result)
	: "r" (&v->counter), "Qo" (v->counter)
	);

	return result;
}

static inline void atomic64_set(atomic64_t *v, s64 i)
{
	s64 tmp;

	prefetchw(&v->counter);
	__asm__ __volatile__("@ atomic64_set\n"
"1:	ldrexd	%0, %H0, [%2]\n"
"	strexd	%0, %3, %H3, [%2]\n"
"	teq	%0, #0\n"
"	bne	1b"
	: "=&r" (tmp), "=Qo" (v->counter)
	: "r" (&v->counter), "r" (i)
	: "cc");
}
#endif

#define ATOMIC64_OP(op, op1, op2)					\
static inline void atomic64_##op(s64 i, atomic64_t *v)			\
{									\
	s64 result;							\
	unsigned long tmp;						\
									\
	prefetchw(&v->counter);						\
	__asm__ __volatile__("@ atomic64_" #op "\n"			\
"1:	ldrexd	%0, %H0, [%3]\n"					\
"	" #op1 " %Q0, %Q0, %Q4\n"					\
"	" #op2 " %R0, %R0, %R4\n"					\
"	strexd	%1, %0, %H0, [%3]\n"					\
"	teq	%1, #0\n"						\
"	bne	1b"							\
	: "=&r" (result), "=&r" (tmp), "+Qo" (v->counter)		\
	: "r" (&v->counter), "r" (i)					\
	: "cc");							\
}									\

#define ATOMIC64_OP_RETURN(op, op1, op2)				\
static inline s64							\
atomic64_##op##_return_relaxed(s64 i, atomic64_t *v)			\
{									\
	s64 result;							\
	unsigned long tmp;						\
									\
	prefetchw(&v->counter);						\
									\
	__asm__ __volatile__("@ atomic64_" #op "_return\n"		\
"1:	ldrexd	%0, %H0, [%3]\n"					\
"	" #op1 " %Q0, %Q0, %Q4\n"					\
"	" #op2 " %R0, %R0, %R4\n"					\
"	strexd	%1, %0, %H0, [%3]\n"					\
"	teq	%1, #0\n"						\
"	bne	1b"							\
	: "=&r" (result), "=&r" (tmp), "+Qo" (v->counter)		\
	: "r" (&v->counter), "r" (i)					\
	: "cc");							\
									\
	return result;							\
}

#define ATOMIC64_FETCH_OP(op, op1, op2)					\
static inline s64							\
atomic64_fetch_##op##_relaxed(s64 i, atomic64_t *v)			\
{									\
	s64 result, val;						\
	unsigned long tmp;						\
									\
	prefetchw(&v->counter);						\
									\
	__asm__ __volatile__("@ atomic64_fetch_" #op "\n"		\
"1:	ldrexd	%0, %H0, [%4]\n"					\
"	" #op1 " %Q1, %Q0, %Q5\n"					\
"	" #op2 " %R1, %R0, %R5\n"					\
"	strexd	%2, %1, %H1, [%4]\n"					\
"	teq	%2, #0\n"						\
"	bne	1b"							\
	: "=&r" (result), "=&r" (val), "=&r" (tmp), "+Qo" (v->counter)	\
	: "r" (&v->counter), "r" (i)					\
	: "cc");							\
									\
	return result;							\
}

#define ATOMIC64_OPS(op, op1, op2)					\
	ATOMIC64_OP(op, op1, op2)					\
	ATOMIC64_OP_RETURN(op, op1, op2)				\
	ATOMIC64_FETCH_OP(op, op1, op2)

ATOMIC64_OPS(add, adds, adc)
ATOMIC64_OPS(sub, subs, sbc)

#define atomic64_add_return_relaxed	atomic64_add_return_relaxed
#define atomic64_sub_return_relaxed	atomic64_sub_return_relaxed
#define atomic64_fetch_add_relaxed	atomic64_fetch_add_relaxed
#define atomic64_fetch_sub_relaxed	atomic64_fetch_sub_relaxed

#undef ATOMIC64_OPS
#define ATOMIC64_OPS(op, op1, op2)					\
	ATOMIC64_OP(op, op1, op2)					\
	ATOMIC64_FETCH_OP(op, op1, op2)

#define atomic64_andnot atomic64_andnot

ATOMIC64_OPS(and, and, and)
ATOMIC64_OPS(andnot, bic, bic)
ATOMIC64_OPS(or,  orr, orr)
ATOMIC64_OPS(xor, eor, eor)

#define atomic64_fetch_and_relaxed	atomic64_fetch_and_relaxed
#define atomic64_fetch_andnot_relaxed	atomic64_fetch_andnot_relaxed
#define atomic64_fetch_or_relaxed	atomic64_fetch_or_relaxed
#define atomic64_fetch_xor_relaxed	atomic64_fetch_xor_relaxed

#undef ATOMIC64_OPS
#undef ATOMIC64_FETCH_OP
#undef ATOMIC64_OP_RETURN
#undef ATOMIC64_OP

static inline s64 atomic64_cmpxchg_relaxed(atomic64_t *ptr, s64 old, s64 new)
{
	s64 oldval;
	unsigned long res;

	prefetchw(&ptr->counter);

	do {
		__asm__ __volatile__("@ atomic64_cmpxchg\n"
		"ldrexd		%1, %H1, [%3]\n"
		"mov		%0, #0\n"
		"teq		%1, %4\n"
		"teqeq		%H1, %H4\n"
		"strexdeq	%0, %5, %H5, [%3]"
		: "=&r" (res), "=&r" (oldval), "+Qo" (ptr->counter)
		: "r" (&ptr->counter), "r" (old), "r" (new)
		: "cc");
	} while (res);

	return oldval;
}
#define atomic64_cmpxchg_relaxed	atomic64_cmpxchg_relaxed

static inline s64 atomic64_xchg_relaxed(atomic64_t *ptr, s64 new)
{
	s64 result;
	unsigned long tmp;

	prefetchw(&ptr->counter);

	__asm__ __volatile__("@ atomic64_xchg\n"
"1:	ldrexd	%0, %H0, [%3]\n"
"	strexd	%1, %4, %H4, [%3]\n"
"	teq	%1, #0\n"
"	bne	1b"
	: "=&r" (result), "=&r" (tmp), "+Qo" (ptr->counter)
	: "r" (&ptr->counter), "r" (new)
	: "cc");

	return result;
}
#define atomic64_xchg_relaxed		atomic64_xchg_relaxed

static inline s64 atomic64_dec_if_positive(atomic64_t *v)
{
	s64 result;
	unsigned long tmp;

	smp_mb();
	prefetchw(&v->counter);

	__asm__ __volatile__("@ atomic64_dec_if_positive\n"
"1:	ldrexd	%0, %H0, [%3]\n"
"	subs	%Q0, %Q0, #1\n"
"	sbc	%R0, %R0, #0\n"
"	teq	%R0, #0\n"
"	bmi	2f\n"
"	strexd	%1, %0, %H0, [%3]\n"
"	teq	%1, #0\n"
"	bne	1b\n"
"2:"
	: "=&r" (result), "=&r" (tmp), "+Qo" (v->counter)
	: "r" (&v->counter)
	: "cc");

	smp_mb();

	return result;
}
#define atomic64_dec_if_positive atomic64_dec_if_positive

static inline s64 atomic64_fetch_add_unless(atomic64_t *v, s64 a, s64 u)
{
	s64 oldval, newval;
	unsigned long tmp;

	smp_mb();
	prefetchw(&v->counter);

	__asm__ __volatile__("@ atomic64_add_unless\n"
"1:	ldrexd	%0, %H0, [%4]\n"
"	teq	%0, %5\n"
"	teqeq	%H0, %H5\n"
"	beq	2f\n"
"	adds	%Q1, %Q0, %Q6\n"
"	adc	%R1, %R0, %R6\n"
"	strexd	%2, %1, %H1, [%4]\n"
"	teq	%2, #0\n"
"	bne	1b\n"
"2:"
	: "=&r" (oldval), "=&r" (newval), "=&r" (tmp), "+Qo" (v->counter)
	: "r" (&v->counter), "r" (u), "r" (a)
	: "cc");

	if (oldval != u)
		smp_mb();

	return oldval;
}
#define atomic64_fetch_add_unless atomic64_fetch_add_unless

#else	/* CONFIG_CPU_FMP626 */
static inline u64 atomic64_read(const atomic64_t *v)
{
	u64 result;

	__asm__ __volatile__("@ atomic64_read\n"
"	ldc	p13, c0, [%1], {3}	@ set address for ldrexd\n"
"	mrc	p13, 0, %0, c0, c0, 0	@ ldrexd\n"
"	mrc	p13, 0, %H0, c0, c0, 1	@ ldrexd\n"
	: "=&r" (result)
	: "r" (&v->counter), "Qo" (v->counter)
	);

	return result;
}

static inline void atomic64_set(atomic64_t *v, u64 i)
{
	u64 tmp;

	__asm__ __volatile__("@ atomic64_set\n"
"1:	ldc	p13, c0, [%2], {3}	@ set address for ldrexd\n"
"	mrc	p13, 0, %0, c0, c0, 0	@ ldrexd\n"
"	mrc	p13, 0, %H0, c0, c0, 1	@ ldrexd\n"
"	mcr	p13, 0, %3, c0, c0, 0	@ data for strexd\n"
"	mcr	p13, 0, %H3, c0, c0, 1	@ data for strexd\n"
"	stc	p13, c0, [%2], {3}	@ strexd to address\n"
"	mrc	p13, 0, %0, c0, c0, 2	@ strexd status\n"
"	teq	%0, #0\n"
"	bne	1b"
	: "=&r" (tmp), "=Qo" (v->counter)
	: "r" (&v->counter), "r" (i)
	: "cc");
}

static inline void atomic64_add(u64 i, atomic64_t *v)
{
	u64 result;
	unsigned long tmp;

	__asm__ __volatile__("@ atomic64_add\n"
"1:	ldc	p13, c0, [%3], {3}	@ set address for ldrexd\n"
"	mrc	p13, 0, %0, c0, c0, 0	@ ldrexd\n"
"	mrc	p13, 0, %H0, c0, c0, 1	@ ldrexd\n"
"	adds	%0, %0, %4\n"
"	adc	%H0, %H0, %H4\n"
"	mcr	p13, 0, %0, c0, c0, 0	@ data for strexd\n"
"	mcr	p13, 0, %H0, c0, c0, 1	@ data for strexd\n"
"	stc	p13, c0, [%3], {3}	@ strexd to address\n"
"	mrc	p13, 0, %1, c0, c0, 2	@ strexd status\n"
"	teq	%1, #0\n"
"	bne	1b"
	: "=&r" (result), "=&r" (tmp), "+Qo" (v->counter)
	: "r" (&v->counter), "r" (i)
	: "cc");
}

static inline u64 atomic64_add_return(u64 i, atomic64_t *v)
{
	u64 result;
	unsigned long tmp;

	smp_mb();

	__asm__ __volatile__("@ atomic64_add_return\n"
"1:	ldc	p13, c0, [%3], {3}	@ set address for ldrexd\n"
"	mrc	p13, 0, %0, c0, c0, 0	@ ldrexd\n"
"	mrc	p13, 0, %H0, c0, c0, 1	@ ldrexd\n"
"	adds	%0, %0, %4\n"
"	adc	%H0, %H0, %H4\n"
"	mcr	p13, 0, %0, c0, c0, 0	@ data for strexd\n"
"	mcr	p13, 0, %H0, c0, c0, 1	@ data for strexd\n"
"	stc	p13, c0, [%3], {3}	@ strexd to address\n"
"	mrc	p13, 0, %1, c0, c0, 2	@ strexd status\n"
"	teq	%1, #0\n"
"	bne	1b"
	: "=&r" (result), "=&r" (tmp), "+Qo" (v->counter)
	: "r" (&v->counter), "r" (i)
	: "cc");

	smp_mb();

	return result;
}

static inline void atomic64_sub(u64 i, atomic64_t *v)
{
	u64 result;
	unsigned long tmp;

	__asm__ __volatile__("@ atomic64_sub\n"
"1:	ldc	p13, c0, [%3], {3}	@ set address for ldrexd\n"
"	mrc	p13, 0, %0, c0, c0, 0	@ ldrexd\n"
"	mrc	p13, 0, %H0, c0, c0, 1	@ ldrexd\n"
"	subs	%0, %0, %4\n"
"	sbc	%H0, %H0, %H4\n"
"	mcr	p13, 0, %0, c0, c0, 0	@ data for strexd\n"
"	mcr	p13, 0, %H0, c0, c0, 1	@ data for strexd\n"
"	stc	p13, c0, [%3], {3}	@ strexd to address\n"
"	mrc	p13, 0, %1, c0, c0, 2	@ strexd status\n"
"	teq	%1, #0\n"
"	bne	1b"
	: "=&r" (result), "=&r" (tmp), "+Qo" (v->counter)
	: "r" (&v->counter), "r" (i)
	: "cc");
}

static inline u64 atomic64_sub_return(u64 i, atomic64_t *v)
{
	u64 result;
	unsigned long tmp;

	smp_mb();

	__asm__ __volatile__("@ atomic64_sub_return\n"
"1:	ldc	p13, c0, [%3], {3}	@ set address for ldrexd\n"
"	mrc	p13, 0, %0, c0, c0, 0	@ ldrexd\n"
"	mrc	p13, 0, %H0, c0, c0, 1	@ ldrexd\n"
"	subs	%0, %0, %4\n"
"	sbc	%H0, %H0, %H4\n"
"	mcr	p13, 0, %0, c0, c0, 0	@ data for strexd\n"
"	mcr	p13, 0, %H0, c0, c0, 1	@ data for strexd\n"
"	stc	p13, c0, [%3], {3}	@ strexd to address\n"
"	mrc	p13, 0, %1, c0, c0, 2	@ strexd status\n"
"	teq	%1, #0\n"
"	bne	1b"
	: "=&r" (result), "=&r" (tmp), "+Qo" (v->counter)
	: "r" (&v->counter), "r" (i)
	: "cc");

	smp_mb();

	return result;
}

static inline u64 atomic64_cmpxchg(atomic64_t *ptr, u64 old, u64 new)
{
	u64 oldval;
	unsigned long res;

	smp_mb();

	do {
		__asm__ __volatile__("@ atomic64_cmpxchg\n"
		"ldc	p13, c0, [%3], {3}	@ set address for ldrexd\n"
		"mrc	p13, 0, %1, c0, c0, 0	@ ldrexd\n"
		"mrc	p13, 0, %H1, c0, c0, 1	@ ldrexd\n"
		"mov	%0, #0\n"
		"teq	%1, %4\n"
		"teqeq	%H1, %H4\n"
		"mcreq	p13, 0, %5, c0, c0, 0	@ data for strexd\n"
		"mcreq	p13, 0, %H5, c0, c0, 1	@ data for strexd\n"
		"stceq	p13, c0, [%3], {3}	@ strexd to address\n"
		"mrceq	p13, 0, %0, c0, c0, 2	@ strexd status\n"
		: "=&r" (res), "=&r" (oldval), "+Qo" (ptr->counter)
		: "r" (&ptr->counter), "r" (old), "r" (new)
		: "cc");
	} while (res);

	smp_mb();

	return oldval;
}

static inline u64 atomic64_xchg(atomic64_t *ptr, u64 new)
{
	u64 result;
	unsigned long tmp;

	smp_mb();

	__asm__ __volatile__("@ atomic64_xchg\n"
"1:	ldc	p13, c0, [%3], {3}	@ set address for ldrexd\n"
"	mrc	p13, 0, %0, c0, c0, 0	@ ldrexd\n"
"	mrc	p13, 0, %H0, c0, c0, 1	@ ldrexd\n"
"	mcr	p13, 0, %4, c0, c0, 0	@ data for strexd\n"
"	mcr	p13, 0, %H4, c0, c0, 1	@ data for strexd\n"
"	stc	p13, c0, [%3], {3}	@ strexd to address\n"
"	mrc	p13, 0, %1, c0, c0, 2	@ strexd status\n"
"	teq	%1, #0\n"
"	bne	1b"
	: "=&r" (result), "=&r" (tmp), "+Qo" (ptr->counter)
	: "r" (&ptr->counter), "r" (new)
	: "cc");

	smp_mb();

	return result;
}

static inline u64 atomic64_dec_if_positive(atomic64_t *v)
{
	u64 result;
	unsigned long tmp;

	smp_mb();

	__asm__ __volatile__("@ atomic64_dec_if_positive\n"
"1:	ldc	p13, c0, [%3], {3}	@ set address for ldrexd\n"
"	mrc	p13, 0, %0, c0, c0, 0	@ ldrexd\n"
"	mrc	p13, 0, %H0, c0, c0, 1	@ ldrexd\n"
"	subs	%0, %0, #1\n"
"	sbc	%H0, %H0, #0\n"
"	teq	%H0, #0\n"
"	bmi	2f\n"
"	mcr	p13, 0, %0, c0, c0, 0	@ data for strexd\n"
"	mcr	p13, 0, %H0, c0, c0, 1	@ data for strexd\n"
"	stc	p13, c0, [%3], {3}	@ strexd to address\n"
"	mrc	p13, 0, %1, c0, c0, 2	@ strexd status\n"
"	teq	%1, #0\n"
"	bne	1b\n"
"2:"
	: "=&r" (result), "=&r" (tmp), "+Qo" (v->counter)
	: "r" (&v->counter)
	: "cc");

	smp_mb();

	return result;
}

static inline int atomic64_add_unless(atomic64_t *v, u64 a, u64 u)
{
	u64 val;
	unsigned long tmp;
	int ret = 1;

	smp_mb();

	__asm__ __volatile__("@ atomic64_add_unless\n"
"1:	ldc	p13, c0, [%4], {3}	@ set address for ldrexd\n"
"	mrc	p13, 0, %0, c0, c0, 0	@ ldrexd\n"
"	mrc	p13, 0, %H0, c0, c0, 1	@ ldrexd\n"
"	teq	%0, %5\n"
"	teqeq	%H0, %H5\n"
"	moveq	%1, #0\n"
"	beq	2f\n"
"	adds	%0, %0, %6\n"
"	adc	%H0, %H0, %H6\n"
"	mcr	p13, 0, %0, c0, c0, 0	@ data for strexd\n"
"	mcr	p13, 0, %H0, c0, c0, 1	@ data for strexd\n"
"	stc	p13, c0, [%4], {3}	@ strexd to address\n"
"	mrc	p13, 0, %2, c0, c0, 2	@ strexd status\n"
"	teq	%2, #0\n"
"	bne	1b\n"
"2:"
	: "=&r" (val), "+r" (ret), "=&r" (tmp), "+Qo" (v->counter)
	: "r" (&v->counter), "r" (u), "r" (a)
	: "cc");

	if (ret)
		smp_mb();

	return ret;
}
#endif	/* CONFIG_CPU_FMP626 */

#endif /* !CONFIG_GENERIC_ATOMIC64 */
#endif
#endif
