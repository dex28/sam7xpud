//------------------------------------------------------------------------------
//      Includes
//------------------------------------------------------------------------------

#include "sam7xpud.hpp"

//------------------------------------------------------------------------------
//      Exported functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Low-level initialization of the chip.
//------------------------------------------------------------------------------
extern "C" void DEV_Init1( void )
{
    // Sets the correct number of wait states in the flash controller
    //
#ifdef AT91C_FLASH_WAIT_STATES
    SET( AT91C_BASE_FLASH->FLA_FMR, AT91C_FLASH_WAIT_STATES );
#endif

    // Disable the watchdog
    //
    AT91C_BASE_WDTC->WDTC_WDMR = AT91C_WDTC_WDDIS;

    // Enable user reset: assertion length programmed to 1 ms
    //
    AT91C_BASE_RSTC->RSTC_RMR = AT91C_RSTC_URSTEN | (0x4 << 8) | ((uint) 0xA5 << 24);
    
    // Start clocks
    //
    DEV_Resume ();

    /////////////////////////////////////////////////////////////////////////////////////
    // Configure PIO
    //
    AT91F_PMC_EnablePeriphClock( AT91C_BASE_PMC, 1 << AT91C_ID_PIOA );

    // NOTE: DO NOT USE AT91F_CfgPullup() function because it both enables and disables
    // pull-ups at the same time (and does that incorrectly).

    // Enable Pull-ups
    AT91C_BASE_PIOA->PIO_PPUER 
        = FPGA_JTAG_TDO | FPGA_DATA | FPGA_INTn | FPGA_RESET | PUSHBUTTON1;

    // Disable Pull-ups
    AT91C_BASE_PIOA->PIO_PPUDR 
        = FPGA_RDn | FPGA_WRn | FPGA_ADDR | FPGA_RESET 
        | LED_POWER | LED_USB
        | FPGA_JTAG_TCK | FPGA_JTAG_TMS | FPGA_JTAG_TDI;

    // Configure Input Pins
    AT91F_PIO_CfgInput( AT91C_BASE_PIOA, 
        FPGA_JTAG_TDO | FPGA_DATA | FPGA_INTn | PUSHBUTTON1
        );

    // Set Output Bits
    AT91F_PIO_SetOutput( AT91C_BASE_PIOA, 
        FPGA_JTAG_TMS | FPGA_RDn | FPGA_WRn | FPGA_DATA | FPGA_RESET 
        | LED_POWER | LED_USB
        );

    // Clear Output Bits
    AT91F_PIO_ClearOutput( AT91C_BASE_PIOA,
        FPGA_JTAG_TCK | FPGA_JTAG_TDI | FPGA_ADDR
        );

    // Configure Output Pins
    AT91F_PIO_CfgOutput( AT91C_BASE_PIOA, 
        FPGA_JTAG_TMS | FPGA_JTAG_TCK | FPGA_JTAG_TDI 
        | FPGA_RDn | FPGA_WRn | FPGA_RESET | FPGA_ADDR 
        | LED_POWER | LED_USB
        );

    // Configure Direct Write Output Pins
    AT91F_PIO_CfgDirectDrive( AT91C_BASE_PIOA,
        FPGA_RDn | FPGA_WRn | FPGA_ADDR | FPGA_DATA 
        );
    }

//---------------------------------------------------------------------------------------
// Additional initialization (called after data & bss init, but before ctor init).
// Initialize trace functionality so we could use tracef() in C++ constructors.
//---------------------------------------------------------------------------------------
extern "C" void DEV_Init2( void )
{
    /////////////////////////////////////////////////////////////////////////////////////
    // Initialize US1 port and open it as trace stream descriptor 0 (stdout)
    //
    uint dMCK = AT91C_MASTER_CLOCK; // Main oscillator frequency
    uint dBaudrate = 115200;        // Desired baudrate

    // Clock US1 serial port and configure its pins
    //
    AT91F_PMC_EnablePeriphClock
    (
        AT91C_BASE_PMC, // PIO controller base address
        uint( 1 << AT91C_ID_US1 )
        );

    // Configure PIO controllers to periph mode
    //
    AT91F_PIO_CfgPeriph
    (
        AT91C_BASE_PIOA, // PIO controller base address
        uint( AT91C_PA22_TXD1 ) |
        uint( AT91C_PA21_RXD1 ), // Peripheral A
        0 // Peripheral B
        );

    // Configure serial port baud rate and sync/async mode
    //
    AT91F_US_Configure
    (
        (AT91PS_USART) AT91C_BASE_US1,
        dMCK,
        AT91C_US_ASYNC_MODE,
        dBaudrate,
        0
        );

    // Enable Transmitter & Receiver
    //
    AT91F_US_EnableTx( (AT91PS_USART) AT91C_BASE_US1 );
    AT91F_US_EnableRx( (AT91PS_USART) AT91C_BASE_US1 );
    
    /////////////////////////////////////////////////////////////////////////////////////
    // Open output descriptors

    tracef_open( 0, us1_putc, /*LF2CRLF=*/ true, /*TimeStamp=*/ true );
    tracef_open( 1, us1_putc, /*LF2CRLF=*/ true, /*TimeStamp=*/ true );
    tracef_open( 2, usb_putc, /*LF2CRLF=*/ false, /*TimeStamp=*/ false );
    tracef_open( 3, usb_putc, /*LF2CRLF=*/ false, /*TimeStamp=*/ false );

    TRACE_INFO( "\nSAM7 XPU-D R2A v%d.%d (Build %d)\n", verMajor, verMinor, verBuild );
    TRACE_INFO( "--------------------------\n" );

    /////////////////////////////////////////////////////////////////////////////////////
    // Report PIO configuration

    TRACE_INFO( "PSR:  %08x\n", AT91F_PIO_GetStatus( AT91C_BASE_PIOA ) );
    TRACE_INFO( "PUSR: %08x\n", AT91F_PIO_GetCfgPullup( AT91C_BASE_PIOA ) );
    TRACE_INFO( "OSR:  %08x\n", AT91F_PIO_GetOutputStatus( AT91C_BASE_PIOA ) );
    TRACE_INFO( "OWSR: %08x\n", AT91F_PIO_GetOutputWriteStatus( AT91C_BASE_PIOA ) );
    TRACE_INFO( "ODSR: %08x\n", AT91F_PIO_GetOutputDataStatus( AT91C_BASE_PIOA ) );
    TRACE_INFO( "--------------------------\n" );
    }

//------------------------------------------------------------------------------
// Puts the device back into a normal operation mode
//------------------------------------------------------------------------------
extern "C" void DEV_Resume( void )
{
    // Enable Main Oscillator
    // Main Oscillator startup time is board specific:
    // Main Oscillator Startup Time (18432KHz) corresponds to 1.5ms
    // (0x08 for AT91C_CKGR_OSCOUNT field)
    //
    AT91C_BASE_PMC->PMC_MOR = ( AT91C_CKGR_OSCOUNT & ( 0x8 << 8 ) )
                              | AT91C_CKGR_MOSCEN;

    // Wait until the oscillator is stabilized
    //
    do ; while( ! ISSET( AT91C_BASE_PMC->PMC_SR, AT91C_PMC_MOSCS ) );

    // Set PLL to 96MHz (96,10971429MHz) and UDP Clock to 48MHz
    // PLL Startup time depends on PLL RC filter
    // UDP Clock (48,05485714MHz=+0,114%) is compliant with the Universal Serial Bus
    // Specification (+/- 0.25% for full speed)
    //
    AT91C_BASE_PMC->PMC_PLLR 
       = AT91C_CKGR_USBDIV_1                        // DIV = 2
       | AT91C_CKGR_OUT_0
       | ( AT91C_CKGR_PLLCOUNT & ( 0x28 << 8 ) )    // PLLCOUNT = 50
       | ( AT91C_CKGR_MUL & ( 0x48 << 16 ) )        // MUL = 72
       | ( AT91C_CKGR_DIV & 0xE );                  // DIV = 14

    // Wait until the PLL is stabilized
    do ; while( ! ISSET( AT91C_BASE_PMC->PMC_SR, AT91C_PMC_LOCK ) );

    // Selection of Master Clock MCK (equal to Processor Clock PCK) equal to
    // PLL/2 = 48MHz
    // The PMC_MCKR register must not be programmed in a single write operation
    // (see. Product Errata Sheet)
    AT91C_BASE_PMC->PMC_MCKR = AT91C_PMC_PRES_CLK_2;
    do ; while( ! ISSET( AT91C_BASE_PMC->PMC_SR, AT91C_PMC_MCKRDY ) );

    SET(AT91C_BASE_PMC->PMC_MCKR, AT91C_PMC_CSS_PLL_CLK );
    do ; while( ! ISSET( AT91C_BASE_PMC->PMC_SR, AT91C_PMC_MCKRDY ) );
    }

//------------------------------------------------------------------------------
// Puts the device into low-power mode.
//------------------------------------------------------------------------------
void DEV_Suspend( void )
{
    // Voltage regulator in standby mode : Enable VREG Low Power Mode
    AT91C_BASE_VREG->VREG_MR |= AT91C_VREG_PSTDBY;

    // Set the master clock on slow clock
    AT91F_PMC_CfgMCKReg( AT91C_BASE_PMC, AT91C_PMC_PRES_CLK_2 );
    do ; while( ! ISSET( AT91C_BASE_PMC->PMC_SR, AT91C_PMC_MCKRDY ) );

    AT91F_PMC_CfgMCKReg( AT91C_BASE_PMC, AT91C_PMC_CSS_SLOW_CLK );
    do ; while( ! ISSET(AT91C_BASE_PMC->PMC_SR, AT91C_PMC_MCKRDY ) );

    // Disable the PLL
    AT91F_CKGR_CfgPLLReg( AT91C_BASE_CKGR, 0 );

    // Disable the main Oscillator
    AT91C_BASE_PMC->PMC_MOR = 0;
    }

/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support  -  ROUSSET  -
 * ----------------------------------------------------------------------------
 * Copyright (c) 2006, Atmel Corporation

 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaiimer below.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the disclaimer below in the documentation and/or
 * other materials provided with the distribution.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */

/*
$Id: device.c 194 2006-10-30 09:53:06Z jjoannic $
*/

