// SPDX-License-Identifier: GPL-2.0
/*
 * Xilinx Zynq MPSoC Firmware layer
 *
 *  Copyright (C) 2014-2021 Xilinx, Inc.
 *
 *  Michal Simek <michal.simek@xilinx.com>
 *  Davorin Mista <davorin.mista@aggios.com>
 *  Jolly Shah <jollys@xilinx.com>
 *  Rajan Vaja <rajanv@xilinx.com>
 */

#include <jailhouse/printk.h>
#include <asm/smc.h>
#include <asm/zynqmp-pm.h>

/**
 * upper_32_bits - return bits 32-63 of a number
 * @n: the number we're accessing
 *
 * A basic shift-right of a 64- or 32-bit quantity.  Use this to suppress
 * the "right shift count >= width of type" warning when that quantity is
 * 32-bits.
 */
#define upper_32_bits(n) ((u32)(((n) >> 16) >> 16))

/**
 * lower_32_bits - return bits 0-31 of a number
 * @n: the number we're accessing
 */
#define lower_32_bits(n) ((u32)(n))


/**
 * zynqmp_pm_ret_code() - Convert PMU-FW error codes to error codes
 * @ret_status:		PMUFW return code
 *
 * Return: corresponding Linux error code
 */
static int zynqmp_pm_ret_code(u32 ret_status)
{
	switch (ret_status) {
	case XST_PM_SUCCESS:
	case XST_PM_DOUBLE_REQ:
		return 0;
	case XST_PM_NO_FEATURE:
		return -1; //-ENOTSUPP;
	case XST_PM_INVALID_VERSION:
		return -1; //-ENOTSUPP;
	case XST_PM_NO_ACCESS:
		return -1; //-EACCES;
	case XST_PM_ABORT_SUSPEND:
		return -1; //-ECANCELED;
	case XST_PM_MULT_USER:
		return -1; //-EUSERS;
	case XST_PM_INTERNAL:
	case XST_PM_CONFLICT:
	case XST_PM_INVALID_NODE:
	case XST_PM_INVALID_CRC:
	default:
		return -1; //-EINVAL;
	}
}


/**
 * do_fw_call_smc() - Call system-level platform management layer (SMC)
 * @arg0:		Argument 0 to SMC call
 * @arg1:		Argument 1 to SMC call
 * @arg2:		Argument 2 to SMC call
 * @arg3:		Argument 3 to SMC call
 * @ret_payload:	Returned value array
 *
 * Invoke platform management function via SMC call (no hypervisor present).
 *
 * Return: Returns status, either success or error+reason
 */
static int do_fw_call_smc(u64 arg0, u64 arg1, u64 arg2, u64 arg3,
				   u32 *ret_payload)
{
	struct arm_smccc_res_values res;
	
	smc_arg3(arg0, arg1, arg2, arg3, &res);
	// DEBUG
	// printk("res.a0 = %ld\n", res.a0);
	// printk("res.a1 = %ld\n", res.a1);
	// printk("res.a2 = %ld\n", res.a2);
	// printk("res.a3 = %ld\n", res.a3);
	
	if (ret_payload) {
		ret_payload[0] = lower_32_bits(res.a0);
		ret_payload[1] = upper_32_bits(res.a0);
		ret_payload[2] = lower_32_bits(res.a1);
		ret_payload[3] = upper_32_bits(res.a1);
		// DEBUG
		// printk("ret_payload[0] = %d\n", ret_payload[0]);
		// printk("ret_payload[1] = %d\n", ret_payload[1]);
		// printk("ret_payload[2] = %d\n", ret_payload[2]);
		// printk("ret_payload[3] = %d\n", ret_payload[3]);
	}

	return zynqmp_pm_ret_code((enum pm_ret_status)res.a0);
	return 0;
}


/**
 * zynqmp_pm_invoke_fn() - Invoke the system-level platform management layer
 *			   caller function depending on the configuration
 * @pm_api_id:		Requested PM-API call
 * @arg0:		Argument 0 to requested PM-API call
 * @arg1:		Argument 1 to requested PM-API call
 * @arg2:		Argument 2 to requested PM-API call
 * @arg3:		Argument 3 to requested PM-API call
 * @arg4:		Argument 4 to requested PM-API call
 * @ret_payload:	Returned value array
 *
 * Invoke platform management function for SMC
 * Following SMC Calling Convention (SMCCC) for SMC64:
 * Pm Function Identifier,
 * PM_SIP_SVC + PM_API_ID =
 *	((SMC_TYPE_FAST << FUNCID_TYPE_SHIFT)
 *	((SMC_64) << FUNCID_CC_SHIFT)
 *	((SIP_START) << FUNCID_OEN_SHIFT)
 *	((PM_API_ID) & FUNCID_NUM_MASK))
 *
 * PM_SIP_SVC	- Registered ZynqMP SIP Service Call.
 * PM_API_ID	- Platform Management API ID.
 *
 * Return: Returns status, either success or error+reason
 */
int zynqmp_pm_invoke_fn(u32 pm_api_id, u32 arg0, u32 arg1,
			u32 arg2, u32 arg3, u32 arg4,
			u32 *ret_payload)
{
	/*
	 * Added SIP service call Function Identifier
	 * Make sure to stay in x0 register
	 */
	u64 smc_arg[4];
	// int ret;

	/* Check if feature is supported or not */
	// ret = zynqmp_pm_feature(pm_api_id);
	// if (ret < 0)
	// 	return ret;

	smc_arg[0] = PM_SIP_SVC | pm_api_id;
	smc_arg[1] = ((u64)arg1 << 32) | arg0;
	smc_arg[2] = ((u64)arg3 << 32) | arg2;
	smc_arg[3] = ((u64)arg4);

	return do_fw_call_smc(smc_arg[0], smc_arg[1], smc_arg[2], smc_arg[3],
			  ret_payload);
}

int zynqmp_pm_get_api_version(u32 *version)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_get_chipid(u32 *idcode, u32 *version)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_query_data(struct zynqmp_pm_query_data qdata,
				       u32 *out)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_clock_enable(u32 clock_id)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_clock_disable(u32 clock_id)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_clock_getstate(u32 clock_id, u32 *state)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_clock_setdivider(u32 clock_id, u32 divider)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_clock_getdivider(u32 clock_id, u32 *divider)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_clock_setrate(u32 clock_id, u64 rate)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_clock_getrate(u32 clock_id, u64 *rate)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_clock_setparent(u32 clock_id, u32 parent_id)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_clock_getparent(u32 clock_id, u32 *parent_id)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_set_pll_frac_mode(u32 clk_id, u32 mode)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_get_pll_frac_mode(u32 clk_id, u32 *mode)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_set_pll_frac_data(u32 clk_id, u32 data)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_get_pll_frac_data(u32 clk_id, u32 *data)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_set_sd_tapdelay(u32 node_id, u32 type, u32 value)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_sd_dll_reset(u32 node_id, u32 type)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_ospi_mux_select(u32 dev_id, u32 select)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_reset_assert(const u32 reset,
					 const enum zynqmp_pm_reset_action assert_flag)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_reset_get_status(const u32 reset,
					     u32 *status)
{
	// Not implemeted yet ...
    return -1;
}

unsigned int zynqmp_pm_bootmode_read(u32 *ps_mode)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_bootmode_write(u32 ps_mode)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_init_finalize(void)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_set_suspend_mode(u32 mode)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_request_node(const u32 node, const u32 capabilities,
					 const u32 qos,
					 const enum zynqmp_pm_request_ack ack)
{
	return zynqmp_pm_invoke_fn(PM_REQUEST_NODE, node, capabilities,
				   qos, ack, 0, NULL);
}

int zynqmp_pm_release_node(const u32 node)
{
	return zynqmp_pm_invoke_fn(PM_RELEASE_NODE, node, 0, 0, 0, 0, NULL);
}

int zynqmp_pm_set_requirement(const u32 node,
					    const u32 capabilities,
					    const u32 qos,
					    const enum zynqmp_pm_request_ack ack)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_aes_engine(const u64 address, u32 *out)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_fpga_load(const u64 address, const u32 size,
				      const u32 flags, u32 *status)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_fpga_get_status(u32 *value)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_write_ggs(u32 index, u32 value)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_read_ggs(u32 index, u32 *value)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_write_pggs(u32 index, u32 value)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_read_pggs(u32 index, u32 *value)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_usb_set_state(u32 node, u32 state, u32 value)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_afi(u32 index, u32 value)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_set_tapdelay_bypass(u32 index, u32 value)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_set_sgmii_mode(u32 enable)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_ulpi_reset(void)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_system_shutdown(const u32 type, const u32 subtype)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_set_boot_health_status(u32 value)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_force_pwrdwn(const u32 node,
					 const enum zynqmp_pm_request_ack ack)
{
	return zynqmp_pm_invoke_fn(PM_FORCE_POWERDOWN, node, ack, 0, 0, 0,
				   NULL);
}


/**
 * zynqmp_pm_request_wake - PM call to wake up selected master or subsystem
 * @node:  Node ID of the master or subsystem
 * @set_addr:  Specifies whether the address argument is relevant
 * @address:   Address from which to resume when woken up
 * @ack:   Flag to specify whether acknowledge requested
 *
 * Return: status, either success or error+reason
 */
int zynqmp_pm_request_wake(const u32 node,
					 const bool set_addr,
					 const u64 address,
					 const enum zynqmp_pm_request_ack ack)
{
	/* set_addr flag is encoded into 1st bit of address */
	return zynqmp_pm_invoke_fn(PM_REQUEST_WAKEUP, node, address | set_addr,
				   address >> 32, ack, 0, NULL);
}

int zynqmp_pm_get_rpu_mode(u32 node_id, enum rpu_oper_mode *rpu_mode)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_set_rpu_mode(u32 node_id, u32 arg1)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_set_tcm_config(u32 node_id, u32 arg1)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_clear_aie_npi_isr(u32 node, u32 irq_mask)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_pinctrl_request(const u32 pin)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_pinctrl_release(const u32 pin)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_pinctrl_get_function(const u32 pin, u32 *id)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_is_function_supported(const u32 api_id, const u32 id)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_pinctrl_set_function(const u32 pin, const u32 id)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_pinctrl_get_config(const u32 pin, const u32 param,
					       u32 *value)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_pinctrl_set_config(const u32 pin, const u32 param,
					       u32 value)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_load_pdi(const u32 src, const u64 address)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_efuse_access(const u64 address, u32 *out)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_sha_hash(const u64 address, const u32 size,
				     const u32 flags)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_rsa(const u64 address, const u32 size,
				const u32 flags)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_config_reg_access(u32 register_access_id,
					      u32 address, u32 mask, u32 value,
					      u32 *out)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_mmio_write(u32 address, u32 mask, u32 value)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_mmio_read(u32 address, u32 *out)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_request_suspend(const u32 node,
					    const enum zynqmp_pm_request_ack ack,
					    const u32 latency, const u32 state)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_set_max_latency(const u32 node, const u32 latency)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_set_configuration(const u32 physical_addr)
{
	// Not implemeted yet ...
    return -1;
}


/**
 * zynqmp_pm_get_node_status - PM call to request a node's current power state
 * @node:		ID of the component or sub-system in question
 * @status:		Current operating state of the requested node
 * @requirements:	Current requirements asserted on the node,
 *			used for slave nodes only.
 * @usage:		Usage information, used for slave nodes only:
 *			PM_USAGE_NO_MASTER	- No master is currently using
 *						  the node
 *			PM_USAGE_CURRENT_MASTER	- Only requesting master is
 *						  currently using the node
 *			PM_USAGE_OTHER_MASTER	- Only other masters are
 *						  currently using the node
 *			PM_USAGE_BOTH_MASTERS	- Both the current and at least
 *						  one other master is currently
 *						  using the node
 *
 * Return:		Returns status, either success or error+reason
 */
int zynqmp_pm_get_node_status(const u32 node, u32 *const status,
					    u32 *const requirements,
					    u32 *const usage)
{
	u32 ret_payload[PAYLOAD_ARG_CNT];
	int ret;

	if (!status)
		return -1;

	ret = zynqmp_pm_invoke_fn(PM_GET_NODE_STATUS, node, 0, 0,
				  0, 0, ret_payload);
	if (ret_payload[0] == XST_PM_SUCCESS) {
		*status = ret_payload[1];
		if (requirements)
			*requirements = ret_payload[2];
		if (usage)
			*usage = ret_payload[3];
	}

	return ret;
}

int zynqmp_pm_get_operating_characteristic(const u32 node,
							 const enum zynqmp_pm_opchar_type type,
							 u32 *const result)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_force_powerdown(const u32 target,
					    const enum zynqmp_pm_request_ack ack)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_request_wakeup(const u32 node, const bool set_addr,
					   const u64 address,
					   const enum zynqmp_pm_request_ack ack)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_set_wakeup_source(const u32 target,
					      const u32 wakeup_node,
					      const u32 enable)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_fpga_read(const u32 reg_numframes,
				      const u64 phys_address, u32 readback_type,
				      u32 *value)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_write_aes_key(const u32 keylen, const u32 keysrc, const u64 keyaddr)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_bbram_write_usrdata(u32 data)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_bbram_read_usrdata(const u64 outaddr)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_bbram_write_aeskey(const u64 keyaddr, u16 keylen)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_bbram_zeroize(void)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_bbram_lock_userdata(void)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_probe_counter_read(u32 deviceid, u32 reg, u32 *value)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_probe_counter_write(u32 domain, u32 reg, u32 value)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_secure_load(const u64 src_addr, u64 key_addr, u64 *dst)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_get_last_reset_reason(u32 *reset_reason)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_register_notifier(const u32 node, const u32 event,
					      const u32 wake, const u32 enable)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_feature(const u32 api_id)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_set_feature_config(enum pm_feature_config_id id,
					       u32 value)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_get_feature_config(enum pm_feature_config_id id,
					       u32 *payload)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_get_uid_info(const u64 address, const u32 size,
					 u32 *count)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_sec_read_reg(u32 node_id, u32 offset, u32 *ret_value)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_sec_mask_write_reg(const u32 node_id, const u32 offset,
					u32 mask, u32 value)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_set_sd_config(u32 node,
					  enum pm_sd_config_type config,
					  u32 value)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_get_qos(u32 node, u32 *const def_qos, u32 *const qos)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_set_gem_config(u32 node,
					   enum pm_gem_config_type config,
					   u32 value)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_set_usb_config(u32 node,
					   enum pm_usb_config_type config,
					   u32 value)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_get_meta_header(const u64 src, const u64 dst,
					    const u32 size, u32 *count)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_register_sgi(u32 sgi_num, u32 reset)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_aie_operation(u32 node, u16 start_col,
					  u16 num_col, u32 operation)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_fpga_get_version(u32 *value)
{
	// Not implemeted yet ...
    return -1;
}

int zynqmp_pm_fpga_get_feature_list(u32 *value)
{
	// Not implemeted yet ...
    return -1;
}
