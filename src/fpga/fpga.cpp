
//---------------------------------------------------------------------------------------
//      Includes
//---------------------------------------------------------------------------------------

#include "sam7xpud.hpp"

//---------------------------------------------------------------------------------------
//      External References
//---------------------------------------------------------------------------------------

extern void ISR_Wrapper_FPGA( void );

//---------------------------------------------------------------------------------------
//      Module Implementation
//---------------------------------------------------------------------------------------

xSEMA fpgaEvent;

//---------------------------------------------------------------------------------------
// Handler for the FPGA state change interrupt
//---------------------------------------------------------------------------------------
void ISR_FPGA( void )
{
    // Disable interrupt until interrupt is handled and enabled again (in some task).
    //
    AT91F_AIC_DisableIt( AT91C_BASE_AIC, AT91C_ID_IRQ0 );
    
    portBASE_TYPE isTaskWokenByPost = pdFALSE;
    
    if ( fpgaEvent.ReleaseFromISR( 1, isTaskWokenByPost ) )
    {
        isTaskWokenByPost = pdTRUE;
        }

    // Acknowledge the interrupt
    //
    AT91F_AIC_AcknowledgeIt( AT91C_BASE_AIC );

    // If an event caused a task to unblock then we call "Yield from ISR" to ensure 
    // that the unblocked task is the task that executes when the interrupt completes
    // if the unblocked task has a priority higher than the interrupted task.
    //
    if( isTaskWokenByPost )
        portYIELD_FROM_ISR ();
    }

//---------------------------------------------------------------------------------------
// FC Bus Command
//---------------------------------------------------------------------------------------
bool FPGA_FC_Command( uint cmd )
{
    uchar FCD[ 8 ];
    for ( int i = 0; i < 8; i ++ )
    {
        FCD[ i ] = ( cmd & 0x80 ) ? XPI_FC_FCD : 0;
        cmd <<= 1;
        }

    taskENTER_CRITICAL ();

    FPGA_BegWrite ();
    FPGA_Write( XPI_W_PAGE_ADDR, 0 ); // Page 0
    FPGA_Write( XPI_W_P0_FC_CONTROL, 0x00 ); // FCC, FCD, FCE = 0

    for ( int i = 0; i < 8; i++ )
    {
        FPGA_Write( XPI_W_P0_FC_CONTROL, FCD[ i ] ); // FCD with FCC = 0
        FCD[ i ] |= XPI_FC_FCC;
        FPGA_Write( XPI_W_P0_FC_CONTROL, FCD[ i ] ); // FCD with FCC = 1
        }

    FPGA_Write( XPI_W_P0_FC_CONTROL, XPI_FC_FCE ); // FCE = 1, FCC = 0

    FPGA_BegRead ();
    cmd = FPGA_Read( XPI_R_P0_FC_STATUS );

    FPGA_BegWrite ();
    FPGA_Write( XPI_W_P0_FC_CONTROL, 0x00 ); // FCE = 0

    taskEXIT_CRITICAL ();

    return ( cmd & XPI_FC_SENSE ) != 0; // Return SENSE
    }

#if 0
//---------------------------------------------------------------------------------------
// FPGA I/O Profiling
//---------------------------------------------------------------------------------------
//    Cycle     Speed     Cycle w/o Loop   Instructions in Loop
//  --------- ----------  ---------------  ----------------------------
//   253 ns   31.57 Mbps        0 ns        NOP;
//   781 ns   10.24 Mbps      528 ns        Write;
//   971 ns    8.23 Mbps      718 ns        BegWrite; Write; BegRead;
//   802 ns    9.97 Mbps      549 ns        Read;
//  1.27 us    6.32 Mbps     1013 ns        BegWrite; Write; BegRead; Read;
//  1.18 us    7.15 Mbps      865 ns        BegWrite; Write; Write; BegRead;
//---------------------------------------------------------------------------------------

static void FPGA_Profile( void )
{
    long N = 10000000;
    
    TRACE_INFO( "T0\n" );
    FPGA_BegWrite ();
    for ( long i = 0; i < N; i++ )
    {
        asm volatile ( "NOP" );
        }
    
    TRACE_INFO( "T1\n" );
    FPGA_BegWrite ();
    for ( long i = 0; i < N; i++ )
    {
        asm volatile ( "NOP" );
        FPGA_Write( i, i );
        }
    
    TRACE_INFO( "T2\n" );
    for ( long i = 0; i < N; i++ )
    {
        asm volatile ( "NOP" );
        FPGA_BegWrite ();
        FPGA_Write( i, i );
        FPGA_BegRead ();
        }
    
    TRACE_INFO( "T3\n" );
    FPGA_BegRead ();
    for ( long i = 0; i < N; i++ )
    {
        asm volatile ( "NOP" );
        (void volatile) FPGA_Read( i );
        }

    TRACE_INFO( "T4\n" );
    for ( long i = 0; i < N; i++ )
    {
        asm volatile ( "NOP" );
        FPGA_BegWrite ();
        FPGA_Write( i, i );
        FPGA_BegRead ();
        (void volatile) FPGA_Read( i );
        }
    
    TRACE_INFO( "T5\n" );
    for ( long i = 0; i < N; i++ )
    {
        asm volatile ( "NOP" );
        FPGA_BegWrite ();
        FPGA_Write( i, i );
        FPGA_Write( i, i );
        FPGA_BegRead ();
        }

    TRACE_INFO( "T6\n" );
    }
#endif

//---------------------------------------------------------------------------------------
// FPGA Interrupt Handler Task
//---------------------------------------------------------------------------------------
portTASK_FUNCTION( FPGA_IrqTasklet, pvParameters )
{
    (void) pvParameters; // The parameters are not used.

    taskENTER_CRITICAL();
    
    TRACE_INFO( "%XPI Main Task\n" );

    // Configure FPGA interrupt. Interrupt will be later enabled when entering 
    // FPGA communication mode.
    //
    AT91F_PMC_EnablePeriphClock
    (
        AT91C_BASE_PMC, // PIO controller base address
        uint( 1 ) << AT91C_ID_IRQ0
        );

    AT91F_AIC_ConfigureIt
    (
        AT91C_BASE_AIC,
        AT91C_ID_IRQ0,
        AT91C_AIC_PRIOR_HIGHEST,
        AT91C_AIC_SRCTYPE_EXT_LOW_LEVEL,
        ISR_Wrapper_FPGA
        );

    taskEXIT_CRITICAL();

    uint oldState = AT91F_PIO_GetInput( AT91C_BASE_PIOA );

    for( ;; )
    {
        // Calculate elapsed time and decrement timer
        //
        xpi.On_Timer (); // Decrement timer

        if ( ! fpgaEvent.Wait( 1, xpi.GetNextTimeout () ) )
        {
            // Calculate elapsed time and decrement timer
            //
            xpi.On_Timer ();
            
            // Post xpi.Goto(IDLE) handler
            //
            xpi.StartTransmissionIfIdle ();

            // Check PUSHBUTTON1
            //
            uint newState =  AT91F_PIO_GetInput( AT91C_BASE_PIOA );

            if ( ISSET( oldState, PUSHBUTTON1 ) && ISCLEARED( newState, PUSHBUTTON1 ) )
            {
                }

            oldState = newState;

            continue;
            }

        int irq_count = 10000; // protection from IRQ flood
        while( --irq_count >= 0 )
        {
            // Post xpi.Goto(IDLE) handler
            //
            xpi.StartTransmissionIfIdle ();

            // Set Page 0 and IRQ status bitmap
            //
            taskENTER_CRITICAL ();
            FPGA_BegWrite ();
            FPGA_Write( XPI_W_PAGE_ADDR, 0 ); // Page 0
            FPGA_BegRead ();
            uint irq_list = FPGA_Read( XPI_R_INT_REQUEST );
            taskEXIT_CRITICAL ();

            if ( ! irq_list )
                break;

            if ( irq_list & XPI_IRQ_CTXE ) // CTXE FIFO
            {
                xpi.On_CTXE ();
                }
            if ( irq_list & XPI_IRQ_CTX ) // CTX FIFO
            {
                xpi.On_CTX ();
                }
            if ( irq_list & XPI_IRQ_CRX ) // CRX FIFO
            {
                xpi.On_CRX ();
                }
            if ( irq_list & XPI_IRQ_EIRQ ) // EIRQ FIFO
            {
                xpi.On_EIRQ ();
                }
            if ( irq_list & XPI_IRQ_FC ) // FC FIFO
            {
                xpi.On_FC ();
                }
            if ( irq_list & XPI_IRQ_PCM ) // PCM FIFO
            {
                taskENTER_CRITICAL ();
                FPGA_BegWrite ();
                FPGA_Write( XPI_W_PAGE_ADDR, 2 ); // Page 2
                FPGA_BegRead ();
                
                // Collect 160 samples and send them as a packet to host.
                // At the same time read-in RPCM data from the last RPCM packet.
                // int octet = FPGA_Read( XPI_R_P2_PCM_R1 );
                
                // Acknowledge interrupt
                FPGA_Read( XPI_R_P2_PCM_ACK );
                taskEXIT_CRITICAL ();
                }
            }

        // Check IRQ flood
        //
        if ( irq_count < 0 )
        {
            taskENTER_CRITICAL ();
            tracef( 2, "SEVERE ERROR: Too many interrupts\n" );
            taskEXIT_CRITICAL ();
            xpi.ResetFPGA ();
            }
        else
        {
            // Enable FPGA interrupt
            //
            AT91F_AIC_EnableIt( AT91C_BASE_AIC, AT91C_ID_IRQ0 );
            }
        }
    }
