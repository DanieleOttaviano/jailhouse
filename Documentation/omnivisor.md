Omnivisor Support

======================

Introduction
------------

The Omnivisor is an experimental research project aimed at enhancing the 
capabilities of static partitioning hypervisors (SPH) to manage asymmetric 
cores and soft-cores on FPGA.

### Motivation
With the increasing trend of building highly heterogeneous MPSoCs that feature
asymmetric cores, such as Cortex-A, Cortex-R/M, and soft-cores on FPGA, there 
is a need for advanced management solutions. The Omnivisor project seeks to 
extend Jailhouse's capabilities to effectively manage these asymmetric cores, 
enabling the execution of isolated cells on them. 
It does integrate the feature of the remoteproc and fpga-region drivers while 
ensuring isolation between the asymmetric cores. 

Usage
-----

### Supported Boards
Supported Boards:

- [x] kria kv260
- [x] zcu104
- [ ] ...

### Enable Omnivisor 

#### 1) Set the Jailhouse Config File
To enable the Omnivisor, modify the config file `include/jailhouse/config.h` 
by appending these defines:

```c
#define CONFIG_OMNIVISOR      1
#define CONFIG_XMPU_ACTIVE    1
#define CONFIG_OMNV_FPGA      1
```

Specifically, for zynqmp platform this is the suggested configuration:

```c
#define CONFIG_MACH_ZYNQMP_ZCU102 1 
#define CONFIG_TRACE_ERROR        1
#define CONFIG_ARM_GIC_V2 	      1
#define CONFIG_DEBUG              1
#define CONFIG_OMNIVISOR          1
#define CONFIG_XMPU_ACTIVE        1
#define CONFIG_OMNV_FPGA          1
```

- `CONFIG_OMNIVISOR`: Enables the Omnivisor capability to create, load, and 
                      start on remote CPUs, such as the RPUs in the ZynqMP 
                      platforms, or soft-cores in FPGA.
- `CONFIG_XMPU_ACTIVE`: Enables spatial isolation between asymmetric cores 
                      by allowing the run-time configuration of the system
                      Memory Protection Units (SMPUs, or XMPUs in Xilinx 
                      terminology).
- `CONFIG_OMNV_FPGA`: Enable the runtime loading of bitstreams in the FPGA
                      during the create phase of a cell that need an accelerator
                      or a soft-core on FPGA. 

#### 2) Enable remoteproc in the Linux Kernel
Remoteproc drivers must be enabled in the kernel configuration

```c
CONFIG_REMOTEPROC=y
```

According to the used platform , enable the specific remoteproc driver.
e.g., for the zynqmp platform:

```c
CONFIG_ZYNQMP_R5_REMOTEPROC=y
```

FPGA drivers must be enabled at compiling time in the Linux kernel used
e.g., for the kria zynqmp:

```c
CONFIG_FPGA=y
CONFIG_XILINX_AFI_FPGA=y
CONFIG_FPGA_BRIDGE=y
CONFIG_XILINX_PR_DECOUPLER=y
CONFIG_FPGA_REGION=y
CONFIG_OF_FPGA_REGION=y
CONFIG_FPGA_MGR_ZYNQMP_FPGA=y
CONFIG_OF_OVERLAY=y
```

For having Jailhouse working correctly we need also the export the ksyms

```c
CONFIG_KALLSYMS_ALL
```

#### 3) Configure the devicetree to expose the remote cores
As required by the remoteproc driver, the remote cores need to be exposed
in the device tree. Specific guides are usually already offered for each 
platform.

e.g., for the two Cortex-R5F in the zynqmp platform: 
WARNING: The nodes must have phandle defined.

```c
rf5ss@ff9a0000 {
    phandle = <0x94>;
    compatible = "xlnx,zynqmp-r5-remoteproc";
    xlnx,cluster-mode = <1>;
    ranges;
    reg = <0x0 0xFF9A0000 0x0 0x10000>;
    #address-cells = <0x2>;
    #size-cells = <0x2>;

    r5f_0 {
        phandle = <0x95>;
        compatible = "xilinx,r5f";
        #address-cells = <2>;
        #size-cells = <2>;
        ranges;
        sram = <0x90 0x91>;
        memory-region = <&rproc_0_reserved>;
        power-domain = <0x0c 0x07>;
        mboxes = <&ipi_mailbox_rpu0 0>, <&ipi_mailbox_rpu0 1>;
        mbox-names = "tx", "rx";
    };
    r5f_1 {
        phandle = <0x96>;
        compatible = "xilinx,r5f";
        #address-cells = <0x2>;
        #size-cells = <0x2>;
        ranges;
        sram = <0x92 0x93>;
        memory-region = <&rproc_1_reserved>;
        power-domain = <0x0c 0x08>;
        mboxes = <&ipi_mailbox_rpu1 0>, <&ipi_mailbox_rpu1 1>;
        mbox-names = "tx", "rx";
    };
};
```

The processor need defined memory regions.
e.g., for the zynqmp platform: 

```c
reserved-memory {
    #address-cells = <2>;
    #size-cells = <2>;
    ranges;
    rproc_1_reserved: rproc@3ad00000{
        no-map;
        reg = <0x0 0x3ad00000 0x0 0x4000000>;
    };
    rproc_0_reserved: rproc@3ed00000 {
        no-map;
        reg = <0x0 0x3ed00000 0x0 0x8000000>;
    };
    
    ...
};
```

### Omnivisor Cell Configurations

#### Root-cell configuration
The Omnivisor cell configuration file needs some modifications as you can see
in the example configuration: `zynqmp-kv260-omnv.cell`

You need to insert the rcpu_devices structure with a number of instance that
is equal to the number of remote processors in the system N. 
Then for each instance you have to specify 
1) the rcpu_id (from 0 to N-1). 
2) The remoteproc name, which have to be the same defined in the device tree.
3) The compatible driver, which also have to be the same as the one defined in the device tree.

```c
struct{
    ...
    struct jailhouse_rcpu_device rcpu_devices[2];
    ...
} __attribute__((packed)) config = {
    ...
	.rcpu_devices = {
		{
			.rcpu_id = 0,
			.name = "r5f_0",
			.compatible = "xlnx,zynqmp-r5-remoteproc",
		},
		{
			.rcpu_id = 1,
			.name = "r5f_1",
			.compatible = "xlnx,zynqmp-r5-remoteproc",
		},
	},
}
```

Then similarly as it is done for the cpus, you need to define the rcpus struct,
initialize the rcpu_set_size and define a mask for the rcpus.

```c
struct{
    ...
	__u64 rcpus[1];
    ...
} __attribute__((packed)) config = {
    .header = {
        ...
        .root_cell = {
            ...
            .rcpu_set_size = sizeof(config.rcpus), 
            ...
    }
    ...
    .rcpus = {
        0x3, // RPU0, RPU1
    },
}
```

#### Non root-cell configuration
A cell that wants to use one of the remote core just need to do exactly the same
that is needed for traditional cpus: define the rcpus struct, initialize the
rcpu_set_size and define a mask of used rcpus. (e.g., `zynqmp-kv260-RPU0-inmate-demo.cell`)

```c
struct{
    ...
	__u64 rcpus[1];
    ...
} __attribute__((packed)) config = {
    .header = {
        ...
        .root_cell = {
            ...
            .rcpu_set_size = sizeof(config.rcpus), 
            ...
    }
    ...
    .rcpus = {
        0x1, // RPU0
    },
}
```

### Omnivisor FPGA soft-core Cell Configurations
To enable also the FPGA, and a soft-core managed as a remoteproc there are more informations
to include into the configurations.

This configurations to run the soft-core are based on the hypotesys of having the following artifacts:
1) FPGA full design with one or more riconfigurable sections. (e.g., `base.bit`)
2) FPGA partial design with a soft-core that can be dynamically integrated into the full design. (e.g., `partial.bit`) 
3) A Linux remoteproc module for the soft-core. (e.g., `softcore_remoteproc`)
4) A device tree overlay for the soft-core which is compatible with the module, (e.g., `softcore.dtbo`)

#### Root-cell configuration
The root-cell have to define an .fpga in the header that explicitly define the base bitstream name,
the starting address of the FPGA, the flags and the number of reconfigurable regions.
```c
struct{
    ...
} __attribute__((packed)) config = {
    .header = {
        ...
        .fpga = {
            .fpga_base_bitstream = "base.bit",
            .fpga_base_addr = 0x80000000,
            .fpga_flags = JAILHOUSE_FPGA_FULL, 
            .fpga_max_regions = 3,
        },
    }
}
```

Then you can define an FPGA mask, exactly as is done for the cpus and the rcpus

```c
struct{
    ...
	__u64 fpga_regions[1];
    ...
} __attribute__((packed)) config = {
    .header = {
        ...
        .root_cell = {
            ...
            .fpga_region_size = sizeof(config.fpga_regions), 
            ...
    }
    ...
    .fpga_regions = {
		0x7, // 3 FPGA regions
    },
}
```

#### Non root-cell configuration
The non root-cell configuration have to contain the FPGA mask of the used regions,
similarly as for the cpus.

```c
struct{
    ...
	__u64 fpga_regions[1];
    ...
} __attribute__((packed)) config = {
    .header = {
        ...
        .root_cell = {
            ...
            .fpga_region_size = sizeof(config.fpga_regions), 
            ...
    }
    ...
    .fpga_regions = {
		0x1, // 1 FPGA regions
    },
}
```

For each FPGA region we need to descrive the FPGA_device struct that indicate the
bitstream name, the kernel module to use, the devicetree overlay, the id of the
device (from 0 to N-1), the number of soft-core in the region, and the conf. addr

```c
struct{
    ...
	struct jailhouse_fpga_device fpga_devices[1];
    ...
} __attribute__((packed)) config = {
    ...
    .fpga_devices = {
        {
			.fpga_dto = "softcore.dtbo",
			.fpga_module = "softcore_remoteproc",
			.fpga_bitstream = "partial.bit",
			.fpga_region_id = 0,
			.fpga_conf_addr = 0x80000000,
		},
	},
}
```

if the FPGA device contains one or more remote cores, the non root cell have to
contains also the description of the rcpu as explained before.
N.B. tha ID and the the mask of the rcpu have to be higher than the one of physical
rcpus. e.g., in the zynqmp there are two Cortex-R5, then the soft-cores ID will start
from 2.
The name and compatible field have to be compliant with the name and compatible
described in the device tree overlay.

```c
struct{
    ...
	__u64 rcpus[1];
    ...
    struct jailhouse_rcpu_device rcpu_devices[1];
    ...
} __attribute__((packed)) config = {
    .header = {
        ...
        .root_cell = {
            ...
            .rcpu_set_size = sizeof(config.rcpus), 
            ...
    }
    ...
    .rcpus = {
		0x4, // Third core is a soft-core on the FPGA
    },
    ...
	.rcpu_devices = {
		{
			.rcpu_id = 2,
			.name = "pico32",
			.compatible = "dottavia,pico32-remoteproc",
		},
}
```

### Test Omnivisor
To test the Omnivisor, compile the hypervisor with the current configuration 
and load it onto the platform:

```sh
make -C "${jailhouse_dir}" ARCH="${ARCH}" CROSS_COMPILE="${CROSS_COMPILE}" KDIR="${linux_dir}"
```

To compile the remote CPUs inmates demos, manually compile for the desired 
architecture. 
e.g., for the Cortex-R5:

```sh
export REMOTE_COMPILE=arm-none-eabi-
make -C "${jailhouse_dir}" remote_armr5 REMOTE_COMPILE="${REMOTE_COMPILE}"
```

e.g., for the riscv32 soft-core:

```sh
export REMOTE_COMPILE=arm-none-eabi-
make -C "${jailhouse_dir}" remote_riscv32 REMOTE_COMPILE="${REMOTE_COMPILE}"
```

Then, on the platform, start the hypervisor using the root-cell configuration 
related to the Omnivisor (e.g., `zynqmp-kv260-omnv.cell`):

```sh
jailhouse enable ${JAILHOUSE_DIR}/configs/arm64/zynqmp-kv260-omnv.cell
```

Once enabled, check the cell list to verify that the hypervisor can see both 
CPUs, remote CPUs (rCPUs), and FPGA regions:

```sh
jailhouse cell list

ID      Name                    State             Assigned CPUs           Assigned rCPUs       Assigned FPGA regions          Failed CPUs             
0       ZynqMP-KV260            running           0-3                     0-1                  0-2
```

To test a cell on a remote core, first copy the image of the remote core in the lib/firmware directory:

```sh
cp \<jailhouse_dir\>/inmates/demos/armr5/src_rpu0-bm/rpu0-bm-demo.elf /lib/firmware/
```

Then use the traditional jailhouse commands to create, load, and start the cell.
To load an elf file for the remote processor use the following notation:

```sh
jailhouse cell load ${CELL_ID} [-r|--rcpu] ${ELF_NAME} ${CORE_ID}
```

e.g., for the kria zynqmp platform:

```sh
jailhouse cell create ${JAILHOUSE_DIR}/configs/arm64/zynqmp-kv260-RPU0-inmate-demo.cell
jailhouse cell load inmate-demo-RPU -r rpu-bm-demo.elf 0
jailhouse cell start inmate-demo-RPU
```

The same exact procedure is neede if the core is a soft-core on FPGA:

```sh
jailhouse cell create ${JAILHOUSE_DIR}/configs/arm64/zynqmp-kv260-RISCV-inmate-demo.cell
jailhouse cell load inmate-demo-RISCV -r riscv-bm-demo.elf 0
jailhouse cell start inmate-demo-RISCV
```


### Notes about the bitstream and elf files
* The elf files to be loaded on rcpus has to be under ```/lib/firmware```.
* The bitstream to be loaded on FPGA has to be under ```/lib/firmware```.
* The base bitstream and each reconfigurable region have a configuration port for reset
    * The base address for the configuration ports is specified in the cell configurations