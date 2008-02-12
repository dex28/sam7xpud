
//---------------------------------------------------------------------------------------
//      Includes
//---------------------------------------------------------------------------------------

#include "sam7xpud.hpp"

#include <string.h> // memcpy

//---------------------------------------------------------------------------------------
//      Defines
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
//      Module Implementation
//---------------------------------------------------------------------------------------

XPI xpi;

//---------------------------------------------------------------------------------------
// XPI interface
//---------------------------------------------------------------------------------------

portTASK_FUNCTION( XPI::MainTask, pvParameters )
{
    (void) pvParameters; // The parameters are not used.

#ifdef TR_INFO
    taskENTER_CRITICAL ();
    TRACE_INFO( "XPI: Main Task\n" );
    taskEXIT_CRITICAL ();
#endif

    for(;;)
    {
        xpi.Transmitter ();
        }
    }

void XPI::DumpStatus( void )
{
    tracef( 2, "fpgaOK = %d, isMCPU = %d, isEIRQ = %d, isCTXE = %d, xsvfRC = %d\n",
           fpgaOK, isMCPU, isEIRQ, isCTXE, xsvf.GetLastRC () );
    
    tracef( 2, "trace = %02x, state = %d, timer = %d, pRead = %d, pWrite = %d, \n",
           traceMask, state, timer, pRead - buf, pWrite - buf, traceMask );

    tracef( 2, "semaMutex = %d, semaFull = %d, semaEmpty = %d, semaSent = %d\n",
           semaMutex.GetCount (), semaFull.GetCount (), semaEmpty.GetCount (), 
           semaSent.GetCount ()
           );
    
    tracef( 2, "EIRQ: Count = %lu, Stuck = %lu\n", eirq_count, stuck_eirq_count );

    tracef( 2, "Active boards %d:", poll_active_cnt );
    for ( int i = 0; i < poll_active_cnt; i++ )
        tracef( 2, " %02x", poll_list[ i ] & 0x3F );
    tracef( 2, "\n" );
    
    tracef( 2, "Passive boards %d:", maxboardc - poll_active_cnt );
    for ( int i = poll_active_cnt; i < 8; i++ )
        tracef( 2, " %02x", poll_list[ i ] & 0x3F );
    tracef( 2, "\n" );
    }

//---------------------------------------------------------------------------------------
// Clearn reset of FPGA. Disable irqs etc.
//---------------------------------------------------------------------------------------
void XPI::ResetFPGA( void )
{
    AT91F_PIO_SetOutput( LED_PIO, LED_POWER ); // LED off

    Goto( DISABLED );

    if ( ! fpgaOK )
        return;

    // Disable FPGA interrupt
    //
    AT91F_AIC_DisableIt( AT91C_BASE_AIC, AT91C_ID_IRQ0 );

    // Reset all boards
    //
    if ( isMCPU )
    {
        for ( int i = 0; i < maxboardc; i++ )
        {
            FPGA_FC_Command( ( i << 2 ) | 0x00 );
            }
        }

    // Keep FPGA in reset mode
    //
    FPGA_SetReset ();

    // Reset important variables dependent on FPGA
    //
    fpgaOK = false;
    isMCPU = false;
    boardPos = 0xFF;

    // SYS message to host
    //
    sMsg.timeStamp = dTimerTick;
    sMsg.type      = XPI_IMSG_FPGA_STATUS;
    sMsg.subtype   = 0;
    sMsg.data[0]   = fpgaOK;
    sMsg.data[1]   = isMCPU;
    sMsg.data[2]   = boardPos;
    sMsg.data[3]   = xsvf.GetLastRC ();

    usbOut.Put( NULL, 0, 1000 );
    usbOut.Put( &sMsg, sizeof( XPI_IMSG_HEADER ) + 4, 1000 );
    }

//---------------------------------------------------------------------------------------
// Initialize FPGA XPI. Verify presence of FPGA, get board position and configure
// mode (passive/MCPU/SDEV). If cold start, then pulse FPGA reset, otherwise
// just take out FPGA from reset or do nothing if FPGA was already running.
//---------------------------------------------------------------------------------------
void XPI::InitializeFPGA( bool coldStart, bool forcePassive )
{
    isMCPU = false;
    fpgaOK = false;

    // Force cold start if requested warm start, but FPGA is under reset.
    //
    if ( ! coldStart )
        coldStart = FPGA_IsReset ();

    // Take FPGA out of Reset.
    //
    if ( coldStart )
        FPGA_PulseReset ();
    else
        FPGA_SetReset( false );

    // Get FPGA magic ID and board position (slot#)
    //
    taskENTER_CRITICAL ();
    FPGA_BegWrite ();
    FPGA_Write( XPI_W_PAGE_ADDR, 1 ); // Page 1
    FPGA_BegRead ();
    uint magic = FPGA_Read( XPI_R_P1_MAGIC_LSB ) 
               | ( FPGA_Read( XPI_R_P1_MAGIC_MSB ) << 8 );
    boardPos = FPGA_Read( XPI_R_P1_BOARD_POS );
    taskEXIT_CRITICAL ();

    // tracef( 2, "FPGA Magic %04X\n", magic );

    if ( magic != 0x11AA )
    {
        // FPGA is not found and we should left it under reset, just in case.
        //
        FPGA_SetReset ();
        }
    else
    {
        // Set passive mode if CPU-D_ card detected on backplane or user has 
        // forced passive mode
        //
        if ( forcePassive || ( boardPos & 0x30 ) != 0x30 ) 
        {
            // Are we CPU-D_?
            //
            taskENTER_CRITICAL ();
            FPGA_BegWrite ();
            FPGA_Write( XPI_W_PAGE_ADDR, 0 ); // Page 0
            isMCPU = ISSET( FPGA_Read( XPI_R_P0_GLB_STATUS ), XPI_GLB_MCPU );
            FPGA_BegRead ();
            taskEXIT_CRITICAL ();

            fpgaOK = true;
            }
        else // No CPU-D_ board found on backplane: try to take over bus
        {
            // Become CPU-D_ (Set MCPU = 1)
            // and get board position again
            //
            taskENTER_CRITICAL ();
            FPGA_BegWrite ();
            FPGA_Write( XPI_W_PAGE_ADDR, 0 ); // Page 0
            FPGA_Write( XPI_W_P0_GLB_CONTROL, XPI_GLB_MCPU );
            FPGA_Write( XPI_W_PAGE_ADDR, 1 ); // Page 1
            FPGA_BegRead ();
            boardPos = FPGA_Read( XPI_R_P1_BOARD_POS );
            taskEXIT_CRITICAL ();

            if ( ( boardPos & 0x30 ) != 0 )
            {
                taskENTER_CRITICAL ();
                tracef( 2, "FP: Something wrong! KA5..4 are not low!\n" );
                taskEXIT_CRITICAL ();
                fpgaOK = false;
                }
            else
            {
                isMCPU = true;
                fpgaOK = true;
                }
            }
        }

    if ( fpgaOK )
    {
        // Set green LED and clear yellow and red LED
        //
        taskENTER_CRITICAL ();
        FPGA_BegWrite ();
        FPGA_Write( XPI_W_PAGE_ADDR, 0 ); // Page 0
        FPGA_Write( XPI_W_P0_LED_SET, XPI_LED_G );
        FPGA_Write( XPI_W_P0_LED_CLEAR, XPI_LED_R | XPI_LED_Y );
        FPGA_BegRead ();
        taskEXIT_CRITICAL ();
        
        // Unmask Interrupts: 
        // 1) EIRQ, CRX, CTX, FC: always
        // 2) CTXE: only if isMCPU mode
        //
        taskENTER_CRITICAL ();
        FPGA_BegWrite ();
        FPGA_Write( XPI_W_PAGE_ADDR, 0 ); // Page 0
        FPGA_Write( XPI_W_P0_IRQ_ENABLE, 
            XPI_IRQ_EIRQ | XPI_IRQ_CRX | XPI_IRQ_CTX | XPI_IRQ_FC
            );
        if ( isMCPU )
            FPGA_Write( XPI_W_P0_IRQ_ENABLE, XPI_IRQ_CTXE );
        FPGA_BegRead ();
        taskEXIT_CRITICAL ();

        // Initialize SC transceiver state
        //
        Goto( IDLE );

        isEIRQ    = false;
        isCTXE    = false;
        crxLen    = 0;
        crxCkSum  = 0xFF;
        ctxLen    = 0;
        ctxCkSum  = 0xFF;

        ResetPollList ();

        // Enable FPGA interrupt
        //
        AT91F_AIC_EnableIt( AT91C_BASE_AIC, AT91C_ID_IRQ0 );
        }

    sMsg.timeStamp = dTimerTick;
    sMsg.type      = XPI_IMSG_FPGA_STATUS;
    sMsg.subtype   = 0;
    sMsg.data[0]   = fpgaOK;
    sMsg.data[1]   = isMCPU;
    sMsg.data[2]   = boardPos;
    sMsg.data[4]   = xsvf.GetLastRC ();

    usbOut.Put( NULL, 0, 1000 );
    usbOut.Put( &sMsg, sizeof( XPI_IMSG_HEADER ) + 4, 1000 );
    }

void XPI::On_FC( void )
{
    taskENTER_CRITICAL ();
    FPGA_BegWrite ();
    FPGA_Write( XPI_W_PAGE_ADDR, 0 ); // Page 0
    FPGA_BegRead ();
    uint fc_cmd = FPGA_Read( XPI_R_P0_FC_FDFA );
    uint fc_sense = FPGA_Read( XPI_R_P0_FC_SENSE );
    taskEXIT_CRITICAL ();
    
    sMsg.timeStamp = dTimerTick;
    sMsg.type      = XPI_IMSG_FC_EVENT;
    sMsg.subtype   = 0;
    sMsg.data[0]   = ( ( fc_cmd ) >> 2 ) & 0x3F;
    sMsg.data[1]   = ( fc_cmd ) & 0x03;
    sMsg.data[2]   = fc_sense;
    sMsg.data[3]   = state;

    usbOut.Put( &sMsg, sizeof( XPI_IMSG_HEADER ) + 4, 1 );
    }

void XPI::On_EIRQ( void )
{
    if ( isMCPU )
    {
        isEIRQ = true;

        // Disable EIRQ IRQ. EIRQ will be enabled and isEIRQ clered later, when
        // we start receiving CRX data. (Enabling EIRQ at that time will generate
        // new EIRQ if present.)
        //
        taskENTER_CRITICAL ();
        FPGA_BegWrite ();
        FPGA_Write( XPI_W_PAGE_ADDR, 0 ); // Page 0
        FPGA_Write( XPI_W_P0_IRQ_DISABLE, XPI_IRQ_EIRQ );
        FPGA_BegRead ();
        taskEXIT_CRITICAL ();
        }
    else
    {
        taskENTER_CRITICAL ();
        FPGA_BegWrite ();
        FPGA_Write( XPI_W_PAGE_ADDR, 0 ); // Page 0
        FPGA_BegRead ();
        isEIRQ = FPGA_Read( XPI_R_P0_SC_EIRQ ); // EIRQ FIFO
        taskEXIT_CRITICAL ();
        }

    if ( traceMask & DBG_EIRQ )
    {
        sMsg.timeStamp = dTimerTick;
        sMsg.type      = XPI_IMSG_TRACE_EIRQ;
        sMsg.subtype   = 0;
        sMsg.data[0]   = isEIRQ;
        sMsg.data[1]   = ctxLen;
        sMsg.data[2]   = state;
        usbOut.Put( &sMsg, sizeof( XPI_IMSG_HEADER ) + 3, 1 );
        }    
    }

void XPI::On_CTXE( void )
{
    isCTXE = true;

#if 0
    taskENTER_CRITICAL ();
    tracef( 2, "SC: On CTXE\n" );
    taskEXIT_CRITICAL ();
#endif

    // Disable CTXE IRQ. CTXE will be enabled and isCTXE clered later, when
    // we start sending CRX data. (Enabling CTXE at that time will generate
    // new CTXE and update isCTXE status when CTX FIFO becomes empty. Until
    // then, it is forbidden to send data. This means that we should put
    // as much data into FIFO as it gets (max 31 octet) from the beginning.)
    //
    taskENTER_CRITICAL ();
    FPGA_BegWrite ();
    FPGA_Write( XPI_W_PAGE_ADDR, 0 ); // Page 0
    FPGA_Write( XPI_W_P0_IRQ_DISABLE, XPI_IRQ_CTXE );
    FPGA_BegRead ();
    taskEXIT_CRITICAL ();

    if ( state == WAIT_SENT )
    {
        // Signal Transmitter() to put next message
        //
        ctx_status = 0;
        semaSent.Release( 1 );
        
        if ( ( pRead[0] & 0xC0 ) == 0xC0 )
            Goto( BLOCKED_SEND, INTER_SEND_DELAY );
        else
            Goto( IDLE );
        }
    else if ( state == WAIT_CTXE )
    {
        Goto( IDLE );
        }
    else if ( state == POLL_EIRQ )
    {
        if ( poll_cur < 0 )
             PollNextBoard ();
        }
    }

void XPI::ResetPollList( void )
{
    // Mark all boards passive
    //
    for ( int i = 0; i < maxboardc; i++ )
        poll_list[ i ] = i;

    poll_cur = -1;
    poll_active_cnt = 0;
    rearrange_poll_list = false;
    }

void XPI::PollNextBoard( void )
{
    // Poll next board
    //
    if ( poll_cur < 0 )
        poll_cur = 0;
    else
        ++poll_cur;

    if ( poll_cur >= maxboardc )
    {
        // The last board reached while polling EIRQ
        // (no more boards to poll, and there is no On_CRX () with data.
        // This means that some device board has stuck and that it should
        // be reset. 
        //
        taskENTER_CRITICAL ();
        tracef( 2, "SC: SEVERE ERROR: EIRQ stuck.\n" );
        taskEXIT_CRITICAL ();

        ++stuck_eirq_count;

        RearrangePollList ();

        // Enable EIRQ interrupt
        //
        taskENTER_CRITICAL ();
        FPGA_BegWrite ();
        FPGA_Write( XPI_W_PAGE_ADDR, 0 ); // Page 0
        FPGA_Write( XPI_W_P0_IRQ_ENABLE, XPI_IRQ_EIRQ );
        FPGA_BegRead ();
        taskEXIT_CRITICAL ();
        
        Goto( IDLE );

        // TODO: Implement protection from stuck EIRQ.
        // When number of eirq stuck consequtive failures reaches 3,
        // reset one by one device board using FC_Command and monitor EIRQ state until 
        // EIRQ is released. If all device boards are reset and EIRQ is still stuck,
        // then reposrt FPGA or BP bus failure and reset FPGA.

        return;
        }

    // Put next EIRQ board poll id into CTX and enable CTXE
    //
    isCTXE = false;
    taskENTER_CRITICAL ();
    FPGA_BegWrite ();
    FPGA_Write( XPI_W_PAGE_ADDR, 1 ); // Page 1
    FPGA_Write( XPI_W_P1_SC_CTX_DATA, poll_list[ poll_cur ] & 0x3F );
    FPGA_Write( XPI_W_P1_SC_CTX_INCFIFO, 0x00 );
    FPGA_BegRead ();
    taskEXIT_CRITICAL ();

    Goto( POLL_EIRQ, EIRQ_POLL_DELAY );

    // tracef( 2, "SC: %02x\n", poll_list[ poll_cur ] & 0x3F ); 
    }

void XPI::MarkBoardActive( bool active )
{
    if ( poll_cur < 0 )
        return;

    if ( active )
        poll_list[ poll_cur ] |= 0x80;
    else
        poll_list[ poll_cur ] &= ~0x80;

    if ( active && poll_cur >= poll_active_cnt ) // change: passive -> active
    {
        tracef( 2, "SC: Board %02x Active\n", poll_list[ poll_cur ] & 0x3F );
        rearrange_poll_list = true;
        }
    else if ( ! active && poll_cur < poll_active_cnt ) // change: active -> passive
    {
        tracef( 2, "SC: Board %02x Passive\n", poll_list[ poll_cur ] & 0x3F );
        rearrange_poll_list = true;
        }
    }

void XPI::RearrangePollList( void )
{
    if ( ! rearrange_poll_list )
        return;
    
    rearrange_poll_list = false;

    int N = poll_cur >= maxboardc ? maxboardc 
          : poll_cur < poll_active_cnt ? poll_active_cnt : poll_cur + 1;

    int new_active = 0; // Number of new active boards
    int new_passive = 0; // Number of new passive boards

    int j = maxboardc; // here goes 1) new active, 2) new passive, 3) old passive

    // Collect all new active boards from the old passive list
    //
    for ( int i = poll_active_cnt; i < N; i++ )
    {
        if ( poll_list[ i ] & 0x80 ) // new active in the old passive list?
        {
            poll_list[ j++ ] = poll_list[ i ];
            new_active++;
            }
        }
    
    // Collect all new passive boards from the old active list
    //
    for ( int i = 0; i < poll_active_cnt; i++ )
    {
        if ( ! ( poll_list[ i ] & 0x80 ) ) // new passive in the old active list
        {
            poll_list[ j++ ] = poll_list[ i ];  
            new_passive++;
            }
        }

    // Collect all old passive boards from the old passive list
    //
    for ( int i = poll_active_cnt; i < N; i++ )
    {
        if ( ! ( poll_list[ i ] & 0x80 ) ) // old passive in the old passive list
        {
            poll_list[ j++ ] = poll_list[ i ];  
            }
        }
    
    int k = 0; // here come sorted boards

    // Collect all old active boards from the old active list
    //
    for ( int i = 0; i < poll_active_cnt; i++ )
    {
        if ( poll_list[ i ] & 0x80 ) // old active in the old active list?
        {
            if ( k != i )
                poll_list[ k++ ] = poll_list[ i ];
            else
                k++;
            }
        }

    // Collect the rest of boards (new active, new passive, old passive)
    //
    for ( int i = maxboardc; i < j; i++ )
    {
        poll_list[ k++ ] = poll_list[ i ];
        }

    // assert( k == N );

    tracef( 2, "SC: Active boards %d+%d-%d\n", poll_active_cnt, 
            new_active, new_passive );
    
    poll_active_cnt += new_active;
    poll_active_cnt -= new_passive;
    }

void XPI::On_Timer( void )
{
    // Check elapsed time
    //
    ulong curTick = dTimerTick;
    long delta = curTick - lastTick;
    lastTick = curTick;

    if ( timer < 0 )
        return;

    timer -= delta;

    if ( timer > 0 )
        return;

    timer = -1;

    // tracef( 2, "SC: Timeout: State = %d\n", state );

    if ( state == POLL_EIRQ )
    {
        // No NAK() or MSG(): Mark board passive and continue polling next 
        // card position from the poll priority list.
        //
        MarkBoardActive( false );
        PollNextBoard ();
        }
    else if ( state == BLOCKED_SEND )
    {
        Goto( IDLE );
        }
    else if ( state == RECEIVE_CTX )
    {
        taskENTER_CRITICAL ();
        tracef( 2, "SC: Timeout in RECEIVE_CTX\n" );
        taskEXIT_CRITICAL ();
        // Abort frame
        ctxLen = 0;
        ctxCkSum = 0xFF;
        Goto( IDLE );
        // TODO Send NACK if isSDEV
        }
    else if ( state == RECEIVE_CRX )
    {
        taskENTER_CRITICAL ();
        tracef( 2, "SC: Timeout in RECEIVE_CRX\n" );
        taskEXIT_CRITICAL ();

        // Abort frame
        crxLen = 0;
        crxCkSum = 0xFF;
        Goto( IDLE );
        // TODO Send NACK if isMCPU
        
        // Enable EIRQ interrupt
        //
        taskENTER_CRITICAL ();
        FPGA_BegWrite ();
        FPGA_Write( XPI_W_PAGE_ADDR, 0 ); // Page 0
        FPGA_Write( XPI_W_P0_IRQ_ENABLE, XPI_IRQ_EIRQ );
        FPGA_BegRead ();
        taskEXIT_CRITICAL ();
        }
    else if ( state == WAIT_CTXE )
    {
        taskENTER_CRITICAL ();
        tracef( 2, "SC: SEVERE ERROR: FPGA Failed. Timeout in WAIT_CTXE.\n" );
        taskEXIT_CRITICAL ();

        ResetFPGA ();
        ctx_status = 0x78; // CTX completion status = ERROR
        semaSent.Release( 1 );
        }
    else if ( state == WAIT_SENT )
    {
        taskENTER_CRITICAL ();
        tracef( 2, "SC: SEVERE ERROR: FPGA Failed. Timeout in WAIT_SENT.\n", state );
        taskEXIT_CRITICAL ();

        ResetFPGA ();
        ctx_status = 0x79; // CTX completion status = ERROR
        semaSent.Release( 1 );
        }
    else if ( state == WAIT_ACK )
    {
#if 0        
        taskENTER_CRITICAL ();
        tracef( 2, "SC: Timeout in WAIT_ACK\n" );
        taskEXIT_CRITICAL ();
#endif        
        ctx_status = 3; // CTX completion status = Error, timeout
        semaSent.Release( 1 );
        Goto( IDLE );
        }
    else
    {
        taskENTER_CRITICAL ();
        tracef( 2, "SC: Timeout in FSM %d\n", state );
        taskEXIT_CRITICAL ();
        }
    }

void XPI::On_CTX( void )
{
    taskENTER_CRITICAL ();
    FPGA_BegWrite ();
    FPGA_Write( XPI_W_PAGE_ADDR, 0 ); // Page 0
    FPGA_BegRead ();
    uint octet = FPGA_Read( XPI_R_P0_SC_CTX );
    taskEXIT_CRITICAL ();

    if ( isMCPU )
    {
        if ( state == IDLE || state == WAIT_ACK  
          || state == WAIT_CTXE || state == WAIT_SENT || state == BLOCKED_SEND )
        {
            if ( ctxLen == 0 )
                sCTX.timeStamp = dTimerTick;

            sCTX.data[ ctxLen ] = octet;

            ctxCkSum ^= sCTX.data[ ctxLen ];
            ctxCkSum <<= 1;
            if ( ctxCkSum & 0x100 )
            {
                ctxCkSum &= 0xFF;
                ctxCkSum |= 0x01;
                }

            ++ctxLen;

            if ( sCTX.data[0] == 0xC0  // Poll octet
               || ( sCTX.data[0] & 0xC0 ) == 0x00  // NACK or poll octet
               )
            {
                if ( traceMask & DBG_EIRQ )
                {
                    sCTX.subtype = 0;
                    usbOut.Put( &sCTX, sizeof( XPI_IMSG_HEADER ) + ctxLen, 1000 );
                    }

                ctxLen = 0;
                ctxCkSum = 0xFF;
                }
            else if ( ( sCTX.data[0] & 0xC0 ) == 0x40 ) // ACK octet; len == 1 
            {
                if ( traceMask & DBG_ACK )
                {
                    sCTX.subtype = 0;
                    usbOut.Put( &sCTX, sizeof( XPI_IMSG_HEADER ) + ctxLen, 1000 );
                    }

                ctxLen = 0;
                ctxCkSum = 0xFF;
                }
            else if ( ctxLen >= 2 && ctxLen == 3 + ( sCTX.data[ 1 ] & 0x0F )) // Got Frame
            {
                sCTX.subtype = ctxCkSum ? 1 : 0;

                if ( traceMask & DBG_CTX )
                {
                    if ( traceMask & DBG_CTX_E0_PKT )
                    {
                        usbOut.Put( &sCTX, sizeof( XPI_IMSG_HEADER ) + ctxLen, 1000 );
                        }
                    else
                    {
                        if ( sCTX.subtype || ( sCTX.data[0] & 0xE0 ) != 0xE0 )
                            usbOut.Put( &sCTX, sizeof( XPI_IMSG_HEADER ) + ctxLen, 1000 );
                        }
                    }

                ctxLen = 0; 
                ctxCkSum = 0xFF;
                }
            else if ( ctxLen > 18 ) // Overflow
            {
                sCTX.subtype = 2;
                usbOut.Put( &sCTX, sizeof( XPI_IMSG_HEADER ) + ctxLen, 1000 );
                ctxLen = 0; 
                ctxCkSum = 0xFF;
                }
            }
        else if ( state == POLL_EIRQ )
        {
            if ( traceMask & DBG_EIRQ )
            {
                sMsg.timeStamp = dTimerTick;
                sMsg.type      = XPI_IMSG_TRACE_CTX;
                sMsg.subtype   = 0;
                sMsg.data[0]   = octet;
                usbOut.Put( &sMsg, sizeof( XPI_IMSG_HEADER ) + 1, 1 );
                }
            }
        else // Ignore octet
        {
            sMsg.timeStamp = dTimerTick;
            sMsg.type      = XPI_IMSG_TRACE_CTX;
            sMsg.subtype   = 3;
            sMsg.data[0]   = octet;
            sMsg.data[1]   = state;
            usbOut.Put( &sMsg, sizeof( XPI_IMSG_HEADER ) + 2, 1 );
            }
        return;
        }

    if ( state == IDLE || state == RECEIVE_CTX )
    {
        if ( state == IDLE )
        {
            sCTX.timeStamp = dTimerTick;
            ctxLen = 0; // Begin frame
            ctxCkSum = 0xFF;
            }

        Goto( RECEIVE_CTX, RECEIVE_TIMEOUT );

        sCTX.data[ ctxLen ] = octet;

        ctxCkSum ^= sCTX.data[ ctxLen ];
        ctxCkSum <<= 1;
        if ( ctxCkSum & 0x100 )
        {
            ctxCkSum &= 0xFF;
            ctxCkSum |= 0x01;
            }

        ++ctxLen;

        if ( isEIRQ && sCTX.data[0] == 0xC0 ) // Poll EIRQ; len == 1 
        {
            sCTX.subtype = 0;
            usbOut.Put( &sCTX, sizeof( XPI_IMSG_HEADER ) + ctxLen, 1000 );
            ctxLen = 0;
            Goto( POLL_EIRQ, 100 );
            }
        if ( ( sCTX.data[0] & 0xC0 ) == 0x40 ) // Acknowledge; len == 1
        {
            sCTX.subtype = 0;
            usbOut.Put( &sCTX, sizeof( XPI_IMSG_HEADER ) + ctxLen, 1000 );
            ctxLen = 0;
            Goto( IDLE );
            }
        else if ( ctxLen >= 2 && ctxLen == 3 + ( sCTX.data[ 1 ] & 0x0F )) // Got Frame
        {
            sCTX.subtype = ctxCkSum ? 1 : 0;
            usbOut.Put( &sCTX, sizeof( XPI_IMSG_HEADER ) + ctxLen, 1000 );
            ctxLen = 0; 
            ctxCkSum = 0xFF;
            Goto( IDLE );
            }
        }
    else if ( state == POLL_EIRQ  )
    {
        sMsg.timeStamp = dTimerTick;
        sMsg.type      = XPI_IMSG_TRACE_CTX;
        sMsg.subtype   = 0;
        sMsg.data[0]   = octet;
        usbOut.Put( &sMsg, sizeof( XPI_IMSG_HEADER ) + 1, 1 );
        }
    else // Ignore octet
    {
        sMsg.timeStamp = dTimerTick;
        sMsg.type      = XPI_IMSG_TRACE_CTX;
        sMsg.subtype   = 3;
        sMsg.data[0]   = octet;
        sMsg.data[1]   = state;
        usbOut.Put( &sMsg, sizeof( XPI_IMSG_HEADER ) + 2, 1 );
        }
    }

void XPI::On_CRX( void )
{
    taskENTER_CRITICAL ();
    FPGA_BegWrite ();
    FPGA_Write( XPI_W_PAGE_ADDR, 0 ); // Page 0
    FPGA_BegRead ();
    uint octet = FPGA_Read( XPI_R_P0_SC_CRX );
    taskEXIT_CRITICAL ();

    if ( isMCPU )
    {
        if ( state == WAIT_ACK )
        {
            uint ackid = 0x40 | ( pRead[0] & 0x3F );
            if ( octet == ackid )
                ctx_status = 0; // CTX completion status = OK
            else
                ctx_status = 0x80 | pRead[0]; // CTX completion status = Error, negative ack

            if ( traceMask & DBG_ACK )
            {
                sMsg.timeStamp = dTimerTick;
                sMsg.type      = XPI_IMSG_TRACE_CRX;
                sMsg.subtype   = ctx_status ? 4 : 0;
                sMsg.data[0]   = octet;
                usbOut.Put( &sMsg, sizeof( XPI_IMSG_HEADER ) + 1, 1 );
                }
                
            semaSent.Release( 1 );

            if ( isCTXE )
                Goto( IDLE ); //Goto( BLOCKED_SEND, INTER_SEND_DELAY );
            else
                Goto( WAIT_CTXE, CTXE_TIMEOUT );
            }
        else if ( state == POLL_EIRQ || state == RECEIVE_CRX )
        {
            if ( state == POLL_EIRQ )
            {
                if ( poll_cur < 0 )
                {
                    // We have received data after sending 0xC0.
                    // Action to this bevaviour is undefined. We choose to ignore
                    // octet and stay in the same state with the same timeout.
                    //
                    return;
                    }
                else if ( ( poll_list[ poll_cur ] & 0x3F ) != ( octet & 0x3F ) )
                {
                    // Garbage: Mark the board passive and continue next board
                    //
                    tracef( 2, "SC: Poll #%d: %02x, Respond %02x\n", 
                            poll_cur, poll_list[ poll_cur ] & 0x3F, octet ); 
                    MarkBoardActive( false );
                    PollNextBoard ();
                    return;
                    }
                else if ( ( octet & 0xC0 ) == 0x00 )
                {
                    // Got NACK(): mark board active and continue polling next board
                    //
                    MarkBoardActive( true );
                    PollNextBoard ();
                    return;
                    }
                else if ( ( octet & 0xC0 ) == 0x40 )
                {
                    // Got ACK(): ignore this and continue polling next board
                    // Stay in the same state.
                    return;
                    }
                
                // Got MSG(): mark board active and start receiving data
                //
                //
                MarkBoardActive( true );
                
                // Begin CRX frame
                //
                sCRX.timeStamp = dTimerTick;
                crxLen = 0;
                crxCkSum = 0xFF;

                // Enable EIRQ interrupt
                //
                taskENTER_CRITICAL ();
                FPGA_BegWrite ();
                FPGA_Write( XPI_W_PAGE_ADDR, 0 ); // Page 0
                FPGA_Write( XPI_W_P0_IRQ_ENABLE, XPI_IRQ_EIRQ );
                FPGA_BegRead ();
                taskEXIT_CRITICAL ();
                }

            // Collect CRX data
            
            sCRX.data[ crxLen ] = octet;

            crxCkSum ^= sCRX.data[ crxLen ];
            crxCkSum <<= 1;
            if ( crxCkSum & 0x100 )
            {
                crxCkSum &= 0xFF;
                crxCkSum |= 0x01;
                }

            ++crxLen;

            if ( crxLen < 2 || crxLen != 3 + ( sCRX.data[ 1 ] & 0x0F ) )
            {
                // Wait more data
                //
                Goto( RECEIVE_CRX, RECEIVE_TIMEOUT );
                }
            else 
            {
                // We have complete frame
                //
                sCRX.subtype = crxCkSum ? 1 : 0;
                usbOut.Put( &sCRX, sizeof( XPI_IMSG_HEADER ) + crxLen, 1000 );
                crxLen = 0; 
                crxCkSum = 0xFF;

                // TODO fix sending NAK. We should stay in POLL_EIRQ and wait
                // receiving same board again.
                //
                if ( ! isCTXE )
                {
                    taskENTER_CRITICAL ();
                    tracef( 2, "SC: Error: trying to ACK CRX but CTXE is not empty\n" );
                    taskEXIT_CRITICAL ();
                    }
                else
                {
                    // Put acknowledge to CTX
                    //
                    int ackid = 0x40 | ( sCRX.data[ 0 ] & 0x3F );

                    isCTXE = false;
                    taskENTER_CRITICAL ();
                    FPGA_BegWrite ();
                    FPGA_Write( XPI_W_PAGE_ADDR, 1 ); // Page 1
                    FPGA_Write( XPI_W_P1_SC_CTX_DATA, ackid );
                    FPGA_Write( XPI_W_P1_SC_CTX_INCFIFO, 0x00 );
                    FPGA_BegRead ();
                    taskEXIT_CRITICAL ();
                    }

                // Rearrange poll list if needed
                //
                RearrangePollList ();
                
                // Wait CTXE
                //
                Goto( WAIT_CTXE, CTXE_TIMEOUT );
                }
            }
        else // Unsolicited CRX octet
        {
            sMsg.timeStamp = dTimerTick;
            sMsg.type      = XPI_IMSG_TRACE_CRX;
            sMsg.subtype   = 3;
            sMsg.data[0]   = octet;
            sMsg.data[1]   = state;
            usbOut.Put( &sMsg, sizeof( XPI_IMSG_HEADER ) + 2, 1 );
            }
        return;
        }

    if ( state == IDLE || state == RECEIVE_CRX || state == POLL_EIRQ )
    {
        if ( state == IDLE || state == POLL_EIRQ )
        {
            crxLen = 0; // Begin frame
            crxCkSum = 0xFF;
            }

        Goto( RECEIVE_CRX, RECEIVE_TIMEOUT );

        sCRX.data[ crxLen ] = octet;

        crxCkSum ^= sCRX.data[ crxLen ];
        crxCkSum <<= 1;
        if ( crxCkSum & 0x100 )
        {
            crxCkSum &= 0xFF;
            crxCkSum |= 0x01;
            }

        ++crxLen;

        if ( ( sCRX.data[0] & 0xC0 ) == 0x40 ) // Acknowledge; len == 1
        {
            sCRX.subtype = 0;
            usbOut.Put( &sCRX, sizeof( XPI_IMSG_HEADER ) + crxLen, 1000 );
            crxLen = 0;
            Goto( IDLE );
            }
        else if ( crxLen >= 2 && crxLen == 3 + ( sCRX.data[ 1 ] & 0x0F ) ) // Got Frame
        {
            sCRX.subtype = crxCkSum ? 1 : 0;
            usbOut.Put( &sCRX, sizeof( XPI_IMSG_HEADER ) + crxLen, 1000 );
            crxLen = 0; 
            crxCkSum = 0xFF;
            Goto( IDLE );
            }
        }
    else // Ignore octet
    {
        sMsg.timeStamp = dTimerTick;
        sMsg.type      = XPI_IMSG_TRACE_CRX;
        sMsg.subtype   = 3;
        sMsg.data[0]   = octet;
        sMsg.data[1]   = state;
        usbOut.Put( &sMsg, sizeof( XPI_IMSG_HEADER ) + 2, 1 );
        }
    }

bool XPI::Put( void* data, uint len, portTickType xTicksToWait )
{
    // Wait enough space to fit 2-byte length + data
    //
    if ( ! semaFull.Wait( len + 2, xTicksToWait ) )
    {
        if ( xTicksToWait != 0 )
        {
            // Report buffer full
            //
            uchar* pReqId = (uchar*) data;
            sMsg.timeStamp = dTimerTick;
            sMsg.type      = XPI_IMSG_FLOW_CTRL;
            sMsg.subtype   = 0x77;
            sMsg.data[0]   = pReqId[0]; // request ID
            sMsg.data[1]   = pReqId[1];
            usbOut.Put( &sMsg, sizeof( XPI_IMSG_HEADER ) + 2, 1 );
            }
        return false;
        }

    // Lock pWrite mutex
    //
    LockWrite ();

    // Put 2-byte length (MSB first) and SC data into circular buffer.
    // Note that because SC message is limited in size and circular buffer
    // has special area at the end to fit overflow, SC message will be copied
    // as linear with pWrite going circular only at the end.
    //
    *pWrite++ = ( len >> 8 ) & 0xFF;
    *pWrite++ = len & 0xFF;

    // Put data into circular buffer.
    //
    memcpy( pWrite, data, len );
    
    // Advance pWrite (circular)
    //
    pWrite += len;
    if ( pWrite >= pMax )
        pWrite -= bufSize;

    // Notify XPI::Transmitter()
    //
    semaEmpty.Release( len + 2 );

    // Unlock pWrite mutex
    //
    UnlockWrite ();

    return true;
    }

void XPI:: Transmitter( void )
{
    // Wait 2-byte header
    //
    do ; while( ! semaEmpty.Wait( 2, 1000 ) );

    // Retrieve data length (MSG first) from packet header
    //
    uint len = *pRead++;
    len = ( len << 8 ) | *pRead++;

    // Wait the rest of data
    //
    do ; while( ! semaEmpty.Wait( len, 1000 ) );

    // Next two octets contains requestID (MSB first).
    //
    requestID = *pRead++;
    requestID = ( requestID << 8 ) | *pRead++;
    len -= 2;

#if 0
    taskENTER_CRITICAL ();
    tracef( 2, "SC: Transmitter %04x %d, L=%d\n", requestID, pRead-buf, len );
    taskEXIT_CRITICAL ();
#endif

    // Send data and wait completion, depending on mode.
    //
    if ( isMCPU )
    {
        for ( int retry = 0; retry < 2; retry++ )
        {
            // Set flags to mark start of transmission (the most important flag is
            // ctx_count, which is 0 if outbound transmission buffer is empty).
            //
            ctx_status = -1; // unspecified error
            pCtx       = pRead;
            ctx_count  = len;
    
            // Signal FPGA irq tasklet that there is something in CTX out buffer
            // (but only in if XPI is in IDLE state and able to send data).
            //
            extern xSEMA fpgaEvent;
            taskENTER_CRITICAL ();
            if ( state == IDLE )
                fpgaEvent.Release( 1 );
            taskEXIT_CRITICAL ();

            // Wait transmission to end
            //
            while( ! semaSent.Wait( 1, 1000 ) ) 
                tracef( 2, "SC: Wait %d\n", state );

            // ctx_status is 0 if transmission was successfull.
            //
            if ( ctx_status == 0 )
            {
                if ( ( pRead[0] & 0xE0 ) != 0xC0 )
                    break;

                // Convert: 0xC* -> 0xE*, i.e. turn on 5-th bit, which was
                // previously turned off.
                //
                pRead[0] |= ( 1 << 5 );

                // Recalculate checksum. Because we have changed 5-th bit and
                // because we had N times XOR and 8-bit ROL operations,
                // we have to inverse bit at position ( 5 + N ) % 8, where
                // N is the length of packet excluding checksum (i.e. len - 1 ).
                //
                uchar* pCkSum = pRead + len - 1;
                *pCkSum ^= ( 1 << ( ( 5 + len - 1 ) & 0x07 ) );

                // Re-send the packet
                //
                retry = 0;
                }
            else if ( retry < 2 )
            {
                taskENTER_CRITICAL ();
                tracef( 2, "SC: CTX Retrying %04x (%d)\n", requestID, retry + 1 );
                taskEXIT_CRITICAL ();
                }
            }

        if ( ctx_status != 0 )
        {
            sMsg.timeStamp = dTimerTick;
            sMsg.type      = XPI_IMSG_FLOW_CTRL;
            sMsg.subtype   = ctx_status;
            sMsg.data[0]   = ( requestID >> 8 ) & 0xFF;
            sMsg.data[1]   = requestID & 0xFF;
            usbOut.Put( &sMsg, sizeof( XPI_IMSG_HEADER ) + 2, 1 );
            }
        }

    // Advance (circular) pRead
    //
    pRead += len;
    if ( pRead >= pMax )
        pRead -= bufSize;

    // Release space back to circular buffer and unblock some USBXMTR::Put
    // waiting for more space. 
    //
    semaFull.Release( len + 4 );
    }

void XPI::StartTransmissionIfIdle( void )
{
    // Start transmission only if MCPU mode with CTX FIFO empty and in IDLE state
    //
    if ( ! ( isMCPU && state == IDLE && isCTXE ) )
        return;
    
    // Clear yellow LED
    //
    taskENTER_CRITICAL ();
    FPGA_BegWrite ();
    FPGA_Write( XPI_W_PAGE_ADDR, 0 ); // Page 0
    FPGA_Write( XPI_W_P0_LED_CLEAR, XPI_LED_Y );
    FPGA_BegRead ();
    taskEXIT_CRITICAL ();

    if ( isEIRQ )
    {
        isEIRQ = false;

        // Poll device boards to find out who originated EIRQ
        //
        poll_cur = -1; // -1: start with 0xC0, 0: start with the first board directly
        rearrange_poll_list = false;
        ++eirq_count;

        // Put 0xC0 and then first board id to CTX (start eirq polling)
        // and enable CTXE IRQ
        //
        isCTXE = false;
        taskENTER_CRITICAL ();
        FPGA_BegWrite ();
        FPGA_Write( XPI_W_PAGE_ADDR, 1 ); // Page 1
        FPGA_Write( XPI_W_P1_SC_CTX_DATA, 
            poll_cur < 0 ? 0xC0 : poll_list[ poll_cur ] & 0x3F
            );
        FPGA_Write( XPI_W_P1_SC_CTX_INCFIFO, 0x00 );
        FPGA_BegRead ();
        taskEXIT_CRITICAL ();

        Goto( POLL_EIRQ, EIRQ_POLL_DELAY );
        
        // Set yellow LED
        //
        taskENTER_CRITICAL ();
        FPGA_BegWrite ();
        FPGA_Write( XPI_W_PAGE_ADDR, 0 ); // Page 0
        FPGA_Write( XPI_W_P0_LED_SET, XPI_LED_Y );
        FPGA_BegRead ();
        taskEXIT_CRITICAL ();
        }
    else if ( ctx_count > 0 )
    {
#if 0
        taskENTER_CRITICAL ();
        tracef( 2, "SC: Start Xmis %04x %d, L=%d\n", requestID, pCtx - buf, ctx_count );
        taskEXIT_CRITICAL ();
#endif
        // Send current CTX data and enable CTXE.
        // Note that each write to CTX FIFO should be followed by 
        // CTXE IRQ enable, which increases FIFO write pointer.
        //
        isCTXE = false;
        taskENTER_CRITICAL ();
        FPGA_BegWrite ();
        FPGA_Write( XPI_W_PAGE_ADDR, 1 ); // Page 0
        for( uint i = 0; i < ctx_count; i++ )
        {
            FPGA_Write( XPI_W_P1_SC_CTX_DATA, *pCtx++ );
            FPGA_Write( XPI_W_P1_SC_CTX_INCFIFO, 0x00 );
            }
        FPGA_BegRead ();
        taskEXIT_CRITICAL ();
        
        // Make current CTXO buffer emtpy
        //
        ctx_count = 0;

        // Next state: Wait acknowledge or wait CTX empty event
        //
        if ( ( pRead[0] & 0xC0 ) == 0x80 ) // Should wait ACK
        {
            Goto( WAIT_ACK, RECEIVE_TIMEOUT );
            }
        else
        {
            Goto( WAIT_SENT, CTXE_TIMEOUT );
            }

        // Set yellow LED
        //
        taskENTER_CRITICAL ();
        FPGA_BegWrite ();
        FPGA_Write( XPI_W_PAGE_ADDR, 0 ); // Page 0
        FPGA_Write( XPI_W_P0_LED_SET, XPI_LED_Y );
        FPGA_BegRead ();
        taskEXIT_CRITICAL ();
        }
    }
