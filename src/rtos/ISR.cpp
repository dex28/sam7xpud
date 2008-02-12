// NOTE: This is ARM module (not THUMB).
// Do not not use -mthumb switch when compiling!

//---------------------------------------------------------------------------------------
//      Includes
//---------------------------------------------------------------------------------------

// Scheduler includes
#include "FreeRTOS.h"

//---------------------------------------------------------------------------------------
//      External References
//---------------------------------------------------------------------------------------

extern void ISR_USB    ( void );
extern void ISR_Timer0 ( void );
extern void ISR_FPGA   ( void );
extern void ISR_VBus   ( void ); 

//---------------------------------------------------------------------------------------
//      Exported Functions
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
// Interrupt service routine (ISR) wrapper functions.
// ISR handler can cause a context switch so MUST be declared "naked".
// All ISR wrapper functions must perform following steps:
// 1. Save the context of the interrupted task.
// 2. Call the handler. This must be a separate function to ensure the 
//    stack frame is correctly set up.
// 3. Restore the context of whichever task will run next.
//
void ISR_Wrapper_USB    ( void ) __attribute__((naked));
void ISR_Wrapper_Timer0 ( void ) __attribute__((naked));
void ISR_Wrapper_FPGA   ( void ) __attribute__((naked));
void ISR_Wrapper_VBus   ( void ) __attribute__((naked));

//---------------------------------------------------------------------------------------

void ISR_Wrapper_USB( void )
{
    portSAVE_CONTEXT ();

    ISR_USB ();

    portRESTORE_CONTEXT ();
    }

void ISR_Wrapper_Timer0( void )
{
    portSAVE_CONTEXT ();

    ISR_Timer0 ();

    portRESTORE_CONTEXT ();
    }

void ISR_Wrapper_FPGA( void )
{
    portSAVE_CONTEXT ();

    ISR_FPGA ();

    portRESTORE_CONTEXT ();
    }

void ISR_Wrapper_VBus( void )
{
    portSAVE_CONTEXT ();

    ISR_VBus ();

    portRESTORE_CONTEXT ();
    }

