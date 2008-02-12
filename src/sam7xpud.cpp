
//---------------------------------------------------------------------------------------
//      Includes
//---------------------------------------------------------------------------------------

#include "sam7xpud.hpp"

//---------------------------------------------------------------------------------------
//      External References
//---------------------------------------------------------------------------------------

extern portTASK_FUNCTION( MainTimer_Task, pvParameters );
extern portTASK_FUNCTION( FPGA_IrqTasklet, pvParameters );

//---------------------------------------------------------------------------------------
//      Module Implementation
//---------------------------------------------------------------------------------------

static xTaskHandle t1, t2, t3, t4, t5, t6;

//---------------------------------------------------------------------------------------
// Put character to US1
//---------------------------------------------------------------------------------------

void us1_putc( int ch )
{
    do ; while( ! AT91F_US_TxReady( (AT91PS_USART)AT91C_BASE_US1) );

    AT91F_US_PutChar( (AT91PS_USART)AT91C_BASE_US1, ch );
    };

//---------------------------------------------------------------------------------------
// Put character to void
//---------------------------------------------------------------------------------------

void null_putc( int ch )
{
    };

//---------------------------------------------------------------------------------------
// Put character to USB
//---------------------------------------------------------------------------------------

void usb_putc( int ch )
{
    // Note: The implementation is NOT THREAD safe and assumes
    // to be used only from single thread (in this case xsvf main task).
    //
    static struct ATTR_PACKED : public XPI_IMSG_HEADER 
    {
        uchar data[ 128 ];
        } sMsg;

    static volatile size_t dataLen = 0;

    if ( ch != '\n' )
    {
        sMsg.data[ dataLen++ ] = ch;
        }
    
    if ( ch == '\n' || dataLen >= sizeof( sMsg.data ) )
    {
        sMsg.magicMSB  = XPI_MSG_MAGIC_MSB;
        sMsg.magicLSB  = XPI_MSG_MAGIC_LSB;
        sMsg.type      = XPI_IMSG_LOG;
        sMsg.subtype   = 0;
        sMsg.timeStamp = dTimerTick;
        usbOut.Put( &sMsg, sizeof( XPI_IMSG_HEADER) + dataLen, 1000 );
        dataLen = 0;
        }
    };

//---------------------------------------------------------------------------------------
//      Main Program Entry Point
//---------------------------------------------------------------------------------------

int main( void )
{
    ////////////////////////////////////////////////////////////////////////////////
    // When using the JTAG debugger the hardware is not always initialised to
    // the correct default state.  This line just ensures that this does not
    // cause all interrupts to be masked at the start
    //
    AT91F_AIC_AcknowledgeIt( AT91C_BASE_AIC );

    ////////////////////////////////////////////////////////////////////////////////
    // Create tasks
    //

    xTaskCreate
    ( 
        XSVF_Player::MainTask, (const signed portCHAR* const) "XSVF", 
        256, NULL, tskIDLE_PRIORITY + 2, &t1 
        );

    xTaskCreate
    ( 
        FPGA_IrqTasklet, (const signed portCHAR* const) "FPGA", 
        256, NULL, tskIDLE_PRIORITY + 3, &t2 
        );

    xTaskCreate
    ( 
        XPI::MainTask, (const signed portCHAR* const) "XPI", 
        256, NULL, tskIDLE_PRIORITY + 3, &t3 
        );

    xTaskCreate
    ( 
        MainTimer_Task, (const signed portCHAR* const) "LEDT", 
        128, NULL, tskIDLE_PRIORITY + 4, &t4 
        );

    xTaskCreate
    ( 
        USBRCVR::MainTask, (const signed portCHAR* const) "USBR", 
        256, NULL, tskIDLE_PRIORITY + 5, &t5 
        );

    // Initialize transmitter
    xTaskCreate
    ( 
        USBXMTR::MainTask, (const signed portCHAR* const) "USBT", 
        192, NULL, tskIDLE_PRIORITY + 6, &t6 
        );

    TRACE_INFO( "--------------------------\n" );
    TRACE_INFO( "Starting FreeRTOS...\n" );
    
    //-----------------------------------------------------------------------------------
    // Start the scheduler.
    // NOTE: Tasks run in system mode and the scheduler runs in Supervisor mode.
    // The processor MUST be in supervisor mode when vTaskStartScheduler is called.
    //-----------------------------------------------------------------------------------
    vTaskStartScheduler ();

    // We should never get here as control is now taken by the scheduler.
    //
    return 0;
    }

//---------------------------------------------------------------------------------------
//  Show FreeRTOS free stack space of the task.
//---------------------------------------------------------------------------------------
void ShowStackFreeSpace( xTaskHandle task )
{
    // Keep this structure synchronized with FreeRTOS's tskTCB
    struct TCB
    {
        volatile portSTACK_TYPE *pxTopOfStack;
        xListItem               xGenericListItem;
        xListItem               xEventListItem;
        unsigned portBASE_TYPE  uxPriority;
        portSTACK_TYPE          *pxStack;
        signed portCHAR         pcTaskName[ configMAX_TASK_NAME_LEN ];
    };

    TCB* pTask = (TCB*)task;
    uchar* pucStackByte = (uchar*)pTask->pxStack;
    uint usCount = 0;

    while( *pucStackByte == 0xa5 )
    {
        pucStackByte++;
        usCount++;
        }

    usCount /= sizeof( portSTACK_TYPE );
    
    tracef( 2, "Task [%-4s]: Stack %3u\n", pTask->pcTaskName, usCount ); 
    }

//---------------------------------------------------------------------------------------
//  Show processor, global heap and task stack space usage
//---------------------------------------------------------------------------------------
void sysDumpStatus( void )
{
    tracef( 2, "CPU %3d.%d%%, ", cpu_usage / 10, cpu_usage % 10 );
    tracef( 2, "Heap %d of %d\n", vPortGetMaxHeap (), configTOTAL_HEAP_SIZE );
    
    ShowStackFreeSpace( t1 );
    ShowStackFreeSpace( t2 );
    ShowStackFreeSpace( t3 );
    ShowStackFreeSpace( t4 );
    ShowStackFreeSpace( t5 );
    ShowStackFreeSpace( t6 );
    }
