# Analysis of this kernel oops

## Brief

Analysis of the faulty device driver which is loaded into kernel using the command below to trigger "kernel oops" scenario.

## Loading the faulty driver

Command `echo “hello_world” > /dev/faulty`. The faulty driver here tries to access a NULL pointer.

## Kernel oops message
Output:

```bash
# echo "hello_world" > /dev/faulty
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
Mem abort info:
  ESR = 0x96000045
  EC = 0x25: DABT (current EL), IL = 32 bits
  SET = 0, FnV = 0
  EA = 0, S1PTW = 0
  FSC = 0x05: level 1 translation fault
Data abort info:
  ISV = 0, ISS = 0x00000045
  CM = 0, WnR = 1
user pgtable: 4k pages, 39-bit VAs, pgdp=000000004262a000
[0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000
Internal error: Oops: 96000045 [#1] SMP
Modules linked in: hello(O) scull(O) faulty(O)
CPU: 0 PID: 159 Comm: sh Tainted: G           O      5.15.18 #1
Hardware name: linux,dummy-virt (DT)
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
pc : faulty_write+0x14/0x20 [faulty]
lr : vfs_write+0xa8/0x2b0
sp : ffffffc008d23d80
x29: ffffffc008d23d80 x28: ffffff80026a0000 x27: 0000000000000000
x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
x23: 0000000040000000 x22: 000000000000000c x21: 0000005567b333c0
x20: 0000005567b333c0 x19: ffffff8002642300 x18: 0000000000000000
x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000
x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
x5 : 0000000000000001 x4 : ffffffc0006f0000 x3 : ffffffc008d23df0
x2 : 000000000000000c x1 : 0000000000000000 x0 : 0000000000000000
Call trace:
 faulty_write+0x14/0x20 [faulty]
 ksys_write+0x68/0x100
 __arm64_sys_write+0x20/0x30
 invoke_syscall+0x54/0x130
 el0_svc_common.constprop.0+0x44/0xf0
 do_el0_svc+0x40/0xa0
 el0_svc+0x20/0x60
 el0t_64_sync_handler+0xe8/0xf0
 el0t_64_sync+0x1a0/0x1a4
Code: d2800001 d2800000 d503233f d50323bf (b900003f)
---[ end trace beebcfaae7e6f6d3 ]---
```
## Analysis:
* Error Description:
The error message indicates a kernel NULL pointer dereference at virtual address 0000000000000000, resulting in a kernel panic.
* Memory Abort Info:
Provides additional information about the abort, including the exception syndrome register (ESR) and exception class (EC). The fault status code (FSC) indicates a level 1 translation fault.
* Data Abort Info:
Provides more details about the data abort, including the instruction specific syndrome (ISS) and information about the fault, such as whether it was a read or write fault.
* CPU and Process Information:
Specifies the CPU and process information at the time of the error, including the process ID (PID) and the name of the command that triggered the error (sh).
* Kernel Stack Trace:
Shows the kernel stack trace leading up to the error, including the function names and addresses. The faulty_write function from the faulty module is identified as the source of the error, along with other related functions such as vfs_write and syscall-related functions.
* Call Trace:
Provides a backtrace of function calls leading up to the error, helping to identify the sequence of function calls that led to the issue.
* Code Dump:
Displays a dump of the kernel code at the point of the error, which can provide additional context for debugging purposes.