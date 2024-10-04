#include <jailhouse/fpga-common.h>
#include "jailhouse.h"
#include <linux/firmware.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include "main.h"

#include <jailhouse/hypercall.h>


int jailhouse_fpga_load(struct jailhouse_fpga_load* fpga_load){

    int ret;
    const struct firmware *fw;
    struct fpga_image_info * info;
    char * buf;
    void *mapped_addr;
    phys_addr_t paddr = 0x7f000000;  // Indirizzo fisico riservato
    int size = 0x10;
    mapped_addr = ioremap_cache(paddr, size);
    if (!mapped_addr) {
    pr_err("Errore nella mappatura dell'indirizzo fisico (cache)\n");
    ret = -ENOMEM;
    goto end;
    } else {
    pr_info("Indirizzo fisico mappato a: %p\n", mapped_addr);
    }


    ret = request_firmware(&fw, fpga_load->fpga_name, NULL);
	if (ret) {
        pr_err("Error requesting firmware");
        goto unmap;
	} 
    memcpy(mapped_addr,fw->data,fw->size);
    pr_info("memcpy\n");
   /*  buf = (char*)kmalloc(fw->size,GFP_USER);
    if(!buf){
        ret = -ENOMEM;
        goto free_buf;
    } */

    info = (struct fpga_image_info*)kmalloc(sizeof(struct fpga_image_info),GFP_KERNEL);
    if(!info){
        ret = -ENOMEM;
        pr_err("Error allocating fpga_image_info");
        goto free_fw;
    }

    info->flags = fpga_load->fpga_flags;
    strcpy(info->firmware_name, fpga_load->fpga_name);
    info->count = fw->size;
    //pr_info("fw->size = %lu",fw->size);
    info->buf = __pa(*(fw->data));
    // -- dovrebbe rimuovere il device per il kernel linux.
    pr_info("Jailhouse writing to FPGA\n");
    /*hypercall to jailhouse. */
    ret = jailhouse_call_arg1(JAILHOUSE_HC_FPGA_LOAD, __pa(info));
   
free:
    kfree(info); 
free_fw:
    release_firmware(fw);
unmap:
    iounmap(mapped_addr);
end:
     return ret;
   

}