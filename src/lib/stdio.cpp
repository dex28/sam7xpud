
//---------------------------------------------------------------------------------------
//      Includes
//---------------------------------------------------------------------------------------

#include "common.h"
#include "trace.h"
#include "device.h" // AT91 functions & constants
#include "board.h"  // AT91C_MASTER_CLOCK

#include <stdarg.h>

//---------------------------------------------------------------------------------------
//      External References
//---------------------------------------------------------------------------------------

extern volatile ulong dTimerTick;

//---------------------------------------------------------------------------------------
//      Structures and Datatypes
//---------------------------------------------------------------------------------------

struct TraceStream
{
    bool LF2CRLF;
    bool TimeStamp;
    volatile bool lastPutcWasEol;
    TracePut_f putc;
    };

//---------------------------------------------------------------------------------------
//      Internal functions
//---------------------------------------------------------------------------------------

// Putchar version that translates LF -> CRLF
//
#define PUTCHAR_EOL(c) \
do { \
    if( (c) == '\n' && sOut->LF2CRLF ) \
        sOut->putc( '\r' ); \
    sOut->putc( (c) ); \
    } while ( 0 )

static inline int isDigit( int c )
{
    return c >= '0' && c <= '9';
    }

//---------------------------------------------------------------------------------------
//      Local variables
//---------------------------------------------------------------------------------------

enum { MAX_OUT_STREAMS = 4 };
static struct TraceStream outStream[ MAX_OUT_STREAMS ] = { { 0 } };

//---------------------------------------------------------------------------------------
//      Exported functions
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
// Open trace stream descriptor
//---------------------------------------------------------------------------------------
extern "C" void tracef_open( int fd, TracePut_f putc, bool LF2CRLF, bool TimeStamp )
{
    if ( fd < 0 || fd >= MAX_OUT_STREAMS )
        return;

    outStream[ fd ].LF2CRLF        = LF2CRLF;
    outStream[ fd ].TimeStamp      = TimeStamp;
    outStream[ fd ].lastPutcWasEol = true;
    outStream[ fd ].putc           = putc;
    }

//---------------------------------------------------------------------------------------
// Printf like trace function.
// Supports: %d, %u, %o, %x, %X, %c, %s and the width, precision, padding modifiers
//---------------------------------------------------------------------------------------
extern "C" void tracef( int fd, const char* f, ... )
{
    static const char HexDigits [] = "0123456789ABCDEF";
    static const char hexDigits [] = "0123456789abcdef";

    if ( fd < 0 || fd >= MAX_OUT_STREAMS )
        return;

    TraceStream* sOut = &outStream[ fd ];

    if ( ! sOut || ! sOut->putc )
        return;

    enum
    {
        FLAG_FORMAT_CHAR    = 0x00FF,   // Flag: current format character ('d', 'u'...)
        FLAG_DO_LONG        = 0x0100,   // Flag: number is declared 'long'
        FLAG_DO_SHORT       = 0x0200,   // Flag: number is declared 'short'
        FLAG_FLUSH_LEFT     = 0x0400,   // Flag: flush left
        FLAG_PAD_WITH_ZERO  = 0x0800,   // Flag: pad with '0' rather than ' '
        FLAG_IS_NEGATIVE    = 0x1000,   // Flag: number is negative
        MAX_FIELD_SIZE      =  32767    // Maximum field size to print
        };

    va_list ap;
    va_start( ap, f );

    for( ; *f; ++f )
    {
        /////////////////////////////////////////////////////////////////////////////////
        // Temporary variables to hold data

        // String to hold digits of number field. 32 bit integer has 11 octal digits. 
        char buf[ 12 ];
        
        // Field description. Note that data is union of various data types 
        // to keep stack small.
        union { int I; long L; ulong UL; char* PC; } data;
        int flags, f_width, prec;
        char* bp;

        // Print timestamp after the sequence of EOLs continued with non-EOL character
        //
        if ( sOut->TimeStamp && sOut->lastPutcWasEol && *f != '\n' ) 
        {
            sOut->lastPutcWasEol = false;

            //Timer is in milliseconds, format is "%10.3lf: ", dTimerTick / 1000.0
            //
            data.UL = dTimerTick;
            f_width = 10;
            bp = buf;

            *bp++ = HexDigits[ data.UL % 10 ]; data.UL /= 10;
            *bp++ = HexDigits[ data.UL % 10 ]; data.UL /= 10;
            *bp++ = HexDigits[ data.UL % 10 ]; data.UL /= 10;

            *bp++ = '.';

            do *bp++ = HexDigits[ data.UL % 10 ];
                while( ( data.UL /= 10 ) != 0 );
            
            f_width = f_width - ( bp - buf );

            while( f_width-- > 0 )
                sOut->putc( ' ' );

            for( bp--; bp >= buf; bp-- )
                sOut->putc( *bp );

            sOut->putc( ':' );
            sOut->putc( ' ' );
            }

        /////////////////////////////////////////////////////////////////////////////////
        // If not a format character, then just output the char
        //
        if ( *f != '%' )
        {
            if ( *f == '\n' )
                sOut->lastPutcWasEol = true;

            PUTCHAR_EOL( *f );
            continue;
            }

        ++f; // If we have a "%" then skip it 

        /////////////////////////////////////////////////////////////////////////////////
        // Clear all flags for the current field
        //
        flags   = 0;
        bp      = buf;
        f_width = 0;
        prec    = MAX_FIELD_SIZE;

        /////////////////////////////////////////////////////////////////////////////////
        // Flush left flag
        //
        if ( *f == '-' )
        {
            flags |= FLAG_FLUSH_LEFT;
            ++f;
            }

        /////////////////////////////////////////////////////////////////////////////////
        // Padding with 0 rather than space flag
        //
        if ( *f == '0' || *f == '.' )
        {
            flags |= FLAG_PAD_WITH_ZERO;
            ++f;
            }

        /////////////////////////////////////////////////////////////////////////////////
        // Field width
        //
        if ( *f == '*' )
        {
            f_width = va_arg( ap, int );
            ++f;
            }
        else if ( isDigit( *f ) )
        {
            f_width = 0;
            while( isDigit( *f ) )
            {
                f_width *= 10;
                f_width += *f++ - '0';
                }
            }

        /////////////////////////////////////////////////////////////////////////////////
        // Field precision
        //
        if( *f == '.' )
        {
            ++f;
            if ( *f == '*' )
            {
                prec = va_arg( ap, int );
                ++f;
                }
            else if( isDigit( *f ) )
            {
                prec = 0;
                while( isDigit( *f ) )
                {
                    prec *= 10;
                    prec += *f++ - '0';
                    }
                }
            }

        /////////////////////////////////////////////////////////////////////////////////
        // Long specifier format flag
        //
        if( *f == 'L' || *f == 'l' )
        {
            flags |= FLAG_DO_LONG;
            ++f;
            }

        /////////////////////////////////////////////////////////////////////////////////
        // Short specifier format flag
        //
        if( *f == 'H' || *f == 'h' )
        {
            flags |= FLAG_DO_SHORT;
            ++f;
            }

        /////////////////////////////////////////////////////////////////////////////////
        // Put the current format character as flag and do the formatting.
        //
        flags |= (unsigned char)*f;

        switch( *f ) 
        {
            /////////////////////////////////////////////////////////////////////////////
            // '%' character
            //
            case '%':
                sOut->putc( '%' );
                break;

            /////////////////////////////////////////////////////////////////////////////
            // Signed decimal
            //
            case 'd':

                if( flags & FLAG_DO_LONG )
                    data.L = va_arg( ap, long );
                else
                    data.L = (long) va_arg( ap, int );

                if ( data.L < 0 )
                {
                    flags |= FLAG_IS_NEGATIVE;
                    data.L = -data.L;
                    }

                do *bp++ = HexDigits[ data.L % 10 ];
                    while( ( data.L /= 10 ) != 0 );

                if( flags & FLAG_IS_NEGATIVE )
                    *bp++ = '-';

                f_width = f_width - ( bp - buf );

                if ( ! ( flags & FLAG_FLUSH_LEFT ) )
                {
                    data.I = flags & FLAG_PAD_WITH_ZERO ? '0' : ' ';
                    while( f_width-- > 0 )
                        sOut->putc( data.I );
                    }

                for ( bp--; bp >= buf; bp-- )
                    sOut->putc( *bp );
                
                if ( flags & FLAG_FLUSH_LEFT )
                {
                    while( f_width-- > 0 )
                        sOut->putc( ' ' );
                    }
                break;

            /////////////////////////////////////////////////////////////////////////////
            // Unsigned decimal, octal or hexadecimal number
            //
            case 'x':
            case 'X':
            case 'u':
            case 'o':

                if( flags & FLAG_DO_LONG )
                    data.UL = va_arg( ap, ulong );
                else
                    data.UL = (ulong) va_arg( ap, uint );

                // Unsigned decimal
                if( ( flags & FLAG_FORMAT_CHAR ) == 'u' )
                {
                    do *bp++ = HexDigits[ data.UL % 10 ];
                        while( ( data.UL /= 10 ) != 0 );
                    }
                // Octal
                else if( ( flags & FLAG_FORMAT_CHAR ) == 'o' )
                {
                    do *bp++ = HexDigits[ data.UL & 0x07 ];
                        while( ( data.UL >>= 3 ) != 0 );
                    }
                // Hexadecimal (capital letters A-F as digits)
                else if( ( flags & FLAG_FORMAT_CHAR ) == 'X' ) 
                {
                    do *bp++ = HexDigits[ data.UL & 0x0F ];
                        while( ( data.UL >>= 4 ) != 0 );
                    }
                // Hexadecimal (small letters a-f as digits)
                else if( ( flags & FLAG_FORMAT_CHAR ) == 'x' ) 
                {
                    do *bp++ = hexDigits[ data.UL & 0x0F ];
                        while( ( data.UL >>= 4 ) != 0 );
                    }

                f_width = f_width - ( bp - buf );

                if( ! ( flags & FLAG_FLUSH_LEFT ) )
                {
                    data.I = flags & FLAG_PAD_WITH_ZERO ? '0' : ' ';
                    while( f_width-- > 0 )
                        sOut->putc( data.I );
                    }

                for( bp--; bp >= buf; bp-- )
                    sOut->putc( *bp );
                
                if( flags & FLAG_FLUSH_LEFT )
                {
                    while( f_width-- > 0 )
                        sOut->putc( ' ' );
                    }
                break;

            /////////////////////////////////////////////////////////////////////////////
            // Character
            //
            case 'c':
                
                data.I = va_arg( ap, int );
                PUTCHAR_EOL( data.I );
                break;
                
            /////////////////////////////////////////////////////////////////////////////
            // Character string
            //
            case 's':

                bp = data.PC = va_arg( ap, char* );

                if( ! bp )
                    bp = (char*)"(null)";
                
                // f_width = f_width - strlen( bp );
                //
                while( *bp++ )
                    f_width--;
                bp = data.PC;

                if( ! ( flags & FLAG_FLUSH_LEFT ) )
                {
                    data.I = flags & FLAG_PAD_WITH_ZERO ? '0' : ' ';
                    while( f_width-- > 0 )
                        sOut->putc( data.I );
                    }

                for( prec--; *bp && prec >= 0; prec--, bp++ )
                {
                    PUTCHAR_EOL( *bp );
                    }

                if( flags & FLAG_FLUSH_LEFT )
                {
                    while( f_width-- > 0 )
                        sOut->putc( ' ' );
                    }
                break;

            /////////////////////////////////////////////////////////////////////////////
            // Unknown format
            //
            default:
                break;
            }
        }

    va_end( ap );
    }
