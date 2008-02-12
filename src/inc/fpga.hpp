#ifndef _FPGA_HPP_INCLUDED
#define _FPGA_HPP_INCLUDED

//------------------------------------------------------------------------------
//      Definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// FPGA JTAG
#define FPGA_JTAG_TCK           AT91C_PIO_PA1      // output
#define FPGA_JTAG_TMS           AT91C_PIO_PA2      // output
#define FPGA_JTAG_TDI           AT91C_PIO_PA3      // output
#define FPGA_JTAG_TDO           AT91C_PIO_PA23     // input

//------------------------------------------------------------------------------
// FPGA I/O
#define FPGA_RESET              AT91C_PIO_PA7      // Output
#define FPGA_A0                 AT91C_PIO_PA24     // Output
#define FPGA_A1                 AT91C_PIO_PA25     // Output
#define FPGA_A2                 AT91C_PIO_PA26     // Output
#define FPGA_RDn                AT91C_PIO_PA27     // Output
#define FPGA_WRn                AT91C_PIO_PA28     // Output

#define FPGA_INTn               AT91C_PIO_PA20     // Input

#define FPGA_D0                 AT91C_PIO_PA8      // Input/Output
#define FPGA_D1                 AT91C_PIO_PA9      // Input/Output
#define FPGA_D2                 AT91C_PIO_PA10     // Input/Output
#define FPGA_D3                 AT91C_PIO_PA11     // Input/Output
#define FPGA_D4                 AT91C_PIO_PA12     // Input/Output
#define FPGA_D5                 AT91C_PIO_PA13     // Input/Output
#define FPGA_D6                 AT91C_PIO_PA14     // Input/Output
#define FPGA_D7                 AT91C_PIO_PA15     // Input/Output

#define FPGA_DATA \
    ( FPGA_D0 | FPGA_D1 | FPGA_D2 | FPGA_D3 \
    | FPGA_D4 | FPGA_D5 | FPGA_D6 | FPGA_D7 )

#define FPGA_ADDR \
    ( FPGA_A0 | FPGA_A1 | FPGA_A2 )

//------------------------------------------------------------------------------
enum // FPGA Read registers
{
    // Page independent
    XPI_R_INT_REQUEST          = 0, // D5..0: CTXE, PCM, EIRQ, CRX, CTX, FC
    // Page 0
    XPI_R_P0_SC_CTX            = 1, // D7..0= CTX data
    XPI_R_P0_SC_CRX            = 2, // D7..0= CRX data
    XPI_R_P0_SC_EIRQ           = 3, // D0= EIRQ
    XPI_R_P0_FC_FDFA           = 4, // D5= FCA5 .. D2= FCA0, D1= FCD1, D0= FCD0
    XPI_R_P0_FC_SENSE          = 5, // D0= SENSE
    XPI_R_P0_FC_STATUS         = 6, // D3= SENSE, D2= FCE, D1= FCD, D0= FCC
    XPI_R_P0_GLB_STATUS        = 7, // D1= EIRQ, D0= MCPU
    // Page 1
    XPI_R_P1_IRQ_ENABLE        = 1, // D5..0= CTXE, PCM, EIRQ, CRX, CTX, FC
    XPI_R_P1_MAGIC_LSB         = 4, // D7..0= '10101010' (0xAA)
    XPI_R_P1_MAGIC_MSB         = 5, // D7..0= '00010001' (0x11)
    XPI_R_P1_BOARD_POS         = 7, // D5..0= KA5..0
    // Page 2
    XPI_R_P2_PCM_ACK           = 0, // Dummy read (data should be ignored)
    XPI_R_P2_PCM_R0            = 4, // D7..0= PCM data
    XPI_R_P2_PCM_R1            = 5, // D7..0= PCM data
    XPI_R_P2_PCM_T0            = 6, // D7..0= PCM data
    XPI_R_P2_PCM_T1            = 7  // D7..0= PCM data
    };

//------------------------------------------------------------------------------
enum // FPGA Write registers
{
    // Page independent
    XPI_W_PAGE_ADDR            = 0, // D1..0= A4..3
    // Page 0
    XPI_W_P0_IRQ_ENABLE        = 1, // D5..0= CTXE, PCM, EIRQ, CRX, CTX, FC
    XPI_W_P0_IRQ_DISABLE       = 2, // D5..0= CTXE, PCM, EIRQ, CRX, CTX, FC
    XPI_W_P0_LED_SET           = 3, // D2= LED_R, D1= LED_Y, D0= LED_G
    XPI_W_P0_LED_CLEAR         = 4, // D2= LED_R, D1= LED_Y, D0= LED_G
    XPI_W_P0_FC_CONTROL        = 5, // D2= FCE, D1= FCD, D0= FCC
    XPI_W_P0_GLB_CONTROL       = 7, // D0= MCPU
    // Page 1
    XPI_W_P1_SC_CTX_DATA       = 1, // D7..0= CTX data
    XPI_W_P1_SC_CTX_INCFIFO    = 2, // D7..0= ignored; inc FIFO & enable CTXE
    // Page 2
    XPI_W_P2_PCM_R0            = 4, // D5..0= PCM time slot
    XPI_W_P2_PCM_R1            = 5, // D5..0= PCM time slot
    XPI_W_P2_PCM_T0            = 6, // D5..0= PCM time slot
    XPI_W_P2_PCM_T1            = 7  // D5..0= PCM time slot
    };

//------------------------------------------------------------------------------
enum // FPGA Register Bitmap
{
    XPI_IRQ_CTXE        = 0x20, // CTX empty
    XPI_IRQ_PCM         = 0x10, // PCM frame sync
    XPI_IRQ_EIRQ        = 0x08, // EIRQ changed event
    XPI_IRQ_CRX         = 0x04, // CRX data received event
    XPI_IRQ_CTX         = 0x02, // CTX data received event
    XPI_IRQ_FC          = 0x01, // FC data received event
    XPI_LED_R           = 0x04, // Red LED
    XPI_LED_Y           = 0x02, // Yellow LED
    XPI_LED_G           = 0x01, // Green LED
    XPI_FC_SENSE        = 0x08, // SENSE bit
    XPI_FC_FCE          = 0x04, // FCE bit
    XPI_FC_FCD          = 0x02, // FCD bit
    XPI_FC_FCC          = 0x01, // FCC bit
    XPI_GLB_MCPU        = 0x01, // MCPU bit
    XPI_GLB_EIRQ        = 0x02, // EIRQ bit
    XPI_GLB_MCTX_BUSY   = 0x04, // MCTX_BUSY bit
    };

//------------------------------------------------------------------------------
// FPGA I/O methods

static inline void FPGA_BegWrite( void )
{
    AT91F_PIO_OutputEnable( AT91C_BASE_PIOA, FPGA_DATA );
    }

static inline void FPGA_BegRead( void )
{
    AT91F_PIO_OutputDisable( AT91C_BASE_PIOA, FPGA_DATA );
    }

static inline void FPGA_Write( uint addr, uint data )
{
    AT91F_PIO_ForceOutput( AT91C_BASE_PIOA,
        FPGA_RDn | ( ( addr & 0x7 ) << 24 ) | ( ( data & 0xFF ) << 8 )
        );

    asm volatile( "NOP" );

    AT91F_PIO_SetOutput( AT91C_BASE_PIOA, FPGA_WRn );
    }

static inline uint FPGA_Read( uint addr )
{
    AT91F_PIO_ForceOutput( AT91C_BASE_PIOA,
        FPGA_WRn | ( ( addr & 0x7 ) << 24 )
        );
    
    asm volatile( "NOP" );

    uint data = ( AT91F_PIO_GetInput( AT91C_BASE_PIOA ) >> 8 ) & 0xFF;

    AT91F_PIO_SetOutput( AT91C_BASE_PIOA, FPGA_RDn );

    return data;
    }

static inline void FPGA_SetReset( bool reset = true )
{
    if ( reset )
        AT91F_PIO_SetOutput( AT91C_BASE_PIOA, FPGA_RESET );
    else
        AT91F_PIO_ClearOutput( AT91C_BASE_PIOA, FPGA_RESET );
    }

static inline bool FPGA_IsReset( void )
{
    return ISSET( AT91F_PIO_GetInput( AT91C_BASE_PIOA ), FPGA_RESET );
    }

static inline void FPGA_PulseReset( void )
{
    AT91F_PIO_SetOutput( AT91C_BASE_PIOA, FPGA_RESET );
    
    asm volatile( "NOP" );

    AT91F_PIO_ClearOutput( AT91C_BASE_PIOA, FPGA_RESET );
    }

extern bool FPGA_FC_Command( uint cmd );

#endif // _FPGA_HPP_INCLUDED
