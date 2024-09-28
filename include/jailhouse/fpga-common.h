#ifndef _JAILHOUSE_FPGA_H
#define _JAILHOUSE_FPGA_H

#define ENCRYPTED_KEY_LEN	64 /* Bytes */

struct fpga_image_info {
    unsigned int flags;
    /*u32 enable_timeout_us;
	u32 disable_timeout_us;
	u32 config_complete_timeout_us; servono ?*/ 
    char key[ENCRYPTED_KEY_LEN];
    char *firmware_name;
    const char *buf;
	unsigned int count;
	/*int region_id;serve?*/
};

/**
 * DOC: FPGA Manager flags
 *
 * Flags used in the &fpga_image_info->flags field
 *
 * %FPGA_MGR_PARTIAL_RECONFIG: do partial reconfiguration if supported
 *
 * %FPGA_MGR_EXTERNAL_CONFIG: FPGA has been configured prior to Linux booting
 *
 * %FPGA_MGR_ENCRYPTED_BITSTREAM: indicates bitstream is encrypted with
 *				  device key
 *
 * %FPGA_MGR_BITSTREAM_LSB_FIRST: SPI bitstream bit order is LSB first
 *
 * %FPGA_MGR_COMPRESSED_BITSTREAM: FPGA bitstream is compressed
 *
 * %FPGA_MGR_USERKEY_ENCRYPTED_BITSTREAM: indicates bitstream is encrypted with
 *					  user key
 * %FPGA_MGR_DDR_MEM_AUTH_BITSTREAM: do bitstream authentication using DDR
 *				     memory if supported
 * %FPGA_MGR_SECURE_MEM_AUTH_BITSTREAM: do bitstream authentication using secure
 *					memory if supported
 */
#define FPGA_MGR_PARTIAL_RECONFIG	BIT(0)
#define FPGA_MGR_EXTERNAL_CONFIG	BIT(1)
#define FPGA_MGR_ENCRYPTED_BITSTREAM	BIT(2)
#define FPGA_MGR_BITSTREAM_LSB_FIRST	BIT(3)
#define FPGA_MGR_COMPRESSED_BITSTREAM	BIT(4)
#define FPGA_MGR_USERKEY_ENCRYPTED_BITSTREAM	BIT(5)
#define FPGA_MGR_DDR_MEM_AUTH_BITSTREAM		BIT(6)
#define FPGA_MGR_SECURE_MEM_AUTH_BITSTREAM	BIT(7)
#define FPGA_MGR_CONFIG_DMA_BUF			BIT(8)



#endif /* _JAILHOUSE_FPGA_H*/