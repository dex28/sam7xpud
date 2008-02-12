
//---------------------------------------------------------------------------------------
//      Includes
//---------------------------------------------------------------------------------------

#include "sam7xpud.hpp"
#include "xsvfPort.hpp" // SetTCK()

//---------------------------------------------------------------------------------------
//      Exported Symbols
//---------------------------------------------------------------------------------------

XSVF_Player xsvf;

//---------------------------------------------------------------------------------------
//      Module Implementation
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
// XSVF Player task
//---------------------------------------------------------------------------------------
portTASK_FUNCTION( XSVF_Player::MainTask, pvParameters )
{
    (void) pvParameters;

    for(;;)
    {
        xsvf.MainLoop ();
        }
    }

//---------------------------------------------------------------------------------------
// Main loop of the XSVF Player task
//---------------------------------------------------------------------------------------
void XSVF_Player::MainLoop( void )
{
#ifdef TR_INFO        
    taskENTER_CRITICAL ();
    TRACE_INFO( "XSVF_Player Task\n" );
    taskEXIT_CRITICAL ();
#endif

    // Peek first XSVF byte and loop while it is EOF.
    // Note that xXsvfFirstByte byte will be later reused by the ReadXSVF()
    // when ReadXSVF() is called by xsvfExecute().
    //
    do firstByte = getc ();
        while( firstByte < 0 );

#ifdef TR_INFO        
    taskENTER_CRITICAL ();
    TRACE_INFO( "Starting XSVF player (XSVF data peek 0x%02X)\n", firstByte );
    // TRACE_INFO( "Datap = %08X, Datac = %u\n", uint( datap ), datac );
    taskEXIT_CRITICAL ();
#endif

    // Reset CRC and byte count
    //
    crc = 0;
    byteCount = 0;

    // Elapsed time
    ulong dTimerStart = dTimerTick;

    // Start XSVF player
    //
    xsvfRC = xsvfExecute( traceLevel, parseOnly );

    long dElapsed = dTimerTick - dTimerStart;

    // Normalize CRC
    //
    crc &= 0xFFFF;

    // Signal that we have ended
    //
    sMsg.timeStamp = dTimerTick;
    sMsg.magicMSB  = XPI_MSG_MAGIC_MSB;
    sMsg.magicLSB  = XPI_MSG_MAGIC_LSB;
    sMsg.type      = XPI_IMSG_XSVF_END;
    sMsg.subtype   = 0;
    sMsg.data[0]   = xsvfRC;
    sMsg.data[1]   = ( crc >> 8 ) & 0xFF;
    sMsg.data[2]   = crc & 0xFF;
    sMsg.data[3]   = ( byteCount >>  24 ) & 0xFF;
    sMsg.data[4]   = ( byteCount >>  16 ) & 0xFF;
    sMsg.data[5]   = ( byteCount >>   8 ) & 0xFF;
    sMsg.data[6]   = ( byteCount >>   0 ) & 0xFF;
    sMsg.data[7]   = ( dElapsed >>  24 ) & 0xFF;
    sMsg.data[8]   = ( dElapsed >>  16 ) & 0xFF;
    sMsg.data[9]   = ( dElapsed >>   8 ) & 0xFF;
    sMsg.data[10]  = ( dElapsed >>   0 ) & 0xFF;

    usbOut.Put( NULL, 0, 1000 ); // Terminate previous message
    usbOut.Put( &sMsg, sizeof( XPI_IMSG_HEADER ) + 11, 1000 ); // Send this message

#ifdef TR_INFO        
    taskENTER_CRITICAL ();
    TRACE_INFO( "XSVF completed; Bytes = %u, CRC16 = 0x%04X, Elapsed = %ld\n", 
                byteCount, crc, dElapsed );
    taskEXIT_CRITICAL ();
#endif

    // CLEANUP:

    // Make xXsvfQueue empty
    //
    if ( datac != 0 )
    {
        datac = 0;
        semaEmpty.Release( 1 );
        }

    datap = NULL;
    firstByte = -1;
    
    // Mark player disabled
    //
    enabled = false;
    }

//---------------------------------------------------------------------------------------
// Start XSVF player
//---------------------------------------------------------------------------------------
void XSVF_Player::Enable( int trace_level, bool parse_only )
{
    if ( enabled )
        return; // TODO: restart?

    // Reset FPGA
    //
    xpi.ResetFPGA ();

    // Configure parameters
    //
    traceLevel = trace_level;
    parseOnly  = parse_only;
    
    // Send end-of-transfer packet (flush usbOut)
    //
    usbOut.Put( NULL, 0, 1000 );

    // Start player
    //
    enabled = true;
    }

//---------------------------------------------------------------------------------------
// Wait at least the specified number of microsec.
//
// Use a timer if possible; otherwise estimate the number of instructions
// necessary to be run based on the microcontroller speed. For this example
// we pulse the TCK port a number of times based on the processor speed.
// Example wroks for systems with TCK rates > 1 MHz.
//
// To calibrate timer, the best way is to send simple XSVF with single XWAIT instruction
// and then measure elapsed time of the xsvfExecute() to excatly match XWAIT time.
//
// Sample XSVF data to XWAIT reset state in 10 seconds (folowed by XCOMPLETE): 
// 17 00 00 00 98 96 80 00
//
//---------------------------------------------------------------------------------------

void uSleep( long microsec )
{
    // Note: Manually adjusted (with oscilloscope) to give 1 us per while loop
    // with 50% duty cycle of TCK on AT91SAM7S256 with flash wait state setting
    // AT91C_MC_FWS_1FWS: 2 cycles for Read, 3 for Write operations
    //
    while( microsec-- > 0)
    {
        SetTCK( 0 );
        asm volatile( "NOP" );
        asm volatile( "NOP" );
        asm volatile( "NOP" );
        asm volatile( "NOP" );
        asm volatile( "NOP" );
        asm volatile( "NOP" );
        asm volatile( "NOP" );
        asm volatile( "NOP" );
        asm volatile( "NOP" );
        asm volatile( "NOP" );
        asm volatile( "NOP" );
        asm volatile( "NOP" );
        asm volatile( "NOP" );
        asm volatile( "NOP" );
        asm volatile( "NOP" );
        asm volatile( "NOP" );
        asm volatile( "NOP" );
        
        SetTCK( 1 );
        asm volatile( "NOP" );
        asm volatile( "NOP" );
        asm volatile( "NOP" );
        asm volatile( "NOP" );
        asm volatile( "NOP" );
        asm volatile( "NOP" );
        asm volatile( "NOP" );
        }
    }

