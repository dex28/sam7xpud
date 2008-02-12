
#include "sam7xpud.hpp"

#include <string.h> // memcmp

//---------------------------------------------------------------------------------------

USBXMTR usbOut;
USBRCVR usbIn;

//---------------------------------------------------------------------------------------
// Handler for the USB controller interrupt
// Defers the call to the USB_Handler function.
//---------------------------------------------------------------------------------------

volatile portBASE_TYPE isTaskWokenByPostInUsbIrq = pdFALSE;

void ISR_USB( void )
{
    isTaskWokenByPostInUsbIrq = pdFALSE;

    // USB_Handler may also call callbacks established by SER_Read()/SER_Write()
    //
    sSer.EventHandler ();

    AT91F_AIC_AcknowledgeIt( AT91C_BASE_AIC );

    // If an event caused a task to unblock then we call "Yield from ISR" to ensure 
    // that the unblocked task is the task that executes when the interrupt completes
    // if the unblocked task has a priority higher than the interrupted task.
    //
    if( isTaskWokenByPostInUsbIrq )
        portYIELD_FROM_ISR();
    }

//---------------------------------------------------------------------------------------
// USBXMTR
//---------------------------------------------------------------------------------------

portTASK_FUNCTION( USBXMTR::MainTask, pvParameters )
{
    (void) pvParameters; // The parameters are not used.

#ifdef TR_INFO    
    taskENTER_CRITICAL ();
    TRACE_INFO( "USBXMTR: Main Task\n" );
    taskEXIT_CRITICAL ();
#endif

    usbOut.Initialize ();

    for(;;)
    {
        usbOut.Transmitter ();
        }
    }

bool USBXMTR::Put( void* data, uint len, portTickType xTicksToWait )
{
    // Wait enough space to fit 2-byte length + data
    //
    if ( ! semaFull.Wait( len + 2, xTicksToWait ) )
        return false;

    // Lock pWrite mutex
    //
    LockWrite ();

    // Put 2-byte length into circular buffer: MSB first
    //
    *pWrite++ = ( len >> 8 ) & 0xFF;
    if ( pWrite >= pMax )
        pWrite = buf;
    *pWrite++ = len & 0xFF;
    if ( pWrite >= pMax )
        pWrite = buf;

    // Put data into circular buffer
    //
    uchar* pSrc = (uchar*) data;

    if ( pWrite + len < pMax )
    {
        // Put flat data (not crossing upper boundary)
        //
        for( uint i = 0; i < len; i++ )
            *pWrite++ = *pSrc++;
        }
    else
    {
        // Put circular data (crossing upper boundary)
        //
        uint len2 = pWrite + len - pMax;
        uint len1 = len - len2;

        for( uint i = 0; i < len1; i++ )
            *pWrite++ = *pSrc++;

        pWrite = buf;

        for( uint i = 0; i < len2; i++ )
            *pWrite++ = *pSrc++;
        }

    // Notify Transmitter()
    //
    semaEmpty.Release( len + 2 );

    // Unlock pWrite mutex
    //
    UnlockWrite ();

    return true;
    }

void USBXMTR:: Transmitter( void )
{
    // Wait 2-byte header
    //
    do ; while( ! semaEmpty.Wait( 2, 1000 ) );

    // Retrieve data length from packet header
    //
    uint len = *pRead++;
    if ( pRead >= pMax )
        pRead = buf;
    len = ( len << 8 ) + *pRead++;
    if ( pRead >= pMax )
        pRead = buf;

    // Wait the rest of data
    //
    do ; while( ! semaEmpty.Wait( len, 1000 ) );

    // Send data over USB
    //
    bool isSent = false;

    for ( int i = 0; i < 100; i++ )
    {
        taskENTER_CRITICAL ();
        int rc = sSer.Write( pRead, len, Callback_f( OnSendCompleted ), this, buf, pMax );
        taskEXIT_CRITICAL ();

        if ( rc == USB::USB_STATUS_SUCCESS )
        {
            isSent = true;
            break;
            }

        // USB endopoint is not ready; wait and retry
        //
        vTaskDelay( 1 );

        if ( i == 0 )
        {
#ifdef TR_ERROR
            taskENTER_CRITICAL ();
            TRACE_ERROR( "USBXMTR: Write failed\n" );
            taskEXIT_CRITICAL ();
#endif
            }
        }

    // Wait for transmission to end
    //
    if ( isSent )
    {
        do ; while( ! semaSent.Wait( 1, 1000 ) );

        if ( bStatus != USB::USB_STATUS_SUCCESS ) 
        {
#ifdef TR_ERROR
            taskENTER_CRITICAL ();
            TRACE_ERROR( "USBXMTR: Transfer error\n" );
            taskEXIT_CRITICAL ();
#endif
            }
        else
        {
#ifdef TR_DEBUG_M
            taskENTER_CRITICAL ();
            TRACE_DEBUG_M( "USBXMTR: Sent %5u, %5u; RC = %d\n", 
                    dBytesTransferred, dBytesRemaining, bStatus );
            taskEXIT_CRITICAL ();
#endif            
            }
        }

    // Advance pRead
    //
    pRead += len;
    if ( pRead >= pMax )
        pRead -= bufSize;

    // Release space back to circular buffer and unblock some USBXMTR::Put
    // waiting for more space.
    //
    semaFull.Release( len + 2 );
    }

//---------------------------------------------------------------------------------------
// USBRCVR
//---------------------------------------------------------------------------------------

portTASK_FUNCTION( USBRCVR::MainTask, pvParameters )
{
    (void) pvParameters; // The parameters are not used.

#ifdef TR_INFO    
    taskENTER_CRITICAL();
    TRACE_INFO( "USBRCVR: Main Task\n" );
    taskEXIT_CRITICAL();
#endif

    // Initialize the USB CDC driver
    sSer.Init ();
    
#ifdef TR_INFO    
    taskENTER_CRITICAL();
    TRACE_INFO( "Connecting USB... \n" );
    taskEXIT_CRITICAL();
#endif

    // Wait for the device to be powered before connecting it
    while ( ! sSer.IsPowered () )
        vTaskDelay( 1 );

    // Connect
    sSer.Connect ();

#ifdef TR_INFO    
    taskENTER_CRITICAL();
    TRACE_INFO( "USB Connected\n" );
    taskEXIT_CRITICAL();
#endif

    // Initialize receiver (start receiving data)
    usbIn.Initialize ();

    for(;;)
    {
        usbIn.Receiver ();
        usbIn.ReadMoreData ();
        }
    }

void USBRCVR::Receiver( void )
{
    // Check whether read callback has returned already found in FIFO. If not
    // then we should wait for read callback to complete.
    //
    if ( bStatus != USB::USB_STATUS_IMMEDREAD )
    {
        // Wait callback to receive data
        //
        do ; while( ! semaReceived.Wait( 1, 1000 ) );
        
        // Check CCDC::Read() callback status
        //
        if ( bStatus != USB::USB_STATUS_SUCCESS ) 
        {
#ifdef TR_ERROR            
            taskENTER_CRITICAL ();
            TRACE_ERROR( "USBRCVR: Transfer error\n" );
            taskEXIT_CRITICAL ();
#endif
            return;
            }
        }

#ifdef TR_DEBUG_M
    taskENTER_CRITICAL ();
    TRACE_DEBUG_M( "USBRCVR: Got %6u, %5u; RC = %d\n", 
            dBytesTransferred, dBytesRemaining, bStatus );
    taskEXIT_CRITICAL ();
#endif

    if ( dBytesTransferred == 0 )
        return;

    int dataLen = dBytesTransferred - sizeof( XPI_OMSG_HEADER );

    // Loopback XPI_OMSG messages without proper header
    //
    if ( dataLen < 0 
        || sMsg.magicMSB != XPI_MSG_MAGIC_MSB 
        || sMsg.magicLSB != XPI_MSG_MAGIC_LSB 
        )
    {
        if ( sMsg.magicMSB == '?' )
            xpi.DumpStatus ();
        else
            usbOut.Put( buf, dBytesTransferred, 1000 );
        return;
        }

    // Process XPI_OMSG depending on message type
    //
    switch( sMsg.type )
    {
        //-------------------------------------------------------------------------------
        case XPI_OMSG_NULL:
            break;

        //-------------------------------------------------------------------------------
        case XPI_OMSG_LOOP:
        {
            if ( sMsg.subtype != 0 )
            {
                // Simulate FPGA I/O
                //
                for ( int i = 0; i <= dataLen / 32; i++ )
                {
                    taskENTER_CRITICAL ();
                    FPGA_BegWrite ();
                    FPGA_Write( XPI_W_PAGE_ADDR, 1 ); // Page 1
                    FPGA_BegRead ();
                    for ( int j = 0; j < 64; j++ )
                        (void) FPGA_Read( XPI_R_P1_BOARD_POS );
                    taskEXIT_CRITICAL ();  
                    }
                }

            // Loopback packet to usbOut.
            //
            sMsg.type    = XPI_IMSG_LOOP;
            sMsg.subtype = 0;
            usbOut.Put( buf, dBytesTransferred, 1000 );
            }
            break;

        //-------------------------------------------------------------------------------
        case XPI_OMSG_XSVF_START:
        {
            // Get XSVF parameters
            //
            bool traceLevel = dataLen >= 1 ? sMsg.data[ 0 ] : 0;
            bool parseOnly  = dataLen >= 2 ? sMsg.data[ 1 ] : false;
            
            // Configure std output for XSVF trace (default is null device)
            //
            if ( dataLen >= 3 && sMsg.data[ 2 ] == 2 )
            {
                tracef_open( 1, us1_putc, /*LF2CRLF=*/ true, /*TimeStamp=*/ true );
                }
            else if ( dataLen >= 3 && sMsg.data[ 2 ] == 1 )
            {
                tracef_open( 1, usb_putc, /*LF2CRLF=*/ false, /*TimeStamp=*/ false );
                }
            else
            {
                tracef_open( 1, null_putc, /*LF2CRLF=*/ false, /*TimeStamp=*/ false );
                }

            // Enable (start) XSVF player
            //
            xsvf.Enable( traceLevel, parseOnly );
            }
            break;

        //-------------------------------------------------------------------------------
        case XPI_OMSG_XSVF_DATA:
        {
            if ( dataLen > 0 )
            {
                xsvf.LockBuffer( sMsg.data, dataLen );
                }
            }
            break;

        //-------------------------------------------------------------------------------
        case XPI_OMSG_FPGA_INIT:
        {
            bool coldStart    = dataLen >= 1 ? sMsg.data[ 0 ] : false;
            bool forcePassive = dataLen >= 2 ? sMsg.data[ 1 ] : false;
            xpi.InitializeFPGA( coldStart, forcePassive );
            }
            break;

        //-------------------------------------------------------------------------------
        case XPI_OMSG_QUERY:
        {
            if ( sMsg.subtype == 0x01 )
                xpi.DumpStatus ();
            else
                sysDumpStatus ();
            }
            break;

        //-------------------------------------------------------------------------------
        case XPI_OMSG_LOG_CFG:
        {
            xpi.SetTraceMask( dataLen >= 1 ? sMsg.data[ 0 ] : 0 );
            }
            break;

        //-------------------------------------------------------------------------------
        case XPI_OMSG_SC_DATA:
        {
            xpi.Put( sMsg.data, dataLen, 100 );
            }
            break;

        //-------------------------------------------------------------------------------
        case XPI_OMSG_FC_CMD:
        {
            if ( dataLen >= 2 ) // Addr with D1 and D0
            {
                int card = sMsg.data[ 0 ] & 0x3F;

                FPGA_FC_Command( ( card << 2 ) | ( sMsg.data[ 1 ] & 0x03 )  );
                vTaskDelay( 2 );
                }
            else if ( dataLen >= 1 ) // Only Addr
            {
                int card = sMsg.data[ 0 ] & 0x3F;

                // Turn off and reset the board
                //
                FPGA_FC_Command( ( card << 2 ) | 0x00 );
                vTaskDelay( 2 );

                // Turn on the board
                //
                FPGA_FC_Command( ( card << 2 ) | 0x01 );
                vTaskDelay( 2 );

                // Is the board installed?
                //
                if ( FPGA_FC_Command( ( card << 2 ) | 0x03 ) )
                {
                    vTaskDelay( 2 );

                    // Is the board turned on and installed?
                    //
                    FPGA_FC_Command( ( card << 2 ) | 0x02 );
                    }
                vTaskDelay( 2 );
                }
            }
            break;

        //-------------------------------------------------------------------------------
        default:
            // TODO: issue warning "unknown XPI_OMSG"
            break;
        }
    }
