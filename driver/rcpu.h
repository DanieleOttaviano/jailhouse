
#ifndef _JAILHOUSE_RCPU_H
#define _JAILHOUSE_RCPU_H

#include <jailhouse/config.h>
#include "cell.h"

#ifdef CONFIG_OMNIVISOR

int jailhouse_rcpus_setup(void);
int jailhouse_rcpus_check(struct cell *cell);
int jailhouse_rcpus_remove(void);

#else

static inline int jailhouse_rcpus_setup(void)
{
    return 0;
}

static inline int jailhouse_rcpus_check(struct cell *cell)
{
    return 0;
}

static inline int jailhouse_rcpus_remove(void)
{
    return 0;
}

#endif /* CONFIG_OMNIVISOR */
#endif /* !_JAILHOUSE_DRIVER_SYSFS_H */