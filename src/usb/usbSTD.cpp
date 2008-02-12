
//---------------------------------------------------------------------------------------
//      Includes
//---------------------------------------------------------------------------------------

#include "usbFramework.hpp"

#include "trace.h"

using namespace USB;

//---------------------------------------------------------------------------------------
//      CSTD class implementation
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
//! \brief Handles standard SETUP requests
//---------------------------------------------------------------------------------------
void CSTD::RequestHandler( void )
{
    S_usb_request* pSetup = pDriver->GetSetup ();

    TRACE_DEBUG_M( "Std " );

    // Handle incoming request
    //
    switch( pSetup->bRequest )
    {
        //-------------------------------------------------------------------------------
        case USB_GET_DESCRIPTOR: // bRequest
        //-------------------------------------------------------------------------------
            TRACE_DEBUG_M( "gDesc " );
    
            // The HBYTE macro returns the upper byte of a word
            //
            switch( HBYTE( pSetup->wValue ) ) 
            {
                //-------------------------
                case USB_DEVICE_DESCRIPTOR:
                //-------------------------
                    TRACE_DEBUG_M( "Dev " );
                    GetDeviceDescriptor( pSetup->wLength );
                    break;
        
                //--------------------------------
                case USB_CONFIGURATION_DESCRIPTOR:
                //--------------------------------
                    TRACE_DEBUG_M( "Cfg " );
                    GetConfigurationDescriptor( pSetup->wLength );
                    break;
        
                //-----------------------------------
                case USB_DEVICE_QUALIFIER_DESCRIPTOR:
                //-----------------------------------
                    TRACE_DEBUG_M( "Qua " );
#if defined(HIGHSPEED)
                    GetQualifierDescriptor( pSetup->wLength );
#else            
                    TRACE_DEBUG_M( "[not supported] " );
                    pDriver->Stall ();
#endif
                    break;

#if defined(HIGHSPEED)
                //--------------------------------------------
                case USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR:
                //--------------------------------------------
                    TRACE_DEBUG_M( "OSC " );
                    GetOSCDescriptor( pSetup->wLength );
                    break;
#endif
    
                //-------------------------
                case USB_STRING_DESCRIPTOR:
                //-------------------------
                    TRACE_DEBUG_M( "Str%d ", LBYTE( pSetup->wValue ) );
                    GetStringDescriptor( pSetup->wLength, LBYTE( pSetup->wValue ) ); 
                    break;
        
                //------
                default:
                //------
                    TRACE_WARNING( 
                        "W: STD::RequestHandler: Unknown GetDescriptor 0x%02X\n",
                        HBYTE( pSetup->wValue )
                    );
                    pDriver->Stall ();
                    break;
                }
                break;
    
        //-------------------------------------------------------------------------------
        case USB_SET_ADDRESS: // bRequest
        //-------------------------------------------------------------------------------
            TRACE_DEBUG_M( "sAddr " );
            pDriver->SendZLP0( Callback_f( OnSetAddress ), this );
            break;
    
        //-------------------------------------------------------------------------------
        case USB_SET_CONFIGURATION: // bRequest
        //-------------------------------------------------------------------------------
            TRACE_DEBUG_M( "sCfg " );
            SetConfiguration( (char) pSetup->wValue );
            break;
    
        //-------------------------------------------------------------------------------
        case USB_GET_CONFIGURATION: // bRequest
        //-------------------------------------------------------------------------------
            TRACE_DEBUG_M( "gCfg " );
            GetConfiguration ();
            break;
    
        //-------------------------------------------------------------------------------
        case USB_CLEAR_FEATURE: // bRequest
        //-------------------------------------------------------------------------------
            TRACE_DEBUG_M( "cFeat " );
    
            switch( pSetup->wValue )
            {
                //---------------------
                case USB_ENDPOINT_HALT:
                //---------------------
                    TRACE_DEBUG_M( "Hlt " );
                    pDriver->Halt( LBYTE( pSetup->wIndex ), USB_CLEAR_FEATURE );
                    pDriver->SendZLP0 ();
                    break;
    
                //----------------------------
                case USB_DEVICE_REMOTE_WAKEUP:
                //----------------------------
                    TRACE_DEBUG_M( "RmWak " );
                    wDeviceStatus &= ~REMOTE_WAKEUP; // Remote wakeup disabled
                    pDriver->SendZLP0 ();
                    break;
    
                //------
                default:
                //------
                    TRACE_DEBUG_M( "Sta " );
                    pDriver->Stall ();
    
            }
            break;
    
        //-------------------------------------------------------------------------------
        case USB_GET_STATUS: // bRequest
        //-------------------------------------------------------------------------------
            TRACE_DEBUG_M( "gSta " );
    
            switch ( USB_REQUEST_RECIPIENT( pSetup->bmRequestType ) )
            {
                //-------------------------
                case USB_RECIPIENT_DEVICE:
                //-------------------------
                    TRACE_DEBUG_M( "Dev " );
                    GetDeviceStatus ();
                    break;
        
                //---------------------------
                case USB_RECIPIENT_ENDPOINT:
                //---------------------------
                    TRACE_DEBUG_M( "Ept " );
                    GetEndpointStatus( LBYTE(pSetup->wIndex) );
                    break;
        
                //------
                default:
                //------
                    TRACE_WARNING(
                        "W: STD::RequestHandler: Unsupported GetStatus 0x%02X\n",
                        pSetup->bmRequestType
                    );
                    pDriver->Stall ();
                    break;
            }
            break;
    
        //-------------------------------------------------------------------------------
        case USB_SET_FEATURE: // bRequest
        //-------------------------------------------------------------------------------
            TRACE_DEBUG_M( "sFeat " );
    
            switch( pSetup->wValue )
            {
                //---------------------
                case USB_ENDPOINT_HALT:
                //---------------------
                    pDriver->Halt( LBYTE(pSetup->wIndex), USB_SET_FEATURE );
                    pDriver->SendZLP0 ();
                    break;
        
                //----------------------------
                case USB_DEVICE_REMOTE_WAKEUP:
                //----------------------------
                    wDeviceStatus |= REMOTE_WAKEUP; // Remote wakeup enabled
                    pDriver->SendZLP0 ();
                    break;
        
                //------
                default:
                //------
                    TRACE_WARNING(
                        "W: STD::RequestHandler: Unsupported SetFeature 0x%04X\n",
                        pSetup->wValue
                    );
                    pDriver->Stall ();
                    break;
                }
            break;
    
        //-------------------------------------------------------------------------------
        case USB_GET_INTERFACE: // bRequest
        //-------------------------------------------------------------------------------
            TRACE_DEBUG_M( "gIface %d ", pSetup->wIndex );
            TRACE_DEBUG_M( "[not supported] " );
            pDriver->Stall ();
            break;

        //-------------------------------------------------------------------------------
        case USB_SET_INTERFACE: // bRequest
        //-------------------------------------------------------------------------------
            TRACE_DEBUG_M( "sIface %d / %d ", pSetup->wIndex, pSetup->wValue );
            TRACE_DEBUG_M( "[not supported] " );
            pDriver->Stall ();
            break;

        //-------------------------------------------------------------------------------
        default: // bRequest
        //-------------------------------------------------------------------------------
            TRACE_WARNING(
                "W: STD::RequestHandler: Unsupported Request 0x%02X\n",
                pSetup->bRequest
            );
            pDriver->Stall ();
            break;
    }
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
Based on:
$Id: standard.c 122 2006-10-17 12:56:03Z jjoannic $
*/
