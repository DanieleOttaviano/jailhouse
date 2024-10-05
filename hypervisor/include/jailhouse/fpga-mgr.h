/* JAILHOUSE FPGA SUPPORT*/
/* SPDX-License-Identifier: GPL-2.0 */
/*
 * FPGA Framework
 *
 *  Copyright (C) 2013-2016 Altera Corporation
 *  Copyright (C) 2017 Intel Corporation
 */
#ifndef _JAILHOUSE_FPGA_MGR_H
#define _JAILHOUSE_FPGA_MGR_H

#include <jailhouse/fpga-common.h>
#include <jailhouse/types.h>


struct fpga_manager;
/**
 * enum fpga_mgr_states - fpga framework states
 * @FPGA_MGR_STATE_UNKNOWN: can't determine state
 * @FPGA_MGR_STATE_POWER_OFF: FPGA power is off
 * @FPGA_MGR_STATE_POWER_UP: FPGA reports power is up
 * @FPGA_MGR_STATE_RESET: FPGA in reset state
 * @FPGA_MGR_STATE_FIRMWARE_REQ: firmware request in progress
 * @FPGA_MGR_STATE_FIRMWARE_REQ_ERR: firmware request failed
 * @FPGA_MGR_STATE_PARSE_HEADER: parse FPGA image header
 * @FPGA_MGR_STATE_PARSE_HEADER_ERR: Error during PARSE_HEADER stage
 * @FPGA_MGR_STATE_WRITE_INIT: preparing FPGA for programming
 * @FPGA_MGR_STATE_WRITE_INIT_ERR: Error during WRITE_INIT stage
 * @FPGA_MGR_STATE_WRITE: writing image to FPGA
 * @FPGA_MGR_STATE_WRITE_ERR: Error while writing FPGA
 * @FPGA_MGR_STATE_WRITE_COMPLETE: Doing post programming steps
 * @FPGA_MGR_STATE_WRITE_COMPLETE_ERR: Error during WRITE_COMPLETE
 * @FPGA_MGR_STATE_OPERATING: FPGA is programmed and operating
 */
enum fpga_mgr_states {
	/* default FPGA states */
	FPGA_MGR_STATE_UNKNOWN,
	FPGA_MGR_STATE_POWER_OFF,
	FPGA_MGR_STATE_POWER_UP,
	FPGA_MGR_STATE_RESET,

	/* getting an image for loading */
	FPGA_MGR_STATE_FIRMWARE_REQ,
	FPGA_MGR_STATE_FIRMWARE_REQ_ERR,

	/* write sequence: parse header, init, write, complete */
	FPGA_MGR_STATE_PARSE_HEADER,
	FPGA_MGR_STATE_PARSE_HEADER_ERR,
	FPGA_MGR_STATE_WRITE_INIT,
	FPGA_MGR_STATE_WRITE_INIT_ERR,
	FPGA_MGR_STATE_WRITE,
	FPGA_MGR_STATE_WRITE_ERR,
	FPGA_MGR_STATE_WRITE_COMPLETE,
	FPGA_MGR_STATE_WRITE_COMPLETE_ERR,

	/* fpga is programmed and operating */
	FPGA_MGR_STATE_OPERATING,
};


/* FPGA manager status: Partial/Full Reconfiguration errors */
#define FPGA_MGR_STATUS_OPERATION_ERR		BIT(0)
#define FPGA_MGR_STATUS_CRC_ERR			BIT(1)
#define FPGA_MGR_STATUS_INCOMPATIBLE_IMAGE_ERR	BIT(2)
#define FPGA_MGR_STATUS_IP_PROTOCOL_ERR		BIT(3)
#define FPGA_MGR_STATUS_FIFO_OVERFLOW_ERR	BIT(4)
#define FPGA_MGR_STATUS_SECURITY_ERR		BIT(5)
#define FPGA_MGR_STATUS_DEVICE_INIT_ERR		BIT(6)
#define FPGA_MGR_STATUS_SIGNAL_ERR		BIT(7)
#define FPGA_MGR_STATUS_HIGH_Z_STATE_ERR	BIT(8)
#define FPGA_MGR_STATUS_EOS_ERR			BIT(9)
#define FPGA_MGR_STATUS_FIRMWARE_REQ_ERR	BIT(10)


/**
 * struct fpga_compat_id - id for compatibility check
 *
 * @id_h: high 64bit of the compat_id
 * @id_l: low 64bit of the compat_id
 */
struct fpga_compat_id {
	u64 id_h;
	u64 id_l;
};


/**
 * struct fpga_manager - fpga manager structure
 * @name: name of low level fpga manager
 * @flags: flags determines the type of Bitstream
 * @key: key value useful for Encrypted Bitstream loading to read the userkey
 * @ref_mutex: only allows one reference to fpga manager
 * @state: state of fpga manager
 * @compat_id: FPGA manager id for compatibility check.
 * @mops: pointer to struct of fpga manager ops
 * @priv: low level driver private date
 * @err: low level driver error code

 */
struct fpga_manager {
	const char *name;
	unsigned long flags;
	char key[ENCRYPTED_KEY_LEN + 1];
	//struct mutex ref_mutex; NIN MI SEVRE
	enum fpga_mgr_states state;
	struct fpga_compat_id *compat_id;
	//const struct fpga_manager_ops *mops;
	void *priv;
	int err;
};

/* 
struct fpga_manager * fpga_mgr_create(const char *name,
			 const struct fpga_manager_ops *mops, void *priv); */

int fpga_buf_load(struct fpga_image_info *info);
int init_fpga(void);

/*TODO
u64 fpga_status(struct fpga_manager* mgr);
fpga_read(struct fpga_manager* mgr,char* buf);
...read configuration registers??
*/
#endif /* _JAILHOUSE_FPGA_MGR_H*/