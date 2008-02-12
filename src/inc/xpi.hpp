#ifndef _XPI_HPP_INCLUDED
#define _XPI_HPP_INCLUDED

enum XPI_OMSG_TYPE
{
    XPI_OMSG_NULL        = 0x00,
    XPI_OMSG_LOOP        = 0x01,
    XPI_OMSG_QUERY       = 0x02,
    XPI_OMSG_LOG_CFG     = 0x03,
    XPI_OMSG_XSVF_START  = 0x04,
    XPI_OMSG_XSVF_DATA   = 0x05,
    XPI_OMSG_FPGA_INIT   = 0x06,
    XPI_OMSG_FC_CMD      = 0x07,
    XPI_OMSG_SC_DATA     = 0x08
    };

enum XPI_IMSG_TYPE
{
    XPI_IMSG_NULL        = 0x00,
    XPI_IMSG_LOOP        = 0x01,
    XPI_IMSG_LOG         = 0x02,
    XPI_IMSG_FPGA_STATUS = 0x03,
    XPI_IMSG_XSVF_END    = 0x04,
    XPI_IMSG_FC_EVENT    = 0x05,
    XPI_IMSG_SC_DATA     = 0x06,
    XPI_IMSG_FLOW_CTRL   = 0x07,
    XPI_IMSG_TRACE_CTX   = 0x08,
    XPI_IMSG_TRACE_CRX   = 0x09,
    XPI_IMSG_TRACE_EIRQ  = 0x0A,
    XPI_IMSG_TRACE_HSSC  = 0x0B
    };

enum
{
    XPI_MSG_MAGIC_MSB = '@',
    XPI_MSG_MAGIC_LSB = '!'
    };

struct XPI_IMSG_HEADER
{
    uchar magicMSB;   // magic: '@'
    uchar magicLSB;   // magic: '!'
    uchar type;       // XPI_IMSG_TYPE
    uchar subtype;    // type dependent qualifier
    ulong timeStamp;

    } ATTR_PACKED; // Total size 8 octets

struct XPI_OMSG_HEADER
{
    uchar magicMSB;   // magic: '@'
    uchar magicLSB;   // magic: '!'
    uchar type;       // XPI_OMSG_TYPE
    uchar subtype;    // type dependent qualifier

    } ATTR_PACKED; // Total size 4 octets

struct XPI_SHORT_MSG : public XPI_IMSG_HEADER
{
    uchar data[ 8 ];

    } ATTR_PACKED; // Total size 16 octets

struct XPI_LONG_MSG : public XPI_IMSG_HEADER
{
    uchar data[ 24 ]; // Max 18 octets of SC data

    } ATTR_PACKED; // Total size 32 octets
    

//---------------------------------------------------------------------------------------
// Backplane interface
//---------------------------------------------------------------------------------------
class XPI
{
    friend portTASK_FUNCTION( FPGA_IrqTasklet, pvParameters );

    enum 
    { 
        XPI_XMTR_BUF_SIZE  = 4096,
        EIRQ_POLL_DELAY    = 4,
        INTER_SEND_DELAY   = 2,
        RECEIVE_TIMEOUT    = 5,
        CTXE_TIMEOUT       = 10,
        MAX_BOARD_COUNT    = 64
        };

    enum
    {
        DBG_EIRQ        = 0x01,
        DBG_ACK         = 0x02,
        DBG_CTX_E0_PKT  = 0x04,
        DBG_CTX         = 0x08,
        DBG_CRX         = 0x10
        };

    xMUTEX semaMutex;
    xSEMA semaFull;
    xSEMA semaEmpty;
    xSEMA semaSent;

    // Circular buffer of MSGBUF packets.
    // MSGBUF length is max 32 octets, so the circular buffer last packet
    // crossing pMax is not split, rather it is kept complete over pMax boundary
    // so it could be accessed directly with linear functions like memcpy().
    //
    // MSGBUF Packet format:
    //    Header:
    //       uint8 len_MSB
    //       uint8 len_LSB
    //    Body:
    //       uint8 data[len]
    //
    uchar  buf[ XPI_XMTR_BUF_SIZE + 32 ];
    uint   bufSize;

    uchar* pRead;
    uchar* pWrite;
    uchar* pMax;

    volatile uint ctx_count;
    uchar* pCtx;
    int    ctx_status;

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
    
    enum STATE
    {
        DISABLED              = 0,
        IDLE                  = 1,
        WAIT_SENT             = 2,
        WAIT_CTXE             = 3,
        WAIT_ACK              = 4,
        BLOCKED_SEND          = 5,
        POLL_EIRQ             = 6,
        RECEIVE_CRX           = 7,
        RECEIVE_CTX           = 8
        };

    bool  fpgaOK;
    bool  isMCPU;
    int   boardPos;
    int   maxboardc;
    uint  traceMask;

    
    volatile STATE state;
    ulong lastTick;
    long  timer;
    bool  isEIRQ;
    bool  isCTXE;
    
    uchar poll_list[ 2 * MAX_BOARD_COUNT ];
    int poll_cur;
    int poll_active_cnt;
    bool rearrange_poll_list;
    ulong eirq_count;
    ulong stuck_eirq_count;

    XPI_SHORT_MSG sMsg;

    XPI_LONG_MSG sCTX;
    int ctxLen;
    int ctxCkSum;
    
    XPI_LONG_MSG sCRX;
    int crxLen;
    int crxCkSum;
    uint requestID;

    void Goto( STATE new_state, long timeout = -1 )
    {
#if 0        
        taskENTER_CRITICAL ();
        tracef( 2, "--: %d -> %d\n", state, new_state );
        taskEXIT_CRITICAL ();
#endif        
        state = new_state;
        timer = timeout < 0 ? -1 : timeout; // timeout in ms
        extern volatile ulong dTimerTick;
        lastTick = dTimerTick;
        }

    long GetNextTimeout( void ) const
    {
        return timer < 0 || timer > 20 ? 20 : timer;
        }

    void ResetPollList( void );
    void RearrangePollList( void );
    void MarkBoardActive( bool active );
    void PollNextBoard( void );

    void On_CTXE( void );
    void On_CTX( void );
    void On_CRX( void );
    void On_EIRQ( void );
    void On_FC( void );
    void On_Timer( void );

public:

    bool IsFpgaOK( void ) const
    {
        return fpgaOK;
        }

    XPI( void )
        : semaFull( XPI_XMTR_BUF_SIZE )
        , semaEmpty( 0 )
        , semaSent( 0 )
    {
        fpgaOK    = false;
        isMCPU    = false;

        boardPos  = 0xFF;
        maxboardc = MAX_BOARD_COUNT;

        traceMask = 0;

        state     = DISABLED;
        timer     = -1;
        lastTick  = 0;
        isEIRQ    = false;
        isCTXE    = false;

        eirq_count = 0;
        stuck_eirq_count = 0;
        ResetPollList ();

        sMsg.magicMSB = XPI_MSG_MAGIC_MSB;
        sMsg.magicLSB = XPI_MSG_MAGIC_LSB;

        sCTX.magicMSB = XPI_MSG_MAGIC_MSB;
        sCTX.magicLSB = XPI_MSG_MAGIC_LSB;
        sCTX.type     = XPI_IMSG_TRACE_CTX;
        crxLen        = 0;
        crxCkSum      = 0xFF;

        sCRX.magicMSB = XPI_MSG_MAGIC_MSB;
        sCRX.magicLSB = XPI_MSG_MAGIC_LSB;
        sCRX.type     = XPI_IMSG_TRACE_CRX;
        ctxLen        = 0;
        ctxCkSum      = 0xFF;

        bufSize   = XPI_XMTR_BUF_SIZE;
        pRead     = buf;
        pWrite    = buf;
        pMax      = buf + bufSize;

        pCtx      = NULL;
        ctx_count = 0;
        ctx_status = 0;
        requestID = 0;
        }

    void SetTraceMask( int mask )
    {
        traceMask = mask;
        }

    static portTASK_FUNCTION( MainTask, pvParameters );

    void DumpStatus( void );
    void ResetFPGA( void );
    void InitializeFPGA( bool coldStart, bool forcePassive );    

    bool Put( void* data, uint len, portTickType xTicksToWait );
    void Transmitter( void );
    void StartTransmissionIfIdle( void );
    };

//---------------------------------------------------------------------------------------
//      External references
//---------------------------------------------------------------------------------------
extern XPI xpi;

#endif // _XPI_HPP_INCLUDED
