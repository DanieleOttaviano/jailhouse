Support for ARM QoS NIC Regulator Device Documentation
=======================================================

This document provides the details of the ARM QoS Regulator.

The board-dependent configuration is integrated in the configuration files
for each board (currently S32V and ZCU102).

Alternatively, for ZCU 102 you can also refer to the link below:

https://www.xilinx.com/html_docs/registers/ug1087/ug1087-zynq-ultrascale-registers.html

### USAGE

  jailhouse module [peripherial you want to regulate],ar_b=[value],aw_b=[value],ar_r=[value],aw_r=[value]

  ar_b and aw_b control the burstiness of the read and write requests respectively.
  ar_r and aw_r control the rate of the read and write requests respectively. 

More details about these parameters can be found here:

https://developer.arm.com/documentation/dsu0026/d/Functional-Description/Operation/QoS-regulators?lang=en

Example to enable qos regulator on gdma:

  modprobe jailhouse
  jailhouse enable configs/arm64/zynqmp-zcu102.cell
  jailhouse qos gdma:ar_b=1,aw_b=1,ar_r=10,aw_r=10
