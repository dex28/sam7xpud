#ifndef _BOARD_H
#define _BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//      Definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Slow clock used at startup (32kHz)
#define SLOWCLOCK       32768

//------------------------------------------------------------------------------
// Main clock
#define AT91C_MASTER_CLOCK      48000000

//------------------------------------------------------------------------------
// USB Bus power
#define AT91C_VBUS              AT91C_PIO_PA18
#define AT91C_PIO_VBUS          AT91C_BASE_PIOA
#define AT91C_ID_VBUS           AT91C_ID_PIOA

//------------------------------------------------------------------------------
// USB Pull-ups
#define AT91C_PULLUP            AT91C_PIO_PA17
#define AT91C_PIO_PULLUP        AT91C_BASE_PIOA
#define AT91C_ID_PULLUP         AT91C_ID_PIOA

//------------------------------------------------------------------------------
// LEDs
#define LED_POWER               AT91C_PIO_PA0   // LED0
#define LED_USB                 AT91C_PIO_PA31  // LED1
#define LED_PIO                 AT91C_BASE_PIOA

//------------------------------------------------------------------------------
// Test Pushbutton
#define PUSHBUTTON1             AT91C_PIO_PA19     // input

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _BOARD_H
