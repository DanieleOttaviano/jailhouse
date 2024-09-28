#include <jailhouse/fpga-mgr.h>
#include <jailhouse/entry.h>


static inline void fpga_mgr_fpga_remove(struct fpga_manager *mgr)
{
	if (mgr->mops->fpga_remove)
		mgr->mops->fpga_remove(mgr);
}

static inline enum fpga_mgr_states fpga_mgr_state(struct fpga_manager *mgr)
{
	if (mgr->mops->state)
		return  mgr->mops->state(mgr);
	return FPGA_MGR_STATE_UNKNOWN;
}

static inline u64 fpga_mgr_status(struct fpga_manager *mgr)
{
	if (mgr->mops->status)
		return mgr->mops->status(mgr);
	return 0;
}

static inline int fpga_mgr_write(struct fpga_manager *mgr, const char *buf, size_t count)
{
	if (mgr->mops->write)
		return  mgr->mops->write(mgr, buf, count);
	return -ENOTSUP;
}

/*
 * After all the FPGA image has been written, do the device specific steps to
 * finish and set the FPGA into operating mode.
 */
static inline int fpga_mgr_write_complete(struct fpga_manager *mgr,
					  struct fpga_image_info *info)
{
	int ret = 0;

	mgr->err = 0;
	mgr->state = FPGA_MGR_STATE_WRITE_COMPLETE;
	if (mgr->mops->write_complete)
		ret = mgr->mops->write_complete(mgr, info);
	if (ret) {
		printk("Error after writing image data to FPGA\n");
		mgr->state = FPGA_MGR_STATE_WRITE_COMPLETE_ERR;
		mgr->err = ret;
		return ret;
	}
	mgr->state = FPGA_MGR_STATE_OPERATING;

	return 0;
}

static inline int fpga_mgr_write_init(struct fpga_manager *mgr,
				      struct fpga_image_info *info,
				      const char *buf, size_t count)
{
	if (mgr->mops->write_init)
		return  mgr->mops->write_init(mgr, info, buf, count);
	return 0;
}

static inline int fpga_mgr_write_sg(struct fpga_manager *mgr,
				    struct sg_table *sgt)
{
	//TO BE IMPLEMENTED
	return -1;
}

/**
 * fpga_image_info_alloc - Allocate an FPGA image info struct
 * @dev: owning device
 *
 * Return: struct fpga_image_info or NULL
 */
struct fpga_image_info *fpga_image_info_alloc(/* struct device *dev */)
{
	
	/* 
	NON SICURA CHE SERVA
	struct fpga_image_info *info;

	get_device(dev);

	info = devm_kzalloc(dev, sizeof(*info), GFP_KERNEL);
	if (!info) {
		put_device(dev);
		return NULL;
	}

	info->dev = dev;

	return info; */
	return -1;
}

/**
 * fpga_image_info_free - Free an FPGA image info struct
 * @info: FPGA image info struct to free
 */
void fpga_image_info_free(struct fpga_image_info *info)
{
	/* NON SICURA CHE SERVA
	struct device *dev;

	if (!info)
		return;

	dev = info->dev;
	if (info->firmware_name)
		devm_kfree(dev, info->firmware_name);

	devm_kfree(dev, info);
	put_device(dev); */
}

/*
 * Call the low level driver's write_init function.  This will do the
 * device-specific things to get the FPGA into the state where it is ready to
 * receive an FPGA image. The low level driver only gets to see the first
 * initial_header_size bytes in the buffer.
 */
static int fpga_mgr_write_init_buf(struct fpga_manager *mgr,
				   struct fpga_image_info *info,
				   const char *buf, size_t count)
{
	int ret;

	mgr->err = 0;
	mgr->state = FPGA_MGR_STATE_WRITE_INIT;
	if (!mgr->mops->initial_header_size)
		ret = fpga_mgr_write_init(mgr, info, NULL, 0);
	else
		ret = fpga_mgr_write_init(mgr, info, buf,
					  min(mgr->mops->initial_header_size, count));

	if (ret) {
		printk("Error preparing FPGA for writing\n");
		mgr->state = FPGA_MGR_STATE_WRITE_INIT_ERR;
		mgr->err = ret;
		return ret;
	}

	return 0;
}

static int fpga_mgr_write_init_sg(struct fpga_manager *mgr,
				  struct fpga_image_info *info,
				  struct sg_table *sgt)
{

	//TO BE IMPLEMENTED.
	/* struct sg_mapping_iter miter;
	size_t len;
	char *buf;
	int ret;

	if (!mgr->mops->initial_header_size)
		return fpga_mgr_write_init_buf(mgr, info, NULL, 0);

	/*
	 * First try to use miter to map the first fragment to access the
	 * header, this is the typical path.
	 */
	/* sg_miter_start(&miter, sgt->sgl, sgt->nents, SG_MITER_FROM_SG);
	if (sg_miter_next(&miter) &&
	    miter.length >= mgr->mops->initial_header_size) {
		ret = fpga_mgr_write_init_buf(mgr, info, miter.addr,
					      miter.length);
		sg_miter_stop(&miter);
		return ret;
	}
	sg_miter_stop(&miter);
 */
	/* Otherwise copy the fragments into temporary memory. */
	/* buf = kmalloc(mgr->mops->initial_header_size, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	len = sg_copy_to_buffer(sgt->sgl, sgt->nents, buf,
				mgr->mops->initial_header_size);
	ret = fpga_mgr_write_init_buf(mgr, info, buf, len);

	kfree(buf);

	return ret */
	return -1;
 
}

/**
 * fpga_mgr_buf_load_sg - load fpga from image in buffer from a scatter list
 * @mgr:	fpga manager
 * @info:	fpga image specific information
 * @sgt:	scatterlist table
 *
 * Step the low level fpga manager through the device-specific steps of getting
 * an FPGA ready to be configured, writing the image to it, then doing whatever
 * post-configuration steps necessary.  This code assumes the caller got the
 * mgr pointer from of_fpga_mgr_get() or fpga_mgr_get() and checked that it is
 * not an error code.
 *
 * This is the preferred entry point for FPGA programming, it does not require
 * any contiguous kernel memory.
 *
 * Return: 0 on success, negative error code otherwise.
 */
static int fpga_mgr_buf_load_sg(struct fpga_manager *mgr,
				struct fpga_image_info *info,
				struct sg_table *sgt)
{
	//To be implemented...
	/* int ret;

	if (info->flags & FPGA_MGR_USERKEY_ENCRYPTED_BITSTREAM)
		memcpy(info->key, mgr->key, ENCRYPTED_KEY_LEN);

	ret = fpga_mgr_write_init_sg(mgr, info, sgt);
	if (ret)
		return ret;

	/* Write the FPGA image to the FPGA. */
	/* mgr->err = 0;
	mgr->state = FPGA_MGR_STATE_WRITE;
	if (mgr->mops->write_sg) {
		ret = fpga_mgr_write_sg(mgr, sgt);
	} else {
		struct sg_mapping_iter miter;

		sg_miter_start(&miter, sgt->sgl, sgt->nents, SG_MITER_FROM_SG);
		while (sg_miter_next(&miter)) {
			ret = fpga_mgr_write(mgr, miter.addr, miter.length);
			if (ret)
				break;
		}
		sg_miter_stop(&miter);
	}

	if (ret) {
		dev_err(&mgr->dev, "Error while writing image data to FPGA\n");
		mgr->state = FPGA_MGR_STATE_WRITE_ERR;
		mgr->err = ret;
		return ret;
	}

	return fpga_mgr_write_complete(mgr, info); */
	return -1;
}

static int fpga_mgr_buf_load_mapped(struct fpga_manager *mgr,
				    struct fpga_image_info *info,
				    const char *buf, size_t count)
{
	int ret;

	ret = fpga_mgr_write_init_buf(mgr, info, buf, count);
	if (ret)
		return ret;

	/*
	 * Write the FPGA image to the FPGA.
	 */
	mgr->err = 0;
	mgr->state = FPGA_MGR_STATE_WRITE;
	ret = fpga_mgr_write(mgr, buf, count);
	if (ret) {
		printk("Error while writing image data to FPGA\n");
		mgr->state = FPGA_MGR_STATE_WRITE_ERR;
		mgr->err = ret;
		return ret;
	}

	return fpga_mgr_write_complete(mgr, info);
}

/**
 * fpga_mgr_buf_load - load fpga from image in buffer
 * @mgr:	fpga manager
 * @info:	fpga image info
 * @buf:	buffer contain fpga image
 * @count:	byte count of buf
 *
 * Step the low level fpga manager through the device-specific steps of getting
 * an FPGA ready to be configured, writing the image to it, then doing whatever
 * post-configuration steps necessary.  This code assumes the caller got the
 * mgr pointer from of_fpga_mgr_get() and checked that it is not an error code.
 *
 * Return: 0 on success, negative error code otherwise.
 */
static int fpga_mgr_buf_load(struct fpga_manager *mgr,
			     struct fpga_image_info *info,
			     const char *buf, size_t count)
{
	/* struct page **pages;
	struct sg_table sgt; */
	const void *p;
	int nr_pages;
	int index;
	int rc;

	/*
	 * This is just a fast path if the caller has already created a
	 * contiguous kernel buffer and the driver doesn't require SG, non-SG
	 * drivers will still work on the slow path.
	 */
	if (mgr->mops->write)
		return fpga_mgr_buf_load_mapped(mgr, info, buf, count);

	/*
	 * Convert the linear kernel pointer into a sg_table of pages for use
	 * by the driver.
	 */
	/* nr_pages = DIV_ROUND_UP((unsigned long)buf + count, PAGE_SIZE) -
		   (unsigned long)buf / PAGE_SIZE;
	pages = kmalloc_array(nr_pages, sizeof(struct page *), GFP_KERNEL);
	if (!pages)
		return -ENOMEM;

	p = buf - offset_in_page(buf);
	for (index = 0; index < nr_pages; index++) {
		if (is_vmalloc_addr(p))
			pages[index] = vmalloc_to_page(p);
		else
			pages[index] = kmap_to_page((void *)p);
		if (!pages[index]) {
			kfree(pages);
			return -EFAULT;
		}
		p += PAGE_SIZE;
	}
 */
	/*
	 * The temporary pages list is used to code share the merging algorithm
	 * in sg_alloc_table_from_pages
	 */
	/*rc = sg_alloc_table_from_pages(&sgt, pages, index, offset_in_page(buf),
				       count, GFP_KERNEL);
	kfree(pages);
	if (rc)
		return rc;

	rc = fpga_mgr_buf_load_sg(mgr, info, &sgt);
	sg_free_table(&sgt);

	return rc; */
}

static int fpga_dmabuf_load(struct fpga_manager *mgr,
			    struct fpga_image_info *info)
{
	//TO BE IMPLEMENTED....
	return -1;
}

/**
 * fpga_mgr_firmware_load - request firmware and load to fpga
 * @mgr:	fpga manager
 * @info:	fpga image specific information
 * @image_name:	name of image file on the firmware search path
 *
 * Request an FPGA image using the firmware class, then write out to the FPGA.
 * Update the state before each step to provide info on what step failed if
 * there is a failure.  This code assumes the caller got the mgr pointer
 * from of_fpga_mgr_get() or fpga_mgr_get() and checked that it is not an error
 * code.
 *
 * Return: 0 on success, negative error code otherwise.
 */
static int fpga_mgr_firmware_load(struct fpga_manager *mgr,
				  struct fpga_image_info *info,
				  const char *image_name)
{
	//const struct firmware *fw;
	int ret=0;

	/* dev_info(dev, "writing %s to %s\n", image_name, mgr->name);

	mgr->err = 0;
	mgr->state = FPGA_MGR_STATE_FIRMWARE_REQ; */

	/* flags indicates whether to do full or partial reconfiguration */
	/* info->flags = mgr->flags;
	memcpy(info->key, mgr->key, ENCRYPTED_KEY_LEN);

	ret = request_firmware(&fw, image_name, dev);
	if (ret) {
		mgr->state = FPGA_MGR_STATE_FIRMWARE_REQ_ERR;
		mgr->err = ret;
		dev_err(dev, "Error requesting firmware %s\n", image_name);
		return ret;
	}

	ret = fpga_mgr_buf_load(mgr, info, fw->data, fw->size);

	release_firmware(fw);
 */
	return ret;
}

/**
 * fpga_mgr_load - load FPGA from scatter/gather table, buffer, or firmware
 * @mgr:	fpga manager
 * @info:	fpga image information.
 *
 * Load the FPGA from an image which is indicated in @info.  If successful, the
 * FPGA ends up in operating mode.
 *
 * Return: 0 on success, negative error code otherwise.
 */
int fpga_mgr_load(struct fpga_manager *mgr, struct fpga_image_info *info)
{
	if (mgr->flags & FPGA_MGR_CONFIG_DMA_BUF)
		return fpga_dmabuf_load(mgr, info);
	/* if (info->sgt) not yet implemented in info.
		return fpga_mgr_buf_load_sg(mgr, info, info->sgt); */
	if (info->buf && info->count)
		return fpga_mgr_buf_load(mgr, info, info->buf, info->count);
	if (info->firmware_name)
		return fpga_mgr_firmware_load(mgr, info, info->firmware_name);
	return -EINVAL;
}

static const char * const state_str[] = {
	[FPGA_MGR_STATE_UNKNOWN] =		"unknown",
	[FPGA_MGR_STATE_POWER_OFF] =		"power off",
	[FPGA_MGR_STATE_POWER_UP] =		"power up",
	[FPGA_MGR_STATE_RESET] =		"reset",

	/* requesting FPGA image from firmware */
	[FPGA_MGR_STATE_FIRMWARE_REQ] =		"firmware request",
	[FPGA_MGR_STATE_FIRMWARE_REQ_ERR] =	"firmware request error",

	/* Preparing FPGA to receive image */
	[FPGA_MGR_STATE_WRITE_INIT] =		"write init",
	[FPGA_MGR_STATE_WRITE_INIT_ERR] =	"write init error",

	/* Writing image to FPGA */
	[FPGA_MGR_STATE_WRITE] =		"write",
	[FPGA_MGR_STATE_WRITE_ERR] =		"write error",

	/* Finishing configuration after image has been written */
	[FPGA_MGR_STATE_WRITE_COMPLETE] =	"write complete",
	[FPGA_MGR_STATE_WRITE_COMPLETE_ERR] =	"write complete error",

	/* FPGA reports to be in normal operating mode */
	[FPGA_MGR_STATE_OPERATING] =		"operating",
};


int fpga_mgr_get_state(){

	//chiedersi a chi interessa lo stato. Io non lo so.
	return 0;
}



//MODIFICA
u64 fpga_mgr_get_status(struct fpga_manager *mgr,/* struct device *dev,
			   struct device_attribute *attr,  */char *buf)
{
	//struct fpga_manager *mgr = to_fpga_manager(dev);
	u64 status;
	int len = 0;

	status = fpga_mgr_status(mgr);

	if (status & FPGA_MGR_STATUS_OPERATION_ERR)
		len += sprintf(buf + len, "reconfig operation error\n");
	if (status & FPGA_MGR_STATUS_CRC_ERR)
		len += sprintf(buf + len, "reconfig CRC error\n");
	if (status & FPGA_MGR_STATUS_INCOMPATIBLE_IMAGE_ERR)
		len += sprintf(buf + len, "reconfig incompatible image\n");
	if (status & FPGA_MGR_STATUS_IP_PROTOCOL_ERR)
		len += sprintf(buf + len, "reconfig IP protocol error\n");
	if (status & FPGA_MGR_STATUS_FIFO_OVERFLOW_ERR)
		len += sprintf(buf + len, "reconfig fifo overflow error\n");
	if (status & FPGA_MGR_STATUS_SECURITY_ERR)
		len += sprintf(buf + len, "reconfig security error\n");
	if (status & FPGA_MGR_STATUS_DEVICE_INIT_ERR)
		len += sprintf(buf + len,
			       "initialization has not finished\n");
	if (status & FPGA_MGR_STATUS_SIGNAL_ERR)
		len += sprintf(buf + len, "device internal signal error\n");
	if (status & FPGA_MGR_STATUS_HIGH_Z_STATE_ERR)
		len += sprintf(buf + len,
			       "all I/Os are placed in High-Z state\n");
	if (status & FPGA_MGR_STATUS_EOS_ERR)
		len += sprintf(buf + len,
			       "start-up sequence has not finished\n");
	if (status & FPGA_MGR_STATUS_FIRMWARE_REQ_ERR)
		len += sprintf(buf + len, "firmware request error\n");

	return len;
}




static int fpga_mgr_read(struct fpga_manager *mgr,/* struct seq_file *s,  */void *data)
{
	/*struct fpga_manager *mgr = (struct fpga_manager *)s->private; */
	int ret = 0;
/* 
	if (!mgr->mops->read)
		return -ENOENT;

	if (!mutex_trylock(&mgr->ref_mutex))
		return -EBUSY;

	if (mgr->state != FPGA_MGR_STATE_OPERATING) {
		ret = -EPERM;
		goto err_unlock;
	}*/

	/* Read the FPGA configuration data from the fabric */
	/* ret = mgr->mops->read(mgr, s);
	if (ret)
		dev_err(&mgr->dev, "Error while reading configuration data from FPGA\n");

err_unlock:
	mutex_unlock(&mgr->ref_mutex);
 */
	return ret;
}


/**
 * fpga_mgr_free - free an FPGA manager created with fpga_mgr_create()
 * @mgr:	fpga manager struct
 *//* FORSE
void fpga_mgr_free(struct fpga_manager *mgr)
{
	ida_simple_remove(&fpga_mgr_ida, mgr->dev.id);
	kfree(mgr);
}*/



struct fpga_manager *fpga_mgr_create(const char *name, 
                    const struct fpga_manager_ops *mops,
                    void *priv)
{
	struct fpga_manager *mgr;
	int id, ret;
	if (!mops) {
		printk("Attempt to create without fpga_manager_ops\n");
		return NULL;
	}
	if (!name || !strlen(name)) {
		printk("Attempt to create with no name!\n");
		return NULL;
	}
	mgr = kzalloc(sizeof(*mgr), GFP_KERNEL); //?? mica sei in kernel
	if (!mgr)
		return NULL;

	mgr->name = name;
	mgr->mops = mops;
	mgr->priv = priv;

	return mgr;
error_kfree:
	kfree(mgr); //non sei in kernel.

	return NULL;
}