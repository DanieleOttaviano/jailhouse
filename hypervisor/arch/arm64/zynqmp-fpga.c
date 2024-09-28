/* JAILHOUSE FPGA SUPPORT*/
// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2019 Xilinx, Inc.
 */

#include <jailhouse/fpga-mgr.h>
#include <jailhouse/entry.h>
#include <asm/zynqmp-pm.h>
#include <asm/processor.h>
#include <jailhouse/fpga-common.h>

/* Constant Definitions */
#define IXR_FPGA_DONE_MASK	BIT(3)

#define READ_DMA_SIZE		0x200
#define DUMMY_FRAMES_SIZE	0x64
#define DUMMY_PAD_BYTE		0xFF
#define FPGA_WORD_SIZE		4

/* Error Register */
#define IXR_FPGA_ERR_CRC_ERR		BIT(0)
#define IXR_FPGA_ERR_SECURITY_ERR	BIT(16)

/* Signal Status Register */
#define IXR_FPGA_END_OF_STARTUP		BIT(4)
#define IXR_FPGA_GST_CFG_B		BIT(5)
#define IXR_FPGA_INIT_B_INTERNAL	BIT(11)
#define IXR_FPGA_DONE_INTERNAL_SIGNAL	BIT(13)

#define IXR_FPGA_CONFIG_STAT_OFFSET	7U
#define IXR_FPGA_READ_CONFIG_TYPE	0U

#define XILINX_ZYNQMP_PM_FPGA_READ_BACK		BIT(6)
#define XILINX_ZYNQMP_PM_FPGA_REG_READ_BACK	BIT(7)

#define DEFAULT_FEATURE_LIST	(XILINX_ZYNQMP_PM_FPGA_FULL | \
				 XILINX_ZYNQMP_PM_FPGA_PARTIAL | \
				 XILINX_ZYNQMP_PM_FPGA_AUTHENTICATION_DDR | \
				 XILINX_ZYNQMP_PM_FPGA_AUTHENTICATION_OCM | \
				 XILINX_ZYNQMP_PM_FPGA_ENCRYPTION_USERKEY | \
				 XILINX_ZYNQMP_PM_FPGA_ENCRYPTION_DEVKEY | \
				 XILINX_ZYNQMP_PM_FPGA_READ_BACK | \
				 XILINX_ZYNQMP_PM_FPGA_REG_READ_BACK)
/**
 * struct zynqmp_configreg - Configuration register offsets
 * @reg:	Name of the configuration register.
 * @offset:	Register offset.
 */
struct zynqmp_configreg {
	char *reg;
	u32 offset;
};

static struct zynqmp_configreg cfgreg[] = {
	{.reg = "CRC",		.offset = 0},
	{.reg = "FAR",		.offset = 1},
	{.reg = "FDRI",		.offset = 2},
	{.reg = "FDRO",		.offset = 3},
	{.reg = "CMD",		.offset = 4},
	{.reg = "CTRL0",	.offset = 5},
	{.reg = "MASK",		.offset = 6},
	{.reg = "STAT",		.offset = 7},
	{.reg = "LOUT",		.offset = 8},
	{.reg = "COR0",		.offset = 9},
	{.reg = "MFWR",		.offset = 10},
	{.reg = "CBC",		.offset = 11},
	{.reg = "IDCODE",	.offset = 12},
	{.reg = "AXSS",		.offset = 13},
	{.reg = "COR1",		.offset = 14},
	{.reg = "WBSTR",	.offset = 16},
	{.reg = "TIMER",	.offset = 17},
	{.reg = "BOOTSTS",	.offset = 22},
	{.reg = "CTRL1",	.offset = 24},
	{}
};


/**
 * struct zynqmp_fpga_priv - Private data structure
 * @dev:	Device data structure
 * @feature_list: Firmware supported feature list
 * @version: Firmware version info. The higher 16 bytes belong to
 *           the major version number and the lower 16 bytes belong
 *           to a minor version number.
 * @flags:	flags which is used to identify the bitfile type
 * @size:	Size of the Bit-stream used for readback
 */
struct zynqmp_fpga_priv {
	u32 feature_list;
	u32 version;
	u32 flags;
	u32 size;
};


static enum fpga_mgr_states zynqmp_fpga_ops_state(struct fpga_manager *mgr)
{
	u32 status = 0;

	zynqmp_pm_fpga_get_status(&status);
	if (status & IXR_FPGA_DONE_MASK)
		return FPGA_MGR_STATE_OPERATING;

	return FPGA_MGR_STATE_UNKNOWN;
}

static struct fpga_manager *zynqmp_fpga_mgr = NULL;

struct fpga_manager* get_fpga_manager(){
	return zynqmp_fpga_mgr;
}

static int zynqmp_fpga_ops_write_init(struct fpga_manager *mgr,
				      struct fpga_image_info *info,
				      const char *buf, size_t size)
{
	struct zynqmp_fpga_priv *priv;
	int  eemi_flags = 0;

	priv = mgr->priv;
	priv->flags = info->flags; // capire

	/* Update firmware flags */
	if (priv->flags & FPGA_MGR_USERKEY_ENCRYPTED_BITSTREAM)
		eemi_flags |= XILINX_ZYNQMP_PM_FPGA_ENCRYPTION_USERKEY;
	else if (priv->flags & FPGA_MGR_ENCRYPTED_BITSTREAM)
		eemi_flags |= XILINX_ZYNQMP_PM_FPGA_ENCRYPTION_DEVKEY;
	if (priv->flags & FPGA_MGR_DDR_MEM_AUTH_BITSTREAM)
		eemi_flags |= XILINX_ZYNQMP_PM_FPGA_AUTHENTICATION_DDR;
	else if (priv->flags & FPGA_MGR_SECURE_MEM_AUTH_BITSTREAM)
		eemi_flags |= XILINX_ZYNQMP_PM_FPGA_AUTHENTICATION_OCM;
	if (priv->flags & FPGA_MGR_PARTIAL_RECONFIG)
		eemi_flags |= XILINX_ZYNQMP_PM_FPGA_PARTIAL;

	/* Validate user flgas with firmware feature list */
	if ((priv->feature_list & eemi_flags) != eemi_flags)
		return -EINVAL;

	return 0;
}

static int zynqmp_fpga_ops_write(struct fpga_manager *mgr,
				 const char *buf, size_t size)
{
	/*BISOGNO DEL DMA*/
	struct zynqmp_fpga_priv *priv;
	int word_align, ret, index;
	//dma_addr_t dma_addr = 0;
	const u64 dma_addr=0;
	u32 eemi_flags = 0;
	u16 dma_size;
	u32 status;
	char *kbuf;

	priv = mgr->priv;
	word_align = size % FPGA_WORD_SIZE;
	if (word_align)
		word_align = FPGA_WORD_SIZE - word_align;

	size = size + word_align;
	priv->size = size;

	if (priv->flags & FPGA_MGR_USERKEY_ENCRYPTED_BITSTREAM)
		dma_size = size + ENCRYPTED_KEY_LEN;
	else
		dma_size = size;

	//kbuf = dma_alloc_coherent(priv->dev, dma_size, &dma_addr, GFP_KERNEL);
	if (!kbuf)
		return -ENOMEM;

	for (index = 0; index < word_align; index++)
		kbuf[index] = DUMMY_PAD_BYTE;

	memcpy(&kbuf[index], buf, size - index);

	if (priv->flags & FPGA_MGR_USERKEY_ENCRYPTED_BITSTREAM) {
		eemi_flags |= XILINX_ZYNQMP_PM_FPGA_ENCRYPTION_USERKEY;
		memcpy(kbuf + size, mgr->key, ENCRYPTED_KEY_LEN);
	} else if (priv->flags & FPGA_MGR_ENCRYPTED_BITSTREAM) {
		eemi_flags |= XILINX_ZYNQMP_PM_FPGA_ENCRYPTION_DEVKEY;
	}

	wmb(); /* ensure all writes are done before initiate FW call */

	if (priv->flags & FPGA_MGR_DDR_MEM_AUTH_BITSTREAM)
		eemi_flags |= XILINX_ZYNQMP_PM_FPGA_AUTHENTICATION_DDR;
	else if (priv->flags & FPGA_MGR_SECURE_MEM_AUTH_BITSTREAM)
		eemi_flags |= XILINX_ZYNQMP_PM_FPGA_AUTHENTICATION_OCM;

	if (priv->flags & FPGA_MGR_PARTIAL_RECONFIG)
		eemi_flags |= XILINX_ZYNQMP_PM_FPGA_PARTIAL;

	if (priv->flags & FPGA_MGR_USERKEY_ENCRYPTED_BITSTREAM)
		ret = zynqmp_pm_fpga_load(dma_addr, dma_addr + size,
					  eemi_flags, &status);
	else
		ret = zynqmp_pm_fpga_load(dma_addr, size,
					  eemi_flags, &status);

	//dma_free_coherent(priv->dev, dma_size, kbuf, dma_addr);

	if (status)
		return status;

	return ret;
}


static u64 zynqmp_fpga_ops_status(struct fpga_manager *mgr)
{
	/*BISOGNO DEL DMA*/
	unsigned int *buf, reg_val;
	//dma_addr_t dma_addr = 0;
	u64 dma_addr = 0;
	u64 status = 0;
	int ret;

	//buf = dma_alloc_coherent(mgr->dev.parent, READ_DMA_SIZE,
				// &dma_addr, GFP_KERNEL);
	if (!buf)
		return FPGA_MGR_STATUS_FIRMWARE_REQ_ERR;

	ret = zynqmp_pm_fpga_read(IXR_FPGA_CONFIG_STAT_OFFSET, dma_addr,
				  IXR_FPGA_READ_CONFIG_TYPE, &reg_val);
	if (ret) {
		status = FPGA_MGR_STATUS_FIRMWARE_REQ_ERR;
		goto free_dmabuf;
	}

	if (reg_val & IXR_FPGA_ERR_CRC_ERR)
		status |= FPGA_MGR_STATUS_CRC_ERR;
	if (reg_val & IXR_FPGA_ERR_SECURITY_ERR)
		status |= FPGA_MGR_STATUS_SECURITY_ERR;
	if (!(reg_val & IXR_FPGA_INIT_B_INTERNAL))
		status |= FPGA_MGR_STATUS_DEVICE_INIT_ERR;
	if (!(reg_val & IXR_FPGA_DONE_INTERNAL_SIGNAL))
		status |= FPGA_MGR_STATUS_SIGNAL_ERR;
	if (!(reg_val & IXR_FPGA_GST_CFG_B))
		status |= FPGA_MGR_STATUS_HIGH_Z_STATE_ERR;
	if (!(reg_val & IXR_FPGA_END_OF_STARTUP))
		status |= FPGA_MGR_STATUS_EOS_ERR;

free_dmabuf:
	//dma_free_coherent(mgr->dev.parent, READ_DMA_SIZE, buf, dma_addr);

	return status;
}

static int zynqmp_fpga_ops_read(struct fpga_manager *mgr,/* struct seq_file *s*/)
{
	/* int ret;

	if (readback_type)
		ret = zynqmp_fpga_read_cfgdata(mgr, s);
	else
		ret = zynqmp_fpga_read_cfgreg(mgr, s);

	return ret; */

	//TO BE IMPLEMENTED
	return -1;
}


static const struct fpga_manager_ops zynqmp_fpga_ops = {
	.state = zynqmp_fpga_ops_state,
	.status = zynqmp_fpga_ops_status,
	.write_init = zynqmp_fpga_ops_write_init,
	.write = zynqmp_fpga_ops_write,
	//.write_sg = zynqmp_fpga_ops_write_sg, non crdo serva
	.read = zynqmp_fpga_ops_read,
};

int zynqmp_fpga_init(){
	
	int ret=0;
	struct zynqmp_fpga_priv *priv;

	if (!(zynqmp_pm_fpga_get_version(&priv->version))) { //GET VERSION E GET FEATURE LIST SONO DEL DRIVER DI BASSO LIVELLO
		if (zynqmp_pm_fpga_get_feature_list(&priv->feature_list))
			priv->feature_list = DEFAULT_FEATURE_LIST;
	} else {
		priv->feature_list = DEFAULT_FEATURE_LIST;
	}
	zynqmp_fpga_mgr = fpga_mgr_create("Xilinx ZynqMP FPGA Manager (Jailhouse)",&zynqmp_fpga_ops,priv);
	if(!zynqmp_fpga_mgr)
		ret = -EINVAL;

	return ret;


}