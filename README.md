JAILHOUSE
=========

The Omnivisor-Enhanced Jailhouse Hypervisor
---------------------
The Omnivisor is a specialized partitioning hypervisor that extends 
the capabilities of the original Jailhouse hypervisor to allow 
virtual machines (VMs) to run on processors with different Instruction 
Set Architectures (ISAs) within the same Multi-Processor System-on-Chip (MPSoC). 
By leveraging the Omnivisor model, this repository enables fine-grained resource
allocation and isolation for mixed-criticality systems.

Specifically this repository is a fork of the Jailhouse maintained by 
[Minerva Systems](https://www.minervasys.tech).

About Minerva Repository
---------------------
This repository is an unofficial fork of the Jailhouse partitioning hypervisor.
It contains a series of experimental extensions to improve memory-related
isolation of workloads running in logically separated Jailhouse virtual
machines.

The extensions contained in this repository implements cache partitioning
via cache-coloring technique and memory bandwidth regulation using a
performance-counter-based approach.
The extensions target high-end SoCs based on Arm v8 Cortex-A cores, and have
been tested on Xilinx Ultrascale+ ZCU 102, NVIDIA Jetson AGX Xavier, NXP
S32V, NXP S32G2, i.MX8MQ (Coral).

The repository is maintained by [Minerva Systems](https://www.minervasys.tech)
and developed in collaboration with researchers from the following research
centers and universities:
 - Technical University of Munich (Chair of CPS in Production Engineering)
   https://www.mec.ed.tum.de/cps/
 - Università di Modena e Reggio Emilia (Hipert Lab)
   https://hipert.unimore.it/
 - Boston University (Cyber-Physical Systems Lab)
   https://cs-people.bu.edu/rmancuso/
 - Università degli Studi di Napoli Federico II (DESSERT Lab)
   https://dessert.unina.it/
  

The extensions have been evaluated in multiple academic publications such as:

> Tomasz Kloda, Marco Solieri, Renato Mancuso, Nicola Capodieci, Paolo Valente,
> Marko Bertogna,
> "Deterministic Memory Hierarchy and Virtualization for Modern Multi-Core
> Embedded Systems," 2019 IEEE Real-Time and Embedded Technology and
> Applications Symposium (RTAS), 2019

> Parul Sohal, Rohan Tabish, Ulrich Drepper, Renato Mancuso, "E-WarP:
> a System-wide Framework for Memory Bandwidth Profiling and
> Management", in Proceedings of the 41st IEEE Real-Time Systems
> Symposium (RTSS 2020), Houston, TX, USA, Dec. 2020

> Gero Schwäricke, Rohan Tabish, Rodolfo Pellizzoni, Renato Mancuso,
> Andrea Bastoni, Alexander Zuepke, Marco Caccamo,
> "A Real-Time Virtio-Based Framework for Predictable Inter-VM Communication,"
> 2021 IEEE Real-Time Systems Symposium (RTSS), 2021

> Shahin Roozkhosh, Denis Hoornaert, Renato Mancuso,
> "CAESAR: Coherence-Aided Elective and Seamless Alternative Routing via on-chip FPGA,"
> 2022 IEEE Real-Time Systems Symposium (RTSS), 2022

> Daniele Ottaviano, Francesco Ciraolo, Renato Mancuso, Marcello Cinque,
> "The Omnivisor: A Real-Time Static Partitioning Hypervisor Extension for 
> Heterogeneous Core Virtualization over MPSoCs,"
> 2024 Euromicro Conference on Real-Time Systems (ECRTS), 2024 

> Daniele Ottaviano, Marco Barletta, Francesco Boccola,
> "Zero-Interference Containers: A Framework to Orchestrate Mixed-Criticality Applications"
> 2025 IEEE/IFIP International Conference on Dependable Systems and Networks (DSN), 2025

There is an ongoing effort to integrate some of the features of this
repository in the mainline [Jailhouse project](https://github.com/siemens/jailhouse),
but please note at the moment, the Jailhouse project does not provide
support for this code. Please, subscribe to the Jailhouse mailing list for
updates on the mainlining effort and for any discussion about this fork
and its development.

Documentation
-------------
In addition to the standard how-to reported below in this file and to
the documentation contained in the `Documentation` directory,
patches, and working documentation for many of the boards supported
are maintained in a separate repository:
- https://github.com/Minervasys/jailhouse-documentation

Introduction
------------

Jailhouse is a partitioning Hypervisor based on Linux. It is able to run
bare-metal applications or (adapted) operating systems besides Linux. For this
purpose, it configures CPU and device virtualization features of the hardware
platform in a way that none of these domains, called "cells" here, can
interfere with each other in an unacceptable way.

Jailhouse is optimized for simplicity rather than feature richness. Unlike
full-featured Linux-based hypervisors like KVM or Xen, Jailhouse does not
support overcommitment of resources like CPUs, RAM or devices. It performs no
scheduling and only virtualizes those resources in software, that are essential
for a platform and cannot be partitioned in hardware.

Once Jailhouse is activated, it runs bare-metal, i.e. it takes full control
over the hardware and needs no external support. However, in contrast to other
bare-metal hypervisors, it is loaded and configured by a normal Linux system.
Its management interface is based on Linux infrastructure. So you boot Linux
first, then you enable Jailhouse and finally you split off parts of the
system's resources and assign them to additional cells.

Community Resources
-------------------

Project home:

 - https://github.com/siemens/jailhouse

Source code:

 - https://github.com/siemens/jailhouse.git
 - git@github.com:siemens/jailhouse.git

Demo and testing images:

 - https://github.com/siemens/jailhouse-images

Frequently Asked Questions (FAQ):

 - See [FAQ file](FAQ.md)

IRC channel:
  - Freenode, irc.freenode.net, #jailhouse
  - [![Webchat](https://img.shields.io/badge/irc-freenode-blue.svg "IRC Freenode")](https://webchat.freenode.net/?channels=jailhouse)

Mailing list:

  - jailhouse-dev@googlegroups.com

  - Subscription:
    - jailhouse-dev+subscribe@googlegroups.com
    - https://groups.google.com/forum/#!forum/jailhouse-dev/join

  - Archives
    - https://groups.google.com/forum/#!forum/jailhouse-dev
    - https://www.mail-archive.com/jailhouse-dev@googlegroups.com

Continuous integration:

  - https://github.com/siemens/jailhouse/actions

  - Status:
    - ![](https://github.com/siemens/jailhouse/actions/workflows/main.yaml/badge.svg?branch=master) on master
    - ![](https://github.com/siemens/jailhouse/actions/workflows/main.yaml/badge.svg?branch=next) on next

Static code analysis:

  - https://scan.coverity.com/projects/siemens-jailhouse

  - Status:
    - ![](https://scan.coverity.com/projects/siemens-jailhouse/badge.svg) on coverity_scan

See the [contribution documentation](CONTRIBUTING.md) for details
on how to write Jailhouse patches and propose them for upstream integration.


Hardware requirements (preliminary)
-----------------------------------

#### x86 architecture:

  - Intel system:

    - support for 64-bit and VMX, more precisely
      - EPT (extended page tables)
      - unrestricted guest mode
      - preemption timer

    - Intel IOMMU (VT-d) with interrupt remapping support
      (except when running inside QEMU)

  - or AMD system:

    - support for 64-bit and SVM (AMD-V), and also
      - NPT (nested page tables); required
      - Decode Assists; recommended

    - AMD IOMMU (AMD-Vi) is unsupported now but will be required in future

  - At least 2 logical CPUs

#### ARM architecture:

  - ARMv7 with virtualization extensions or ARMv8

  - At least 2 logical CPUs

  - Supported ARM boards:

    - Banana Pi ([see more](Documentation/setup-on-banana-pi-arm-board.md))

    - Orange Pi Zero (256 MB version)

    - NVIDIA Jetson TK1

    - ARM Versatile Express with Cortex-A15 or A7 cores
      (includes ARM Fast Model)

    - emtrion emCON-RZ/G1x series based on Renesas RZ/G ([see more](Documentation/setup-on-emtrion-emcon-rz-boards.md))

  - Supported ARM64 boards:

    - AMD Seattle / SoftIron Overdrive 3000

    - LeMaker HiKey

    - NVIDIA Jetson TX1 and TX2

    - Xilinx ZCU102 (ZynqMP evaluation board)

    - NXP MCIMX8M-EVK

    - Renesas RZ/G2M (HopeRun HiHope RZ/G2M platform)


Software requirements
---------------------

#### x86 architecture:

  - x86-64 Linux kernel (tested against 3.14+)

    - VT-d IOMMU usage (DMAR) has to be disabled in the Linux kernel, e.g. via
      the command line parameter:

          intel_iommu=off

    - To exploit the faster x2APIC, interrupt remapping needs to be on in the
      kernel (check for CONFIG_IRQ_REMAP)

  - The hypervisor requires a contiguous piece of RAM for itself and each
    additional cell. This currently has to be pre-allocated during boot-up.
    On x86 this is typically done by adding

        memmap=82M$0x3a000000

    as parameter to the command line of the virtual machine's kernel. Note that
    if you plan to put this parameter in GRUB2 variables in /etc/default/grub,
    then you will need three escape characters before the dollar
    (e.g. ```GRUB_CMDLINE_LINUX_DEFAULT="memmap=82M\\\$0x3a000000"```).

#### ARM architecture:

  - Linux kernel:
    - 3.19+ for ARM
    - 4.7+ for ARM64

  - Appropriate boot loader support (typically U-Boot)
     - Linux is started in HYP mode
     - PSCI support for CPU offlining

  - The hypervisor requires a contiguous piece of RAM for itself and each
    additional cell. This currently has to be pre-allocated during boot-up.
    On ARM this can be obtained by reducing the amount of memory seen by the
    kernel (through the `mem=` kernel boot parameter) or by modifying the
    Device Tree (i.e. the `reserved-memory` node).


Build & Installation
--------------------

Simply run `make`, optionally specifying the target kernel directory:

    make [KDIR=/path/to/kernel/objects]


#### Installation

It is recommended to install all of Jailhouse on your target machine. That will
take care of a kernel module, the firmware, tools etc. Just call

    make install

from the top-level directory.

The traditional Linux cross-compilation (i.e. `ARCH=` and `CROSS_COMPILE=`) and
installation (i.e. `DESTDIR=`) flags are supported as well.

#### Running without Installation

Except for the hypervisor image `jailhouse*.bin`, that has to be available in
the firmware search path, you can run Jailhouse from the build directory.
If you cannot or do not want to use `make install`, you can either install just
the firmware using `make firmware_install` or customize the firmware search
path:

    echo -n /path/to/jailhouse/hypervisor/ \
        > /sys/module/firmware_class/parameters/path


Configuration
-------------

Jailhouse requires one configuration file for the complete system and one for
each additional cell besides the primary Linux. These .cell files have to be
passed to the jailhouse command line tool for enabling the hypervisor or
creating new cells.

On x86, the required hardware capabilities can be validated by running

    jailhouse hardware check

A system configuration can be created on an x86 target system by running the
following command:

    jailhouse config create sysconfig.c

In order to translate this into the required binary form, place this file in
the configs/x86/ directory. The build system will pick up every .c file from
there and generate a corresponding .cell file.

Currently, there is no config generator for the ARM architecture; therefore the
config file must be manually written by starting from the reference examples
and checking hardware-specific datasheets, DTS and /proc entries.

Depending on the target system, the C structures may require some adjustments to
make Jailhouse work properly or to reduce the desired access rights of the Linux
root cell.

Configurations for additional (non-root) cells currently require manual
creation. To study the structures, use one of the demo cell configurations files
as reference, e.g. configs/x86/apic-demo.c or configs/x86/e1000-demo.c.


x86 Demonstration in QEMU/KVM
-----------------------------

**NOTE**: You can also build and execute the following demo steps with the
help of the jailhouse-images side project at
https://github.com/siemens/jailhouse-images.

The included system configuration qemu-x86.c can be used to run Jailhouse in
QEMU/KVM virtual machine on x86 hosts (Intel and AMD are supported). Currently
it requires Linux 4.4 or newer on the host side. QEMU version 2.8 or newer is
required.

You also need a Linux guest image with a recent kernel (tested with >= 3.9) and
the ability to build a module for this kernel. Further steps depend on the type
of CPU you have on your system.

For Intel CPUs: Make sure the kvm-intel module was loaded with nested=1 to
enable nested VMX support. Start the virtual machine as follows:

    qemu-system-x86_64 -machine q35,kernel_irqchip=split -m 1G -enable-kvm \
        -smp 4 -device intel-iommu,intremap=on,x-buggy-eim=on \
        -cpu host,-kvm-pv-eoi,-kvm-pv-ipi,-kvm-asyncpf,-kvm-steal-time,-kvmclock \
        -drive file=LinuxInstallation.img,format=raw|qcow2|...,id=disk,if=none \
        -device ide-hd,drive=disk -serial stdio -serial vc \
        -netdev user,id=net -device e1000e,addr=2.0,netdev=net \
        -device intel-hda,addr=1b.0 -device hda-duplex -device pcie-pci-bridge

For AMD CPUs: Make sure the kvm-amd module was loaded with nested=1 to enable
nested SVM support. Start the virtual machine as follows:

    qemu-system-x86_64 -machine q35 -m 1G -enable-kvm -smp 4 \
        -cpu host,-kvm-pv-eoi,-kvm-pv-ipi,-kvm-asyncpf,-kvm-steal-time,-kvmclock \
        -drive file=LinuxInstallation.img,format=raw|qcow2|...,id=disk,if=none \
        -device ide-hd,drive=disk -serial stdio -serial vc \
        -netdev user,id=net -device e1000e,addr=2.0,netdev=net \
        -device intel-hda,addr=1b.0 -device hda-duplex -device pcie-pci-bridge

Inside the VM, make sure that `jailhouse-*.bin`, generated by the build process,
are available for firmware loading (typically /lib/firmware), see above for
installation steps.

The Jailhouse QEMU cell config will block use of the serial port by the guest
OS, so make sure that the guest kernel command line does NOT have its console
set to log to the serial port (ie remove any 'console=ttyS0' arguments from the
grub config). Reboot the guest and load jailhouse.ko. Then enable Jailhouse
like this:

    jailhouse enable /path/to/qemu-x86.cell

Next you can create a cell with a demonstration application as follows:

    jailhouse cell create /path/to/apic-demo.cell
    jailhouse cell load apic-demo /path/to/apic-demo.bin
    jailhouse cell start apic-demo

apic-demo.bin is left by the built process in the inmates/demos/x86 directory.
This application will program the APIC timer interrupt to fire at 10 Hz,
measuring the jitter against the PM timer and displaying the result on the
console. Given that this demonstration runs in a virtual machine, obviously
no decent latencies should be expected.

After creation, cells are addressed via the command line tool by providing
their names or their runtime-assigned IDs. You can obtain information about
active cells this way:

    jailhouse cell list

Cell destruction is performed by specifying the configuration file of the
desired cell. This command will destroy the apic-demo:

    jailhouse cell destroy apic-demo

Note that the first destruction or shutdown request on the apic-demo cell will
fail. The reason is that this cell contains logic to demonstrate an ordered
shutdown as well as the ability of a cell to reject shutdown requests.

The apic-demo cell has another special property for demonstration purposes: As
long as it is running, no cell reconfigurations can be performed - the
apic-demo locks the hypervisor in this regard. In order to destroy another cell
or create an additional one, shut down the apic-demo first.

    jailhouse cell shutdown apic-demo  # call again if error is returned

To demonstrate the execution of a second, non-Linux cell, issue the following
commands:

    jailhouse cell create /path/to/pci-demo.cell
    jailhouse cell load pci-demo /path/to/pci-demo.bin \
        -s "con-base=0x2f8" -a 0x1000
    jailhouse cell start pci-demo

The pci-demo will use the second serial port provided by QEMU. You will find
its output in a virtual console of the QEMU window. The purpose of this demo is
to show basic PCI device configuration and MSI handling.

While cell configurations are locked, it is still possible, though, to reload
the content of existing cell (provided they accept their shutdown first). To
reload and restart the tiny-demo, issue the following commands:

    jailhouse cell start apic-demo
    jailhouse cell load pci-demo /path/to/pci-demo.bin \
        -s "con-base=0x2f8" -a 0x1000
    jailhouse cell start pci-demo

Finally, Jailhouse is can be stopped completely again:

    jailhouse disable  # call again on error due to running apic-demo

All non-Linux cells running at that point will be destroyed, and resources
will be returned to Linux.


ARM64 Demonstration in QEMU
---------------------------

Similarly like x86, Jailhouse can be tried out in a completely emulated ARM64
(aarch64) environment under QEMU. QEMU version 3.0 or later is required.

Start the QEMU machine like this:

    qemu-system-aarch64 -cpu cortex-a57 -smp 16 -m 1G \
        -machine virt,gic-version=3,virtualization=on,its=off -nographic \
        -netdev user,id=net -device virtio-net-device,netdev=net \
        -drive file=LinuxInstallation.img,format=raw|qcow2|...,id=disk,if=none \
        -device virtio-blk-device,drive=disk \
        -kernel /path/to/kernel-image -append "root=/dev/vda1 mem=768M"

Jailhouse can be started after loading its kernel module. Run:

    jailhouse enable /path/to/qemu-arm64.cell

The corresponding test to apic-demo on x86 is the gic-demo:

    jailhouse cell create /path/to/qemu-arm64-inmate-demo.cell
    jailhouse cell load inmate-demo /path/to/gic-demo.bin
    jailhouse cell start inmate-demo
