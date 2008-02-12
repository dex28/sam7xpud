#ifndef _COMMON_H
#define _COMMON_H

#ifdef __cplusplus
extern "C" {
#endif
    
//------------------------------------------------------------------------------
//      Definitions
//------------------------------------------------------------------------------

/*
  \brief ATTR_PACKED
  
  \f[
    |I_2|=\left| \int_{0}^T \psi(t) 
             \left\{ 
                u(a,t)-
                \int_{\gamma(t)}^a 
                \frac{d\theta}{k(\theta,t)}
                \int_{a}^\theta c(\xi)u_t(\xi,t)\,d\xi
             \right\} dt
          \right|
  \f]

*/    

#define ATTR_PACKED __attribute__((__packed__))

//------------------------------------------------------------------------------
//      Types
//------------------------------------------------------------------------------

// Boolean type (if not C++)
#ifndef __cplusplus
typedef enum { false = 0, true = 1 } bool;
#endif

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned long ulong;

// \brief  Generic callback function type
// Since ARM Procedure Call standard allow for 4 parameters to be stored in r0-r3 
// instead of being pushed on the stack, functions with less than 4 parameters can 
// be cast into a callback in a transparent way.
//
typedef void (*Callback_f)( uint, uint, uint, uint );

//------------------------------------------------------------------------------
//      Macros
//------------------------------------------------------------------------------

// Set or clear flag(s) in a register
#define SET(register, flags)        ((register) |= (flags))
#define CLEAR(register, flags)      ((register) &= ~(flags))

// Poll the status of flags in a register
#define ISSET(register, flags)      (((register) & (flags)) == (flags))
#define ISCLEARED(register, flags)  (((register) & (flags)) == 0)

// Returns the higher/lower byte of a word
#define HBYTE(word)                 ((uchar) ((word) >> 8))
#define LBYTE(word)                 ((uchar) ((word) & 0x00FF))

// \brief  Converts a byte array to a word value using the big endian format
#define WORDB(bytes)            ((ushort) ((bytes[0] << 8) | bytes[1]))

// \brief  Converts a byte array to a word value using the big endian format
#define WORDL(bytes)            ((ushort) ((bytes[1] << 8) | bytes[0]))

// \brief  Converts a byte array to a dword value using the big endian format
#define DWORDB(bytes)   ((uint) ((bytes[0] << 24) | (bytes[1] << 16) \
                                         | (bytes[2] << 8) | bytes[3]))

// \brief  Converts a byte array to a dword value using the big endian format
#define DWORDL(bytes)   ((uint) ((bytes[3] << 24) | (bytes[2] << 16) \
                                         | (bytes[1] << 8) | bytes[0]))

// \brief  Stores a dword value in a byte array, in big endian format
#define STORE_DWORDB(dword, bytes) \
    bytes[0] = (uchar) ((dword >> 24) & 0xFF); \
    bytes[1] = (uchar) ((dword >> 16) & 0xFF); \
    bytes[2] = (uchar) ((dword >> 8) & 0xFF); \
    bytes[3] = (uchar) (dword & 0xFF);

// \brief  Stores a word value in a byte array, in big endian format
#define STORE_WORDB(word, bytes) \
    bytes[0] = (uchar) ((word >> 8) & 0xFF); \
    bytes[1] = (uchar) (word & 0xFF);

//------------------------------------------------------------------------------
//      Inline functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// \brief  Returns the minimum value between two integers
// \param  dValue1 First value to compare
// \param  dValue2 Second value to compare
// \return Minimum value between two integers
//------------------------------------------------------------------------------
static inline uint min( uint dValue1, uint dValue2 )
{
    return dValue1 < dValue2 ? dValue1 : dValue2;
}

//------------------------------------------------------------------------------
// \brief  Returns the index of the last set (1) bit in an integer
// \param  dValue Integer value to parse
// \return Position of the leftmost set bit in the integer
//------------------------------------------------------------------------------
static inline signed char lastSetBit(uint dValue)
{
    int bIndex = -1;

    if( dValue & 0xFFFF0000 )
    {
        bIndex += 16;
        dValue >>= 16;
    }

    if( dValue & 0xFF00 )
    {
        bIndex += 8;
        dValue >>= 8;
    }

    if( dValue & 0xF0 )
    {
        bIndex += 4;
        dValue >>= 4;
    }

    if( dValue & 0xC )
    {
        bIndex += 2;
        dValue >>= 2;
    }

    if( dValue & 0x2 )
    {
        bIndex += 1;
        dValue >>= 1;
    }

    if( dValue & 0x1 )
    {
        bIndex++;
    }

    return bIndex;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _COMMON_H

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
$Id: common.h 193 2006-10-30 09:52:32Z jjoannic $
*/
