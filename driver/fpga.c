#include <jailhouse/fpga-common.h>
#include "jailhouse.h"
#include <linux/firmware.h>
#include <linux/slab.h>

#include <jailhouse/hypercall.h>


int jailhouse_fpga_load(struct jailhouse_fpga_load* fpga_load){
   
    /*
    1. crea una struttura jailhouse_fpga_image_info
    2. fai request firmware 
    3. metti nella struttura: dati del firmware, count
    4. metti nella struttura i flags
    6. id della regione???
    -- dovrebbe rimuovere il device per il kernel linux.
    7. fa hypercall all'hypervisor passando come parametro questa struttura
    8. fa release firmware
    */
    int ret;
    const struct firmware *fw;
    struct fpga_image_info * info;
    info = (struct fpga_image_info *)kmalloc(sizeof(struct fpga_image_info),GFP_KERNEL);
    info->flags = fpga_load->fpga_flags; //stesso formato?
    info->firmware_name = fpga_load->fpga_name;

    ret = request_firmware(&fw, fpga_load->fpga_name, NULL);
	if (ret) {
        pr_err("Error requesting firmware");
        return ret;
	}
    info->buf = fw->data; //should be contiguous. i will check.
    info->count = fw->size;
    // -- dovrebbe rimuovere il device per il kernel linux.
    pr_info("Jailhouse writing to FPGA\n");
    /*hypercall to jailhouse. */
    ret = jailhouse_call_arg1(JAILHOUSE_HC_FPGA_LOAD, __pa(info));

    release_firmware(fw);
    return ret;

}

