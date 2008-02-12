#ifndef _USB_CDC_H_INCLUDED
#define _USB_CDC_H_INCLUDED

//---------------------------------------------------------------------------------------
//       Includes
//---------------------------------------------------------------------------------------

#include "usbFramework.hpp"

//---------------------------------------------------------------------------------------
//! \brief USB Framework namespace.
//---------------------------------------------------------------------------------------
namespace USB {
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
//       Definitions
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
//! \brief Communication device class specification version 1.10
//---------------------------------------------------------------------------------------
enum 
{
    CDC1_10                                 = 0x0110
};

//---------------------------------------------------------------------------------------
//! \brief Interface class codes
//---------------------------------------------------------------------------------------
enum 
{
    CDC_INTERFACE_COMMUNICATION             = 0x02,
    CDC_INTERFACE_DATA                      = 0x0A
};

//---------------------------------------------------------------------------------------
//! \brief Communication interface class subclass codes
//! \see usbcdc11.pdf - Section 4.3 - Table 16
//---------------------------------------------------------------------------------------
enum 
{
    CDC_DIRECT_LINE_CONTROL_MODEL           = 0x01,
    CDC_ABSTRACT_CONTROL_MODEL              = 0x02,
    CDC_TELEPHONE_CONTROL_MODEL             = 0x03,
    CDC_MULTI_CHANNEL_CONTROL_MODEL         = 0x04,
    CDC_CAPI_CONTROL_MODEL                  = 0x05,
    CDC_ETHERNET_NETWORKING_CONTROL_MODEL   = 0x06,
    CDC_ATM_NETWORKING_CONTROL_MODEL        = 0x07
};

//---------------------------------------------------------------------------------------
//! \brief Communication interface class control protocol codes
//! \see usbcdc11.pdf - Section 4.4 - Table 17
//---------------------------------------------------------------------------------------
enum 
{
    CDC_PROTOCOL_COMMON_AT_COMMANDS         = 0x01
};

//---------------------------------------------------------------------------------------
//! \brief Data interface class protocol codes
//! \see usbcdc11.pdf - Section 4.7 - Table 19
//---------------------------------------------------------------------------------------
enum 
{
    CDC_PROTOCOL_ISDN_BRI                   = 0x30,
    CDC_PROTOCOL_HDLC                       = 0x31,
    CDC_PROTOCOL_TRANSPARENT                = 0x32,
    CDC_PROTOCOL_Q921_MANAGEMENT            = 0x50,
    CDC_PROTOCOL_Q921_DATA_LINK             = 0x51,
    CDC_PROTOCOL_Q921_MULTIPLEXOR           = 0x52,
    CDC_PROTOCOL_V42                        = 0x90,
    CDC_PROTOCOL_EURO_ISDN                  = 0x91,
    CDC_PROTOCOL_V24_RATE_ADAPTATION        = 0x92,
    CDC_PROTOCOL_CAPI                       = 0x93,
    CDC_PROTOCOL_HOST_BASED_DRIVER          = 0xFD,
    CDC_PROTOCOL_DESCRIBED_IN_PUFD          = 0xFE
};

//---------------------------------------------------------------------------------------
//! \brief CDC class-specific request codes
//! \details Values of the bRequest field for the various class-specific requests defined 
//! in the CDC specification.
//! \see usbcdc11.pdf - Section 6.2 - Table 45
//---------------------------------------------------------------------------------------
enum
{
    CDC_SEND_ENCAPSULATED_COMMAND       = 0x00,
    CDC_GET_ENCAPSULATED_COMMAND        = 0x01,
    CDC_SET_COMM_FEATURE                = 0x02,
    CDC_GET_COMM_FEATURE                = 0x03,
    CDC_CLEAR_COMM_FEATURE              = 0x04,
    CDC_SET_AUX_LINE_STATE              = 0x10,
    CDC_SET_HOOK_STATE                  = 0x11,
    CDC_PULSE_SETUP                     = 0x12,
    CDC_SEND_PULSE                      = 0x13,
    CDC_SET_PULSE_TIME                  = 0x14,
    CDC_RING_AUX_JACK                   = 0x15,
    CDC_SET_LINE_CODING                 = 0x20,
    CDC_GET_LINE_CODING                 = 0x21,
    CDC_SET_CONTROL_LINE_STATE          = 0x22,
    CDC_SEND_BREAK                      = 0x23,
    CDC_SET_RINGER_PARMS                = 0x30,
    CDC_GET_RINGER_PARMS                = 0x31,
    CDC_SET_OPERATION_PARMS             = 0x32,
    CDC_GET_OPERATION_PARMS             = 0x33,
    CDC_SET_LINE_PARMS                  = 0x34,
    CDC_GET_LINE_PARMS                  = 0x35,
    CDC_DIAL_DIGITS                     = 0x36,
    CDC_SET_UNIT_PARAMETER              = 0x37,
    CDC_GET_UNIT_PARAMETER              = 0x38,
    CDC_CLEAR_UNIT_PARAMETER            = 0x39,
    CDC_GET_PROFILE                     = 0x3A,
    CDC_SET_ETHERNET_MULTICAST_FILTERS  = 0x40,
    CDC_SET_ETHERNET_PMP_FILTER         = 0x41,
    CDC_GET_ETHERNET_PMP_FILTER         = 0x42,
    CDC_SET_ETHERNET_PACKET_FILTER      = 0x43,
    CDC_GET_ETHERNET_STATISTIC          = 0x44,
    CDC_SET_ATM_DATA_FORMAT             = 0x50,
    CDC_GET_ATM_DEVICE_STATISTICS       = 0x51,
    CDC_SET_ATM_DEFAULT_VC              = 0x52,
    CDC_GET_ATM_VC_STATISTICS           = 0x53
};
    
//---------------------------------------------------------------------------------------
//! \brief Type values for the bDescriptorType field of functional descriptors
//! \see usbcdc11.pdf - Section 5.2.3 - Table 24
//---------------------------------------------------------------------------------------
enum
{
    CDC_CS_INTERFACE                    = 0x24,
    CDC_CS_ENDPOINT                     = 0x25
};

//---------------------------------------------------------------------------------------
//! \brief Type values for the bDescriptorSubtype field of functional descriptors
//! \see usbcdc11.pdf - Section 5.2.3 - Table 25
//---------------------------------------------------------------------------------------
enum
{
    CDC_HEADER                          = 0x00,
    CDC_CALL_MANAGEMENT                 = 0x01,
    CDC_ABSTRACT_CONTROL_MANAGEMENT     = 0x02,
    CDC_DIRECT_LINE_MANAGEMENT          = 0x03,
    CDC_TELEPHONE_RINGER                = 0x04,
    CDC_REPORTING_CAPABILITIES          = 0x05,
    CDC_UNION                           = 0x06,
    CDC_COUNTRY_SELECTION               = 0x07,
    CDC_TELEPHONE_OPERATIONAL_MODES     = 0x08,
    CDC_USB_TERMINAL                    = 0x09,
    CDC_NETWORK_CHANNEL                 = 0x0A,
    CDC_PROTOCOL_UNIT                   = 0x0B,
    CDC_EXTENSION_UNIT                  = 0x0C,
    CDC_MULTI_CHANNEL_MANAGEMENT        = 0x0D,
    CDC_CAPI_CONTROL_MANAGEMENT         = 0x0E,
    CDC_ETHERNET_NETWORKING             = 0x0F,
    CDC_ATM_NETWORKING                  = 0x10
};

//---------------------------------------------------------------------------------------
//! \brief Control signal bitmap values for the SetControlLineState request
//! \see usbcdc11.pdf - Section 6.2.14 - Table 51
//---------------------------------------------------------------------------------------
enum
{
    CDC_DTE_PRESENT                     = (1 << 0),
    CDC_ACTIVATE_CARRIER                = (1 << 1)
};

//---------------------------------------------------------------------------------------
//! \brief Serial state notification bitmap values.
//! \see usbcdc11.pdf - Section 6.3.5 - Table 69
//---------------------------------------------------------------------------------------
enum
{
    CDC_SERIAL_STATE_OVERRUN            = (1 << 6),
    CDC_SERIAL_STATE_PARITY             = (1 << 5),
    CDC_SERIAL_STATE_FRAMING            = (1 << 4),
    CDC_SERIAL_STATE_RING               = (1 << 3),
    CDC_SERIAL_STATE_BREAK              = (1 << 2),
    CDC_SERIAL_STATE_TX_CARRIER         = (1 << 1),
    CDC_SERIAL_STATE_RX_CARRIER         = (1 << 0)
};

//---------------------------------------------------------------------------------------
//! \brief Notification requests
//! \see usbcdc11.pdf - Section 6.3 - Table 68
//---------------------------------------------------------------------------------------
enum
{
    CDC_NOTIFICATION_NETWORK_CONNECTION = 0x00,
    CDC_NOTIFICATION_SERIAL_STATE       = 0x20
};

//---------------------------------------------------------------------------------------
//       Structures
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
//! \brief   Header functional descriptor
//! \details This header must precede any list of class-specific descriptors.
//! \see     usbcdc11.pdf - Section 5.2.3.1
//---------------------------------------------------------------------------------------
struct S_cdc_header_descriptor
{
    uchar  bFunctionLength;    //!< Size of this descriptor in bytes
    uchar  bDescriptorType;    //!< CS_INTERFACE descriptor type
    uchar  bDescriptorSubtype; //!< Header functional descriptor subtype
    ushort bcdCDC;             //!< USB CDC specification release version

} ATTR_PACKED;

//---------------------------------------------------------------------------------------
//! \brief   Call management functional descriptor
//! \details Describes the processing of calls for the communication class
//!          interface.
//! \see     usbcdc11.pdf - Section 5.2.3.2
//---------------------------------------------------------------------------------------
struct S_cdc_call_management_descriptor
{
    uchar bFunctionLength;    //!< Size of this descriptor in bytes
    uchar bDescriptorType;    //!< CS_INTERFACE descriptor type
    uchar bDescriptorSubtype; //!< Call management functional descriptor subtype
    uchar bmCapabilities;     //!< The capabilities that this configuration supports
    uchar bDataInterface;     //!< Interface number of the data class interface used
                              //!< for call management (optional)
} ATTR_PACKED;

//---------------------------------------------------------------------------------------
//! \brief   Abstract control management functional descriptor
//! \details Describes the command supported by the communication interface class
//!          with the Abstract Control Model subclass code.
//! \see     usbcdc11.pdf - Section 5.2.3.3
//---------------------------------------------------------------------------------------
struct S_cdc_abstract_control_management_descriptor
{
    uchar bFunctionLength;    //!< Size of this descriptor in bytes
    uchar bDescriptorType;    //!< CS_INTERFACE descriptor type
    uchar bDescriptorSubtype; //!< Abstract control management functional
                              //!< descriptor subtype
    uchar bmCapabilities;     //!< Capabilities supported by this configuration

} ATTR_PACKED;

//---------------------------------------------------------------------------------------
//! \brief   Union functional descriptors
//! \details Describes the relationship between a group of interfaces that can
//!          be considered to form a functional unit.
//! \see     usbcdc11.pdf - Section 5.2.3.8
//---------------------------------------------------------------------------------------
struct S_cdc_union_descriptor
{
    uchar bFunctionLength;    //!< Size of this descriptor in bytes
    uchar bDescriptorType;    //!< CS_INTERFACE descriptor type
    uchar bDescriptorSubtype; //!< Union functional descriptor subtype
    uchar bMasterInterface;   //!< The interface number designated as master

} ATTR_PACKED;

//---------------------------------------------------------------------------------------
//! \brief  Union functional descriptors with one slave interface
//! \see    S_cdc_union_descriptor
//---------------------------------------------------------------------------------------
struct S_cdc_union_1slave_descriptor
{
    S_cdc_union_descriptor  sUnion;                 //!< Union functional descriptor
    uchar                   bSlaveInterfaces[ 1 ];  //!< Slave interface 0
} ATTR_PACKED;

//---------------------------------------------------------------------------------------
//! \brief   Line coding structure
//! \details Format of the data returned when a GetLineCoding request is received
//! \see     usbcdc11.pdf - Section 6.2.13
//---------------------------------------------------------------------------------------
struct S_cdc_line_coding
{
    uint  dwDTERate;          //! Data terminal rate in bits per second
    char  bCharFormat;        //! Number of stop bits
    char  bParityType;        //! Parity bit type
    char  bDataBits;          //! Number of data bits

} ATTR_PACKED;

//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
// CDC Serial Driver --------------------------------------------------------------------
//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
//       Definitions
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
enum //!< Various constants
{
    SER_EPT_DATA_OUT      = 0x01,   //!< Address of the Bulk-OUT endpoint used by the 
                                    //!<   data class interface
    SER_EPT_DATA_IN       = 0x02,   //!< Address of the Bulk-IN endpoint used by the 
                                    //!<   data class interface
    SER_EPT_NOTIFICATION  = 0x03,   //!< Address of the Interrupt-IN notification 
                                    //!<   endpoint
    SER_VENDOR_ID         = 0x03EB, //!< ATMEL Vendor ID
    SER_PRODUCT_ID        = 0x6119, //!< Product ID expected by the host serial driver
    SER_RELEASE_NUMBER    = 0x0001  //!< Device Release Number 0.01
};

//---------------------------------------------------------------------------------------
//      Structures and Classes
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
//! \brief  Configuration descriptor for an Abstract Control Model device.
//! \see    usbcdc11.pdf - Section 3.6.2
//---------------------------------------------------------------------------------------
struct S_ser_configuration_descriptor
{
    //! Standard USB configuration descriptor
    S_usb_configuration_descriptor                sCfg;
    //! Communication class interface
    S_usb_interface_descriptor                    sCommunication;
    //! Header functional descriptor
    S_cdc_header_descriptor                       sHeader;
    //! Call management functional descriptor
    S_cdc_call_management_descriptor              sCallManagement;
    //! Abstract control management functional descriptor
    S_cdc_abstract_control_management_descriptor  sAbstract;
    //! Union functional descriptor
    S_cdc_union_1slave_descriptor                 sUnion;
    //! Notification endpoint descriptor
    S_usb_endpoint_descriptor                     sNotification;
    //! Data class interface
    S_usb_interface_descriptor                    sData;
    //! Data out endpoint descriptor
    S_usb_endpoint_descriptor                     sDataOut;
    //! Data in endpoint descriptor
    S_usb_endpoint_descriptor                     sDataIn;

} ATTR_PACKED;

//---------------------------------------------------------------------------------------
//! \brief  CDC class driver structure
//---------------------------------------------------------------------------------------
class CCDC : public CSTD
{
    //-----------------------------------------------------------------------------------
    //! List of endpoints (including endpoint 0) used by the device.
    //-----------------------------------------------------------------------------------
    enum { NUM_ENDPOINTS = 4 };
    CEndpoint EndpointList[ NUM_ENDPOINTS ];

public:    
    S_cdc_line_coding  sLineCoding;  //!< Contains line coding i.e. data rate,
                                     //!< data bit count, stop bit and parity information
    bool  isCarrierActivated;        //!< Indicates if the device's carrier is activated
    bool  isPresentDTE;              //!< Indicates if the terminal is present

private:
    //-----------------------------------------------------------------------------------
    //       Internal methods
    //-----------------------------------------------------------------------------------

    //-----------------------------------------------------------------------------------
    //! \brief   Sets asynchronous line-character formatting properties
    //! \details This function is used as a callback when receiving the data part
    //!          of a SET_LINE_CODING request.
    //! \see     usbcdc11.pdf - Section 6.2.12
    //-----------------------------------------------------------------------------------
    static void OnSetLineCoding( CCDC* pThis );

public:
    //-----------------------------------------------------------------------------------
    //      Public methods
    //-----------------------------------------------------------------------------------

    //-----------------------------------------------------------------------------------
    //! \brief   Initializes a CDC serial driver
    //! \details This method sets the standard descriptors of the device and the
    //!          default CDC configuration.
    //! \param   usbDriver Pointer to the CUsbDriver instance to use
    //! \param   eventSink Pointer to the CEventSink driver instance to use
    //-----------------------------------------------------------------------------------
    CCDC( CUsbDriver* usbDriver, CBoard* board, CEventSink* eventSink );

    //-----------------------------------------------------------------------------------
    //! \brief  SETUP request handler for an Abstract Control Model device
    //! \see    usbcdc11.pdf - Section 6.2
    //-----------------------------------------------------------------------------------
    void RequestHandler( void );

    //-----------------------------------------------------------------------------------
    //! \brief  Reads data from the Data OUT endpoint
    //! \param  pBuffer   Buffer in which to store the received data
    //! \param  dLength   Length of data buffer
    //! \param  fCallback Optional callback function
    //! \param  pArgument Optional parameter for the callback function
    //! \return SER_STATUS_SUCCESS if transfer has started successfully;
    //!         SER_STATUS_LOCKED if endpoint is currently in use;
    //!         SER_STATUS_ERROR if transfer cannot be started.
    //-----------------------------------------------------------------------------------
    EnumStandardReturnValue Read
    (
        void* pBuffer, uint dLength,
        Callback_f fCallback, void *pArgument 
        )
    {
        return pDriver->Read( SER_EPT_DATA_OUT, pBuffer, 
                              dLength, fCallback, pArgument );
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Sends data through the Data IN endpoint
    //! \param  pBuffer   Buffer holding the data to transmit
    //! \param  dLength   Length of data buffer
    //! \param  fCallback Optional callback function
    //! \param  pBufferLowerBound Lower bound of the circular buffer
    //! \param  pBufferUpperBound Upper bound of the circular buffer
    //! \param  pArgument Optional parameter for the callback function
    //! \return SER_STATUS_SUCCESS if transfer has started successfully;
    //!         SER_STATUS_LOCKED if endpoint is currently in use;
    //!         SER_STATUS_ERROR if transfer cannot be started.
    //-----------------------------------------------------------------------------------
    EnumStandardReturnValue Write( 
        const void* pBuffer, uint dLength,
        Callback_f fCallback = 0, void* pArgument = 0,
        const void* pBufferLowerBound = 0, const void* pBufferUpperBound = 0
        )
    {
        return pDriver->Write( SER_EPT_DATA_IN, pBuffer, dLength, 
                               fCallback, pArgument,
                               pBufferLowerBound, pBufferUpperBound
                               );
    }    
};

//---------------------------------------------------------------------------------------
} // namespace USB
//---------------------------------------------------------------------------------------

#endif // _USB_CDC_H_INCLUDED

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
$Id: cdc.h 107 2006-10-16 08:28:50Z jjoannic $
$Id: serial_driver.h 107 2006-10-16 08:28:50Z jjoannic $
*/
