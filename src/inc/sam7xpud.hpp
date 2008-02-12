#ifndef _SAM7XPUD_H_INCLUDED
#define _SAM7XPUD_H_INCLUDED

//---------------------------------------------------------------------------------------
//      Common, hardware description and trace includes
//---------------------------------------------------------------------------------------
#include "common.h"
#include "device.h"
#include "board.h"
#include "trace.h"

//---------------------------------------------------------------------------------------
//      FPGA hardware description
//---------------------------------------------------------------------------------------
#include "fpga.hpp"

//---------------------------------------------------------------------------------------
//      USB Framework includes
//---------------------------------------------------------------------------------------
#include "usbFramework.hpp"
#include "usbCDC.hpp"

//---------------------------------------------------------------------------------------
//      Scheduler includes
//---------------------------------------------------------------------------------------
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "sema.hpp"

//---------------------------------------------------------------------------------------
//      Backplane interface
//---------------------------------------------------------------------------------------
#include "xpi.hpp"

//---------------------------------------------------------------------------------------
//      External references & defines
//---------------------------------------------------------------------------------------

extern volatile int cpu_usage;

extern USB::CCDC sSer;

extern "C" unsigned int vPortGetMaxHeap( void );

extern volatile portBASE_TYPE isTaskWokenByPostInUsbIrq;

extern void sysDumpStatus( void );
extern int xsvfExecute( int dbgLevel, bool parseOnly );

extern void us1_putc( int ch );
extern void usb_putc( int ch );
extern void null_putc( int ch );

extern volatile ulong dTimerTick;

extern "C" const int verMajor, verMinor, verBuild;

//---------------------------------------------------------------------------------------
//      USB TRANSMITTER Class
//---------------------------------------------------------------------------------------

enum 
{ 
    USB_XMTR_BUF_SIZE = 4096 + 256, 
    USB_RCVR_BUF_SIZE = 4096, 
    };

class USBXMTR
{
    xMUTEX semaMutex;
    xSEMA semaFull;
    xSEMA semaEmpty;
    xSEMA semaSent;

    // Circular buffer of MSGBUF packets.
    //
    // MSGBUF Packet format:
    //    Header:
    //       uint8 len_MSB
    //       uint8 len_LSB
    //    Body:
    //       uint8 data[len]
    //
    uchar  buf[ USB_XMTR_BUF_SIZE ];
    uint   bufSize;
 
    uchar* pRead;
    uchar* pWrite;
    uchar* pMax;

    // CCDC::Write callback status
    volatile int bStatus;
    volatile uint dBytesTransferred;
    volatile uint dBytesRemaining;

    static void OnSendCompleted
    (
        USBXMTR* pThis,
        uchar bStatus,
        uint dBytesTransferred,
        uint dBytesRemaining
        )
    {
        pThis->bStatus = bStatus;
        pThis->dBytesTransferred = dBytesTransferred;
        pThis->dBytesRemaining = dBytesRemaining;

        if ( pThis->semaSent.ReleaseFromISR( 1, isTaskWokenByPostInUsbIrq ) )
        {
            isTaskWokenByPostInUsbIrq = pdTRUE;
            }
        }

public:
    
    USBXMTR( void )
        : semaFull( USB_XMTR_BUF_SIZE )
        , semaEmpty( 0 )
        , semaSent( 0 )
    {
        bufSize = USB_XMTR_BUF_SIZE;
        pRead   = buf;
        pWrite  = buf;
        pMax    = buf + bufSize;
        }

    void Initialize( void )
    {
#ifdef TR_INFO        
        taskENTER_CRITICAL ();
        TRACE_INFO( "USBXMTR: Initialize(): Size=%u\n", bufSize );
        taskEXIT_CRITICAL ();
#endif
        }

    void LockWrite( void )
    {
        // Lock pWrite mutex
        //
        do ; while( ! semaMutex.Lock( 100 ) );
        }

    void UnlockWrite( void )
    {
        // Unlock pWrite mutex
        //
        semaMutex.Unlock ();
        }

    bool Put( void* data, uint len, portTickType xTicksToWait );
    void Transmitter( void );

    static portTASK_FUNCTION( MainTask, pvParameters );
    };

//---------------------------------------------------------------------------------------
//      USB RECEIVER Class
//---------------------------------------------------------------------------------------

class USBRCVR
{
private:
    
    // Buffer for receiving data from the USB
    //
    uint bufSize;
    union
    {
        uchar buf[ USB_RCVR_BUF_SIZE ];

        struct : public XPI_OMSG_HEADER
        {
            uchar data[ 0 ];
            } ATTR_PACKED sMsg ;
        };

    // CCDC::Read callback status
    volatile int bStatus;
    volatile uint dBytesTransferred;
    volatile uint dBytesRemaining;
    
    // Queue used to pass message between the USB callback and the task
    //
    xSEMA semaReceived; 

    // Forwards data receiving from the USB host through the USART
    // This function operates asynchronously.
    //
    static void OnReceiveUSB
    (
        USBRCVR* pThis,
        uchar bStatus,
        uint dBytesTransferred,
        uint dBytesRemaining
        )
    {
        pThis->bStatus = bStatus;
        pThis->dBytesTransferred = dBytesTransferred;
        pThis->dBytesRemaining = dBytesRemaining;

        if ( bStatus == USB::USB_STATUS_IMMEDREAD )
            return; // Must not call ReleaseFromISR() out of ISR

        if ( pThis->semaReceived.ReleaseFromISR( 1, isTaskWokenByPostInUsbIrq ) )
        {
            isTaskWokenByPostInUsbIrq = pdTRUE;
            }
        }
    
public:
    
    USBRCVR( void )
        : semaReceived( 0 )
    {
        bufSize = USB_RCVR_BUF_SIZE;
        }

    void Initialize( void )
    {
#ifdef TR_INFO        
        taskENTER_CRITICAL ();
        TRACE_INFO( "USBRCVR: Initialize(): Size=%u\n", bufSize );
        taskEXIT_CRITICAL ();
#endif

        // Wait a bit for USB endpoint to be ready
        vTaskDelay( 100 );

        bStatus = USB::USB_STATUS_ABORTED;
        
        for( ;; ) 
        {
            taskENTER_CRITICAL ();
            int rc = sSer.Read( buf, bufSize, Callback_f( OnReceiveUSB ), this );
            taskEXIT_CRITICAL ();
            
            if ( rc == USB::USB_STATUS_SUCCESS )
                break;
            
            // USB endopoint is not ready; wait and retry
            vTaskDelay( 10 );
            }

#ifdef TR_INFO
        taskENTER_CRITICAL ();
        TRACE_INFO( "USBRCVR: Posted initial CCDC::Read\n" );
        taskEXIT_CRITICAL ();
#endif
        }

    void ReadMoreData( void )
    {
        bStatus = USB::USB_STATUS_ABORTED;
        
        for( ;; ) 
        {
            taskENTER_CRITICAL ();
            int rc = sSer.Read( buf, bufSize, Callback_f( OnReceiveUSB ), this );
            taskEXIT_CRITICAL ();
            
            if ( rc == USB::USB_STATUS_SUCCESS )
                break;

            // USB endopoint is not ready; wait and retry
            TRACE_ERROR( "!R " );
            vTaskDelay( 1 );
            }
        }

    void Receiver( void );

    static portTASK_FUNCTION( MainTask, pvParameters );
    };
    
//---------------------------------------------------------------------------------------
//     XSVF Player Task Class
//---------------------------------------------------------------------------------------
class XSVF_Player
{
    bool enabled;

    xSEMA semaFull;
    xSEMA semaEmpty;
    volatile uchar* datap;
    volatile uint datac;
    volatile int firstByte;
    int traceLevel;
    bool parseOnly;
    uint crc;
    uint byteCount;
    int xsvfRC;
    
    XPI_LONG_MSG sMsg;    
    
    // Update the CRC for transmitted and received data using
    // the CCITT 16-bit algorithm (X^16 + X^12 + X^5 + 1).
    //
    void CRC16( uchar ser_data )
    {
        crc  = uchar( crc >> 8 ) | ( crc << 8 );
        crc ^= ser_data;
        crc ^= uchar( crc & 0xff ) >> 4;
        crc ^= ( crc << 8 ) << 4;
        crc ^= ( ( crc & 0xFF ) << 4 ) << 1;

        ++byteCount;
        }

    void MainLoop( void );

public:    

    XSVF_Player( void )
        : semaFull( 0 )
        , semaEmpty( 0 )
    {
        enabled    = false;
        xsvfRC     = -1; // undefined error
        datap      = NULL;
        datac      = 0;
        firstByte  = -1;
        traceLevel = 0;
        parseOnly  = false;
        }

    void Enable( int trace_level, bool parse_only );

    int GetLastRC( void ) const
    {
        return enabled ? -1 : xsvfRC;
        }

    int getc( void )
    {
        uchar data;
        
        // Emit first byte that was previously peek by the player
        //
        if ( firstByte >= 0 )
        {
            data = firstByte;
            firstByte = -1;
            CRC16( data );
            return data;
            }

        // Get next XSVF data. Consider end of XSVF stream on timeout.
        //
        if ( datac == 0 )
        {
            if ( ! semaFull.Wait( 1, 2000 ) )
                return -1;
            
            if ( datac == 0 )
                return -1;
            }

        data = *datap++;
        --datac;

        if ( datac == 0 )
        {
            datap = NULL;
            semaEmpty.Release( 1 );
            }

        CRC16( data );
        return data;
        }

    void LockBuffer( uchar* buf, uint len )
    {
        if ( ! enabled )
            return; // ignore if not enabled

        datap = buf;
        datac = len;

        semaFull.Release( 1 );
        
        do ; while( ! semaEmpty.Wait( 1, 100 ) );
        }

    static portTASK_FUNCTION( MainTask, pvParameters );
    };

//---------------------------------------------------------------------------------------
//     Exported Symbols
//---------------------------------------------------------------------------------------

extern USBRCVR usbIn;
extern USBXMTR usbOut;
extern XSVF_Player xsvf;

#endif // _SAM7XPUD_H_INCLUDED
