#include <jailhouse/fpga-mgr.h>
#include <jailhouse/paging.h>
#include <jailhouse/fpga-arch.h>
#include <jailhouse/printk.h>

static struct fpga_manager* mgr = NULL;


/*
 * Call the low level driver's write_init function.  This will do the
 * device-specific things to get the FPGA into the state where it is ready to
 * receive an FPGA image. The low level driver only gets to see the first
 * initial_header_size bytes in the buffer.
 */
static int fpga_write_init_buf(struct fpga_image_info *info,
				   const char *buf, size_t count)
{
	int ret;
	mgr->err = 0;
	mgr->state = FPGA_MGR_STATE_WRITE_INIT;
	printk("copio\n");
	/* memcpy(info->key, mgr->key, ENCRYPTED_KEY_LEN);
	printk("memcopiato, chiamo sotto\n"); */
	if (!arch_fpga_initial_header_size())
		ret = arch_fpga_write_init(mgr, info, NULL, 0);
	else
		ret = arch_fpga_write_init(mgr, info, buf,
					 MIN(arch_fpga_initial_header_size(), count));
	
	if (ret) {
		printk("Error preparing FPGA for writing\n");
		mgr->state = FPGA_MGR_STATE_WRITE_INIT_ERR;
		mgr->err = ret;
		return ret;
	}

	return 0;
}

/*
 * After all the FPGA image has been written, do the device specific steps to
 * finish and set the FPGA into operating mode.
 */
static inline int fpga_write_complete( struct fpga_image_info *info)
{
	int ret = 0;

	mgr->err = 0;
	mgr->state = FPGA_MGR_STATE_WRITE_COMPLETE;
    ret = arch_fpga_write_complete(mgr, info);
	if (ret) {
		printk("Error after writing image data to FPGA\n");
		mgr->state = FPGA_MGR_STATE_WRITE_COMPLETE_ERR;
		mgr->err = ret;
		return ret;
	}
	mgr->state = FPGA_MGR_STATE_OPERATING;
	printk("fatto complete\n");
	return 0;
}

int fpga_buf_load(struct fpga_image_info *info){

    int ret;
    printk("BUF LOAD. A breve write-init\n");
	ret = fpga_write_init_buf(info, info->buf, info->count);
	if (ret)
		return ret;
    printk("WRITE INIT. A breve arch-write\n");

	/*
	 * Write the FPGA image to the FPGA.
	 */
	mgr->err = 0;
	mgr->state = FPGA_MGR_STATE_WRITE;
	ret = arch_fpga_write(mgr, info->buf, info->count);
	if (ret) {
		printk("Error while writing image data to FPGA\n");
		mgr->state = FPGA_MGR_STATE_WRITE_ERR;
		mgr->err = ret;
		return ret;
	}
	printk("Fatto arch write, faccio complete\n");

	return fpga_write_complete(info);
}

int init_fpga(){
	
    mgr = page_alloc(&mem_pool, PAGES(sizeof(struct fpga_manager)));
	//NE ABBIAMO BISOGNO???
    if(!mgr){
		return -ENOMEM;
	}    
	mgr->err =0;
	mgr->state = arch_fpga_state();

	return arch_fpga_init(mgr);
}