//---------------------------------------------------------------------------------------
// Abstract:     This file contains the function xsvfExecute()
//               used to to interpret the XSVF commands.
// Usage:        Call xsvfExecute() to process XSVF data.
//               The XSVF data is retrieved by ReadXSVF().
//               ReadXSVF and portable JTAG low-level functions are declared,
//               and probably defined as inline, in "xsvfPort.h" include file.
// Debugging:    Define TRACE_XSVF <level> to compile with debugging features.
// History:      v5.01 - Original XSVF implementation based on XAPP058
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
// Debugging:
//
// #undef DEBUG_XSVF           // to disable verbose mode
// #define DEBUG_XSVF <level>  // to enable verbose mode, where <level> is one of:
//
//    0 - Success/Error reporting 
//    1 - XCOMMENT, Constructor/Destructor mesages
//    2 - XSVF command names
//    3 - XSVF command parameters and Tap State transitions
//    4 - TDI/TDO shift data (presented up to 16 bytes)
//    5 - TDI/TDO shift data (all data)
//
//---------------------------------------------------------------------------------------
#define TRACE_XSVF 0

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
#include "xsvfPort.hpp"

//---------------------------------------------------------------------------------------
// Debug Facility Macros
//---------------------------------------------------------------------------------------

#ifdef TRACE_XSVF

#define TRACE_DBG(level,...) \
    do { \
        if ( mTraceLevel >= level ) \
        { \
            xsvfPrintf(__VA_ARGS__); \
        } \
    } while(0)

#define TRACE_ARR(level,pOctetArray) \
    do { \
        if ( mTraceLevel >= level && (pOctetArray) ) \
        { \
            xsvfPrintf( (pOctetArray)->len ? "0x" : "<empty>" ); \
            int c = (pOctetArray)->len; \
            if ( mTraceLevel <= level && c > 16 ) \
                c = 16; \
            for ( int i = 0; i < c; ++i ) \
            { \
                xsvfPrintf( "%02X", int( (pOctetArray)->val[ i ] ) ); \
            } \
            if ( (pOctetArray)->len > c ) \
                xsvfPrintf( "..." ); \
        } \
    } while(0)

#else // ! TRACE_XSVF

#define TRACE_DBG(mTraceLevel,...)
#define TRACE_ARR(mTraceLevel,pOctetArray)

#endif // TRACE_XSVF

//---------------------------------------------------------------------------------------
// Type Declarations
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
// The OctetArray structure is a byte oriented type used to store an arbitrary 
// length binary value. As an example, the hex value 0x0e3d is represented as 
// a structure with:
//
//     len = 2  (since 2 bytes) 
//     val[0] = 0e
//     val[1] = 3d  
//     val[2..MAX_LEN] are undefined
//
//---------------------------------------------------------------------------------------
struct OctetArray
{
    enum 
    { 
        MAX_LEN = 128    // MAX_LEN = ceil( max( XSDRSIZE ) / 8 )
        //-------------------------------------------------------------------------------
        // This MAX_LEN defines the maximum length (in bytes) of predefined
        // buffers in which the XSVF player stores the current shift data.
        // This length must be greater than the longest shift length (in bytes)
        // in the XSVF files that will be processed. 
        // 
        // How to find the "shift length" in bits?
        // Look at the ASCII version of the XSVF (generated with the -a option
        // for the SVF2XSVF translator) and search for the XSDRSIZE command
        // with the biggest parameter. XSDRSIZE is equivalent to the SVF's
        // SDR length plus the lengths of applicable HDR and TDR commands.
        // Remember that the MAX_LEN is defined in bytes. Therefore, the
        // minimum MAX_LEN = ceil( max( XSDRSIZE ) / 8 );
        // 
        // The following MAX_LEN values have been tested and provide relatively
        // good margin for the corresponding devices:
        // 
        //  DEVICE        MAX_LEN   Resulting Max Shift Length (in bits)
        //  ---------     -------   ----------------------------------------------
        //  XC9500/XL/XV       32     256
        // 
        //  CoolRunner/II     256    2048  - actual max 1 device = 1035 bits
        // 
        //  FPGA              128    1024  - svf2xsvf -rlen 1024
        // 
        //  XC18V00/XCF00    1100    8800  - no blank check performed (default)
        //                                 - actual max 1 device = 8192 bits verify
        //                                 - max 1 device = 4096 bits program-only
        //                   2500   20000  - required for blank check
        //                                 - blank check max 1 device = 16384 bits
        //-------------------------------------------------------------------------------
        };

    //-----------------------------------------------------------------------------------
    //      Members
    //-----------------------------------------------------------------------------------

    int len;                           // number of chars in this value
    unsigned char val[ MAX_LEN + 1];   // bytes of data

    //-----------------------------------------------------------------------------------
    // Extract the long value from the octet array.
    // Returns the extracted value.
    //-----------------------------------------------------------------------------------
    long GetValue( void )
    {
        long lValue  = 0; // result to hold the accumulated result

        for ( int i = 0; i < len ; ++i )
        {
            lValue <<= 8;        // shift the accumulated result
            lValue |= val[ i];   // get the last byte first
            }

        return lValue;
        }

    //-----------------------------------------------------------------------------------
    // Compare two octet arrays with an optional mask.
    // Returns: bool; true if equal
    //-----------------------------------------------------------------------------------
    bool IsEqual
    ( 
        OctetArray* plvVal, // ptr to octet array #2. 
        OctetArray* plvMask // optional ptr to mask; NULL if no mask 
        )
    {
        // Start at least significant bit and compare bytes
        //
        for ( int i = len - 1; i >= 0; i-- )
        {
            int val1 = val[ i ];
            int val2 = plvVal->val[ i ];
            if ( plvMask )
            {
                int mask = plvMask->val[ i ];
                val1 &= mask;
                val2 &= mask;
                }
            if ( val1 != val2 )
                return false;
            }

        return true;
        }

    //-----------------------------------------------------------------------------------
    // Read from XSVF numBytes bytes of data into.
    // Method returns false if premature stream is encountered.
    //-----------------------------------------------------------------------------------
    bool ReadXSVF
    (
        int numBytes // the number of bytes to read.
        )
    {
        len = numBytes;
        for ( int i = 0; i < numBytes; i++ )
        {
            int ch = ::ReadXSVF ();
            if ( ch < 0 )
                return false;
            val[ i ] = ch;
            }
        return true;
        }

    //-----------------------------------------------------------------------------------
    // Add addendum to local octet array
    // Assumes *this and addendum octet arrays are of equal length.
    //-----------------------------------------------------------------------------------
    void Add
    ( 
         OctetArray& addendum
         )
    {
        int carry = 0;

        // Start at least significant bit and add bytes
        //
        for ( int i = len - 1; i >= 0; i-- )
        {
            // Add the two bytes plus carry from previous addition
            int sum = val[ i ] + addendum.val[ i ] + carry;

            // Set the i'th byte of the result
            val[ i ] = sum & 0xFF;

            // Set up carry for next byte
            carry = sum >> 8;
            }
        }
    };

//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
// Struct:       XSVF_Class
// Description:  This structure contains all of the data used during the
//               execution of the XSVF. Some data is persistent, predefined
//               information (e.g. lRunTestTime). The bulk of this struct's
//               size is due to the OctetArray structs
//               which contain buffers for the active shift data. The MAX_LEN
//               in OctetArray struct defines the size of these buffers.
//               These buffers must be large enough to store the longest
//               shift data in your XSVF file. For example:
//                   MAX_LEN >= ( longest_shift_data_in_bits / 8 )
//               Because the OctetArray struct dominates the space usage of this
//               struct, the rough size of this struct is:
//                   sizeof( XSVF_Class ) ~= MAX_LEN * 7 (number of OctetArrays)
//               xsvfInitialize() contains initialization code for the data
//               in this struct.
//---------------------------------------------------------------------------------------
class XSVF_Class
{
    //-----------------------------------------------------------------------------------
    // Debug Facility
    //-----------------------------------------------------------------------------------

#ifdef TRACE_XSVF
    int mTraceLevel;
    static const char* pzCommandName [];
    static const char* pzTapState [];
    static const char* pzErrorName [];
#endif // TRACE_XSVF

    //-----------------------------------------------------------------------------------
    // Private Enumerated Constants
    //-----------------------------------------------------------------------------------

    enum XSVF_COMMAND // encodings of xsvf instructions
    {
        XCOMPLETE      =  0,
        XTDOMASK       =  1,
        XSIR           =  2,
        XSDR           =  3,
        XRUNTEST       =  4,
        XCMDRESERVED5  =  5,
        XCMDRESERVED6  =  6,
        XREPEAT        =  7,
        XSDRSIZE       =  8,
        XSDRTDO        =  9,
        XSETSDRMASKS   =  10,
        XSDRINC        =  11,
        XSDRB          =  12,
        XSDRC          =  13,
        XSDRE          =  14,
        XSDRTDOB       =  15,
        XSDRTDOC       =  16,
        XSDRTDOE       =  17,
        XSTATE         =  18,
        XENDIR         =  19,
        XENDDR         =  20,
        XSIR2          =  21,
        XCOMMENT       =  22,
        XWAIT          =  23,
        XLASTCMD       =  24,
        };

    enum XSVF_XENDXR // XSVF Command Parameter Values
    {
        XENDXR_RUNTEST =  0,            // parameter for XENDIR/DR
        XENDXR_PAUSE   =  1             // parameter for XENDIR/DR
        };

    enum XSVF_TAPSTATE // TAP states
    {
        XTAPSTATE_RESET     = 0x00,
        XTAPSTATE_RUNTEST   = 0x01,     // a.k.a. IDLE
        XTAPSTATE_SELECTDR  = 0x02,
        XTAPSTATE_CAPTUREDR = 0x03,
        XTAPSTATE_SHIFTDR   = 0x04,
        XTAPSTATE_EXIT1DR   = 0x05,
        XTAPSTATE_PAUSEDR   = 0x06,
        XTAPSTATE_EXIT2DR   = 0x07,
        XTAPSTATE_UPDATEDR  = 0x08,
        XTAPSTATE_IRSTATES  = 0x09,     // All IR states begin here
        XTAPSTATE_SELECTIR  = 0x09,
        XTAPSTATE_CAPTUREIR = 0x0A,
        XTAPSTATE_SHIFTIR   = 0x0B,
        XTAPSTATE_EXIT1IR   = 0x0C,
        XTAPSTATE_PAUSEIR   = 0x0D,
        XTAPSTATE_EXIT2IR   = 0x0E,
        XTAPSTATE_UPDATEIR  = 0x0F,
        ///////////////////////////
        XTAPSTATE_FIRST     = 0x00,
        XTAPSTATE_LAST      = 0x0F
        };

    //-----------------------------------------------------------------------------------
    // Status Information
    //-----------------------------------------------------------------------------------

    bool            mParseOnly;         // Parse mode enable: do not shift
    bool            mComplete;          // false = running; true = complete
    int             mCommand;           // Current XSVF command byte
    long            mCommandCount;      // Number of commands processed
    int             mErrorCode;         // An error code. 0 = no error.

    // TAP state/sequencing information
    //
    XSVF_TAPSTATE   mTapState;          // Current TAP state
    XSVF_TAPSTATE   mTapStateEndIR;     // ENDIR TAP state (See SVF)
    XSVF_TAPSTATE   mTapStateEndDR;     // ENDDR TAP state (See SVF)

    // RUNTEST information
    //
    int             mMaxRepeat;         // Max repeat loops (for xc9500/xl)
    long            mRunTestTime;       // Pre-specified RUNTEST time (usec)

    // Shift Data Info and Buffers
    //
    long            mShiftLengthBits;   // Len. current shift data in bits
    int             mShiftLengthBytes;  // Len. current shift data in bytes

    // TDI/TDO Data Buffers
    //
    OctetArray      lvTdi;              // Current TDI shift data
    OctetArray      lvTdoExpected;      // Expected TDO shift data
    OctetArray      lvTdoCaptured;      // Captured TDO shift data
    OctetArray      lvTdoMask;          // TDO mask: 0=dontcare; 1=compare

    // XSDRINC Data Buffers
    //
    OctetArray      lvAddressMask;      // Address mask for XSDRINC
    OctetArray      lvDataMask;         // Data mask for XSDRINC
    OctetArray      lvNextData;         // Next data for XSDRINC

    //-----------------------------------------------------------------------------------
    // Utility Functions
    //-----------------------------------------------------------------------------------

    //-----------------------------------------------------------------------------------
    // Method:       GetAsNumBytes
    // Description:  Calculate the number of bytes the given number of bits
    //               consumes.
    // Returns:      the number of bytes to store the number of bits.
    //-----------------------------------------------------------------------------------
    static inline int GetAsNumBytes
    (
        long lNumBits // the number of bits
        )
    {
        return int( ( lNumBits + 7L ) / 8L );
        }

    //-----------------------------------------------------------------------------------
    // Method:       TmsTransition
    // Description:  Apply TMS and transition TAP controller by applying one TCK
    //               cycle.
    //-----------------------------------------------------------------------------------
    static inline void TmsTransition
    ( 
        int sTms // TMS value 
        )
    {
        SetTMS( sTms );
        SetTCK( 0 );
        SetTCK( 1 );
        }

    //-----------------------------------------------------------------------------------
    // Method:       GotoTapState
    // Description:  From the current TAP state, go to the named TAP state.
    //               A target state of RESET ALWAYS causes TMS reset sequence.
    //               All SVF standard stable state paths are supported.
    //               All state transitions are supported except for the following
    //               which cause an XSVF_ERROR_ILLEGALSTATE:
    //                   - Target==DREXIT2;  Start!=DRPAUSE
    //                   - Target==IREXIT2;  Start!=IRPAUSE
    //-----------------------------------------------------------------------------------
    void GotoTapState
    (
        XSVF_TAPSTATE ucTargetState // New target TAP state
        );

    //-----------------------------------------------------------------------------------
    // Method:       ShiftOnly
    // Description:  Assumes that starting TAP state is SHIFT-DR or SHIFT-IR.
    //               Shift the given TDI data into the JTAG scan chain.
    //               Optionally, save the TDO data shifted out of the scan chain.
    //               Last shift cycle is special:  capture last TDO, set last TDI,
    //               but does not pulse TCK. Caller must pulse TCK and optionally
    //               set TMS=1 to exit shift state.
    // Note:         Method is called only within Shift()
    //-----------------------------------------------------------------------------------
    void ShiftOnly
    (
        long        lNumBits,       // Number of bits to shift.
        bool        bTdoCaptured,   // Store captured TDO data
        bool        bExitShift      // 1= exit at end of shift; 0= stay in Shift-DR
        );

    //-----------------------------------------------------------------------------------
    // Method:       Shift
    // Description:  Goes to the given starting TAP state.
    //               Calls ShiftOnly to shift in the given TDI data and
    //               optionally capture the TDO data.
    //               Compares the TDO captured data against the TDO expected
    //               data.
    //               If a data mismatch occurs, then executes the exception
    //               handling loop upto mMaxRepeat times.
    //-----------------------------------------------------------------------------------
    void Shift
    ( 
        XSVF_TAPSTATE ucStartState,       // Starting state: Shift-DR or Shift-IR
        long          lNumBits,           // Number of bits to shift
        XSVF_TAPSTATE ucEndState,         // State in which to end the shift
        long          runTestTime,        // Amount of time to wait after the shift
        bool          captureTDO = false, // Capture TDO
        int           maxRepeat = 0       // Maximum number of retries on TDO mismatch
        );

    //-----------------------------------------------------------------------------------
    // Method:       BasicXSDRTDO
    // Description:  Get the XSDRTDO parameters and execute the XSDRTDO command.
    //               This is the common function for all XSDRTDO commands.
    //-----------------------------------------------------------------------------------
    void BasicXSDRTDO
    (
        XSVF_TAPSTATE ucEndState,         // State in which to end the shift
        long          runTestTime = 0,    // Maximum xc9500/xl retries
        bool          captureTDO = false, // Capture TDO
        int           maxRepeat = 0       // Maximum xc9500/xl retries
        )
    {
        if ( ! lvTdi.ReadXSVF( mShiftLengthBytes ) )
        {
            mErrorCode = XSVF_ERROR_ENDOFFILE;
            return;
            }

        if ( captureTDO )
        {
            if ( ! lvTdoExpected.ReadXSVF( mShiftLengthBytes ) )
            {
                mErrorCode = XSVF_ERROR_ENDOFFILE;
                return;
                }
            }

        Shift
        ( 
            XTAPSTATE_SHIFTDR, mShiftLengthBits, ucEndState, runTestTime, 
            captureTDO, maxRepeat 
            );
        }

    //-----------------------------------------------------------------------------------
    // Method:       DoSDRMasking
    //               Method is called within Do_XSDRINC()
    // Description:  Update the data value with the next XSDRINC data and address.
    // Example:      dataVal=0x01ff, nextData=0xab, addressMask=0x0100,
    //               dataMask=0x00ff, should set dataVal to 0x02ab
    //-----------------------------------------------------------------------------------
    void DoSDRMasking( void );

    //-----------------------------------------------------------------------------------
    // XSVF Command Handlers
    //-----------------------------------------------------------------------------------

    void Do_ILLEGALCMD( void );
    void Do_XCOMPLETE( void );
    void Do_XTDOMASK( void );
    void Do_XSIR( void );
    void Do_XSIR2( void );
    void Do_XSDR( void );
    void Do_XRUNTEST( void );
    void Do_XREPEAT( void );
    void Do_XSDRSIZE( void );
    void Do_XSDRTDO( void );
    void Do_XSETSDRMASKS( void );
    void Do_XSDRINC( void );
    void Do_XSDRBCE( void );
    void Do_XSDRTDOBCE( void );
    void Do_XSTATE( void );
    void Do_XENDXR( void );
    void Do_XCOMMENT( void );
    void Do_XWAIT( void );

public:

    //-----------------------------------------------------------------------------------
    // Public Methods and Constants
    //-----------------------------------------------------------------------------------

    enum XSVF_RC // Error codes for xsvfExecute.
    {
        XSVF_ERROR_NONE         = 0,
        XSVF_ERROR_UNKNOWN      = 1,
        XSVF_ERROR_TDOMISMATCH  = 2,
        XSVF_ERROR_MAXRETRIES   = 3,  // TDO mismatch after max retries
        XSVF_ERROR_ILLEGALCMD   = 4,
        XSVF_ERROR_ILLEGALSTATE = 5,
        XSVF_ERROR_DATAOVERFLOW = 6,  // Data > OctetArray::MAX_LEN buffer size
        XSVF_ERROR_ENDOFFILE    = 7,  // Premature end of file
        XSVF_ERROR_LAST         = 8
        };

    //-----------------------------------------------------------------------------------
    // Method:       XSVF_Class Object Constructor
    // Description:  Initialize the xsvf player.
    //-----------------------------------------------------------------------------------
    XSVF_Class( void );

    //-----------------------------------------------------------------------------------
    // Method:       XSVF_Class::_Class Destructor
    // Description:  Cleanup remnants of the xsvf player.
    //-----------------------------------------------------------------------------------
    // ~XSVF_Class( void );

    //-----------------------------------------------------------------------------------
    // Method:       XSVF_Class::Intialize
    // Description:  Initialize the TAPs.
    //               Call this before running the player to initialize the data.
    //-----------------------------------------------------------------------------------
    void Initialize( int traceLevel, bool parseOnly );

    //-----------------------------------------------------------------------------------
    // Method:       XSVF_Class::Run
    // Description:  Run the xsvf player commands in loop and return if completed
    //               or error encountered.
    //               First call Initialize, then call this function.
    // Returns:      XSVF_RC; 0 = success; otherwise error
    //-----------------------------------------------------------------------------------
    int Run( void );
    };

#ifdef TRACE_XSVF
    
const char* XSVF_Class::pzCommandName [] =
{
    "XCOMPLETE",
    "XTDOMASK",
    "XSIR",
    "XSDR",
    "XRUNTEST",
    "Reserved5",
    "Reserved6",
    "XREPEAT",
    "XSDRSIZE",
    "XSDRTDO",
    "XSETSDRMASKS",
    "XSDRINC",
    "XSDRB",
    "XSDRC",
    "XSDRE",
    "XSDRTDOB",
    "XSDRTDOC",
    "XSDRTDOE",
    "XSTATE",
    "XENDIR",
    "XENDDR",
    "XSIR2",
    "XCOMMENT",
    "XWAIT"
    };

const char* XSVF_Class::pzTapState [] =
{
    "RESET",        // 0x00
    "RUNTEST/IDLE", // 0x01
    "DRSELECT",     // 0x02
    "DRCAPTURE",    // 0x03
    "DRSHIFT",      // 0x04
    "DREXIT1",      // 0x05
    "DRPAUSE",      // 0x06
    "DREXIT2",      // 0x07
    "DRUPDATE",     // 0x08
    "IRSELECT",     // 0x09
    "IRCAPTURE",    // 0x0A
    "IRSHIFT",      // 0x0B
    "IREXIT1",      // 0x0C
    "IRPAUSE",      // 0x0D
    "IREXIT2",      // 0x0E
    "IRUPDATE"      // 0x0F
    };

const char* XSVF_Class::pzErrorName [] =
{
    "No error",
    "Unknown",
    "TDO mismatch",
    "TDO mismatch and exceeded max retries",
    "Unsupported XSVF command",
    "Illegal state specification",
    "Data overflows allocated MAX_LEN buffer size",
    "Premature end of XSVF data"
    };

#endif  // TRACE_XSVF

//---------------------------------------------------------------------------------------
// Method:       GotoTapState
// Description:  From the current TAP state, go to the named TAP state.
//               A target state of RESET ALWAYS causes TMS reset sequence.
//               All SVF standard stable state paths are supported.
//               All state transitions are supported except for the following
//               which cause an XSVF_ERROR_ILLEGALSTATE:
//                 - Target == DREXIT2;  Start != DRPAUSE
//                 - Target == IREXIT2;  Start != IRPAUSE
//---------------------------------------------------------------------------------------
void XSVF_Class::GotoTapState
(
    XSVF_TAPSTATE ucTargetState // New target TAP state
    )
{
    mErrorCode = XSVF_ERROR_NONE;

    if ( ucTargetState == XTAPSTATE_RESET )
    {
        // If RESET, always perform TMS reset sequence to reset/sync TAPs
        //
        TmsTransition( 1 );

        for ( int i = 0; i < 5; ++i )
        {
            SetTCK( 0 );
            SetTCK( 1 );
            }

        mTapState = XTAPSTATE_RESET;

        TRACE_DBG( 3, "      TMS Reset Sequence -> Test-Logic-Reset\n" );
        TRACE_DBG( 3, "      TAP State    = %s\n", pzTapState[ mTapState ] );
        }
    else if ( ucTargetState != mTapState 
         && ( ( ucTargetState == XTAPSTATE_EXIT2DR && mTapState != XTAPSTATE_PAUSEDR ) 
           || ( ucTargetState == XTAPSTATE_EXIT2IR && mTapState != XTAPSTATE_PAUSEIR ) ) 
        )
    {
        // Trap illegal TAP state path specification
        //
        mErrorCode = XSVF_ERROR_ILLEGALSTATE;
        }
    else
    {
        if ( ucTargetState == mTapState )
        {
            // Already in target state. Do nothing except when in DRPAUSE
            // or in IRPAUSE to comply with SVF standard
            //
            if ( ucTargetState == XTAPSTATE_PAUSEDR )
            {
                TmsTransition( 1 );
                mTapState = XTAPSTATE_EXIT2DR;
                TRACE_DBG( 3, "      TAP State    = %s\n", pzTapState[ mTapState ] );
                }
            else if ( ucTargetState == XTAPSTATE_PAUSEIR )
            {
                TmsTransition( 1 );
                mTapState = XTAPSTATE_EXIT2IR;
                TRACE_DBG( 3, "      TAP State    = %s\n", pzTapState[ mTapState ] );
                }
            }

        // Perform TAP state transitions to get to the target state
        //
        while ( ucTargetState != mTapState )
        {
            switch ( mTapState )
            {
                case XTAPSTATE_RESET:
                    TmsTransition( 0 );
                    mTapState = XTAPSTATE_RUNTEST;
                    break;

                case XTAPSTATE_RUNTEST:
                    TmsTransition( 1 );
                    mTapState = XTAPSTATE_SELECTDR;
                    break;

                case XTAPSTATE_SELECTDR:
                    if ( ucTargetState >= XTAPSTATE_IRSTATES )
                    {
                        TmsTransition( 1 );
                        mTapState = XTAPSTATE_SELECTIR;
                        }
                    else
                    {
                        TmsTransition( 0 );
                        mTapState = XTAPSTATE_CAPTUREDR;
                        }
                    break;

                case XTAPSTATE_CAPTUREDR:
                    if ( ucTargetState == XTAPSTATE_SHIFTDR )
                    {
                        TmsTransition( 0 );
                        mTapState = XTAPSTATE_SHIFTDR;
                        }
                    else
                    {
                        TmsTransition( 1 );
                        mTapState = XTAPSTATE_EXIT1DR;
                        }
                    break;

                case XTAPSTATE_SHIFTDR:
                    TmsTransition( 1 );
                    mTapState = XTAPSTATE_EXIT1DR;
                    break;

                case XTAPSTATE_EXIT1DR:
                    if ( ucTargetState == XTAPSTATE_PAUSEDR )
                    {
                        TmsTransition( 0 );
                        mTapState = XTAPSTATE_PAUSEDR;
                        }
                    else
                    {
                        TmsTransition( 1 );
                        mTapState = XTAPSTATE_UPDATEDR;
                        }
                    break;

                case XTAPSTATE_PAUSEDR:
                    TmsTransition( 1 );
                    mTapState = XTAPSTATE_EXIT2DR;
                    break;

                case XTAPSTATE_EXIT2DR:
                    if ( ucTargetState == XTAPSTATE_SHIFTDR )
                    {
                        TmsTransition( 0 );
                        mTapState = XTAPSTATE_SHIFTDR;
                        }
                    else
                    {
                        TmsTransition( 1 );
                        mTapState = XTAPSTATE_UPDATEDR;
                        }
                    break;

                case XTAPSTATE_UPDATEDR:
                    if ( ucTargetState == XTAPSTATE_RUNTEST )
                    {
                        TmsTransition( 0 );
                        mTapState = XTAPSTATE_RUNTEST;
                        }
                    else
                    {
                        TmsTransition( 1 );
                        mTapState = XTAPSTATE_SELECTDR;
                        }
                    break;

                case XTAPSTATE_SELECTIR:
                    TmsTransition( 0 );
                    mTapState = XTAPSTATE_CAPTUREIR;
                    break;

                case XTAPSTATE_CAPTUREIR:
                    if ( ucTargetState == XTAPSTATE_SHIFTIR )
                    {
                        TmsTransition( 0 );
                        mTapState = XTAPSTATE_SHIFTIR;
                        }
                    else
                    {
                        TmsTransition( 1 );
                        mTapState = XTAPSTATE_EXIT1IR;
                        }
                    break;

                case XTAPSTATE_SHIFTIR:
                    TmsTransition( 1 );
                    mTapState = XTAPSTATE_EXIT1IR;
                    break;

                case XTAPSTATE_EXIT1IR:
                    if ( ucTargetState == XTAPSTATE_PAUSEIR )
                    {
                        TmsTransition( 0 );
                        mTapState = XTAPSTATE_PAUSEIR;
                        }
                    else
                    {
                        TmsTransition( 1 );
                        mTapState = XTAPSTATE_UPDATEIR;
                        }
                    break;

                case XTAPSTATE_PAUSEIR:
                    TmsTransition( 1 );
                    mTapState = XTAPSTATE_EXIT2IR;
                    break;

                case XTAPSTATE_EXIT2IR:
                    if ( ucTargetState == XTAPSTATE_SHIFTIR )
                    {
                        TmsTransition( 0 );
                        mTapState = XTAPSTATE_SHIFTIR;
                        }
                    else
                    {
                        TmsTransition( 1 );
                        mTapState = XTAPSTATE_UPDATEIR;
                        }
                    break;

                case XTAPSTATE_UPDATEIR:
                    if ( ucTargetState == XTAPSTATE_RUNTEST )
                    {
                        TmsTransition( 0 );
                        mTapState = XTAPSTATE_RUNTEST;
                        }
                    else
                    {
                        TmsTransition( 1 );
                        mTapState = XTAPSTATE_SELECTDR;
                        }
                    break;

                default:
                    mErrorCode = XSVF_ERROR_ILLEGALSTATE;
                    mTapState = ucTargetState;    // Exit while loop
                    break;
                }

            TRACE_DBG( 3, "      TAP State    = %s\n", pzTapState[ mTapState ] );
            }
        }
    }

//---------------------------------------------------------------------------------------
// Method:       XSVF_Class::ShiftOnly
//               Method is called only within Shift()
// Description:  Assumes that starting TAP state is SHIFT-DR or SHIFT-IR.
//               Shift the given TDI data into the JTAG scan chain.
//               Optionally, save the TDO data shifted out of the scan chain.
//               Last shift cycle is special:  capture last TDO, set last TDI,
//               but does not pulse TCK. Caller must pulse TCK and optionally
//               set TMS=1 to exit shift state.
//---------------------------------------------------------------------------------------
void XSVF_Class::ShiftOnly
(
    long        lNumBits,       // Number of bits to shift.
    bool        bTdoCaptured,   // Store captured TDO data
    bool        bExitShift      // 1= exit at end of shift; 0= stay in Shift-DR
    )
{
    // assert( ( ( lNumBits + 7 ) / 8 ) == lvTdi.len );

    // Initialize TDO storage len == TDI len
    //
    unsigned char* pucTdo = 0;
    if ( bTdoCaptured )
    {
        lvTdoCaptured.len = lvTdi.len;
        pucTdo            = lvTdoCaptured.val + lvTdi.len;
        }

    // Shift LSB first: val[N-1] == LSB, val[0] == MSB
    //
    unsigned char* pucTdi = lvTdi.val + lvTdi.len;
    while ( lNumBits )
    {
        // Process on a byte-basis
        //
        int ucTdiByte = *(--pucTdi);
        int ucTdoByte = 0;
        for ( int i = 0; lNumBits && i < 8; ++i )
        {
            --lNumBits;
            if ( bExitShift && ! lNumBits )
            {
                // Exit Shift-DR state
                //
                SetTMS( 1 );
                }

            // Set the new TDI value
            //
            SetTDI( ucTdiByte & 1 );
            ucTdiByte >>= 1;

            // Set TCK low
            //
            SetTCK( 0 );

            // Save the TDO value
            //
            if ( pucTdo )
            {
                ucTdoByte |= ( GetTDO () << i );
                }

            // Set TCK high
            //
            SetTCK( 1 );
            }

        // Save the TDO byte value
        //
        if ( pucTdo )
        {
            *(--pucTdo) = ucTdoByte;
            }
        }
    }

//---------------------------------------------------------------------------------------
// Method:       XSVF_Class::Shift
// Description:  Goes to the given starting TAP state.
//               Calls ShiftOnly to shift in the given TDI data and
//               optionally capture the TDO data.
//               Compares the TDO captured data against the TDO expected
//               data.
//               If a data mismatch occurs, then executes the exception
//               handling loop upto mMaxRepeat times.
// Notes:        XC9500XL-only Optimization:
//               Skip the uSleep() if plvTdoMask->val[0:plvTdoMask->len-1]
//               is NOT all zeros and sMatch==1.
//---------------------------------------------------------------------------------------
void XSVF_Class::Shift
( 
    XSVF_TAPSTATE ucStartState,   // Starting shift state: Shift-DR or Shift-IR
    long          lNumBits,       // number of bits to shift
    XSVF_TAPSTATE ucEndState,     // state in which to end the shift
    long          runTestTime,    // amount of time to wait after the shift
    bool          captureTDO,     // capture TDO
    int           maxRepeat       // Maximum number of retries on TDO mismatch
    )
{
    mErrorCode = XSVF_ERROR_NONE;

    if ( ! lNumBits )
    {
        // Compatibility with XSVF2.00
        // XSDR 0 means: "no shift, but wait in run test"
        //
        if ( runTestTime )
        {
            // Wait for prespecified XRUNTEST time
            //
            GotoTapState( XTAPSTATE_RUNTEST );
            TRACE_DBG( 3, "      Wait         = %ld usec\n", runTestTime );
            if ( ! mParseOnly )
            {
                uSleep( runTestTime );
                }
            }

        return;
        }

    TRACE_DBG( 3, "      Shift Length = %ld\n", lNumBits );
    TRACE_DBG( 4, "      TDI          = " );
    TRACE_ARR( 4, &lvTdi );
    TRACE_DBG( 4, "\n");

    if ( captureTDO )
    {
        TRACE_DBG( 4, "      TDO Expected = " );
        TRACE_ARR( 4, &lvTdoExpected );
        TRACE_DBG( 4, "\n" );
        }

    bool bMismatch = false;

    int retry = 0;
    do
    {
        bool bExitShift = ucStartState != ucEndState;

        // Goto Shift-DR or Shift-IR
        //
        GotoTapState( ucStartState );

        // Shift TDI and optionally capture TDO
        //
        if ( ! mParseOnly )
        {
            ShiftOnly( lNumBits, captureTDO, bExitShift );

            // Compare TDO data to expected TDO data
            //
            if ( captureTDO )
            {
                bMismatch = ! lvTdoCaptured.IsEqual( &lvTdoExpected, &lvTdoMask );
                }
            }

        if ( bExitShift )
        {
            // Update TAP state:  Shift->Exit
            //
            switch( mTapState )
            {
                case XTAPSTATE_SHIFTDR: 
                    mTapState = XTAPSTATE_EXIT1DR; 
                    break;
                case XTAPSTATE_SHIFTIR: 
                    mTapState = XTAPSTATE_EXIT1IR; 
                    break;
                default:
                    mErrorCode = XSVF_ERROR_ILLEGALSTATE;
                    return;
                }

            TRACE_DBG( 3, "      TAP State    = %s\n", pzTapState[ mTapState ] );

            if ( bMismatch && runTestTime && retry < maxRepeat )
            {
                TRACE_DBG( 4, "      TDO Mismatch\n" );
                TRACE_DBG( 4, "      TDO Captured = " );
                TRACE_ARR( 4, &lvTdoCaptured );
                TRACE_DBG( 4, "\n" );
                TRACE_DBG( 4, "      TDO Expected = " );
                TRACE_ARR( 4, &lvTdoExpected );
                TRACE_DBG( 4, "\n" );
                TRACE_DBG( 4, "      TDO Mask     = " );
                TRACE_ARR( 4, &lvTdoMask );
                TRACE_DBG( 4, "\n" );

                // Do exception handling retry - ShiftDR only
                //
                GotoTapState( XTAPSTATE_PAUSEDR );

                // Shift 1 extra bit
                //
                GotoTapState( XTAPSTATE_SHIFTDR );

                // Increment RUNTEST time by an additional 25%
                //
                runTestTime += ( runTestTime >> 2 );
                }
            else
            {
                // Do normal exit from Shift-XR
                //
                GotoTapState( ucEndState );
                }

            if ( runTestTime )
            {
                // Wait for prespecified XRUNTEST time
                //
                GotoTapState( XTAPSTATE_RUNTEST );
                TRACE_DBG( 3, "      Wait         = %ld usec\n", runTestTime );
                if ( ! mParseOnly )
                {
                    uSleep( runTestTime );
                    }
                }

            if ( bMismatch && retry < maxRepeat )
            {
                TRACE_DBG( 3, "----> RETRY        # %d\n", retry + 1 );
                }
            }
        } while( bMismatch && retry++ < maxRepeat );

    if ( bMismatch )
    {
        if ( maxRepeat && retry > maxRepeat )
            mErrorCode = XSVF_ERROR_MAXRETRIES;
        else
            mErrorCode = XSVF_ERROR_TDOMISMATCH;
        }
    }

//---------------------------------------------------------------------------------------
// Method:       XSVF_Class::DoSDRMasking
//               Method is called only within Do_XSDRINC()
// Description:  Update the data value with the next XSDRINC data and address.
// Example:      dataVal=0x01ff, nextData=0xab, addressMask=0x0100,
//               dataMask=0x00ff, should set dataVal to 0x02ab
//---------------------------------------------------------------------------------------
void XSVF_Class::DoSDRMasking( void )
{
    // Add the address Mask to dataVal and return as a new dataVal
    //
    lvTdi.Add( lvAddressMask );

    int ucNextData = 0;
    int ucNextMask = 0;
    int sNextData  = lvNextData.len;

    for ( int i = lvDataMask.len - 1; i >= 0; --i )
    {
        // Go through data mask in reverse order looking for mask (1) bits
        //
        int ucDataMask = lvDataMask.val[ i ];
        if ( ucDataMask )
        {
            // Retrieve the corresponding TDI byte value
            //
            int ucTdi = lvTdi.val[ i ];

            // For each bit in the data mask byte, look for 1's
            //
            int ucTdiMask = 1;
            while ( ucDataMask )
            {
                if ( ucDataMask & 1 )
                {
                    if ( ! ucNextMask )
                    {
                        // Get the next data byte
                        //
                        ucNextData = lvNextData.val[ --sNextData ];
                        ucNextMask = 1;
                        }

                    // Set or clear the data bit according to the next data
                    //
                    if ( ucNextData & ucNextMask )
                    {
                        ucTdi |= ucTdiMask; // Set bit
                        }
                    else
                    {
                        ucTdi &= ~ucTdiMask; // Clear bit
                        }

                    // Update the next data
                    //
                    ucNextMask  <<= 1;
                    }

                ucTdiMask  <<= 1;
                ucDataMask >>= 1;
                }

            // Update the TDI value
            //
            lvTdi.val[ i ] = ucTdi;
            }
        }
    }

//---------------------------------------------------------------------------------------
// XSVF_Class XSVF Command Functions
// These functions update mErrorCode on an error.
// Otherwise, the error code is left alone.
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
// Method:       XSVF_Class::Do_ILLEGALCMD
// Description:  Function place holder for illegal/unsupported commands.
//---------------------------------------------------------------------------------------
void XSVF_Class::Do_ILLEGALCMD( void )
{
    mErrorCode = XSVF_ERROR_ILLEGALCMD;
    }

//---------------------------------------------------------------------------------------
// Method:       XSVF_Class::Do_XCOMPLETE
// Description:  XCOMPLETE (no parameters)
//               Update complete status for XSVF player.
//---------------------------------------------------------------------------------------
void XSVF_Class::Do_XCOMPLETE( void )
{
    mComplete = true;
    mErrorCode = XSVF_ERROR_NONE;
    }

//---------------------------------------------------------------------------------------
// Method:       XSVF_Class::Do_XTDOMASK
// Description:  XTDOMASK <OctetArray.TdoMask[XSDRSIZE]>
//               Prespecify the TDO compare mask.
//---------------------------------------------------------------------------------------
void XSVF_Class::Do_XTDOMASK( void )
{
    if ( ! lvTdoMask.ReadXSVF( mShiftLengthBytes ) )
    {
        mErrorCode = XSVF_ERROR_ENDOFFILE;
        return;
        }

    TRACE_DBG( 4, "      TDO Mask     = ");
    TRACE_ARR( 4, &lvTdoMask );
    TRACE_DBG( 4, "\n");

    mErrorCode = XSVF_ERROR_NONE;
    }

//---------------------------------------------------------------------------------------
// Method:       XSVF_Class::Do_XSIR
// Description:  XSIR <(byte)shiftlen> <OctetArray.TDI[shiftlen]>
//               Get the instruction and shift the instruction into the TAP.
//               If prespecified XRUNTEST!=0, goto RUNTEST and wait after
//               the shift for XRUNTEST usec.
//---------------------------------------------------------------------------------------
void XSVF_Class::Do_XSIR( void )
{
    // Get the shift length and store
    //
    int ucShiftIrBits = ReadXSVF ();
    if ( ucShiftIrBits < 0 )
    {
        mErrorCode = XSVF_ERROR_ENDOFFILE;
        return;
        }
        
    int sShiftIrBytes = GetAsNumBytes( ucShiftIrBits );
    if ( sShiftIrBytes < 0 )
    {
        mErrorCode = XSVF_ERROR_ENDOFFILE;
        return;
        }
        
    TRACE_DBG( 3, "      IR Length    = %d\n", ucShiftIrBits );

    if ( sShiftIrBytes > OctetArray::MAX_LEN )
    {
        mErrorCode = XSVF_ERROR_DATAOVERFLOW;
        return;
        }

    // Get and store instruction to shift in
    //
    if ( ! lvTdi.ReadXSVF( GetAsNumBytes( ucShiftIrBits ) ) )
    {
        mErrorCode = XSVF_ERROR_ENDOFFILE;
        return;
        }

    // Shift the data
    //
    Shift( XTAPSTATE_SHIFTIR, ucShiftIrBits, mTapStateEndIR, mRunTestTime );
    }

//---------------------------------------------------------------------------------------
// Method:       XSVF_Class::Do_XSIR2
// Description:  XSIR <(2-byte)shiftlen> <OctetArray.TDI[shiftlen]>
//               Get the instruction and shift the instruction into the TAP.
//               If prespecified XRUNTEST!=0, goto RUNTEST and wait after
//               the shift for XRUNTEST usec.
//---------------------------------------------------------------------------------------
void XSVF_Class::Do_XSIR2( void )
{
    // Get the shift length and store
    //
    if ( ! lvTdi.ReadXSVF( 2 ) )
    {
        mErrorCode = XSVF_ERROR_ENDOFFILE;
        return;
        }
        
    long lShiftIrBits  = lvTdi.GetValue ();
    int sShiftIrBytes  = GetAsNumBytes( lShiftIrBits );

    TRACE_DBG( 3, "      IR Length    = %ld\n", lShiftIrBits);

    if ( sShiftIrBytes > OctetArray::MAX_LEN )
    {
        mErrorCode  = XSVF_ERROR_DATAOVERFLOW;
        }
    else
    {
        // Get and store instruction to shift in
        //
        if ( ! lvTdi.ReadXSVF( GetAsNumBytes( lShiftIrBits ) ) )
        {
            mErrorCode = XSVF_ERROR_ENDOFFILE;
            return;
            }

        // Shift the data
        //
        Shift( XTAPSTATE_SHIFTIR, lShiftIrBits, mTapStateEndIR, mRunTestTime );
        }
    }

//---------------------------------------------------------------------------------------
// Method:       XSVF_Class::Do_XSDR
// Description:  XSDR <OctetArray.TDI[XSDRSIZE]>
//               Shift the given TDI data into the JTAG scan chain.
//               Compare the captured TDO with the expected TDO from the
//               previous XSDRTDO command using the previously specified
//               XTDOMASK.
//---------------------------------------------------------------------------------------
void XSVF_Class::Do_XSDR( void )
{
    if ( ! lvTdi.ReadXSVF( mShiftLengthBytes ) )
    {
        mErrorCode = XSVF_ERROR_ENDOFFILE;
        return;
        }

    // Use TDOExpected from last XSDRTDO instruction
    //
    Shift
    ( 
        XTAPSTATE_SHIFTDR, mShiftLengthBits, mTapStateEndDR, mRunTestTime,
        /*captureTDO*/true, mMaxRepeat 
        );
    }

//---------------------------------------------------------------------------------------
// Method:       XSVF_Class::Do_XRUNTEST
// Description:  XRUNTEST <uint32>
//               Prespecify the XRUNTEST wait time for shift operations.
//---------------------------------------------------------------------------------------
void XSVF_Class::Do_XRUNTEST( void )
{
    if ( ! lvTdi.ReadXSVF( 4 ) )
    {
        mErrorCode = XSVF_ERROR_ENDOFFILE;
        return;
        }

    mRunTestTime = lvTdi.GetValue ();

    TRACE_DBG( 3, "      Test Time    = %ld usec\n", mRunTestTime );

    mErrorCode = XSVF_ERROR_NONE;
    }

//---------------------------------------------------------------------------------------
// Method:       XSVF_Class::Do_XREPEAT
// Description:  XREPEAT <byte>
//               Prespecify the maximum number of XC9500/XL retries.
//---------------------------------------------------------------------------------------
void XSVF_Class::Do_XREPEAT( void )
{
    mMaxRepeat = ReadXSVF ();
    if ( mMaxRepeat < 0 )
    {
        mErrorCode = XSVF_ERROR_ENDOFFILE;
        return;
        }

    TRACE_DBG( 3, "      Max Repeat   = %d\n", mMaxRepeat );

    mErrorCode = XSVF_ERROR_NONE;
    }

//---------------------------------------------------------------------------------------
// Method:       XSVF_Class::Do_XSDRSIZE
// Description:  XSDRSIZE <uint32>
//               Prespecify the XRUNTEST wait time for shift operations.
//---------------------------------------------------------------------------------------
void XSVF_Class::Do_XSDRSIZE( void )
{
    mErrorCode  = XSVF_ERROR_NONE;

    if ( ! lvTdi.ReadXSVF( 4 ) )
    {
        mErrorCode = XSVF_ERROR_ENDOFFILE;
        return;
        }
        
    mShiftLengthBits  = lvTdi.GetValue ();
    mShiftLengthBytes = GetAsNumBytes( mShiftLengthBits );

    TRACE_DBG( 3, "      DR Size      = %ld\n", mShiftLengthBits );

    if ( mShiftLengthBytes > OctetArray::MAX_LEN )
    {
        mErrorCode = XSVF_ERROR_DATAOVERFLOW;
        }
    }

//---------------------------------------------------------------------------------------
// Method:       XSVF_Class::Do_XSDRTDO
// Description:  XSDRTDO <OctetArray.TDI[XSDRSIZE]> <OctetArray.TDO[XSDRSIZE]>
//               Get the TDI and expected TDO values. Then, shift.
//               Compare the expected TDO with the captured TDO using the
//               prespecified XTDOMASK.
//---------------------------------------------------------------------------------------
void XSVF_Class::Do_XSDRTDO( void )
{
    BasicXSDRTDO( mTapStateEndDR, mRunTestTime, /*captureTDO=*/ true, mMaxRepeat );
    }

//---------------------------------------------------------------------------------------
// Method:       XSVF_Class::Do_XSETSDRMASKS
// Description:  XSETSDRMASKS <OctetArray.AddressMask[XSDRSIZE]>
//                            <OctetArray.DataMask[XSDRSIZE]>
//               Get the prespecified address and data mask for the XSDRINC
//               command.
//               Used for xc9500/xl compressed XSVF data.
//---------------------------------------------------------------------------------------
void XSVF_Class::Do_XSETSDRMASKS( void )
{
    // Read the addressMask
    //
    if ( ! lvAddressMask.ReadXSVF( mShiftLengthBytes ) )
    {
        mErrorCode = XSVF_ERROR_ENDOFFILE;
        return;
        }

    // Read the dataMask
    //
    if ( ! lvDataMask.ReadXSVF( mShiftLengthBytes ) )
    {
        mErrorCode = XSVF_ERROR_ENDOFFILE;
        return;
        }

    TRACE_DBG( 4, "       Addr Mask   = " );
    TRACE_ARR( 4, &lvAddressMask );
    TRACE_DBG( 4, "\n" );
    TRACE_DBG( 4, "       Data Mask   = " );
    TRACE_ARR( 4, &lvDataMask );
    TRACE_DBG( 4, "\n" );

    mErrorCode = XSVF_ERROR_NONE;
    }

//---------------------------------------------------------------------------------------
// Method:       XSVF_Class::Do_XSDRINC
// Description:  XSDRINC <OctetArray.firstTDI[XSDRSIZE]> <byte(numTimes)>
//                       <OctetArray.data[XSETSDRMASKS.dataMask.len]> ...
//               Get the XSDRINC parameters and execute the XSDRINC command.
//               XSDRINC starts by loading the first TDI shift value.
//               Then, for numTimes, XSDRINC gets the next piece of data,
//               replaces the bits from the starting TDI as defined by the
//               XSETSDRMASKS.dataMask, adds the address mask from
//               XSETSDRMASKS.addressMask, shifts the new TDI value,
//               and compares the TDO to the expected TDO from the previous
//               XSDRTDO command using the XTDOMASK.
//               Used for xc9500/xl compressed XSVF data.
//---------------------------------------------------------------------------------------
void XSVF_Class::Do_XSDRINC( void )
{
    if ( ! lvTdi.ReadXSVF( mShiftLengthBytes ) )
    {
        mErrorCode = XSVF_ERROR_ENDOFFILE;
        return;
        }

    Shift
    ( 
        XTAPSTATE_SHIFTDR, mShiftLengthBits, mTapStateEndDR, mRunTestTime, 
        /*captureTDO=*/true, mMaxRepeat 
        );

    if ( ! mErrorCode )
    {
        // Calculate number of data mask bits
        //
        int iDataMaskLen = 0;
        for ( int i = 0; i < lvDataMask.len; ++i )
        {
            for( int ucDataMask  = lvDataMask.val[ i ]; ucDataMask; ucDataMask >>= 1 )
            {
                iDataMaskLen += ( ucDataMask & 1 );
                }
            }

        // Get the number of data pieces, i.e. number of times to shift
        //
        int ucNumTimes = ReadXSVF ();
        if ( ucNumTimes < 0 )
        {
            mErrorCode = XSVF_ERROR_ENDOFFILE;
            return;
            }

        // For numTimes, get data, fix TDI, and shift
        //
        for ( int i = 0; ! mErrorCode && i < ucNumTimes; ++i )
        {
            if ( ! lvNextData.ReadXSVF( GetAsNumBytes( iDataMaskLen ) ) )
            {
                mErrorCode = XSVF_ERROR_ENDOFFILE;
                return;
                }

            DoSDRMasking ();

            Shift
            (
                XTAPSTATE_SHIFTDR, mShiftLengthBits, mTapStateEndDR, mRunTestTime, 
                /*captureTDO=*/ true, mMaxRepeat
                );
            }
        }
    }

//---------------------------------------------------------------------------------------
// Method:       XSVF_Class::Do_XSDRBCE
// Description:  XSDRB/XSDRC/XSDRE <OctetArray.TDI[XSDRSIZE]>
//               If not already in SHIFTDR, goto SHIFTDR.
//               Shift the given TDI data into the JTAG scan chain.
//               Ignore TDO.
//               If cmd==XSDRE, then goto ENDDR. Otherwise, stay in ShiftDR.
//               XSDRB, XSDRC, and XSDRE are the same implementation.
//---------------------------------------------------------------------------------------
void XSVF_Class::Do_XSDRBCE( void )
{
    BasicXSDRTDO( mCommand == XSDRE ? mTapStateEndDR : XTAPSTATE_SHIFTDR );
    }

//---------------------------------------------------------------------------------------
// Method:       XSVF_Class::Do_XSDRTDOBCE
// Description:  XSDRB/XSDRC/XSDRE <OctetArray.TDI[XSDRSIZE]> <OctetArray.TDO[XSDRSIZE]>
//               If not already in SHIFTDR, goto SHIFTDR.
//               Shift the given TDI data into the JTAG scan chain.
//               Compare TDO, but do NOT use XTDOMASK.
//               If cmd==XSDRTDOE, then goto ENDDR. Otherwise, stay in ShiftDR.
//               XSDRTDOB, XSDRTDOC, and XSDRTDOE are the same implementation.
//---------------------------------------------------------------------------------------
void XSVF_Class::Do_XSDRTDOBCE( void )
{
    BasicXSDRTDO( mCommand == XSDRTDOE ? mTapStateEndDR : XTAPSTATE_SHIFTDR );
    }

//---------------------------------------------------------------------------------------
// Method:       XSVF_Class::Do_XSTATE
// Description:  XSTATE <byte>
//               <byte> == XTAPSTATE;
//               Get the state parameter and transition the TAP to that state.
//---------------------------------------------------------------------------------------
void XSVF_Class::Do_XSTATE( void )
{
    int ucNextState = ReadXSVF ();
    if ( ucNextState < 0 )
    {
        mErrorCode = XSVF_ERROR_ENDOFFILE;
        return;
        }

    if ( ucNextState < XTAPSTATE_FIRST || ucNextState > XTAPSTATE_LAST )
    {
        mErrorCode = XSVF_ERROR_ILLEGALSTATE;
        return;
        }

    GotoTapState( XSVF_TAPSTATE( ucNextState ) );
    }

//---------------------------------------------------------------------------------------
// Method:       XSVF_Class::Do_XENDXR
// Description:  XENDIR/XENDDR <byte>
//               <byte>:  0 = RUNTEST;  1 = PAUSE.
//               Get the prespecified XENDIR or XENDDR.
//               Both XENDIR and XENDDR use the same implementation.
//---------------------------------------------------------------------------------------
void XSVF_Class::Do_XENDXR( void )
{
    mErrorCode  = XSVF_ERROR_NONE;

    int ucEndState = ReadXSVF ();
    if ( ucEndState < 0 )
    {
        mErrorCode = XSVF_ERROR_ENDOFFILE;
        return;
        }

    if ( ucEndState != XENDXR_RUNTEST && ucEndState != XENDXR_PAUSE )
    {
        mErrorCode = XSVF_ERROR_ILLEGALSTATE;
        }
    else
    {
        if ( mCommand == XENDIR )
        {
            if ( ucEndState == XENDXR_RUNTEST )
            {
                mTapStateEndIR = XTAPSTATE_RUNTEST;
                }
            else
            {
                mTapStateEndIR = XTAPSTATE_PAUSEIR;
                }

            TRACE_DBG( 3, "      End IR State = %s\n", pzTapState[ mTapStateEndIR ] );
            }
        else // XENDDR 
        {
            if ( ucEndState == XENDXR_RUNTEST )
            {
                mTapStateEndDR = XTAPSTATE_RUNTEST;
                }
            else
            {
                mTapStateEndDR = XTAPSTATE_PAUSEDR;
                }

            TRACE_DBG( 3, "      End DR State = %s\n", pzTapState[ mTapStateEndDR ] );
            }
        }
    }

//---------------------------------------------------------------------------------------
// Method:       XSVF_Class::Do_XCOMMENT
// Description:  XCOMMENT <text string ending in \0>
//               <text string ending in \0> == text comment;
//               Arbitrary comment embedded in the XSVF.
//---------------------------------------------------------------------------------------
void XSVF_Class::Do_XCOMMENT( void )
{
    // Read through the comment to the end '\0' and display message.
    //
    TRACE_DBG( 1, "      " );

    for( ;; )
    {
        int ucText = ReadXSVF ();
        if ( ucText < 0 )
        {
            mErrorCode = XSVF_ERROR_ENDOFFILE;
            return;
            }

        if ( ucText == 0 )
        {
            TRACE_DBG( 1, "\n" );
            break;
            }
        else
        {
            TRACE_DBG( 1, "%c", ucText );
            }
        }

    mErrorCode = XSVF_ERROR_NONE;
    }

//---------------------------------------------------------------------------------------
// Method:       XSVF_Class::Do_XWAIT
// Description:  XWAIT <wait_state> <end_state> <wait_time>
//               If not already in <wait_state>, then go to <wait_state>.
//               Wait in <wait_state> for <wait_time> microseconds.
//               Finally, if not already in <end_state>, then goto <end_state>.
//---------------------------------------------------------------------------------------
void XSVF_Class::Do_XWAIT( void )
{
    // Get Parameters
    // <wait_state>
    //
    if ( ! lvTdi.ReadXSVF( 1 ) )
    {
        mErrorCode = XSVF_ERROR_ENDOFFILE;
        return;
        }

    int ucWaitState = lvTdi.val[0];
    if ( ucWaitState < XTAPSTATE_FIRST || ucWaitState > XTAPSTATE_LAST )
    {
        mErrorCode = XSVF_ERROR_ILLEGALSTATE;
        return;
        }

    // <end_state>
    //
    if ( ! lvTdi.ReadXSVF( 1 ) )
    {
        mErrorCode = XSVF_ERROR_ENDOFFILE;
        return;
        }

    int ucEndState = lvTdi.val[0];
    if ( ucEndState < XTAPSTATE_FIRST || ucEndState > XTAPSTATE_LAST )
    {
        mErrorCode = XSVF_ERROR_ILLEGALSTATE;
        return;
        }

    // <wait_time>
    //
    if ( ! lvTdi.ReadXSVF( 4 ) )
    {
        mErrorCode = XSVF_ERROR_ENDOFFILE;
        return;
        }

    long lWaitTime = lvTdi.GetValue ();

    TRACE_DBG( 3, "      Wait / State = %s, Time = %ld usec\n", 
               pzTapState[ ucWaitState ], lWaitTime );

    // If not already in <wait_state>, go to <wait_state>
    //
    if ( mTapState != ucWaitState )
    {
        GotoTapState( XSVF_TAPSTATE( ucWaitState ) );
        }

    // Wait for <wait_time> microseconds
    //
    if ( ! mParseOnly )
    {
        uSleep( lWaitTime );
        }

    // If not already in <end_state>, go to <end_state>
    //
    if ( mTapState != ucEndState )
    {
        GotoTapState( XSVF_TAPSTATE( ucEndState ) );
        }

    mErrorCode = XSVF_ERROR_NONE;
    }

//---------------------------------------------------------------------------------------
// Execution Control XSVF_Class Methods
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
// Method:       XSVF_Class Object Constructor
// Description:  Initialize the xsvf player.
//---------------------------------------------------------------------------------------
XSVF_Class::XSVF_Class( void )
{
#ifdef TRACE_XSVF
    mTraceLevel = TRACE_XSVF;
#endif

    TRACE_DBG( 1, "XSVF_Class: sizeof() = %d bytes\n", sizeof( XSVF_Class ) );
    }

void XSVF_Class::Initialize( int traceLevel, bool parseOnly )
{
    mTraceLevel       = traceLevel;
    mParseOnly        = parseOnly;
        
    TRACE_DBG( 1, "\nXSVF_Class: Initialize: Verbose=%d%s\n", 
                  mTraceLevel, mParseOnly ? ", Parse Only" : "" );

    mComplete         = false;
    mCommand          = XCOMPLETE;
    mCommandCount     = 0;
    mErrorCode        = XSVF_ERROR_NONE;
    
    mTapState         = XTAPSTATE_RESET;
    mTapStateEndIR    = XTAPSTATE_RUNTEST;
    mTapStateEndDR    = XTAPSTATE_RUNTEST;

    mMaxRepeat        = 0;
    mRunTestTime      = 0L;
    
    mShiftLengthBits  = 0L;
    mShiftLengthBytes = 0;
    
    lvTdi.len         = 0;
    lvTdoExpected.len = 0;
    lvTdoCaptured.len = 0;
    lvTdoMask.len     = 0;
    lvAddressMask.len = 0;
    lvDataMask.len    = 0;
    lvNextData.len    = 0;

    // Initialize the TAPs
    //
    GotoTapState( XTAPSTATE_RESET );
    }

//---------------------------------------------------------------------------------------
// Method:       XSVF_Class::_Class Destructor
// Description:  Cleanup remnants of the xsvf player.
//---------------------------------------------------------------------------------------
//XSVF_Class::~XSVF_Class( void )
//{
//    }

//---------------------------------------------------------------------------------------
// Method:       XSVF_Class::Run
// Description:  Run the xsvf player commands in loop and return if completed
//               or error encountered.
//               First call Initialize, then call this function.
// Returns:      0 = success; otherwise error.
//---------------------------------------------------------------------------------------
int XSVF_Class::Run( void )
{
    // Process the XSVF commands
    //
    while( ! mErrorCode && ! mComplete )
    {
        // Read 1 byte for the instruction
        //
        mCommand = ReadXSVF ();
        if ( mCommand < 0 )
        {
            mErrorCode = XSVF_ERROR_ENDOFFILE;
            break;
            }
        if ( mCommand >= XLASTCMD )
        {
            mErrorCode = XSVF_ERROR_ILLEGALCMD;
            break;
            }

        ++mCommandCount;

        TRACE_DBG( 4, "\n" );
        TRACE_DBG( 2, "%04ld: %s\n", mCommandCount, pzCommandName[ mCommand ] );

        // Execute the command. Func sets error code.
        //
        switch( mCommand )
        {
            case XCOMPLETE      : Do_XCOMPLETE   (); break;
            case XTDOMASK       : Do_XTDOMASK    (); break;
            case XSIR           : Do_XSIR        (); break;
            case XSDR           : Do_XSDR        (); break;
            case XRUNTEST       : Do_XRUNTEST    (); break;
            case XCMDRESERVED5  : Do_ILLEGALCMD  (); break;
            case XCMDRESERVED6  : Do_ILLEGALCMD  (); break;
            case XREPEAT        : Do_XREPEAT     (); break;
            case XSDRSIZE       : Do_XSDRSIZE    (); break;
            case XSDRTDO        : Do_XSDRTDO     (); break;
            case XSETSDRMASKS   : Do_XSETSDRMASKS(); break;
            case XSDRINC        : Do_XSDRINC     (); break;
            case XSDRB          : Do_XSDRBCE     (); break;
            case XSDRC          : Do_XSDRBCE     (); break;
            case XSDRE          : Do_XSDRBCE     (); break;
            case XSDRTDOB       : Do_XSDRTDOBCE  (); break;
            case XSDRTDOC       : Do_XSDRTDOBCE  (); break;
            case XSDRTDOE       : Do_XSDRTDOBCE  (); break;
            case XSTATE         : Do_XSTATE      (); break;
            case XENDIR         : Do_XENDXR      (); break;
            case XENDDR         : Do_XENDXR      (); break;
            case XSIR2          : Do_XSIR2       (); break;
            case XCOMMENT       : Do_XCOMMENT    (); break;
            case XWAIT          : Do_XWAIT       (); break;
            default             : Do_ILLEGALCMD  (); break;
        }
    }

    if ( mErrorCode )
    {
        TRACE_DBG( 0, "\nERROR: %s; Near XSVF ASCII File Line #%ld\n",
            pzErrorName[ mErrorCode < XSVF_ERROR_LAST ? mErrorCode : XSVF_ERROR_UNKNOWN ],
            mCommandCount
            );

        if ( mErrorCode == XSVF_ERROR_ILLEGALCMD )
        {
            TRACE_DBG( 0, 
                "Encountered unsupported command 0x%02X (%s)\n", 
                mCommand,
                mCommand >= 0 && mCommand < XLASTCMD 
                    ? pzCommandName[ mCommand ] : "Unknown"
                );
            }
        else if ( mErrorCode == XSVF_ERROR_MAXRETRIES 
               || mErrorCode == XSVF_ERROR_TDOMISMATCH )
        {
            TRACE_DBG( 0, "TDO Captured = " );
            TRACE_ARR( 0, &lvTdoCaptured );
            TRACE_DBG( 0, "\n" );
            TRACE_DBG( 0, "TDO Expected = " );
            TRACE_ARR( 0, &lvTdoExpected );
            TRACE_DBG( 0, "\n" );
            TRACE_DBG( 0, "TDO Mask     = " );
            TRACE_ARR( 0, &lvTdoMask );
            TRACE_DBG( 0, "\n" );
            }

        // Initialize the TAPs
        //
        TRACE_DBG( 3, "\n" );
        int rc = mErrorCode; // Remember error code
        GotoTapState( XTAPSTATE_RESET );
        mErrorCode = rc; // Restore error code
        }
    else
    {
        TRACE_DBG( 0, "\nSUCCESS: Completed XSVF execution (%ld commands).\n",
                      mCommandCount );
        }

    TRACE_DBG( 0, "\n" );

    return mErrorCode;
    }

static XSVF_Class xsvfObj;

//---------------------------------------------------------------------------------------
// xsvfExecute() - The primary entry point to the XSVF player
//---------------------------------------------------------------------------------------
int xsvfExecute( int traceLevel, bool parseOnly )
{
    xsvfObj.Initialize( traceLevel, parseOnly );
    return xsvfObj.Run ();
    }
