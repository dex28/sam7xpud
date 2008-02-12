
//---------------------------------------------------------------------------------------
//       Includes
//---------------------------------------------------------------------------------------

#if defined(AT91SAM7S321)
#include "AT91SAM7S321_inc.h"
#elif defined(AT91SAM7S64)
#include "AT91SAM7S64_inc.h"
#elif defined(AT91SAM7S128)
#include "AT91SAM7S128_inc.h"
#elif defined(AT91SAM7S256)
#include "AT91SAM7S256_inc.h"
#elif defined(AT91SAM7S512)
#include "AT91SAM7S512_inc.h"

#elif defined(AT91SAM7SE32)
#include "AT91SAM7SE32_inc.h"
#elif defined(AT91SAM7SE256)
#include "AT91SAM7SE256_inc.h"
#elif defined(AT91SAM7SE512)
#include "AT91SAM7SE512_inc.h"

#elif defined(AT91SAM7X128)
#include "AT91SAM7X128_inc.h"
#elif defined(AT91SAM7X256)
#include "AT91SAM7X256_inc.h"
#elif defined(AT91SAM7X512)
#include "AT91SAM7X512_inc.h"

#elif defined(AT91SAM7A3)
#include "AT91SAM7A3_inc.h"

#elif defined(AT91RM9200)
#include "AT91RM9200_inc.h"

#elif defined(AT91SAM9260)
#include "AT91SAM9260_inc.h"
#elif defined(AT91SAM9261)
#include "AT91SAM9261_inc.h"
#elif defined(AT91SAM9263)
#include "AT91SAM9263_inc.h"
#elif defined(AT91SAM9265)
#include "AT91SAM9265_inc.h"
#elif defined(AT91SAM926C)
#include "AT91SAM926C_inc.h"
#else
#error "No defintion of target"
#endif

//---------------------------------------------------------------------------------------
//  Constants
//---------------------------------------------------------------------------------------

//- ARM processor modes
    .equ ARM_MODE_USER,                0x10 // User Mode
    .equ ARM_MODE_FIQ,                 0x11 // FIQ Mode
    .equ ARM_MODE_IRQ,                 0x12 // IRQ Mode
    .equ ARM_MODE_SVC,                 0x13 // Supervisor Mode
    .equ ARM_MODE_ABORT,               0x17 // Abort Mode
    .equ ARM_MODE_UNDEF,               0x1B // Undefined Mode
    .equ ARM_MODE_SYS,                 0x1F // System Mode
    
//- Status register bits
    .equ I_BIT,                        0x80 // when I bit is set, IRQ is disabled
    .equ F_BIT,                        0x40 // when F bit is set, FIQ is disabled
    
//- Stack sizes
    .equ IRQ_STACK_SIZE,               0x400
    .equ FIQ_STACK_SIZE,               0x004
    .equ ABT_STACK_SIZE,               0x004
    .equ UND_STACK_SIZE,               0x004
    .equ SVC_STACK_SIZE,               0x800
    .equ SYS_STACK_SIZE,               0x400

//---------------------------------------------------------------------------------------
// Exception vectors ( before Remap )
//---------------------------------------------------------------------------------------
// These vectors are read at address 0.
// They absolutely requires to be in relative addresssing mode in order to
// guarantee a valid jump. For the moment, all are just looping (what may be
// dangerous in a final system). If an exception occurs before remap, this
// would result in an infinite loop.
//---------------------------------------------------------------------------------------
// -> will be used during startup before remapping with target ROM_RUN
// -> will be used "always" in code without remapping or with target RAM_RUN
// Mapped to Address relative address 0 of .text
// Absolute addressing mode must be used.
// Dummy Handlers are implemented as infinite loops which can be modified.
//---------------------------------------------------------------------------------------
    .text
    .arm
    .section .vectrom, "ax"

Vectors:
    ldr     pc, Reset_Addr      // 0x00 Reset handler         
    ldr     pc, Undef_Addr      // 0x04 Undefined Instruction
    ldr     pc, SWI_Addr        // 0x08 Software Interrupt
    ldr     pc, Undef_Addr      // 0x0C Prefetch Abort
    ldr     pc, Undef_Addr      // 0x10 Data Abort
    nop                         // 0x14 reserved
    ldr     pc, [pc,#-0xF20]    // 0x18 Vector From AIC_IVR
    ldr     pc, [pc,#-0xF20]    // 0x1C Vector From AIC_FVR

Reset_Addr:     
    .word   Reset_Handler
SWI_Addr:       
    .extern vPortYieldProcessor
    .word   vPortYieldProcessor
Undef_Addr:     
    .word   __main_exit

//---------------------------------------------------------------------------------------
//  Starupt Code must be linked first at Address at which it expects to run.

    .text
    .arm
    .section .init, "ax"

    .global _startup
    .func   _startup
    
_startup:
    ldr     pc, =Reset_Handler

//---------------------------------------------------------------------------------------
//  Reset Handler

Reset_Handler:

//- Stack setup
//- End of RAM (start of stack) address in r1
//
#if defined(AT91SAM9261)
    ldr     r0, =AT91C_IRAM
    add     r0, r0, #AT91C_IRAM_SIZE
#elif defined(AT91SAM9260)
    ldr     r0, =AT91C_IRAM_1
    add     r0, r0, #AT91C_IRAM_1_SIZE
#else
    ldr     r0, =AT91C_ISRAM
    add     r0, r0, #AT91C_ISRAM_SIZE
#endif

//- Undefined instruction mode stack setup
//
    msr     CPSR_c, #ARM_MODE_UNDEF | I_BIT | F_BIT
    mov     sp, r0
    sub     r0, r0, #UND_STACK_SIZE

//- Abort mode stack setup
//
    msr     CPSR_c, #ARM_MODE_ABORT | I_BIT | F_BIT
    mov     sp, r0
    sub     r0, r0, #ABT_STACK_SIZE

//- Fast interrupt mode stack setup
//
    msr     CPSR_c, #ARM_MODE_FIQ | I_BIT | F_BIT
    mov     sp, r0
    sub     r0, r0, #FIQ_STACK_SIZE

//- Interrupt mode stack setup
//
    msr     CPSR_c, #ARM_MODE_IRQ | I_BIT | F_BIT
    mov     sp, r0
    sub     r0, r0, #IRQ_STACK_SIZE

//- Supervisor mode stack setup
//
    msr     CPSR_c, #ARM_MODE_SVC | I_BIT | F_BIT
    mov     sp, r0
    sub     r0, r0, #SVC_STACK_SIZE

//- User/System mode stack setup
//
    msr     CPSR_c, #ARM_MODE_SYS | F_BIT
    mov     sp, r0

//- We want to start FreeRTOS in supervisor mode. Scheduler will switch to system
//- mode when the first task starts.
//
    msr     CPSR_c, #ARM_MODE_SVC  | I_BIT | F_BIT

//---------------------------------------------------------------------------------------
//  Low-level init (First part: PIO, Flash WS & Clocks)
//---------------------------------------------------------------------------------------

    .extern DEV_Init1

    ldr     r0, =DEV_Init1
    mov     lr, pc
    bx      r0

//---------------------------------------------------------------------------------------
//  Remap
//---------------------------------------------------------------------------------------

#if defined(REMAP)

//- copy the flash code to RAM
//- Start of RAM in r0, end of stack space in r1, current address in r2
    ldr     r0, =AT91C_ISRAM
    add     r1, r0, #AT91C_ISRAM_SIZE
    ldr     r2, =AT91C_IFLASH

Remap_copy:
    ldr     r3, [r2], #4
    str     r3, [r0], #4
    cmp     r0, r1
    bne     Remap_copy

//- Perform remap operation
    ldr     r0, =AT91C_MC_RCR
    mov     r1, #1
    str     r1, [r0]

#else

//---------------------------------------------------------------------------------------
//  RW data preinitialization
//---------------------------------------------------------------------------------------

#if defined(DEBUG)

#else

//- Load addresses
    add     r0, pc, #-(8+.-RW_addresses)
    ldmia   r0, {r1, r2, r3}

//- Initialize RW data
RW_loop:
    cmp     r2, r3
    ldrne   r0, [r1], #4
    strne   r0, [r2], #4
    bne     RW_loop
    b       RW_end

RW_addresses:
     .word _etext // End of code
     .word __data_start // Start of data
     .word _edata // End of data

RW_end:

#endif
#endif

//---------------------------------------------------------------------------------------
//  BSS data preinitialization
//---------------------------------------------------------------------------------------

//- Load addresses
    add     r0, pc, #-(8+.-ZI_addresses)
    ldmia   r0, {r1, r2}
    mov     r0, #0

//- Initialize ZI data
ZI_loop:
    cmp     r1, r2
    strcc   r0, [r1], #4
    bcc     ZI_loop
    b       ZI_end

ZI_addresses:
    .word __bss_start__
    .word __bss_end__ 
ZI_end:

//---------------------------------------------------------------------------------------
//  Low-level init (Second part: DBGU)
//---------------------------------------------------------------------------------------

    .extern DEV_Init2

    ldr     r0, =DEV_Init2
    mov     lr, pc
    bx      r0

//---------------------------------------------------------------------------------------
// Call C++ constructors (for objects in "global scope")
//---------------------------------------------------------------------------------------

    ldr     r0, =__ctors_start__
    ldr     r1, =__ctors_end__
ctor_loop:
    cmp     r0, r1
    beq     ctor_end
    ldr     r2, [r0], #4   // this ctor's address
    stmfd   sp!, {r0-r1}   // save loop counters
    mov     lr, pc         // set return address
    bx      r2             // call ctor
    ldmfd   sp!, {r0-r1}   // restore loop counters
    b       ctor_loop
ctor_end:

//---------------------------------------------------------------------------------------
// Branch on C code Main function (with interworking)
//---------------------------------------------------------------------------------------
// Branch must be performed by an interworking call as either an ARM or Thumb
// main C function must be supported. This makes the code not position-
// independant. A Branch with link would generate errors
//---------------------------------------------------------------------------------------
.extern main

    mov   r0, #0           // no arguments (argc = 0)
    mov   r1, r0
    mov   r2, r0
    mov   fp, r0           // null frame pointer
    mov   r7, r0           // null frame pointer for thumb
    ldr   r10, =main
    adr   lr, __main_exit
    bx    r10              // enter main()

//- Endless loop
__main_exit:    
    b     __main_exit

    .size   _startup, . - _startup
    .endfunc // _startup

//---------------------------------------------------------------------------------------
// Function          : IRQ_Handler_Entry
// Treatments        : IRQ Controller Interrupt Handler used WITHOUT FreeRTOS
// Called Functions  : AIC_IVR[interrupt]
//---------------------------------------------------------------------------------------

IRQ_Handler:

//- Adjust and save return address on the stack
    sub     lr, lr, #4
    stmfd   sp!, {lr}

//- Save r0 and SPSR on the stack
    mrs     r14, SPSR
    stmfd   sp!, {r0, r14}

//- Write in the IVR to support Protect mode
//- No effect in Normal Mode
//- De-assert NIRQ and clear the source in Protect mode
    ldr     r14, =AT91C_BASE_AIC
    ldr     r0, [r14, #AIC_IVR]
    str     r14, [r14, #AIC_IVR]

//- Enable nested interrupts and switch to Supervisor mode
    msr     CPSR_c, #ARM_MODE_SVC

//- Save scratch/used registers and LR on the stack
    stmfd   sp!, {r1-r3, r12, r14}

//- Branch to the routine pointed by AIC_IVR
    mov     r14, pc
    bx      r0

//- Restore scratch/used registers and LR from the stack
    ldmia   sp!, {r1-r3, r12, r14}

//- Disable nested interrupts and switch back to IRQ mode
    msr     CPSR_c, #I_BIT | ARM_MODE_IRQ

//- Acknowledge interrupt by writing AIC_EOICR
    ldr     r14, =AT91C_BASE_AIC
    str     r14, [r14, #AIC_EOICR]

//- Restore SPSR and r0 from the stack
    ldmia   sp!, {r0, r14}
    msr     SPSR_cxsf, r14

//- Return from interrupt handler
    ldmia   sp!, {pc}^

//---------------------------------------------------------------------------------------
.end
