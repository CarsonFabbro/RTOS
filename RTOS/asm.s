; Assembly Library (RTOS)
; Carson Fabbro

;-----------------------------------------------------------------------------
; Hardware Target
;-----------------------------------------------------------------------------

; Target Platform: EK-TM4C123GXL Evaluation Board
; Target uC:       TM4C123GH6PM
; System Clock:    40 MHz

;-----------------------------------------------------------------------------
; Device includes, defines, and assembler directives
;-----------------------------------------------------------------------------

	.def setPSPAddress
	.def setThreadStackToPSP
	.def setThreadModeUnprivileged
	.def getPSP
	.def getMSP
	.def launchTaskUnprivileged
	.def pushHwHandledRegsToPsp
	.def getPid
	.def getMutexInfo
	.def getSemaphoreInfo
	.def getTaskInfo
	.def runThread
	.def killThread
	.def enablePreemption
	.def disablePreemption
	.def setSchedPriority
	.def setSchedRoundRobin
	.def changeThreadPriority

;-----------------------------------------------------------------------------
; Register values and large immediate values
;-----------------------------------------------------------------------------

.thumb

;-----------------------------------------------------------------------------
; Subroutines
;-----------------------------------------------------------------------------

.text

;set address that the psp points to, parameter in R0 holding the address desired
	.global setPSPAddress
setPSPAddress:
			  MSR PSP, R0
			  BX LR

; Sets thread stack pointer to PSP
	.global setThreadStackToPSP
setThreadStackToPSP:
			   MOV    R0, #2
               MSR    CONTROL, R0
               ISB
               BX LR

; Sets thread mode to unprivileged
	.global setThreadModeUnprivileged
setThreadModeUnprivileged:
			   MRS	  R0, CONTROL
			   ORR    R0, R0, #1
               MSR    CONTROL, R0
               BX LR

; Gets address that the process stack pointer is currently pointing to
	.global getPSP
getPSP:
			   MRS	  R0, PSP
			   BX LR

; Gets address that the main stack pointer is currently pointing to
	.global getMSP
getMSP:
			   MRS	  R0, MSP
			   BX LR

; Launches first task by storing fn ptr in R0 and setting TMPL bit (loads PC with fn ptr)
	.global launchTaskUnprivileged
launchTaskUnprivileged:
			   MRS 	  R1, CONTROL
			   ORR 	  R1, R1, #1
			   MSR	  CONTROL, R1
			   BX R0

; Pushes hardware handled regs to PSP (R0 -> PSP address, R1 -> xPSR, R2 -> @fn)
	.global pushHwHandledRegsToPsp
pushHwHandledRegsToPsp:
			   STR 	  R1, [R0, #-4]!
			   STR 	  R2, [R0, #-4]!
			   STR 	  LR, [R0, #-4]!
			   STR 	  R12, [R0, #-4]!
			   STR 	  R3, [R0, #-4]!
			   STR 	  R2, [R0, #-4]!
			   STR 	  R1, [R0, #-4]!
			   STR 	  R1, [R0, #-4]!
			   MSR	  PSP, R0
			   BX LR

; Gets pid of process given the name (R0->ptr to start of str)
	.global getPid
getPid:
			   SVC	 #6
			   BX LR

; Gets info of mutex given the number (R0->ptr to start of struct, R1->mutex num)
	.global getMutexInfo
getMutexInfo:
			   SVC	 #7
			   BX LR

; Gets info of semaphore give the number (R0->ptr to start of struct, R1->semaphore num)
	.global getSemaphoreInfo
getSemaphoreInfo:
			   SVC	 #8
			   BX LR

; Gets info for ps command (R0-> ptr to start of struct, R1-> task num)
	.global getTaskInfo
getTaskInfo:
			   SVC	 #9
			   BX LR

; Runs thread (R0-> fn ptr / pid)
	.global runThread
runThread:
			   SVC	 #10
			   BX LR

; Kills thread (R0-> fn ptr / pid)
	.global killThread
killThread:
			   SVC	 #11
			   BX LR

; Enables preemption
	.global enablePreemption
enablePreemption:
			   SVC	 #12
			   BX LR

; Disables preemption
	.global disablePreemption
disablePreemption:
			   SVC	 #13
			   BX LR


; Enables priority scheduling
	.global setSchedPriority
setSchedPriority:
			   SVC	 #14
			   BX LR

; Enables round robin scheduling
	.global setSchedRoundRobin
setSchedRoundRobin:
			   SVC	 #15
			   BX LR

; Changes thread priority
	.global changeThreadPriority
changeThreadPriority:
			   SVC	 #16
			   BX LR

.endm
