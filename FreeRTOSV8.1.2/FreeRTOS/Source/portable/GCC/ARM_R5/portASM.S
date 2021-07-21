/*
    FreeRTOS V8.1.2 - Copyright (C) 2014 Real Time Engineers Ltd.
    All rights reserved
    Copyright (c) 2014, NVIDIA CORPORATION.  All rights reserved.

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that has become a de facto standard.             *
     *                                                                       *
     *    Help yourself get started quickly and support the FreeRTOS         *
     *    project by purchasing a FreeRTOS tutorial book, reference          *
     *    manual, or both from: http://www.FreeRTOS.org/Documentation        *
     *                                                                       *
     *    Thank you!                                                         *
     *                                                                       *
    ***************************************************************************

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

    >>! NOTE: The modification to the GPL is included to allow you to distribute
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available from the following
    link: http://www.freertos.org/a00114.html

    1 tab == 4 spaces!

    ***************************************************************************
     *                                                                       *
     *    Having a problem?  Start by reading the FAQ "My application does   *
     *    not run, what could be wrong?"                                     *
     *                                                                       *
     *    http://www.FreeRTOS.org/FAQHelp.html                               *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org - Documentation, books, training, latest versions,
    license and Real Time Engineers Ltd. contact details.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.OpenRTOS.com - Real Time Engineers ltd license FreeRTOS to High
    Integrity Systems to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/* FreeRTOS includes. */
/*
 * The scheduler can only be started from ARM mode, hence the inclusion of this
 * function here.
 */
#include "FreeRTOSConfig.h"

    .global vPortStartFirstTask
    .global vPortYieldProcessor
    .global vPortPreemptiveTick
    .global irq_end

    .text

.macro portSAVE_CONTEXT
    DSB
    /* Push R0 as we are going to use it */
    STMDB   SP!, {R0}

    /* Set R0 to point to the task stack pointer.*/
    STMDB   SP,{SP}^
    SUB     SP, SP, #4
    LDMIA   SP!,{R0}

    /* Push the return address onto the stack. */
    STMDB   R0!, {LR}

    /* Now LR has been saved, it can be used instead of R0.*/
    MOV     LR, R0

    /* Pop R0 so it can be saved onto the task stack.*/
    LDMIA   SP!, {R0}

    /* Push all the system mode registers onto the task stack.*/
    STMDB   LR,{R0-LR}^
    SUB     LR, LR, #60

    /* Push the SPSR onto the task stack.*/
    MRS     R0, SPSR
    STMDB   LR!, {R0}

    /*Determine if the task maintains an FPU context.*/
    LDR     R0, =ulTaskHasFPUContext
    LDR     R0, [R0]

    /* Test the flag */
    CMP     R0, #0

    /*If the task is not using a floating point context then skip the*/
    /* saving of the FPU registers. */
    BEQ     1f
    FSTMDBD LR!, {D0-D15}
    FMRX    R1,  FPSCR
    STMFD   LR!, {R1}
1:
    /* Save the flag */
    STMDB   LR!, {R0}

    /* Save ulCriticalNesting */
    LDR     R0, =ulCriticalNesting
    LDR     R0, [R0]
    STMDB   LR!, {R0}

    /* Store the new top of stack for the task.*/
    LDR     R0, =pxCurrentTCB
    LDR     R0, [R0]
    STR     LR, [R0]
    .endm

vPortStartFirstTask:
portRESTORE_CONTEXT:
    LDR     R0, =pxCurrentTCB
    LDR     R0, [R0]
    LDR     LR, [R0]

    /* The critical nest context counter is the first thing on the stack.*/
    LDR     R0, =ulCriticalNesting
    LDMFD   LR!, {R1}
    STR     R1, [R0]

    /* The floating point context flag is the second thing on the stack.*/
    LDR     R0, =ulTaskHasFPUContext
    LDMFD   LR!, {R1}
    STR     R1, [R0]

    /* Test the flag */
    CMP     R1, #0

    /*If the task is not using a floating point context then */
    /*skip the VFP register loads. */
    BEQ     1f

    /* Restore the floating point context. */
    LDMFD   LR!, {R0}
    FLDMIAD LR!, {D0-D15}
    FMXR    FPSCR, R0
1:
    /* Get the SPSR from the stack.*/
    LDMFD   LR!, {R0}
    MSR     spsr_csxf, R0

    /* Restore all system mode registers for the task.*/
    LDMFD   LR, {R0-R14}^

    /*  Restore the return address.*/
    LDR     LR, [LR, #+60]

    /* And return - correcting the offset in the LR to obtain the*/
    /* correct address. */
    SUBS    PC, LR, #4

/*
 * Called by portYIELD() or taskYIELD() to manually force a context switch.
 *
 * When a context switch is performed from the task level the saved task
 * context is made to look as if it occurred from within the tick ISR.  This
 * way the same restore context function can be used when restoring the context
 * saved from the ISR or that saved from a call to vPortYieldProcessor.
 */
vPortYieldProcessor:
    /* Within an IRQ ISR the link register has an offset from the true return
    address, but an SWI ISR does not.  Add the offset manually so the same
    ISR return code can be used in both cases. */
    ADD     LR, LR, #4

    /* Perform the context switch.  First save the context of the current task. */
    portSAVE_CONTEXT

    /* Find the highest priority task that is ready to run. */
    BLX     vTaskSwitchContext

    /* Restore the context of the new task. */
    b portRESTORE_CONTEXT

vPortPreemptiveTick:
    PUSH    { R0-R3, IP, LR }

    /* Clear tick IRQ status early */
    configCLEAR_TICK_INTERRUPT

    /* Increment the tick count, making any adjustments to the blocked lists
    that may be necessary.*/
    BLX     xTaskIncrementTick

    LDR     R1, =ulPortYieldRequired
    STR     R0, [R1]

    configACKNOWLEDGE_TICK_INTERRUPT

/* irq_end can be used in end of an irq handler and do a context switch if required.
 * "PUSH { R0-R3, IP, LR }" must be used in irq handler before jumping to irq_end.
 *
 * An example to use irq_end:
 * IRQ_Handler:
 *     PUSH    { R0-R3, IP, LR }
 *     BLX     Application_IRQ_Handler
 *     B       irq_end
 */
irq_end:
    LDR     R1, =ulPortYieldRequired
    LDR     R0, [R1]
    CMP     R0, #0
    BNE     irq_end_with_context_switch

    POP     { R0-R3, IP, LR }
    SUBS    PC, LR, #4

irq_end_with_context_switch:
    /* Clear context swtich pending flag. */
    MOV     R0, #0
    STR     R0, [R1]
    POP     { R0-R3, IP, LR }

    /* Save the context of the current task.*/
    portSAVE_CONTEXT
    BLX     vTaskSwitchContext
    /* Restore the context of the task selected to execute.*/
    b       portRESTORE_CONTEXT
