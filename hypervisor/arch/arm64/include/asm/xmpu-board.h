
#ifndef _JAILHOUSE_ARM64_XMPU_BOARD_H
#define _JAILHOUSE_ARM64_XMPU_BOARD_H

#include <asm/smc.h>


#define ZCU102_XMPU_READ_SMC     0x8400ff04
#define ZCU102_XMPU_WRITE_SMC    0x8400ff05

static inline u32 xmpu_read32(void *addr)
{
	int ret;
	ret = smc_arg1((u64)ZCU102_XMPU_READ_SMC, (unsigned long)(addr));
#ifdef CONFIG_DEBUG
	if (ret < 0)
		printk("XMPU READ: %d\n", ret);
#endif
	return ret;
}

static inline void xmpu_write32(void *addr, u32 val)
{
	int ret __attribute__((unused));
	ret = smc_arg2((u64)ZCU102_XMPU_WRITE_SMC, (unsigned long)addr, val);
#ifdef CONFIG_DEBUG
	if (ret < 0)
		printk("XMPU WRITE: %d\n", ret);
#endif
	return;
}


#endif /* _JAILHOUSE_ARM64_XMPU_BOARD_H */