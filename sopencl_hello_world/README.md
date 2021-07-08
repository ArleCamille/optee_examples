# TA for OpenCL with ARM Mali GPU
This folder contains the attempt for protecting OpenCL with TrustZone. As in the plugins project, this program is divided into three parts:

 * `host`, which invokes the TA,
 * `ta`, which resides in the secure world and flushes the memory in between when switching to normal world,
 * `syslog`, which acts as the wrapper for OpenCL.