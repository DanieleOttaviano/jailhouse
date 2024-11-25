Omnivisor Support

======================

Introduction
------------

The Omnivisor is an experimental research project aimed at enhancing the 
capabilities of static partitioning hypervisors (SPH) to manage asymmetric 
cores and soft-cores on FPGA.

### Motivation
With the increasing trend of building highly heterogeneous MPSoCs that feature
asymmetric cores, such as Cortex-A and Cortex-R/M and soft-cores on FPGA, there 
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
```

Specifically, for zynqmp platform this is the suggested configuration:

```c
#define CONFIG_MACH_ZYNQMP_ZCU102 1 
#define CONFIG_TRACE_ERROR        1
#define CONFIG_ARM_GIC_V2 	      1
#define CONFIG_DEBUG              1
#define CONFIG_OMNIVISOR          1
#define CONFIG_XMPU_ACTIVE        1
```

- `CONFIG_OMNIVISOR`: Enables the Omnivisor capability to create, load, and 
                      start on remote CPUs, such as the RPUs in the ZynqMP 
                      platforms.
- `CONFIG_XMPU_ACTIVE`: Enables spatial isolation between asymmetric cores 
                      by allowing the run-time configuration of the system-MPUs 
                      (XMPUs in Xilinx terminology).

#### 2) Configure remoteproc the Linux Kernel
Remoteproc drivers must be enabled in the kernel configuration
```c
CONFIG_REMOTEPROC=y
```

According to the used platform , enable the specific remoteproc driver.

e.g., for the zynqmp platform:
```c
CONFIG_ZYNQMP_R5_REMOTEPROC=y
```

#### 3) Configure the devicetree to export the remote cores
As required by the remoteproc driver, the remote cores need to be exposed
in the device tree. Specific guides are usually alread offered for each 
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


### Test Omnivisor
To test the Omnivisor, compile the hypervisor with the current configuration 
and load it onto the platform:

```sh
make -C "${jailhouse_dir}" ARCH="${ARCH}" CROSS_COMPILE="${CROSS_COMPILE}" KDIR="${linux_dir}"
```

To compile the remote CPUs inmates demos, manually compile for the desired 
architecture. For example, for the Cortex-R5:

```sh
export REMOTE_COMPILE=arm-none-eabi-
make -C "${jailhouse_dir}" remote_armr5 REMOTE_COMPILE="${REMOTE_COMPILE}"
```

Then, on the platform, start the hypervisor using the root-cell configuration 
related to the Omnivisor (e.g., `zynqmp-kv260-omnv.cell`):

```sh
jailhouse enable ${JAILHOUSE_DIR}/configs/arm64/zynqmp-kv260-omnv.cell
```

Once enabled, check the cell list to verify that the hypervisor can see both 
CPUs and remote CPUs (rCPUs):

```sh
jailhouse cell list

ID      Name                    State             Assigned CPUs           Assigned rCPUs       Assigned FPGA regions          Failed CPUs             
0       ZynqMP-KV260            running           0-3                     0-2   
```

To test a cell on a remote core, first copy the image of the remote core in the lib/firmware directory:

```sh
cp \<jailhouse_dir\>/inmates/demos/armr5/src_rpu0-bm/rpu0-bm-demo.elf /lib/firmware/
```

Then use the traditional jailhouse commands to create, load, and start the cell.
To load an elf file for the remote processor use the following notation:

```sh
jailhouse cell load ${CELL_ID} [-r|--rpu] ${ELF_NAME} ${CORE}
```

e.g., for the kria zynqmp platform:

```sh
jailhouse cell create ${JAILHOUSE_DIR}/configs/arm64/zynqmp-kv260-RPU0-inmate-demo.cell
jailhouse cell load inmate-demo-RPU -r rpu-bm-demo.elf 0
jailhouse cell start inmate-demo-RPU
```

## FPGA support
The Omnivisor also enables the use of FPGA regions as additional resources for the cells. 

To enable FPGA support, modify the config file `include/jailhouse/config.h` 
by appending this define:

```c
#define CONFIG_OMNV_FPGA 1
```
Compile with the new configuration as described before, then start the hypervisor using 
a configuration that includes FPGA regions (e.g ```zynqmp-kv260-omnv.cell```)

```sh
jailhouse enable ${JAILHOUSE_DIR}/configs/arm64/zynqmp-kv260-omnv.cell
```
Once enabled, check the cell list to verify that the hypervisor can see the FPGA:

```sh
jailhouse cell list

ID      Name                    State             Assigned CPUs           Assigned rCPUs       Assigned FPGA regions   Failed CPUs             
0       ZynqMP-KV260            running           0-3                     0-2                  0
```

Now, after a cell creation, you can load a bitstream along with traditional images, 
specifying the FPGA region to be loaded in:
```sh
jailhouse cell load ${CELL_ID} ${IMAGE} [-b|--bitstream] ${BITSTREAM_NAME} ${REGION_ID}
```

To test a cell with a FPGA region on the kria zynqmp platform, use the following command:

```sh
jailhouse cell create ${JAILHOUSE_DIR}/configs/arm64/zynqmp-kv260-RISCV-inmate-demo.cell
jailhouse cell load inmate-demo-RISCV ${JAILHOUSE_DIR}/inmates/demos/riscv/riscv-demo.bin -b pico32_tg.bit 0
jailhouse cell start inmate-demo-RISCV
```

### Notes about the bitstream
* The bitstream to be loaded has to be under ```/lib/firmware```.
* Each region can have a configuration port for reset
    * The base address for the configuration ports is specified in the root cell configuration
    * The configuration port for region ```n``` is at ```base_address + 4 * n```