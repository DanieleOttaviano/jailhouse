Omnivisor Support

======================

Introduction
------------

The Omnivisor is an experimental research project aimed at enhancing the 
capabilities of static partitioning hypervisors (SPH) to manage asymmetric 
cores.

### Motivation
With the increasing trend of building highly heterogeneous MPSoCs that feature
asymmetric cores, such as Cortex-A and Cortex-R/M and soft-cores on FPGA, there 
is a need for advanced management solutions. The Omnivisor project seeks to 
extend Jailhouse's capabilities to effectively manage these asymmetric cores, 
enabling the execution of isolated cells on them.

Usage
-----

### Supported Boards
Supported Boards:

- [x] kria kv260
- [x] zcu104
- [ ] ...

### Enable Omnivisor
To enable the Omnivisor, modify the config file `include/jailhouse/config.h` 
by appending these defines:

```c
#define CONFIG_OMNIVISOR      1
#define CONFIG_XMPU_ACTIVE    1
```

- `CONFIG_OMNIVISOR`: Enables the Omnivisor capability to create, load, and 
                      start on remote CPUs, such as the RPUs in the ZynqMP 
                      platforms.
- `CONFIG_XMPU_ACTIVE`: Enables spatial isolation between asymmetric cores 
                      by allowing the run-time configuration of the system-MPUs 
                      (XMPUs in Xilinx terminology).

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

ID      Name                    State             Assigned CPUs           Assigned rCPUs          Failed CPUs             
0       ZynqMP-KV260            running           0-3                     0-2   
```

To test a cell on a remote core, use the traditional Jailhouse command. For the Cortex-R, manually load part of the binary into the TCM:

```sh
jailhouse cell create ${JAILHOUSE_DIR}/configs/arm64/zynqmp-kv260-RPU-inmate-demo.cell
jailhouse cell load inmate-demo-RPU ${JAILHOUSE_DIR}/inmates/demos/armr5/baremetal-demo_tcm.bin -a 0xffe00000 ${JAILHOUSE_DIR}/inmates/demos/armr5/baremetal-demo.bin
jailhouse cell start inmate-demo-RPU
```
