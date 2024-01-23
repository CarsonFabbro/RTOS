# RTOS
## RTOS for the TM4C123GH6PM Microcontoroller

### Overview
A pseudo-parallel real-time operating system developed for the Tiva™ C Series TM4C123GH6PM Microcontroller. The operating system occupies 4K of memory, with 28K of memory for tasks. The available memory is broken up into 5 regions, with 3 4K blocks and 2 8K blocks. This allows for interleaving, which in turn allows space for 512B tasks, 1024B tasks, and a few 1536B tasks. The total number of tasks that can run on the OS is 36 in a perfect case. The OS and pre-loaded tasks can be tested using a circuit board containing leds and pushbuttons connected to the Tiva board.

The operating system consists of an MPU, which manages the memory ensuring that unprivilledged processes cannot access privilleged memory, and execute privilleged memory such as the Flash. Additionally, it shields any one process, from accessing the memory of any other task.

The OS also supports the use of mutexes, and semaphores, allowing for tasks to use commands like lock(), unlock(), wait() and post(). Additionally, the OS can handle both floating point, and non-floating point variables, eliminating the problems invlolved with lazy stacking.

Finally, the OS has a shell running as a process, which utilizes service calls to access privilleged data. Some of the implemented commands include: ps, ipcs, kill, pkill, preempt (toggles preemption), sched (toggles between round-robin scheduling and priority scheduling), pidof, and run (runs a specified task stored in memory). The OS could run with an average CPU utilization of 0.1% - 1%.

### Instructions

1. Download code composer studio from [here](https://software-dl.ti.com/ccs/esd/documents/ccs_downloads.html)
2. Open CCS and create a project
3. Select Tiva C Series and Tiva TM4C123GH6PM as the target
4. Select Stellaris In-Circuit Debug Interface under the connection dropdown
5. Name the Project
6. Select empty project and click finish
7. Copy paste files from the RTOS file of the repo into the new project folder
8. Download the TivaWare library SW-EK-TM4C123GXL-2.2.0.295.exe from [here](https://www.ti.com/tool/SW-TM4C)
9. Extract the tm4c123gh6pm.h file from the /inc folder and move it to the project folder
10. Build and run the project
