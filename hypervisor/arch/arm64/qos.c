/*
 * ARM QoS Support for Jailhouse
 *
 * Copyright (c) Boston University, 2020
 *
 * Authors:
 *  Renato Mancuso <rmancuso@bu.edu>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 * See the COPYING file in the top-level directory.
 */

#include <jailhouse/printk.h>
#include <jailhouse/mmio.h>
#include <jailhouse/cell.h>
#include <jailhouse/entry.h>
#include <jailhouse/paging.h>
#include <jailhouse/control.h>
#include <jailhouse/qos-common.h>
#include <jailhouse/assert.h>
#include <asm/sysregs.h>
#include <asm/paging.h>
#include <asm/qos-board.h>
#include <asm/qos-400.h>
#include <asm/qos.h>

#ifdef CONFIG_DEBUG
#define qos_print(fmt, ...)			\
	printk("[QoS] " fmt, ##__VA_ARGS__)
#else
#define qos_print(fmt, ...) do { } while (0)
#endif

/* Mapped NIC device */
static void *nic_base = NULL;

static const struct qos_param params[QOS_PARAMS] = {
	{
		.name = "read_qos",
		.reg = READ_QOS,
		.enable = EN_NO_ENABLE,
		.shift = READ_QOS_SHIFT,
		.mask = READ_QOS_MASK,
	},
	{
		.name = "write_qos",
		.reg = WRITE_QOS,
		.enable = EN_NO_ENABLE,
		.shift = WRITE_QOS_SHIFT,
		.mask = WRITE_QOS_MASK,
	},
	{
		.name = "aw_max_otf",
		.reg = MAX_OT,
		.enable = EN_AW_OT_SHIFT,
		.shift = AW_MAX_OTF_SHIFT,
		.mask = AW_MAX_OTF_MASK,
	},
	{
		.name = "aw_max_oti",
		.reg = MAX_OT,
		.enable = EN_AW_OT_SHIFT,
		.shift = AW_MAX_OTI_SHIFT,
		.mask = AW_MAX_OTI_MASK,
	},
	{
		.name = "ar_max_otf",
		.reg = MAX_OT,
		.enable = EN_AR_OT_SHIFT,
		.shift = AR_MAX_OTF_SHIFT,
		.mask = AR_MAX_OTF_MASK,
	},
	{
		.name = "ar_max_oti",
		.reg = MAX_OT,
		.enable = EN_AR_OT_SHIFT,
		.shift = AR_MAX_OTI_SHIFT,
		.mask = AR_MAX_OTI_MASK,
	},
	{
		.name = "awar_max_otf",
		.reg = MAX_COMB_OT,
		.enable = EN_AWAR_OT_SHIFT,
		.shift = AWAR_MAX_OTF_SHIFT,
		.mask = AWAR_MAX_OTF_MASK,
	},
	{
		.name = "awar_max_oti",
		.reg = MAX_COMB_OT,
		.enable = EN_AWAR_OT_SHIFT,
		.shift = AWAR_MAX_OTI_SHIFT,
		.mask = AWAR_MAX_OTI_MASK,
	},
	{
		.name = "aw_p",
		.reg = AW_P,
		.enable = EN_AW_RATE_SHIFT,
		.shift = AW_P_SHIFT,
		.mask = AW_P_MASK,
	},
	{
		.name = "aw_b",
		.reg = AW_B,
		.enable = EN_AW_RATE_SHIFT,
		.shift = AW_B_SHIFT,
		.mask = AW_B_MASK,
	},
	{
		.name = "aw_r",
		.reg = AW_R,
		.enable = EN_AW_RATE_SHIFT,
		.shift = AW_R_SHIFT,
		.mask = AW_R_MASK,
	},
	{
		.name = "ar_p",
		.reg = AR_P,
		.enable = EN_AR_RATE_SHIFT,
		.shift = AR_P_SHIFT,
		.mask = AR_P_MASK,
	},
	{
		.name = "ar_b",
		.reg = AR_B,
		.enable = EN_AR_RATE_SHIFT,
		.shift = AR_B_SHIFT,
		.mask = AR_B_MASK,
	},
	{
		.name = "ar_r",
		.reg = AR_R,
		.enable = EN_AR_RATE_SHIFT,
		.shift = AR_R_SHIFT,
		.mask = AR_R_MASK,
	},
	{
		.name = "ar_tgt_latency",
		.reg = TGT_LATENCY,
		.enable = EN_AR_LATENCY_SHIFT,
		.shift = AR_TGT_LAT_SHIFT,
		.mask = AR_TGT_LAT_MASK,
	},
	{
		.name = "aw_tgt_latency",
		.reg = TGT_LATENCY,
		.enable = EN_AW_LATENCY_SHIFT,
		.shift = AW_TGT_LAT_SHIFT,
		.mask = AW_TGT_LAT_MASK,
	},
	{
		.name = "ar_ki",
		.reg = KI,
		.enable = EN_AR_LATENCY_SHIFT,
		.shift = AR_KI_SHIFT,
		.mask = AR_KI_MASK,
	},
	{
		.name = "aw_ki",
		.reg = KI,
		.enable = EN_AW_LATENCY_SHIFT,
		.shift = AW_KI_SHIFT,
		.mask = AW_KI_MASK,
	},
	{
		.name = "ar_max_qos",
		.reg = QOS_RANGE,
		.enable = EN_AW_LATENCY_SHIFT,
		.shift = AR_MAX_QOS_SHIFT,
		.mask = AR_MAX_QOS_MASK,
	},
	{
		.name = "ar_min_qos",
		.reg = QOS_RANGE,
		.enable = EN_AW_LATENCY_SHIFT,
		.shift = AR_MIN_QOS_SHIFT,
		.mask = AR_MIN_QOS_MASK,
	},
	{
		.name = "aw_max_qos",
		.reg = QOS_RANGE,
		.enable = EN_AW_LATENCY_SHIFT,
		.shift = AW_MAX_QOS_SHIFT,
		.mask = AW_MAX_QOS_MASK,
	},
	{
		.name = "aw_min_qos",
		.reg = QOS_RANGE,
		.enable = EN_AW_LATENCY_SHIFT,
		.shift = AW_MIN_QOS_SHIFT,
		.mask = AW_MIN_QOS_MASK,
	},
};

/* Return the address of the configuration register to be used to set
 * the desired paramter for the selected device */
static inline void* reg_qos_par(
		const struct jailhouse_qos_device *dev,
		const struct qos_param *param)
{
	assert(nic_base != NULL);
	return (void *)(nic_base + (__u64)dev->base + (__u64)param->reg);
}

static inline void* reg_qos_off(
		const struct jailhouse_qos_device *dev,
		__u64 off)
{
	assert(nic_base != NULL);
	return (void *)(nic_base + (__u64)dev->base + off);
}

/* Find QoS-enabled device by name */
static inline const struct jailhouse_qos_device* qos_dev_find_by_name(char *name)
{
	const struct jailhouse_qos_device *devices;
	devices = jailhouse_cell_qos_devices(root_cell.config);

	for (unsigned i = 0; i < root_cell.config->num_qos_devices; ++i) {
		if (strncmp(name, devices[i].name, QOS_DEV_NAMELEN) == 0) {
			return &devices[i];
		}
	}

	return NULL;
}


/* Find QoS parameter by name */
static inline const struct qos_param *qos_param_find_by_name(char *name)
{
	for (unsigned i = 0; i < QOS_PARAMS; ++i) {
		if (strncmp(name, params[i].name, QOS_PARAM_NAMELEN) == 0) {
			return &params[i];
		}
	}

	return NULL;
}

/* Low-level functions to configure different aspects of the QoS
 * infrastructure */

/* This function sets a given parameter to the desired value. It does
 * not enable the corresponding interface */
static inline int qos_set_param(
	const struct jailhouse_qos_device *dev,
	const struct qos_param *param,
	unsigned long value)
{
	void *reg = reg_qos_par(dev, param);
	__u32 regval;

	/* TODO check that device supports this parameter */
	qos_print("QoS: Dev [%s], Param [%s] = 0x%08lx (reg off: +0x%08llx)\n",
	       dev->name, param->name, value, (__u64)(reg - nic_base));

	regval = qos_read32(reg);
	regval &= ~(param->mask << param->shift);
	regval |= ((value & param->mask) << param->shift);
	qos_write32(reg, regval);

	return 0;
}

/* This function returns true if the selected device supports setting the
 * considered parameter */
static inline bool qos_dev_is_capable(
	const struct jailhouse_qos_device *dev __attribute__((unused)),
	const struct qos_param *param __attribute__((unused)))
{
	/* TODO to be implemented */
	return true;
}

/* Once we are done setting all the parameters, enable all the affected interfaces */
static inline void qos_set_enable(
	const struct jailhouse_qos_device *dev,
	__u32 value)
{
	void *reg = reg_qos_off(dev, QOS_CNTL);

	/* Mask away the no-enable bit */
	value &= ~(1 << EN_NO_ENABLE);
	/* Set the enable bit in the corresponding device */
	qos_write32(reg, value);
}

/* Main function to apply a set of QoS paramters passed via the array
 * settings. The length of the array is specified in the second
 * parameter. */
static int qos_apply_settings(struct qos_setting *settings, int count)
{
	const struct jailhouse_qos_device *cur_dev = NULL;
	const struct qos_param *param;
	__u32 enable_val = 0;

	for (int i = 0; i < count; ++i) {
		char *dev_name = settings[i].dev_name;

		/* We are about to change device. Set the enable
		 * register for the current device */
		if (dev_name[0] && cur_dev) {
			qos_set_enable(cur_dev, enable_val);
			enable_val = 0;
		}

		if (dev_name[0])
			cur_dev = qos_dev_find_by_name(dev_name);

		/* At this point, the cur_dev should not be NULL */
		if(cur_dev == NULL)
			return -ENODEV;

		param = qos_param_find_by_name(settings[i].param_name);
		if(param == NULL)
			return -EINVAL;

		/* Check that this device implements this QoS interface */
		if(!qos_dev_is_capable(cur_dev, param))
			return -ENOSYS;

		enable_val |= 1 << param->enable;
		qos_set_param(cur_dev, param, settings[i].value);
	}

	/* Apply settings for the last device */
	qos_set_enable(cur_dev, enable_val);

	return 0;
}

/* Clear the QOS_CNTL register for all the devices */
static int qos_disable_all(void)
{
	const struct jailhouse_qos_device *devices;
	devices = jailhouse_cell_qos_devices(root_cell.config);

	for (unsigned i = 0; i < root_cell.config->num_qos_devices; i++) {
		qos_set_enable(devices, 0);
		devices++;
	}

	return 0;
}

/* Main entry point for QoS management call */
int qos_call(unsigned long count, unsigned long settings_ptr)
{
	unsigned long sett_page_offs = settings_ptr & ~PAGE_MASK;
	unsigned int sett_pages;
	void *sett_mapping;
	struct qos_setting *settings;

	/* Check if the NIC needs to be mapped */
	if (nic_base == NULL) {
		nic_base = qos_map_device(
			(unsigned long)system_config->platform_info.qos.nic_base,
			(unsigned long)system_config->platform_info.qos.nic_size);
		if (nic_base == NULL)
			return -ENOSYS;
	}

	/* The settings currently reside in kernel memory. Use
	 * temporary mapping to make the settings readable by the
	 * hypervisor. No need to clean up the mapping because this is
	 * only temporary by design. */
	sett_pages = PAGES(sett_page_offs + sizeof(struct qos_setting) * count);
	sett_mapping = paging_get_guest_pages(NULL, settings_ptr, sett_pages,
					      PAGE_READONLY_FLAGS);

	if (sett_mapping == NULL) {
		return -ENOMEM;
	}

	settings = (struct qos_setting *)(sett_mapping + sett_page_offs);
	/* Check if the user has requestes QoS control to be disabled */
	if ((count > 0) && (strncmp("disable", settings[0].dev_name, 8) == 0)) {
		return qos_disable_all();
	}

	/* Otherwise, just apply the parameters */
	return qos_apply_settings(settings, count);
}
