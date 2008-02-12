
//---------------------------------------------------------------------------------------
//      Includes
//---------------------------------------------------------------------------------------

#include "sam7xpud.hpp"

//---------------------------------------------------------------------------------------
//      External Refernces
//---------------------------------------------------------------------------------------

extern void ISR_Wrapper_Timer0( void );

//---------------------------------------------------------------------------------------
//      Implementation
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
// Variables to keep track of the CPU usage.
// (Actually, we are keeping track when the CPU is idle.)
//---------------------------------------------------------------------------------------

volatile int cpu_usage = 0; // in promilles

static volatile ulong dIdleTick = 0;

extern "C" void vApplicationIdleHook( void )
{
    ++dIdleTick;
    }

static inline void Calc_CPU_Usage_Every1s( void )
{
    static ulong lastTick = 0;
    static long maxTick = 1;
    
    ulong curTick = dIdleTick;
    long delta = curTick - lastTick;
    lastTick = curTick;
    
    if ( delta > maxTick )
        maxTick = delta;
    
    cpu_usage = int( double( maxTick - delta ) * 1000.0 / maxTick ); 
    }

//---------------------------------------------------------------------------------------
// TC0: 1ms Timer keeping dTimerTick
//---------------------------------------------------------------------------------------

volatile ulong dTimerTick = 0;

// Called when a timer tick occurs (every 1 ms).
//
void ISR_Timer0( void )
{
    ++dTimerTick;
    
    (void) AT91C_BASE_TC0->TC_SR; // Clear interrupt
    AT91F_AIC_AcknowledgeIt( AT91C_BASE_AIC );
    }

//---------------------------------------------------------------------------------------
// Main Timer task: Drives status LED and keeps track of CPU Usage
//---------------------------------------------------------------------------------------

portTASK_FUNCTION( MainTimer_Task, pvParameters )
{
    (void) pvParameters; // The parameters are not used.

    taskENTER_CRITICAL ();

    TRACE_INFO( "Main Timer task\n" );

    ////////////////////////////////////////////////////////////////////////////////
    // Configure Timer 0: 1 ms precision
    //
    AT91F_TC0_CfgPMC ();
    AT91C_BASE_TC0->TC_CMR = AT91C_TC_WAVE | AT91C_TC_WAVESEL_UP_AUTO;
    AT91C_BASE_TC0->TC_IER = AT91C_TC_CPCS;

    AT91F_AIC_ConfigureIt
    ( 
        AT91C_BASE_AIC,
        AT91C_ID_TC0,
        AT91C_AIC_PRIOR_LOWEST,
        0,
        ISR_Wrapper_Timer0
        );
    AT91F_AIC_EnableIt( AT91C_BASE_AIC, AT91C_ID_TC0 );

    AT91C_BASE_TC0->TC_RC = ( AT91C_MASTER_CLOCK / 2 ) / 1000;
    AT91C_BASE_TC0->TC_CCR = AT91C_TC_CLKEN;
    AT91C_BASE_TC0->TC_CCR = AT91C_TC_SWTRG;
    
    taskEXIT_CRITICAL ();

    vTaskDelay( 1 ); // Allow ISR_Timer0 to work at least 1 tick

    ////////////////////////////////////////////////////////////////////////////////
    // We need to initialize xLastFlashTime prior to the first call to vTaskDelayUntil ()
    //
    portTickType tLastFlashTime = xTaskGetTickCount ();

    // Loop exactly every 1000 ms
    //
    for(;;)
    {
        static bool once_only = true;
        
        // Delay for 900 ms the flash period then turn the LED on
        vTaskDelayUntil( &tLastFlashTime, 900 / portTICK_RATE_MS );
        if ( once_only )
        AT91F_PIO_ClearOutput( LED_PIO, LED_POWER ); //LED on
        if ( once_only ) once_only = false;
        
        if ( xpi.IsFpgaOK () )
        {
            // Set green LED and clear yellow and red LED
            //
            taskENTER_CRITICAL ();
            FPGA_BegWrite ();
            FPGA_Write( XPI_W_PAGE_ADDR, 0 ); // Page 0
            FPGA_Write( XPI_W_P0_LED_CLEAR, XPI_LED_G );
            FPGA_BegRead ();
            taskEXIT_CRITICAL ();
            }
        

        // Delay for 100 ms the flash period then turn the LED off
        vTaskDelayUntil( &tLastFlashTime, 100 / portTICK_RATE_MS );
        // AT91F_PIO_SetOutput( LED_PIO, LED_POWER ); // LED off

        
        if ( xpi.IsFpgaOK () )
        {
            // Set green LED and clear yellow and red LED
            //
            taskENTER_CRITICAL ();
            FPGA_BegWrite ();
            FPGA_Write( XPI_W_PAGE_ADDR, 0 ); // Page 0
            FPGA_Write( XPI_W_P0_LED_SET, XPI_LED_G );
            FPGA_BegRead ();
            taskEXIT_CRITICAL ();
            }
        
        // Snapshot of the CPU usage
        Calc_CPU_Usage_Every1s ();
        }
    }

