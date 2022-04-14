
#include <inmate.h>

/* create 64-bit mask with all bits in [last:first] set */
#define BIT_MASK(last, first) \
	((0xffffffffffffffffULL >> (64 - ((last) + 1 - (first)))) << (first))

/* extract the field value at [last:first] from an input of up to 64 bits */
#define GET_FIELD(value, last, first) \
	(((value) & BIT_MASK((last), (first))) >> (first))

#define ESR_EC_SHIFT		(26)
#define ESR_EC(esr)		GET_FIELD((esr), 31, ESR_EC_SHIFT)
/* instruction length */
#define ESR_IL(esr)		GET_FIELD((esr), 25, 25)
/* Instruction specific syndrome */
#define ESR_ISS(esr)		GET_FIELD((esr), 24, 0)
#define SPSR_EL(spsr)           (((spsr) & 0xc) >> 2)

void __attribute__((unused)) vector_sync(struct trap_context *ctx)
{
	printk("== Sync exception:\n");
	printk("elr: %016llx  far: %016llx\n"
	       " lr: %016lx  esr: %02llx %01llx %07llx\n",
			ctx->elr, ctx->far, ctx->regs[30],
			ESR_EC(ctx->esr), ESR_IL(ctx->esr),
			ESR_ISS(ctx->esr));

	unsigned char i;
	for (i = 0; i < 31; i++)
		printk("%sx%d: %016lx%s", i < 10 ? " " : "", i,
				ctx->regs[i], i % 3 == 2 ? "\n" : "  ");
	printk("\n");

	stop();
}
