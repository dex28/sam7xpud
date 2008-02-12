
//---------------------------------------------------------------------------------------
//     Includes
//---------------------------------------------------------------------------------------

#include "usbCDC.hpp"

#include "trace.h"
#include "device.h" //! AT91 functions and constants
#include "board.h" //! USB_ENDPOINT0_MAXPACKETSIZE

using namespace USB;

//---------------------------------------------------------------------------------------
// Descriptors
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
//! \brief  Standard USB device descriptor
//! \see    S_usb_device_descriptor
//---------------------------------------------------------------------------------------
static const S_usb_device_descriptor sDevice = 
{
    sizeof(S_usb_device_descriptor), //!< Size of this descriptor
    USB_DEVICE_DESCRIPTOR,           //!< DEVICE Descriptor Type
    USB2_00,                         //!< USB 2.0 specification
    USB_CLASS_COMMUNICATION,         //!< USB Communication class code
    0x00,                            //!< No device subclass code
    0x00,                            //!< No device protocol code
    USB_ENDPOINT0_MAXPACKETSIZE,     //!< Maximum packet size for endpoint zero
    SER_VENDOR_ID,                   //!< Vendor ID
    SER_PRODUCT_ID,                  //!< Product ID
    SER_RELEASE_NUMBER,              //!< Device release number
    0x01,                            //!< Index of manufacturer description
    0x02,                            //!< Index of product description
    0x03,                            //!< Index of serial number description
    0x01                             //!< One possible configuration
};

//---------------------------------------------------------------------------------------
//! \brief  Device configuration descriptor
//! \see    S_ser_configuration_descriptor
//---------------------------------------------------------------------------------------
static const S_ser_configuration_descriptor sConfiguration = 
{
    //! Standard configuration descriptor
    {
        sizeof(S_usb_configuration_descriptor), //!< Descriptor size
        USB_CONFIGURATION_DESCRIPTOR,           //!< CONFIGURATION descriptor type
        sizeof(S_ser_configuration_descriptor), //!< Total size of this configuration 
                                                //!< (including other descriptors)
        0x02,                   //!< Two interfaces are used by this configuration
        0x01,                   //!< Value 0x01 is used to select this configuration
        0x00,                   //!< No string is used to describe this configuration
        //! TODO: Is this OK?
        //! bmAttributes: Device is self-powered and supports remote wakeup
        (1 << 7) | USB_CONFIG_SELF_POWERED | USB_CONFIG_REMOTE_WAKEUP,
        USB_POWER_MA( 100 )     //!< Maximum power consumption of the device is 100mA
    },
    //! Communication class interface descriptor
    {
        sizeof(S_usb_interface_descriptor),
        USB_INTERFACE_DESCRIPTOR,        //!< INTERFACE Descriptor Type
        0x00,                            //!< Interface 0
        0x00,                            //!< No alternate settings
        0x01,                            //!< One endpoint used
        CDC_INTERFACE_COMMUNICATION,     //!< Communication interface class
        CDC_ABSTRACT_CONTROL_MODEL,      //!< Abstract control model subclass
        0x01,                            //!< No protocol code
        0x00                             //!< No associated string descriptor
    },
    //! Header functional descriptor
    {
        sizeof(S_cdc_header_descriptor),
        CDC_CS_INTERFACE,                //!< CS_INTERFACE descriptor type
        CDC_HEADER,                      //!< Header functional descriptor
        CDC1_10,                         //!< CDC version 1.10
    },
    //! Call management functional descriptor
    {
        sizeof(S_cdc_call_management_descriptor),
        CDC_CS_INTERFACE,                //!< CS_INTERFACE type
        CDC_CALL_MANAGEMENT,             //!< Call management descriptor
        0x01,                            //!< Call management is handled by the device
        0x01                             //!< Data interface is 0x01
    },
    //! Abstract control management functional descriptor
    {
        sizeof(S_cdc_abstract_control_management_descriptor),
        CDC_CS_INTERFACE,                //!< CS_INTERFACE descriptor type
        CDC_ABSTRACT_CONTROL_MANAGEMENT, //!< Abstract control management 
                                         //!<   functional descriptor
        0x07                             //!< Every notification/request except 
                                         //!<   NetworkConnection supported
    },
    //! Union functional descriptor with one slave interface
    {
        //! Union functional descriptor
        {
            sizeof(S_cdc_union_descriptor) + 1,
            CDC_CS_INTERFACE,            //!< CS_INTERFACE descriptor type
            CDC_UNION,                   //!< Union functional descriptor
            0x00,                        //!< Master interface is 0x00
        },                               //!<   (Communication class interface)
        { 0x01 }                         //!< First slave interface is 0x01
    },                                   //!<   (Data class interface)
    //! Notification endpoint descriptor
    {
        sizeof(S_usb_endpoint_descriptor),
        USB_ENDPOINT_DESCRIPTOR,            //!< ENDPOINT descriptor type
        USB_ENDPOINT_IN | SER_EPT_NOTIFICATION, //!< IN endpoint, address = 0x03
        ENDPOINT_TYPE_INTERRUPT,            //!< INTERRUPT endpoint type
        64,                                 //!< Maximum packet size is 64 bytes
        0x10                                //!< Endpoint polled every 10 ms
    },
    //! Data class interface descriptor
    {
        sizeof(S_usb_interface_descriptor),
        USB_INTERFACE_DESCRIPTOR,           //!< INTERFACE descriptor type
        0x01,                               //!< Interface 0x01
        0x00,                               //!< No alternate settings
        0x02,                               //!< Two endpoints used
        CDC_INTERFACE_DATA,                 //!< Data class code
        0x00,                               //!< No subclass code
        0x00,                               //!< No protocol code TODO? CDC_PROTOCOL_TRANSPARENT
        0x00                                //!< No description string
    },
    //! Bulk-OUT endpoint descriptor
    {
        sizeof(S_usb_endpoint_descriptor),
        USB_ENDPOINT_DESCRIPTOR,           //!< ENDPOINT descriptor type
        USB_ENDPOINT_OUT | SER_EPT_DATA_OUT, //!< OUT endpoint, address = 0x01
        ENDPOINT_TYPE_BULK,                //!< Bulk endpoint
        64,                                //!< Endpoint size is 64 bytes
        0x00                               //!< Must be 0x00 for full-speed bulk endpoints
    },    
    //! Bulk-IN endpoint descriptor
    {
        sizeof(S_usb_endpoint_descriptor),
        USB_ENDPOINT_DESCRIPTOR,           //!< ENDPOINT descriptor type
        USB_ENDPOINT_IN | SER_EPT_DATA_IN, //!< IN endpoint, address = 0x02
        ENDPOINT_TYPE_BULK,                //!< Bulk endpoint
        64,                                //!< Endpoint size is 64 bytes
        0x00                               //!< Must be 0x00 for full-speed bulk endpoints
    },  
};

//---------------------------------------------------------------------------------------
//! \brief  Language ID string descriptor
//---------------------------------------------------------------------------------------
static const S_usb_language_id sLanguageID =
{
    USB_STRING_DESCRIPTOR_SIZE( 1 ),
    USB_STRING_DESCRIPTOR,
    USB_LANGUAGE_ENGLISH_US
};

//---------------------------------------------------------------------------------------
//! \brief  Manufacturer string descriptor
//---------------------------------------------------------------------------------------
static const char pManufacturer [] =
{
    USB_STRING_DESCRIPTOR_SIZE( 5 ),
    USB_STRING_DESCRIPTOR,
    USB_UNICODE('A'),
    USB_UNICODE('T'),
    USB_UNICODE('M'),
    USB_UNICODE('E'),
    USB_UNICODE('L')
};

//---------------------------------------------------------------------------------------
//! \brief  Product string descriptor
//---------------------------------------------------------------------------------------
static const char pProduct [] =
{
    USB_STRING_DESCRIPTOR_SIZE( 13 ),
    USB_STRING_DESCRIPTOR,
    USB_UNICODE('A'),
    USB_UNICODE('T'),
    USB_UNICODE('9'),
    USB_UNICODE('1'),
    USB_UNICODE('U'),
    USB_UNICODE('S'),
    USB_UNICODE('B'),
    USB_UNICODE('S'),
    USB_UNICODE('e'),
    USB_UNICODE('r'),
    USB_UNICODE('i'),
    USB_UNICODE('a'),
    USB_UNICODE('l')
};

//---------------------------------------------------------------------------------------
//! \brief  Serial number string descriptor
//---------------------------------------------------------------------------------------
static const char pSerialNumber [] =
{
    USB_STRING_DESCRIPTOR_SIZE( 12 ),
    USB_STRING_DESCRIPTOR,
    USB_UNICODE('0'),
    USB_UNICODE('1'),
    USB_UNICODE('2'),
    USB_UNICODE('3'),
    USB_UNICODE('4'),
    USB_UNICODE('5'),
    USB_UNICODE('6'),
    USB_UNICODE('7'),
    USB_UNICODE('8'),
    USB_UNICODE('9'),
    USB_UNICODE('A'),
    USB_UNICODE('F')
};

//---------------------------------------------------------------------------------------
//! \brief  List of string descriptors
//---------------------------------------------------------------------------------------
static const char* pStrings [] =
{
    (char*) &sLanguageID,
    pManufacturer,
    pProduct,
    pSerialNumber
};

//---------------------------------------------------------------------------------------
//! \brief List of endpoint descriptors
//---------------------------------------------------------------------------------------
static const S_usb_endpoint_descriptor* pEndpoints [] =
{
    &sConfiguration.sDataOut,
    &sConfiguration.sDataIn,
    &sConfiguration.sNotification
};

//---------------------------------------------------------------------------------------
//! \brief  Standard descriptors list
//---------------------------------------------------------------------------------------
static const S_std_descriptors sDescriptors = 
{
    &sDevice,
    (S_usb_configuration_descriptor*) &sConfiguration,
    pStrings,
    pEndpoints
};

//---------------------------------------------------------------------------------------
//      Internal methods
//---------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------
//! \brief   Sets asynchronous line-character formatting properties
//! \details This function is used as a callback when receiving the data part
//!          of a SET_LINE_CODING request.
//! \see     usbcdc11.pdf - Section 6.2.12
//-----------------------------------------------------------------------------------
 void CCDC::OnSetLineCoding( CCDC* pThis )
{
    pThis->pDriver->SendZLP0 ();

    TRACE_INFO
    ( 
        "SetLineCoding(%d,%d,%d,%d)\n",
        pThis->sLineCoding.dwDTERate,
        pThis->sLineCoding.bCharFormat,
        pThis->sLineCoding.bParityType,
        pThis->sLineCoding.bDataBits
        );
}

//---------------------------------------------------------------------------------------
//      Public methods
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
//! \brief  SETUP request handler for an Abstract Control Model device
//! \see    usbcdc11.pdf - Section 6.2
//---------------------------------------------------------------------------------------
void CCDC::RequestHandler( void )
{
    S_usb_request* pSetup = pDriver->GetSetup ();

    TRACE_DEBUG_M( "NewReq " );

    // Handle the request
    //
    switch( pSetup->bRequest )
    {
        //-----------------------
        case CDC_SET_LINE_CODING:
        //-----------------------
            TRACE_DEBUG_M( "sLineCoding " );
    
            // Start the read operation with ACM_SetLineCoding as the callback
            pDriver->Read
            (
                 0, // endpoint
                 (void*) &(sLineCoding), // data
                 sizeof(S_cdc_line_coding), // data len
                 Callback_f( OnSetLineCoding ), this // callback
                 );
            break;
    
        //-----------------------
        case CDC_GET_LINE_CODING:
        //-----------------------
            TRACE_DEBUG_M( "gLineCoding " );
    
            // Sends the currently configured line coding to the host
            // \see usbcdc11.pdf - Section 6.2.13
            //
            pDriver->Write( 0, &sLineCoding,  sizeof( S_cdc_line_coding ) );
            break;
    
        //------------------------------
        case CDC_SET_CONTROL_LINE_STATE:
        //------------------------------
            TRACE_DEBUG_M( "sControlLineState " );
    
            // Sets the state of control line parameters.
            // \see usbcdc11.pdf - Section 6.2.14
            //
            isCarrierActivated = ISSET( pSetup->wValue, CDC_ACTIVATE_CARRIER );
            isPresentDTE = ISSET( pSetup->wValue, CDC_DTE_PRESENT );
    
            TRACE_INFO( "SetControlLineState(DTE=%d,DCD=%d)\n", 
                        isPresentDTE, isCarrierActivated );
    
            pDriver->SendZLP0 ();
                
            break;
    
        //------
        default:
        //------
            // Forward request to standard request handler
            //
            CSTD::RequestHandler ();
    
            break;
    }
}

//---------------------------------------------------------------------------------------
//! \brief   Initializes a CDC serial driver
//! \details This method sets the standard descriptors of the device and the
//!          default CDC configuration.
//! \param   usbDriver USB driver instance that servers this object
//! \param   board Hardware board abstraction instance
//! \param   eventSink Event sink isntance that handles this object
//---------------------------------------------------------------------------------------
CCDC::CCDC( CUsbDriver* usbDriver, CBoard* board, CEventSink* eventSink )
    : CSTD( usbDriver, board, eventSink )
{
    TRACE_INFO( "CCDC %08x, Driver=%08x\n", uint( this ), pDriver );

    // Initialize the list of endpoints
    //
    EndpointList[ 0 ].Init( 1 ); // Endpoint 0 (Control endpoint 0) with Single bank
    EndpointList[ 1 ].Init( 2 ); // Endpoint 1 (Data out endpoint) with Dual bank
    EndpointList[ 2 ].Init( 2 ); // Endpoint 2 (Data in endpoint) with Dual bank
    EndpointList[ 3 ].Init( 1 ); // Endpoint 3 (Notification endpoint) with Single bank

    // Announce our endpoints to usb device driver
    //
    pDriver->SetEndpointList( EndpointList, NUM_ENDPOINTS );

    // Initialize standard class attributes
    //
    pDescriptors = &sDescriptors;

    // Initialize Line Coding ACM attributes
    //
    sLineCoding.dwDTERate   = 0;
    sLineCoding.bCharFormat = 0;
    sLineCoding.bParityType = 0;
    sLineCoding.bDataBits   = 0;

    // Initialize Carrier ACM attributes
    //
    isCarrierActivated = false;
    isPresentDTE       = false;
}

/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support  -  ROUSSET  -
 * ----------------------------------------------------------------------------
 * Copyright (c) 2006, Atmel Corporation
 *
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
$Id: serial_driver.c 107 2006-10-16 08:28:50Z jjoannic $
*/
