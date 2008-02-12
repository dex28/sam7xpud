#ifndef _DEVICE_H_INCLUDED
#define _DEVICE_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//      Includes
//------------------------------------------------------------------------------

// mthomas
#ifdef __GNUC__
#define __inline static inline
#endif

#if defined(AT91SAM7S321)
    #include "AT91SAM7S321.h"
    #include "lib_AT91SAM7S321.h"
#elif defined(AT91SAM7S64)
    #include "AT91SAM7S64.h"
    #include "lib_AT91SAM7S64.h"
#elif defined(AT91SAM7S128)
    #include "AT91SAM7S128.h"
    #include "lib_AT91SAM7S128.h"
#elif defined(AT91SAM7S256)
    #include "AT91SAM7S256.h"
    #include "lib_AT91SAM7S256.h"
#elif defined(AT91SAM7S512)
    #include "AT91SAM7S512.h"
    #include "lib_AT91SAM7S512.h"
#elif defined(AT91SAM7SE32)
    #include "AT91SAM7SE32.h"
    #include "lib_AT91SAM7SE32.h"
#elif defined(AT91SAM7SE256)
    #include "AT91SAM7SE256.h"
    #include "lib_AT91SAM7SE256.h"
#elif defined(AT91SAM7SE512)
    #include "AT91SAM7SE512.h"
    #include "lib_AT91SAM7SE512.h"
#elif defined(AT91SAM7X128)
    #include "AT91SAM7X128.h"
    #include "lib_AT91SAM7X128.h"
#elif defined(AT91SAM7X256)
    #include "AT91SAM7X256.h"
    #include "lib_AT91SAM7X256.h"
#elif defined(AT91SAM7X512)
    #include "AT91SAM7X512.h"
    #include "lib_AT91SAM7X512.h"
#elif defined(AT91SAM7A3)
    #include "AT91SAM7A3.h"
    #include "lib_AT91SAM7A3.h"
#elif defined(AT91RM9200)
    #include "AT91RM9200.h"
    #include "lib_AT91RM9200.h"
#elif defined(AT91SAM9260)
    #include "AT91SAM9260.h"
    #include "lib_AT91SAM9260.h"
#elif defined(AT91SAM9261)
    #include "AT91SAM9261.h"
    #include "lib_AT91SAM9261.h"
#elif defined(AT91SAM9263)
    #include "AT91SAM9263.h"
    #include "lib_AT91SAM9263.h"
#else
    #error No device selected.
#endif

#undef __inline

//------------------------------------------------------------------------------
//      USB Controller
//------------------------------------------------------------------------------

#if    defined(AT91SAM7S321) || defined(AT91SAM7S64)   || defined(AT91SAM7S128)\
    || defined(AT91SAM7S256) || defined(AT91SAM7S512)  || defined(AT91SAM7X128)\
    || defined(AT91SAM7X256) || defined(AT91SAM7X512)  || defined(AT91SAM7A3)  \
    || defined(AT91RM9200)

    #define UDP
    #define FULLSPEED
    #define USB_ENDPOINT0_MAXPACKETSIZE       8

    #define AT91C_FLASH_WAIT_STATES AT91C_MC_FWS_1FWS


#elif defined(AT91SAM7SE32) || defined(AT91SAM7SE256) || defined(AT91SAM7SE512)

    #define UDP
    #define UDP_INTERNAL_PULLUP
    #define FULLSPEED
    #define USB_ENDPOINT0_MAXPACKETSIZE       8

#elif defined(AT91SAM9261)

    #define UDP
    #define UDP_INTERNAL_PULLUP_BY_MATRIX
    #define FULLSPEED
    #define USB_ENDPOINT0_MAXPACKETSIZE       8

#elif defined(AT91SAM9260) || defined(AT91SAM9263)

    #define UDP
    #define UDP_INTERNAL_PULLUP
    #define FULLSPEED
    #define USB_ENDPOINT0_MAXPACKETSIZE      64

#else
    #error core_device.h
#endif

//------------------------------------------------------------------------------
//      Flash
//------------------------------------------------------------------------------

//! \brief  Memory controller & registers names
#if defined(AT91SAM7S321) || defined(AT91SAM7S64) || defined(AT91SAM7S128) \
    || defined(AT91SAM7S256) || defined(AT91SAM7X128) || defined(AT91SAM7X256) \
    || defined(AT91SAM7A3)

    #define AT91C_BASE_FLASH                    AT91C_BASE_MC
    #define AT91PS_FLASH                        AT91PS_MC
    #define FLA_FCR                             MC_FCR
    #define FLA_FMR                             MC_FMR
    #define FLA_FSR                             MC_FSR

#elif defined(AT91SAM7SE32) || defined(AT91SAM7SE256)

    #define AT91C_BASE_FLASH                    AT91C_BASE_EFC
    #define AT91PS_FLASH                        AT91PS_EFC
    #define FLA_FCR                             EFC_FCR
    #define FLA_FMR                             EFC_FMR
    #define FLA_FSR                             EFC_FSR

#elif defined(AT91SAM7SE512)

    #define AT91C_BASE_FLASH                    AT91C_BASE_EFC0
    #define AT91PS_FLASH                        AT91PS_EFC
    #define FLA_FCR                             EFC_FCR
    #define FLA_FMR                             EFC_FMR
    #define FLA_FSR                             EFC_FSR
#endif

//------------------------------------------------------------------------------
//      Prototypes
//------------------------------------------------------------------------------

extern void DEV_Init1(void);
extern void DEV_Init2(void);
extern void DEV_Resume(void);
extern void DEV_Suspend(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _DEVICE_H_INCLUDED

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
$Id: device.h 195 2006-10-30 09:55:01Z jjoannic $
*/
