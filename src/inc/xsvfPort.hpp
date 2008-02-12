#ifndef _XSVFPORT_H_INCLUDED
#define _XSVFPORT_H_INCLUDED

//---------------------------------------------------------------------------------------
// External functions used by XSVF player:
// 
//     ReadXSVF : Read byte from XSVF data stream
//     SetTMS   : Set TMS pin on JTAG port
//     SetTCK   : Set TCK pin on JTAG port
//     SetTDI   : Set TDI pin on JTAG port
//     GetTDO   : Read TDO pin on JTAG port
//     uSleep   : Delay execution (in microseconds)
//
//---------------------------------------------------------------------------------------

#include "sam7xpud.hpp" 

#define xsvfPrintf(...)  tracef( 1, __VA_ARGS__ )

//---------------------------------------------------------------------------------------

static inline int ReadXSVF( void )
{
    return xsvf.getc ();
    }

static inline void SetTMS( int val )
{
    if ( val )
        AT91F_PIO_SetOutput( AT91C_BASE_PIOA, FPGA_JTAG_TMS );
    else
        AT91F_PIO_ClearOutput( AT91C_BASE_PIOA, FPGA_JTAG_TMS );
    }

static inline void SetTCK( int val )
{
    if ( val )
        AT91F_PIO_SetOutput( AT91C_BASE_PIOA, FPGA_JTAG_TCK );
    else
        AT91F_PIO_ClearOutput( AT91C_BASE_PIOA, FPGA_JTAG_TCK );

    asm volatile( "NOP" ); // We need this so TDO can settle before GetTDO()
    asm volatile( "NOP" );
    }

static inline void SetTDI( int val )
{
    if ( val )
        AT91F_PIO_SetOutput( AT91C_BASE_PIOA, FPGA_JTAG_TDI );
    else
        AT91F_PIO_ClearOutput( AT91C_BASE_PIOA, FPGA_JTAG_TDI );
    }

static inline int GetTDO( void )
{
    return ISSET( AT91F_PIO_GetInput( AT91C_BASE_PIOA ), FPGA_JTAG_TDO );
    }

extern void uSleep( long microsec );

#endif // _XSVFPORT_H_INCLUDED
