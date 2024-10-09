
#include <jailhouse/fpga-arch.h>
#if defined(CONFIG_MACH_ZYNQMP)
#include <asm/zynqmp-pm.h>
#include <jailhouse/paging.h>
#include <jailhouse/fpga-common.h>
#include <jailhouse/printk.h>
#include <asm/dcaches.h>

#define BIT(nr)			(1 << (nr))

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

int arch_fpga_init(struct fpga_manager* mgr){

    struct zynqmp_fpga_priv * priv= page_alloc(&mem_pool,1);
    if(!priv)
        return -ENOMEM;
    mgr->priv = priv;

    if (!(zynqmp_pm_fpga_get_version(&priv->version))) {
		if (zynqmp_pm_fpga_get_feature_list(&priv->feature_list))
			priv->feature_list = DEFAULT_FEATURE_LIST;
	} else {
		priv->feature_list = DEFAULT_FEATURE_LIST;
	} 

	mgr->name = "Xilinx ZynqMP FPGA Manager - Jailhouse";

    return 0;

}

enum fpga_mgr_states arch_fpga_state(){

	u32 status = 0;

	zynqmp_pm_fpga_get_status(&status);
	if (status & IXR_FPGA_DONE_MASK)
		return FPGA_MGR_STATE_OPERATING;
	return FPGA_MGR_STATE_UNKNOWN;
}

int arch_fpga_write_init(struct fpga_manager* mgr, struct fpga_image_info *info,
				    const char *buf, size_t size)
{
    struct zynqmp_fpga_priv *priv;
	int eemi_flags = 0;

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
	if (info->flags & FPGA_MGR_PARTIAL_RECONFIG)
		eemi_flags |= XILINX_ZYNQMP_PM_FPGA_PARTIAL;

	/* Validate user flgas with firmware feature list */
	if ((int)(priv->feature_list & eemi_flags) != eemi_flags)
		return -EINVAL;


	return 0;
}

int arch_fpga_write_complete(struct fpga_manager* mgr, struct fpga_image_info *info){
    //Nothing to be done
    return 0;
}

int arch_fpga_write(struct fpga_manager *mgr, const char *buf, size_t size){

	struct zynqmp_fpga_priv *priv;
	int word_align, ret, index;
	//dma_addr_t dma_addr = 0;
	//u64 dma_addr=0;
	u32 eemi_flags = 0;
	u16 dma_size;
	u32 status;
	char *kbuf;
	

	priv = mgr->priv;
	word_align = size % FPGA_WORD_SIZE;
	if (word_align)
		word_align = FPGA_WORD_SIZE - word_align;

	printk("bitstream size was %lu, word align is %d\n",size,word_align);
	size = size + word_align;
	priv->size = size;
	printk("now size is %lu\n",size);

	if (priv->flags & FPGA_MGR_USERKEY_ENCRYPTED_BITSTREAM)
		dma_size = size + ENCRYPTED_KEY_LEN;
	else
		dma_size = size;

	//kbuf = dma_alloc_coherent(priv->dev, dma_size, &dma_addr, GFP_KERNEL);
	printk("Sto mappando\n");
	u64 dma_addr = 0x7f000000;
	kbuf = paging_map_device(dma_addr,dma_size);
	//kbuf = (char*) phyiscal_address;
	/* if (!kbuf){
		printk("Memory allocation failed\n");
		return -ENOMEM;} */
	printk("Ho mappato\n");
	
	//printk("kbuf è %p, &kbuf[word_align] è %p\n",kbuf,&kbuf[word_align]);
	//memmove(kbuf,&kbuf[word_align],size-word_align);
/* 	for(index = size-2; index > 0; index--){
		printk("Moving kbuf[%d] into kbuf[%d]\n",index,index+word_align);
		kbuf[index+word_align] = kbuf[index];
	} */
/* size_t i;
	for (i = 0; i < 1000; i++){
		printk("kbuf[%ld]=%x\n",i+8000,kbuf[i+8000]);
	}  */
	

	printk("ho fatto memmove\n");
	for (index = 0; index < word_align; index++);
		//kbuf[index] = DUMMY_PAD_BYTE;
	printk("Ho scritto dummy, index era %d, wa era %d\n",index,word_align);
	//dma_addr = (u64) buf;
	/*memcpy(&kbuf[index], buf, size - index); //VORREI DIRETTAMENTE METTERE FW QUA :((((()))))
	printk("Ho memcopiato\n");*/
	printk("eeemi flags = %d\n",eemi_flags);

	if (priv->flags & FPGA_MGR_USERKEY_ENCRYPTED_BITSTREAM) {
		eemi_flags |= XILINX_ZYNQMP_PM_FPGA_ENCRYPTION_USERKEY;
		//memcpy(kbuf + size, mgr->key, ENCRYPTED_KEY_LEN);
	} else if (priv->flags & FPGA_MGR_ENCRYPTED_BITSTREAM) {
		eemi_flags |= XILINX_ZYNQMP_PM_FPGA_ENCRYPTION_DEVKEY;
	}

	wmb(); /* ensure all writes are done before initiate FW call */
	//flush; coerenza non gestita dal dma.
	//dma_addr = _paging_virt2phys)
	arm_dcaches_flush(kbuf,dma_size,DCACHE_INVALIDATE);

	if (priv->flags & FPGA_MGR_DDR_MEM_AUTH_BITSTREAM)
		eemi_flags |= XILINX_ZYNQMP_PM_FPGA_AUTHENTICATION_DDR;
	else if (priv->flags & FPGA_MGR_SECURE_MEM_AUTH_BITSTREAM)
		eemi_flags |= XILINX_ZYNQMP_PM_FPGA_AUTHENTICATION_OCM;

	if (priv->flags & FPGA_MGR_PARTIAL_RECONFIG)
		eemi_flags |= XILINX_ZYNQMP_PM_FPGA_PARTIAL;

	  if (priv->flags & FPGA_MGR_USERKEY_ENCRYPTED_BITSTREAM){
		ret = zynqmp_pm_fpga_load(dma_addr, dma_addr + size,
					  eemi_flags, &status);
	  }
	else
		ret = zynqmp_pm_fpga_load(dma_addr, dma_size,
					  eemi_flags, &status); 
 
	//dma_free_coherent(priv->dev, dma_size, kbuf, dma_addr);
	//page_free(&mem_pool,kbuf,PAGES(dma_size));	
	//paging_unmap_device(phyiscal_address,kbuf,dma_size);

	if (status){
		printk("status is %u\n",(enum pm_ret_status) status);
		return status;
	}
	printk("ret is %d\n",ret);
	return ret;


}


size_t arch_fpga_initial_header_size(){
    //no need for this specific fpga
    return 0;
}

/*
u64 arch_fpga_status(struct fpga_manager* mgr)
{
	unsigned int *buf=NULL, reg_val;
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

	return status;}

int arch_fpga_read(struct fpga_manager* mgr,char* buf){
.....
}
...read configuration registers??
*/


#endif /*CONFIG_MACH_ZYNQMP*/