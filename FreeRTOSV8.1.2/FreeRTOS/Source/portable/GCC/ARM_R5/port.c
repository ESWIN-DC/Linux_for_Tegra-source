/*
    FreeRTOS V8.1.2 - Copyright (C) 2014 Real Time Engineers Ltd.
    All rights reserved
    Copyright (c) 2014-2016 NVIDIA CORPORATION.  All rights reserved.

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

    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
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
#include "FreeRTOS.h"
#include "task.h"

/*-----------------------------------------------------------*/

/* Count of the critical section nesting depth. */
uint32_t ulCriticalNesting = 9999;

/*-----------------------------------------------------------*/

/* Constants required to set up the initial stack of each task. */
#define portINITIAL_SPSR		( ( StackType_t ) 0x1F )
#define portINITIAL_FPSCR		( ( StackType_t ) 0x00 )
#define portINSTRUCTION_SIZE	( ( StackType_t ) 0x04 )
#define portTHUMB_MODE_BIT		( ( StackType_t ) 0x20 )

/* The number of words on the stack frame between the saved Top Of Stack and
R0 (in which the parameters are passed. */
#define portSPACE_BETWEEN_TOS_AND_PARAMETERS	( 12 )

/*-----------------------------------------------------------*/

/* vPortStartFirstSTask() is defined in portASM.asm */
extern void vPortStartFirstTask( void );

/*-----------------------------------------------------------*/

/* Saved as part of the task context.  Set to pdFALSE if the task does not
require an FPU context. */
uint32_t ulTaskHasFPUContext = 0;

/* Set to 1 to pend a context switch from an ISR. */
uint32_t ulPortYieldRequired = pdFALSE;

/*-----------------------------------------------------------*/

/** \brief  Get CPSR

    This function returns the current value of the CPSR
    register.

    \return               CPSR register value
*/
__attribute__( ( always_inline ) ) static inline unsigned long _get_CPSR(void)
{
unsigned long result;

	__asm volatile ("MRS %0, cpsr" : "=r" (result) );
	return(result);
}
/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters )
{
StackType_t *pxOriginalTOS;

	pxOriginalTOS = pxTopOfStack;

	/* Setup the initial stack of the task.  The stack is set exactly as
	expected by the portRESTORE_CONTEXT() macro. */

	/* First on the stack is the return address - which is the start of the as
	the task has not executed yet.  The offset is added to make the return
	address appear as it would within an IRQ ISR. */
	*pxTopOfStack = ( StackType_t ) pxCode + portINSTRUCTION_SIZE;
	pxTopOfStack--;

	*pxTopOfStack = ( StackType_t ) 0x00000000;	/* R14 */
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) pxOriginalTOS; /* Stack used when task starts goes in R13. */
	pxTopOfStack--;

	#ifdef portPRELOAD_TASK_REGISTERS
	{
		*pxTopOfStack = ( StackType_t ) 0x12121212;	/* R12 */
		pxTopOfStack--;
		*pxTopOfStack = ( StackType_t ) 0x11111111;	/* R11 */
		pxTopOfStack--;
		*pxTopOfStack = ( StackType_t ) 0x10101010;	/* R10 */
		pxTopOfStack--;
		*pxTopOfStack = ( StackType_t ) 0x09090909;	/* R9 */
		pxTopOfStack--;
		*pxTopOfStack = ( StackType_t ) 0x08080808;	/* R8 */
		pxTopOfStack--;
		*pxTopOfStack = ( StackType_t ) 0x07070707;	/* R7 */
		pxTopOfStack--;
		*pxTopOfStack = ( StackType_t ) 0x06060606;	/* R6 */
		pxTopOfStack--;
		*pxTopOfStack = ( StackType_t ) 0x05050505;	/* R5 */
		pxTopOfStack--;
		*pxTopOfStack = ( StackType_t ) 0x04040404;	/* R4 */
		pxTopOfStack--;
		*pxTopOfStack = ( StackType_t ) 0x03030303;	/* R3 */
		pxTopOfStack--;
		*pxTopOfStack = ( StackType_t ) 0x02020202;	/* R2 */
		pxTopOfStack--;
		*pxTopOfStack = ( StackType_t ) 0x01010101;	/* R1 */
		pxTopOfStack--;
	}
	#else
	{
		pxTopOfStack -= portSPACE_BETWEEN_TOS_AND_PARAMETERS;
	}
	#endif

	/* Function parameters are passed in R0. */
	*pxTopOfStack = ( StackType_t ) pvParameters; /* R0 */
	pxTopOfStack--;

	/* Set the status register for system mode, with interrupts enabled. */
	*pxTopOfStack = ( StackType_t ) ( ( _get_CPSR() & ~0xFF ) | portINITIAL_SPSR );

	if( ( ( uint32_t ) pxCode & 0x01UL ) != 0x00 )
	{
		/* The task will start in thumb mode. */
		*pxTopOfStack |= portTHUMB_MODE_BIT;
	}

	/* ulUsingFPU, which by default is set to indicate
	that the stack frame does not include FPU registers. */
	pxTopOfStack--;
	*pxTopOfStack = pdFALSE;

	/*  ulCriticalNesting, which by default is set to 0 */
	pxTopOfStack--;
	*pxTopOfStack = 0;

	return pxTopOfStack;
}
/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
BaseType_t xPortStartScheduler(void)
{
	configSETUP_TICK_INTERRUPT();

	/* Reset the critical section nesting count read to execute the first task. */
	ulCriticalNesting = 0;

	/* Start the first task.  This is done from portASM.S as ARM mode must be
	used. */
	vPortStartFirstTask();

	/* Should not get here! */
	return pdFAIL;
}
/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
void vPortEndScheduler(void)
{
	/* Not implemented in ports where there is nothing to return to.
	Artificially force an assert. */
	configASSERT( ulCriticalNesting == 1000UL );
}
/*-----------------------------------------------------------*/

/*
 * Disable interrupts, and keep a count of the nesting depth.
 */
void vPortEnterCritical( void )
{
	/* Disable interrupts as per portDISABLE_INTERRUPTS(); */
	portDISABLE_INTERRUPTS();

	/* Now interrupts are disabled ulCriticalNesting can be accessed
	directly.  Increment ulCriticalNesting to keep a count of how many times
	portENTER_CRITICAL() has been called. */
	ulCriticalNesting++;
}
/*-----------------------------------------------------------*/

/*
 * Decrement the critical nesting count, and if it has reached zero, re-enable
 * interrupts.
 */
void vPortExitCritical( void )
{
	if( ulCriticalNesting > 0 )
	{
		/* Decrement the nesting count as we are leaving a critical section. */
		ulCriticalNesting--;

		/* If the nesting level has reached zero then interrupts should be
		re-enabled. */
		if( ulCriticalNesting == 0 )
		{
			/* Enable interrupts as per portENABLE_INTERRUPTS(). */
			portENABLE_INTERRUPTS();
		}
	}
}
/*-----------------------------------------------------------*/

void vPortTaskUsesFPU( void )
{
	/* A task is registering the fact that it needs an FPU context.  Set the
	FPU flag (saved as part of the task context. */
	ulTaskHasFPUContext = pdTRUE;

	/* Initialise the floating point status register. */
	__asm ( "MOV     r0, #0     \n\t"\
	        "VMSR    FPSCR, r0  \n\t");
}
/*-----------------------------------------------------------*/

void FreeRTOS_Tick_Handler( void )
{
	/* Increment the RTOS tick. */
	if( xTaskIncrementTick() != pdFALSE )
		ulPortYieldRequired = pdTRUE;
}
/*-----------------------------------------------------------*/
