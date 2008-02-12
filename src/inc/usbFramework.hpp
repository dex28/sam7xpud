#ifndef _USB_FRAMEWORK_H_INCLUDED
#define _USB_FRAMEWORK_H_INCLUDED

//---------------------------------------------------------------------------------------
//      Includes
//---------------------------------------------------------------------------------------

#include "common.h"
#include "trace.h"

//---------------------------------------------------------------------------------------
namespace USB {
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
//      Enumerated Constants and Macros
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
//! \defgroup usb_std USB standard definitions
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
//! \ingroup usb_std 
//! \brief Standard Device Requests
//---------------------------------------------------------------------------------------
//! \details These are the standard request defined for a SETUP transaction. Please refer
//! to Section 9.4 of the USB 2.0 specification for more information. 
//! Table 9.4 defines the bRequest values for each request.
//! \see S_usb_request
//! \see_usb20 - Section 9.4
//---------------------------------------------------------------------------------------
enum EnumStandardDeviceRequests // Standard Device Requests
{
    USB_GET_STATUS         = 0x00, //!< Returns the status for the specified recipient.
                                   //!< \see get_status_const
                                   //!< \see_usb20 - Section 9.4.5
    USB_CLEAR_FEATURE      = 0x01, //!< Disables a specific feature of the device
                                   //!< \see_usb20 - Section 9.4.1
                                   //!< \see EnumStandardFeature
    USB_RESERVED_REQ02     = 0x02, //!< Reserved for future use
    USB_SET_FEATURE        = 0x03, //!< Enables a specific feature of the device
                                   //!< \see EnumStandardFeature
                                   //!< \see set_feat_const
                                   //!< \see_usb20 - 9.4.9
    USB_RESERVED_REQ04     = 0x04, //!< Reserved for future use
    USB_SET_ADDRESS        = 0x05, //!< Sets the device address for subsequent accesses
                                   //!< \see_usb20 - Section 9.4.6
    USB_GET_DESCRIPTOR     = 0x06, //!< Returns the specified descriptor if it exists
                                   //!< \see_usb20 - Section 9.4.3
    USB_SET_DESCRIPTOR     = 0x07, //!< Updates existing descriptors or creates new 
                                   //!< descriptors. This request is optional.
                                   //!< \see_usb20 - Section 9.4.8
    USB_GET_CONFIGURATION  = 0x08, //!< Returns the current configuration value of the 
                                   //!< device.
                                   //!< \see_usb20 - Section 9.4.2
    USB_SET_CONFIGURATION  = 0x09, //!< Sets the configuration of the device
                                   //!< \see_usb20 - Section 9.4.7
    USB_GET_INTERFACE      = 0x0A, //!< Returns the specified alternate setting for an 
                                   //!< interface.
                                   //!< \see_usb20 - Section 9.4.4
    USB_SET_INTERFACE      = 0x0B, //!< Selects an alternate setting for the selected 
                                   //!< interface.
                                   //!< \see_usb20 - Section 9.4.10
    USB_SYNCH_FRAME        = 0x0C  //!< Sets and reports an endpoint synchronization frame
                                   //!< \see_usb20 - Section 9.4.11
};

//---------------------------------------------------------------------------------------
//! \ingroup usb_std 
//! \brief Clear/Set Feature - Constants
//---------------------------------------------------------------------------------------
//! \details Useful constants when declaring a Clear Feature or Set Feature
//! standard request.
//! \see EnumStandardDeviceRequests
//! \see S_usb_request
//! \see_usb20 - Section 9.4 - Table 9.6
//---------------------------------------------------------------------------------------
enum EnumStandardFeature //!< Standard Feature Selectors
{
    //! Possible values for the wValue field of the Clear Feature and Set Feature
    //! standard requests.
    //
    USB_ENDPOINT_HALT         = 0x00, //!< Halt feature of an endpoint
    USB_DEVICE_REMOTE_WAKEUP  = 0x01, //!< Remote wake-up feature of the device
    USB_TEST_MODE             = 0x02  //!< USB test mode
};

//---------------------------------------------------------------------------------------
//! \brief Set Feature - Constants
//---------------------------------------------------------------------------------------
//! \details Useful constants when declaring a Set Feature standard request
//! \see_usb20 - Section 7.1.20
//! \see_usb20 - Section 9.2.9 - Table 9.7
//---------------------------------------------------------------------------------------
enum EnumTestMode //!< Test Mode Selectors
{
    //! Test modes available to probe an USB device.
    //
    TEST_J             = 0x01, //!< Tests the high-output drive level on the D+ line
    TEST_K             = 0x02, //!< Tests the high-output drive level on the D- line
    TEST_SEO_NAK       = 0x03, //!< Tests the output impedance, low-level output voltage 
                               //!< and loading characteristics
    TEST_PACKET        = 0x04, //!< Tests rise and fall times, eye patterns and jitter
    TEST_FORCE_ENABLE  = 0x05  //!< Tests the hub disconnect detection
};

//---------------------------------------------------------------------------------------
//! \ingroup usb_std
//! \brief Get/Set Descriptor - Constants
//---------------------------------------------------------------------------------------
//! \details Useful constants when declaring a Get Descriptor or Set Descriptor
//! standard request
//! \see S_usb_device_descriptor
//! \see S_usb_configuration_descriptor
//! \see S_usb_endpoint_descriptor
//! \see S_usb_device_qualifier_descriptor
//! \see S_USB_LANGUAGE_ENGLISH_US
//! \see_usb20 - Section 9.5 - Table 9.5
//---------------------------------------------------------------------------------------
enum EnumDescriptorType //! Descriptor Types
{
    //! Possible bDescriptorType values for the descriptor structures.
    //! They can be used with Get Descriptor and Set Descriptor standard requests
    //! to retrieve/modify them
    //
    USB_DEVICE_DESCRIPTOR                     = 0x01, //!< Device descriptor
    USB_CONFIGURATION_DESCRIPTOR              = 0x02, //!< Configuration descriptor
    USB_STRING_DESCRIPTOR                     = 0x03, //!< String descriptor
    USB_INTERFACE_DESCRIPTOR                  = 0x04, //!< Interface descriptor
    USB_ENDPOINT_DESCRIPTOR                   = 0x05, //!< Endpoint descriptor
    USB_DEVICE_QUALIFIER_DESCRIPTOR           = 0x06, //!< Device qualifier descriptor
    USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR  = 0x07, //!< Other speed configuration 
                                                      //!< descriptor
    USB_INTERFACE_POWER_DESCRIPTOR            = 0x08  //!< Interface power descriptor
};

//---------------------------------------------------------------------------------------
//! \ingroup usb_std
//! \brief Endpoint Descriptor - Constants
//---------------------------------------------------------------------------------------
//! \details Useful constants when declaring an endpoint descriptor
//! \see S_usb_endpoint_descriptor
//! \see_usb20 - Section 9.6.6 - Table 9.13
//---------------------------------------------------------------------------------------

enum EnumEndpointAddress //!< bEndpointAddress field
{
    //!< Values for the bEndpointAddress field of an endpoint descriptor.
    //
    USB_ENDPOINT_OUT    = (0 << 7), //!< Defines an OUT endpoint
    USB_ENDPOINT_IN     = (1 << 7)  //!< Defines an IN endpoint
};

enum EnumEndpointAttribute //!< bmAttributes field
{
    //! These are the four possible tranfer type values for the bmAttributes
    //! field of an endpoint descriptor.
    //
    ENDPOINT_TYPE_CONTROL        = 0x00, //!< Defines a CONTROL endpoint
    ENDPOINT_TYPE_ISOCHRONOUS    = 0x01, //!< Defines a ISOCHRONOUS endpoint
    ENDPOINT_TYPE_BULK           = 0x02, //!< Defines a BULK endpoint
    ENDPOINT_TYPE_INTERRUPT      = 0x03  //!< Defines an INTERRUPT endpoint
};

//! Get the endpoint type from bmAttributes field
#define USB_ENDPOINT_TYPE(bmAttributes)  ( (bmAttributes) & 0x03 )

//---------------------------------------------------------------------------------------
//! \ingroup usb_std
//! \brief bmRequestType bitmapped field identifies the characteristics of the specific 
//! request.
//! \see_usb20 - Table 9-2. Format of Setup Data
//---------------------------------------------------------------------------------------

enum EnumEndpointDirType //!< bmRequestType field -- bit [7]: Data transfer direction
{
    USB_DIR_HOST2DEVICE      = 0x00, //!< Host to device data transfer direction
    USB_DIR_DEVICE2HOST      = 0x01, //!< Host to device data transfer direction
};

enum EnumEndpointReqType //!< bmRequestType field -- bits [6..5]: Type of the request
{
    USB_STANDARD_REQUEST     = 0x00, //!< Defines a standard request
    USB_CLASS_REQUEST        = 0x01, //!< Defines a class request
    USB_VENDOR_REQUEST       = 0x02  //!< Defines a vendor request
};

enum EnumEndpointRecipient //!< bmRequestType field -- bits [4..0]: Recipient
{
    USB_RECIPIENT_DEVICE     = 0x00, //!< Recipient is the whole device
    USB_RECIPIENT_INTERFACE  = 0x01, //!< Recipient is an interface
    USB_RECIPIENT_ENDPOINT   = 0x02  //!< Recipient is an endpoint
};

//! Get the type of the request bits [6..5] from the bmRequestType
#define USB_REQUEST_TYPE(reqType)       ( (reqType) & = 0x60) >> 5 )

//! Get the receipient bits [4..0] from the bmRequestType
#define USB_REQUEST_RECIPIENT(reqType)  ( (reqType) &= 0x1F )

//! Get the data transfer direction bits [4..0] from the bmRequestType
#define USB_REQUEST_DIR(reqType)  ( ( (reqType) &= 0x80 ) >> 7 )

//---------------------------------------------------------------------------------------
//! \ingroup usb_std
//! \brief Endpoint Descriptor - Macros
//---------------------------------------------------------------------------------------
//! \details Useful macros when declaring an endpoint descriptor
//! \see S_usb_endpoint_descriptor
//! \see_usb20 - Section 9.6.6 - Table 9.13
//---------------------------------------------------------------------------------------

//! bEndpointAddress field macros

//! Returns an endpoint number
#define USB_ENDPOINT_NUMBER(bEndpointAddress)    ( (bEndpointAddress) & 0x0F )

//! Returns an endpoint direction (IN or OUT)
#define USB_ENDPOINT_DIRECTION(bEndpointAddress) ( (bEndpointAddress) & 0x80 )

//---------------------------------------------------------------------------------------
//! USB Class Codes
//---------------------------------------------------------------------------------------
//! These are the class codes approved by the USB-IF organization. They can be
//! used for the bDeviceClass value of a device descriptor, or the
//! bInterfaceClass value of an interface descriptor.
//! \see S_usb_device_descriptor
//! \see S_usb_interface_descriptor
//! \see http://www.usb.org/developers/defined_class
//---------------------------------------------------------------------------------------
enum EnumUsbClassCodes //!< USB Class Codes
{
    USB_CLASS_DEVICE                = 0x00, //!< Indicates that the class information is 
                                            //!< determined by the interface descriptor.
    USB_CLASS_AUDIO                 = 0x01, //!< Audio capable devices
    USB_CLASS_COMMUNICATION         = 0x02, //!< Communication devices
    USB_CLASS_HID                   = 0x03, //!< Human-interface devices
    USB_CLASS_PHYSICAL              = 0x05, //!< Human-interface devices requiring 
                                            //!< real-time physical feedback
    USB_CLASS_STILL_IMAGING         = 0x06, //!< Still image capture devices
    USB_CLASS_PRINTER               = 0x07, //!< Printer devices
    USB_CLASS_MASS_STORAGE          = 0x08, //!< Mass-storage devices
    USB_CLASS_HUB                   = 0x09, //!< Hub devices
    USB_CLASS_CDC_DATA              = 0x0A, //!< Raw-data communication device
    USB_CLASS_SMARTCARDS            = 0x0B, //!< Smartcards devices
    USB_CLASS_CONTENT_SECURITY      = 0x0D, //!< Protected content devices
    USB_CLASS_VIDEO                 = 0x0E, //!< Video recording devices
    USB_CLASS_DIAGNOSTIC_DEVICE     = 0xDC, //!< Devices that diagnostic devices
    USB_CLASS_WIRELESS_CONTROLLER   = 0xE0, //!< Wireless controller devices
    USB_CLASS_MISCELLANEOUS         = 0xEF, //!< Miscellaneous devices
    USB_CLASS_APPLICATION_SPECIFIC  = 0xFE, //!< Application-specific class code
    USB_CLASS_VENDOR_SPECIFIC       = 0xFF  //!< Vendor-specific class code
};

//---------------------------------------------------------------------------------------
//! Device Descriptor - Constants
//---------------------------------------------------------------------------------------
//! Several useful constants when declaring a device descriptor
//! \see S_usb_device_descriptor
//! \see S_usb_device_qualifier_descriptor
//! \see_usb20 - Section 9.6.1 - Table 9.8
//---------------------------------------------------------------------------------------

enum EnumUsbSpecificationRelease //!< USB specification release codes; bcdUSB field
{
    USB2_00             = 0x0200,    //!< USB 2.00 specification code
    USB1_10             = 0x0110     //!< USB 1.10 specification code
};
    
//---------------------------------------------------------------------------------------
//! Configuration Descriptor - Constants
//---------------------------------------------------------------------------------------
//! Several useful constants when declaring a configuration descriptor
//! \see S_usb_configuration_descriptor
//! \see_usb20 - Section 9.6.3 - Table 9.10
//---------------------------------------------------------------------------------------

enum EnumConfigDescriptorAttributes //!< bmAttributes field
{
    //!< These are the possible bitmap flags for the bmAttributes field of a
    //!< S_usb_configuration_descriptor.
    //
    USB_CONFIG_BUS_POWERED     = (0 << 6), //!< Device is bus-powered 
    USB_CONFIG_SELF_POWERED    = (1 << 6), //!< Device is self-powered 
    USB_CONFIG_NO_WAKEUP       = (0 << 5), //!< Device does not support remote wakeup
    USB_CONFIG_REMOTE_WAKEUP   = (1 << 5)  //!< Device supports remote wakeup
};

//! Power consumption macro for the Configuration descriptor
//
#define USB_POWER_MA(power)    ((power) / 2)

//---------------------------------------------------------------------------------------
//! String Descriptor - Constants
//---------------------------------------------------------------------------------------
//! \brief Useful constants when declaring a string descriptor.
//! \see S_usb_string_descriptor
//! \see USB_LANGIDs.pdf
//---------------------------------------------------------------------------------------

enum EnumLanguageID //! Language IDs
{
    //! These are the supported language IDs as defined by the USB-IF group.
    //! They can be used to specified the languages supported by the string
    //! descriptors of a USB device.
    //
    USB_LANGUAGE_ENGLISH_US = 0x0409 //!< English (United States)
};

//---------------------------------------------------------------------------------------
//! String Descriptor - Macros
//---------------------------------------------------------------------------------------
//! \brief Several useful macros when declaring a string descriptor.
//! \see S_usb_string_descriptor
//---------------------------------------------------------------------------------------

//! Converts an ASCII character to its Unicode equivalent
#define USB_UNICODE(a)                     (a), 0x00

//! Calculates the size of a string descriptor given the number of ASCII
//! characters in it
#define USB_STRING_DESCRIPTOR_SIZE(size)   (((size) * 2) + 2)

//---------------------------------------------------------------------------------------
//! Standard return values
//! \brief Values returned by the usb device driver API methods.
//---------------------------------------------------------------------------------------
enum EnumStandardReturnValue //!< Standard return values
{
    USB_STATUS_SUCCESS      = 0, //!< Last method has completed successfully
    USB_STATUS_LOCKED       = 1, //!< Method was aborted because the recipient 
                                 //!< (device, endpoint, ...) was busy
    USB_STATUS_ABORTED      = 2, //!< Method was aborted because of abnormal status
    USB_STATUS_RESET        = 3, //!< Method was aborted because the endpoint or the 
                                 //!< device has been reset
    USB_STATUS_IMMEDREAD    = 4, //!< Immediate read. Passed to the CUsbDriver::Read()'s 
                                 //!< callback in case that data was already in FIFO
                                 //!< when Read() was posted.
};

//---------------------------------------------------------------------------------------
//! USB Device States
//! \brief Constant values used to track which USB state the device is currently in.
//---------------------------------------------------------------------------------------
enum EnumUsbDeviceStates //!< USB Device States
{
    USB_STATE_ATTACHED      = (1 << 0), //!< Attached state
    USB_STATE_POWERED       = (1 << 1), //!< Powered state
    USB_STATE_DEFAULT       = (1 << 2), //!< Default state
    USB_STATE_ADDRESS       = (1 << 3), //!< Address state
    USB_STATE_CONFIGURED    = (1 << 4), //!< Configured state
    USB_STATE_SUSPENDED     = (1 << 5)  //!< Suspended state
};

//---------------------------------------------------------------------------------------
//!      USB Standard Structures
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
//! USB standard structures
//! \brief Chapter 9 of the USB specification 2.0 (usb_20.pdf) describes a
//!        standard USB device framework. Several structures and associated
//!        constants have been defined on that model and are described here.
//! \see   usb_20.pdf - Section 9
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
//! \brief This structure represents a standard SETUP request
//! \see_usb20 - Section 9.3 - Table 9.2
//---------------------------------------------------------------------------------------
struct S_usb_request
{
    uchar   bmRequestType :  8; //!< Characteristics of the request
    uchar   bRequest      :  8; //!< Particular request
    ushort  wValue        : 16; //!< Request-specific parameter
    ushort  wIndex        : 16; //!< Request-specific parameter
    ushort  wLength       : 16; //!< Length of data for the data phase

} ATTR_PACKED;

//---------------------------------------------------------------------------------------
//! \brief This descriptor structure is used to provide information on
//!        various parameters of the device
//! \see_usb20 - Section 9.6.1
//---------------------------------------------------------------------------------------
struct S_usb_device_descriptor
{
   uchar  bLength;              //!< Size of this descriptor in bytes
   uchar  bDescriptorType;      //!< DEVICE descriptor type
   ushort bscUSB;               //!< USB specification release number
   uchar  bDeviceClass;         //!< Class code
   uchar  bDeviceSubClass;      //!< Subclass code
   uchar  bDeviceProtocol;      //!< Protocol code
   uchar  bMaxPacketSize0;      //!< Control endpoint 0 max. packet size
   ushort idVendor;             //!< Vendor ID
   ushort idProduct;            //!< Product ID
   ushort bcdDevice;            //!< Device release number
   uchar  iManufacturer;        //!< Index of manufacturer string descriptor
   uchar  iProduct;             //!< Index of produdct string descriptor
   uchar  iSerialNumber;        //!< Index of serial number string descriptor
   uchar  bNumConfigurations;   //!< Number of possible configurations

} ATTR_PACKED;

//---------------------------------------------------------------------------------------
//! \brief This is the standard configuration descriptor structure. It is used
//!        to report the current configuration of the device.
//! \see_usb20 - Section 9.6.3
//---------------------------------------------------------------------------------------
struct S_usb_configuration_descriptor
{
   uchar  bLength;              //!< Size of this descriptor in bytes
   uchar  bDescriptorType;      //!< CONFIGURATION descriptor type
   ushort wTotalLength;         //!< Total length of data returned for this configuration
   uchar  bNumInterfaces;       //!< Number of interfaces for this
                                //!< configuration
   uchar  bConfigurationValue;  //!< Value to use as an argument for the Set Configuration 
                                //!< request to select this configuration
   uchar  iConfiguration;       //!< Index of string descriptor describing this 
                                //!< configuration
   uchar  bmAttibutes;          //!< Configuration characteristics
   uchar  bMaxPower;            //!< Maximum power consumption of the device

} ATTR_PACKED;

//---------------------------------------------------------------------------------------
//! \brief Standard interface descriptor. Used to describe a specific interface
//!        of a configuration.
//! \see_usb20 - Section 9.6.5
//---------------------------------------------------------------------------------------
struct S_usb_interface_descriptor
{
   uchar bLength;               //!< Size of this descriptor in bytes
   uchar bDescriptorType;       //!< INTERFACE descriptor type
   uchar bInterfaceNumber;      //!< Number of this interface
   uchar bAlternateSetting;     //!< Value used to select this alternate setting
   uchar bNumEndpoints;         //!< Number of endpoints used by this interface 
                                //!< (excluding endpoint zero)
   uchar bInterfaceClass;       //!< Class code
   uchar bInterfaceSubClass;    //!< Sub-class
   uchar bInterfaceProtocol;    //!< Protocol code
   uchar iInterface;            //!< Index of string descriptor describing this interface

} ATTR_PACKED;

//---------------------------------------------------------------------------------------
//! \brief This structure is the standard endpoint descriptor. It contains
//!        the necessary information for the host to determine the bandwidth
//!        required by the endpoint.
//! \see_usb20 - Section 9.6.6
//---------------------------------------------------------------------------------------
struct S_usb_endpoint_descriptor
{
   uchar  bLength;              //!< Size of this descriptor in bytes
   uchar  bDescriptorType;      //!< ENDPOINT descriptor type
   uchar  bEndpointAddress;     //!< Address of the endpoint on the USB device described
                                //!< this descriptor
   uchar  bmAttributes;         //!< Endpoint attributes when configured
   ushort wMaxPacketSize;       //!< Maximum packet size this endpoint is capable of 
                                //!< sending or receiving
   uchar  bInterval;            //!< Interval for polling endpoint fordata transfers

} ATTR_PACKED;

//---------------------------------------------------------------------------------------
//! \brief The device qualifier structure provide information on a high-speed
//!        capable device if the device was operating at the other speed.
//! \see_usb20 - Section 9.6.2
//---------------------------------------------------------------------------------------
struct S_usb_device_qualifier_descriptor
{
   uchar  bLength;              //!< Size of this descriptor in bytes
   uchar  bDescriptorType;      //!< DEVICE_QUALIFIER descriptor type
   ushort bscUSB;               //!< USB specification release number
   uchar  bDeviceClass;         //!< Class code
   uchar  bDeviceSubClass;      //!< Sub-class code
   uchar  bDeviceProtocol;      //!< Protocol code
   uchar  bMaxPacketSize0;      //!< Control endpoint 0 max. packet size
   uchar  bNumConfigurations;   //!< Number of possible configurations
   uchar  bReserved;            //!< Reserved for future use, must be 0

} ATTR_PACKED;

//---------------------------------------------------------------------------------------
//! \brief The S_usb_language_id structure represents the string descriptor
//!        zero, used to specify the languages supported by the device. This
//!        structure only define one language ID.
//! \see_usb20 - Section 9.6.7 - Table 9.15
//---------------------------------------------------------------------------------------
struct S_usb_language_id
{
   uchar  bLength;              //!< Size of this descriptor in bytes
   uchar  bDescriptorType;      //!< STRING descriptor type
   ushort wLANGID;              //!< LANGID code zero

} ATTR_PACKED;

//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
// USB Framework
//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
//! \brief Abstract class that declares various event sinks (callbacks) called by the USB 
//!        low-level driver. These callback functions are used by the USB API 
//!        to notify the user application of incoming events or actions to perform.
//---------------------------------------------------------------------------------------
class CEventSink 
{
public:

    //-----------------------------------------------------------------------------------
    // METHODS
    //-----------------------------------------------------------------------------------

    //-----------------------------------------------------------------------------------
    // Pure Virtual Methods (callbacks are originated in CUsbDriver)
    //-----------------------------------------------------------------------------------

    //-----------------------------------------------------------------------------------
    //! \brief Callback API (usb_api_callbacks)
    //-----------------------------------------------------------------------------------
    //! \details These callback functions are used by the USB API to notify the
    //!        user application of incoming events or actions to perform.
    //-----------------------------------------------------------------------------------

    //-----------------------------------------------------------------------------------
    //! \brief Initialization callback function
    //! \details This callback is invoked whenever the USB API is initialized using the
    //! CUsbDriver::Init function. It should perform the following operations:
    //!  -#  If an OS is being used, install the USB driver
    //!  -#  Configure the USB controller interrupt
    //!  -#  Configure the VBus monitoring interrupt
    //! Attention: Implementation of this callback is mandatory
    //! \see CUsbDriver::Init
    //-----------------------------------------------------------------------------------
    virtual void OnInit( void ) = 0;

    //-----------------------------------------------------------------------------------
    //! \brief Reset callback function
    //! \details Invoked whenever the device is reset by the host. This function should
    //! perform initialization or re-initialization of the user application.
    //! Attention: Implementation of this callback is optional
    //! \see CUsbDriver::EventHandler
    //-----------------------------------------------------------------------------------
    virtual void OnReset( void ) = 0;

    //-----------------------------------------------------------------------------------
    //! \brief Suspend callback function
    //! \details Invoked when the device is suspended by the host or detached from the bus.
    //! If the device must enter low-power mode when suspended, then the necessary
    //! code must be implemented here.
    //! \see CUsbDriver::Attach
    //! \see CUsbDriver::EventHandler
    //-----------------------------------------------------------------------------------
    virtual void OnSuspend( void ) = 0;
    
    //-----------------------------------------------------------------------------------
    //! \brief Resume callback function
    //! \details Invoked when the device is resumed by the host or attached to the bus.
    //! If the suspend callback has put the device into low-power mode, then this
    //! function must perform the necessary actions to return it to a normal mode of
    //! operation.
    //! \see CEventSink::OnSuspend
    //! \see CUsbDriver::Attach
    //! \see CUsbDriver::EventHandler
    //-----------------------------------------------------------------------------------
    virtual void OnResume( void ) = 0;

    //-----------------------------------------------------------------------------------
    //! \brief New Request callback function
    //! \details Invoked when a new SETUP request is received. The request can then be
    //! retrieved by using the GetSetup function on the CUsbDriver instance.
    //! \see CUsbDriver::EventHandler
    //-----------------------------------------------------------------------------------
    virtual void OnNewRequest( void ) = 0;

    //-----------------------------------------------------------------------------------
    //! \brief  Interrupt SOF callback function
    //!         Invoked when a SOF interrupt is received.
    //! \see CUsbDriver::EventHandler
    //-----------------------------------------------------------------------------------
    virtual void OnStartOfFrame( void ) = 0;
};

//---------------------------------------------------------------------------------------
//! \brief Abstract class that declares board specific control of D+ pull up and 
//!        VBUS detection.
//---------------------------------------------------------------------------------------
class CBoard
{
public:

    //-----------------------------------------------------------------------------------
    //! \brief   Indicates the state of the VBus power line associated with the
    //!          specified interface.
    //! \return  true if VBus is detected, false otherwise
    //-----------------------------------------------------------------------------------
    virtual bool IsVBusConnected( void ) = 0;

    //-----------------------------------------------------------------------------------
    //! \brief   Enables the external pull-up on D+ associated with the specified
    //!          USB controller
    //-----------------------------------------------------------------------------------
    virtual void ConnectPullUp( void ) = 0;

    //-----------------------------------------------------------------------------------
    //! \brief   Disables the external pull-up on D+ associated with the specified
    //!          USB controller
    //-----------------------------------------------------------------------------------
    virtual void DisconnectPullUp( void ) = 0;

    //-----------------------------------------------------------------------------------
    //! \brief   Indicates the state of the external pull-up associated with the
    //!          specified interface.
    //! \return  true if the pull-up is currently connected, false otherwise.
    //-----------------------------------------------------------------------------------
    virtual bool IsPullUpConnected( void ) = 0;

    //-----------------------------------------------------------------------------------
    //! \brief   Configures the external pull-up on the D+ line associated with
    //!          the specified USB controller.
    //-----------------------------------------------------------------------------------
    virtual void ConfigurePullUp( void ) = 0;

    //-----------------------------------------------------------------------------------
    //! \brief   Configures the VBus monitoring PIO associated with the specified
    //!          USB controller.
    //-----------------------------------------------------------------------------------
    virtual void ConfigureVBus( void ) = 0;
};

//---------------------------------------------------------------------------------------
//! \brief This class is used to track the current status of an endpoint,
//!        i.e. the current transfer descriptors, the number of FIFO banks used,
//!        and so forth.
//! \details Each endpoint used by the firmware must have a corresponding CEndpoint
//! object instance associated with it.
//! \see CUsbDriver
//---------------------------------------------------------------------------------------
class CEndpoint
{
public:    
    //-----------------------------------------------------------------------------------
    //      Types
    //-----------------------------------------------------------------------------------
    
    enum STATE //!< Endpoint State Machine
    {
        StateDisabled = 0,
        StateIdle     = 1,
        StateWrite    = 2,
        StateRead     = 3,
        StateHalted   = 4
    };

    //-----------------------------------------------------------------------------------
    // Transfer Descriptor
    //-----------------------------------------------------------------------------------
    uchar*        pData;             //!< Transfer descriptor pointer to a 
                                     //!< buffer where the data is read/stored
    uchar*        pDataLowerBound;   //!< Circular buffer lower bound; pData >= lower bound
    uchar*        pDataUpperBound;   //!< Circular buffer upper bound; pData < upper bound
    uint          dBytesRemaining;   //!< Number of remaining bytes to transfer
    uint          dBytesBuffered;    //!< Number of bytes which have been buffered
                                     //!< but not yet transferred
    uint          dBytesTransferred; //!< Number of bytes transferred for the 
                                     //!< current operation
    bool          bCompletePacket;   //!< Send End-of-packet after sending 
                                     //!< data that was multiple of wMaxPacketSize.
                                     //!< If false next Write will continue the same packet.
    Callback_f    fCallback;         //!< Callback to invoke after the current 
                                     //!< transfer is complete
    void*         pArgument;         //!< Argument to pass to the callback function

    //-----------------------------------------------------------------------------------
    // Hardware Information
    //-----------------------------------------------------------------------------------
    uint          wMaxPacketSize;    //!< Maximum packet size for this endpoint
    uint          dFlag;             //!< Hardware flag to clear upon data reception
    uint          dNumFIFO;          //!< Number of FIFO buffers defined for this endpoint
    volatile uint dState;            //!< Endpoint internal state
    
    //-----------------------------------------------------------------------------------
    //! \brief Initialize endpoint with single/dualbank.
    //-----------------------------------------------------------------------------------
    CEndpoint( void )
    {
        TRACE_INFO( "CEndpoint %08x\n", uint( this ) );

        pData             = 0;
        pDataLowerBound   = 0;
        pDataUpperBound   = 0;
        dBytesRemaining   = 0;
        dBytesBuffered    = 0;
        dBytesTransferred = 0;
        bCompletePacket   = true;
        fCallback         = 0;
        pArgument         = 0;
        wMaxPacketSize    = 0;
        dFlag             = 0;
        dNumFIFO          = 0;
        dState            = StateDisabled;
    }
    
    //-----------------------------------------------------------------------------------
    //! \brief Initialize endpoint with single/dualbank.
    //! \param numFIFO  number of FIFO buffers: 1 for singlebank, 2 for dualbank
    //-----------------------------------------------------------------------------------
    void Init( int numFIFO )
    {
        dNumFIFO = numFIFO;
    }
    
    //-----------------------------------------------------------------------------------
    //! \brief  Invokes the callback associated with a finished transfer on an
    //!         endpoint
    //! \param  bStatus   Status code returned by the transfer operation
    //! \see    Status codes
    //-----------------------------------------------------------------------------------
    void EndOfTransfer( int bStatus )
    {
        if ( dState == CEndpoint::StateWrite || dState == CEndpoint::StateRead )
        {
            TRACE_DEBUG_L( "EoT " );

            //! Endpoint returns in Idle state
            //
            dState = CEndpoint::StateIdle;

            //! Invoke callback is present
            //
            if ( fCallback != 0 ) 
            {
                fCallback
                (
                     uint( pArgument ),
                     uint( bStatus ),
                     dBytesTransferred,
                     dBytesRemaining + dBytesBuffered
                     );
            }
        }
    }
};

//---------------------------------------------------------------------------------------
//! \brief Low-level USB device driver abstract class.
//! \details This class is used to provide an abstraction over which USB controller
//! is used by the chip. This means the USB framework is fully portable between
//! AT91 chips and supports chips with more than one controller.
//! The structure holds information about the USB controller used, such as
//! a pointer to the physical address of the peripheral, to the endpoints FIFO, etc.
//! In most case, it is not necessary to declare a CUsbDriver instance: the
//! defaultDriver global variable can be used. This is not possible for chips
//! which have more than one USB controller in them, where each controller must
//! have separate CUsbDriver instance.
//! \see CEventSink
//---------------------------------------------------------------------------------------
class CUsbDriver
{
protected:

    CEventSink*       pEventSink;      //!< Pointer to associated CEventSink instance
    CBoard*           pBoard;          //!< Pointer to associated CBoard instance
    CEndpoint*        pEndpoints;      //!< Endpoints list
    int               dNumEndpoints;   //!< Number of endpoints in list
    S_usb_request     sSetup;          //!< Pointer to the last received SETUP packet
    volatile uint     dState;          //!< Current state of the device
    bool              useSOFCallback;  //!< Indicates wether to forward StartOfFrame events

    //-----------------------------------------------------------------------------------
    //! \brief  Constructor. Just nullify/disable everything.
    //-----------------------------------------------------------------------------------
    CUsbDriver( void )
    {
        TRACE_INFO( "CUsbDriver %08x\n", uint( this ) );

        pEventSink     = 0;
        pBoard         = 0;
        pEndpoints     = 0;
        dNumEndpoints  = 0;
        dState         = 0;
        useSOFCallback = false;
    }
    
    //-----------------------------------------------------------------------------------
    //! \brief  Set flag(s) in dStatus register
    //-----------------------------------------------------------------------------------
    void SetState( uint flags )
    {
        dState |= flags;
    }
    //-----------------------------------------------------------------------------------
    //! \brief  Clear flag(s) in dStatus register.
    //!         If the method is called without arguments, it will set dState to 0.
    //-----------------------------------------------------------------------------------
    void ClearState( uint flags = 0xFFFF )
    {
        dState &= ~flags;
    }

public:
    //-----------------------------------------------------------------------------------
    //      Pure Virtual Methods
    //-----------------------------------------------------------------------------------

    //-----------------------------------------------------------------------------------
    //! \brief USB API Methods
    //-----------------------------------------------------------------------------------
    //! \details Methods provided by the USB API to manipulate a USB driver.
    //-----------------------------------------------------------------------------------

    //-----------------------------------------------------------------------------------
    //! \brief  Returns a pointer to the UDP controller interface used by an USB
    //!         driver. The pointer is cast to the correct type (AT91PS_UDP).
    //-----------------------------------------------------------------------------------
    virtual void* GetInterface( void ) = 0;
    
    //-----------------------------------------------------------------------------------
    //! \brief  Returns the USB controller peripheral ID of a CUsbDriver instance.
    //! \return USB controller peripheral ID
    //-----------------------------------------------------------------------------------
    virtual uint GetDriverID( void ) = 0;
    
    //-----------------------------------------------------------------------------------
    //! \brief  Initializes the USB API and the USB controller.
    //! \details This method must be called prior to using an other USB method. Before
    //! finishing, it invokes the CEventSink::OnInit callback.
    //! \see    CEventSink::OnInit
    //-----------------------------------------------------------------------------------
    virtual void Init( void ) = 0;

    //-----------------------------------------------------------------------------------
    //! \brief  Sends data through an USB endpoint.
    //! \details This method sends the specified amount of data through a particular
    //! endpoint. The transfer finishes either when the data has been completely
    //! sent, or an abnormal condition causes the API to abort this operation.
    //! An optional user-provided callback can be invoked once the transfer is
    //! complete.
    //! On control endpoints, this function automatically send a Zero-Length Packet
    //! (ZLP) if the data payload size is a multiple of the endpoint maximum packet
    //! size. This is not the case for bulk, interrupt or isochronous endpoints.
    //! \param  bEndpoint Number of the endpoint through which to send the data
    //! \param  pData     Pointer to a buffer containing the data to send
    //! \param  dLength   Size of the data buffer
    //! \param  fCallback Callback function to invoke when the transfer finishes
    //! \param  pArgument Optional parameter to pass to the callback function
    //! \param  pDataLowerBound Lower bound of the circular buffer
    //! \param  pDataUpperBound Upper bound of the circular buffer
    //! \return Result of operation
    //! \see    EnumStandardReturnValue
    //-----------------------------------------------------------------------------------
    virtual EnumStandardReturnValue Write
    ( 
        int bEndpoint,
        const void* pData, uint dLength,
        Callback_f fCallback = 0, void* pArgument = 0,
        const void* pDataLowerBound = 0, const void* pDataUpperBound = 0
        ) = 0;

    //-----------------------------------------------------------------------------------
    //! \brief  Receives data on the specified USB endpoint.
    //! \details This functions receives data on a particular endpoint. It finishes either
    //! when the provided buffer is full, when a short packet (payload size inferior
    //! to the endpoint maximum packet size) is received, or if an abnormal
    //! condition causes a transfer abort. An optional user-provided callback can be
    //! invoked upon the transfer completion
    //! \param  bEndpoint Number of the endpoint on which to receive the data
    //! \param  pData     Pointer to the buffer in which to store the received data
    //! \param  dLength   Size of the receive buffer
    //! \param  fCallback Optional user-provided callback function invoked upon the
    //!                   transfer completion
    //! \param  pArgument Optional parameter to pass to the callback function
    //! \return Result of operation
    //! \see    EnumStandardReturnValue
    //-----------------------------------------------------------------------------------
    virtual EnumStandardReturnValue Read
    (
        int bEndpoint,
        void* pData, uint dLength,
        Callback_f fCallback = 0, void* pArgument = 0
        ) = 0;

    //-----------------------------------------------------------------------------------
    //! \brief  Sends a STALL handshake for the next received packet.
    //! \details This function only send one STALL handshake, and only if the next packet
    //! is not a SETUP packet (when using this function on a control endpoint).
    //! \param  bEndpoint Number of endpoint on which to send the STALL
    //! \return Result of operation
    //! \see    EnumStandardReturnValue
    //! \see    CUsbDriver::Halt
    //-----------------------------------------------------------------------------------
    virtual EnumStandardReturnValue Stall( int bEndpoint = 0 ) = 0;

    //-----------------------------------------------------------------------------------
    //! \brief  Clears, sets or retrieves the halt state of the specified endpoint.
    //! \details While an endpoint is in Halt state, it acknowledges every received packet
    //! with a STALL handshake.
    //! \param  bEndpoint Number of the endpoint to alter
    //! \param  bRequest  The operation to perform (set, clear or get)
    //! \return true if the endpoint is halted, false otherwise
    //-----------------------------------------------------------------------------------
    virtual bool Halt( int bEndpoint, int bRequest ) = 0;

    //-----------------------------------------------------------------------------------
    //! \brief  Starts a remote wakeup procedure
    //-----------------------------------------------------------------------------------
    virtual void RemoteWakeUp( void ) = 0;

    //-----------------------------------------------------------------------------------
    //! \brief  Configures the specified endpoint using the provided endpoint
    //!         descriptor.
    //! \details An endpoint must be configured prior to being used. This is not necessary
    //! for control endpoint 0, as this operation is automatically performed during
    //! initialization.
    //! \param  pEpDesc Pointer to an endpoint descriptor
    //! \return true if the endpoint has been configured, false otherwise
    //! \see    S_usb_endpoint_descriptor
    //-----------------------------------------------------------------------------------
    virtual bool ConfigureEndpoint( const S_usb_endpoint_descriptor* pEpDesc ) = 0;

    //-----------------------------------------------------------------------------------
    //! \brief  Handles the attachment or detachment of the device to or from the USB
    //! \details This method should be called whenever the VBus power line changes state,
    //! i.e. the device becomes powered/unpowered. Alternatively, it can also be
    //! called to poll the status of the device.
    //! When the device is detached from the bus, the CEventSink::OnSuspend callback is
    //! invoked. Conversely, when the device is attached, the CEventSink::OnResume 
    //! callback is triggered.
    //! \return true if the device is currently attached, false otherwise
    //! \see    CEventSink::OnSuspend
    //! \see    CEventSink::OnResume
    //! \see    usb_api_callbacks
    //-----------------------------------------------------------------------------------
    virtual bool Attach( void ) = 0;

    //-----------------------------------------------------------------------------------
    //! \brief  Sets the device address using the last received SETUP packet.
    //! \details This method must only be called after a SET_ADDRESS standard request has
    //! been received. This is because it uses the last received SETUP packet stored
    //! in the sSetup structure to determine which address the device should use.
    //! \see    EnumStandardDeviceRequests
    //-----------------------------------------------------------------------------------
    virtual void SetAddress( void ) = 0;

    //-----------------------------------------------------------------------------------
    //! \brief  Sets the device configuration using the last received SETUP packet.
    //! \details This method must only be called after a SET_CONFIGURATION standard request
    //! has been received. This is necessary because it uses the last received
    //! SETUP packet (stored in the sSetup structure) to determine which
    //! configuration it should adopt.
    //! \see    EnumStandardDeviceRequests
    //-----------------------------------------------------------------------------------
    virtual void SetConfiguration( void ) = 0;

    //-----------------------------------------------------------------------------------
    //! \brief  Event handler for the USB controller peripheral.
    //! \details This function handles low-level events comming from the USB controller
    //! peripheral. It then dispatches those events through the user-provided
    //! callbacks. The following callbacks can be triggered:
    //!   -#  CEventSink::OnReset
    //!   -#  CEventSink::OnSuspend
    //!   -#  CEventSink::OnResume
    //!   -#  CEventSink::OnNewRequest
    //! \see usb_api_callbacks
    //-----------------------------------------------------------------------------------
    virtual void EventHandler( void ) = 0;

    //-----------------------------------------------------------------------------------
    //! \brief  Connects the device to the USB.
    //! \details This method enables the pull-up resistor on the D+ line, notifying the 
    //! host that the device wishes to connect to the bus.
    //! \see   CUsbDriver::Disconnect
    //-----------------------------------------------------------------------------------
    virtual void Connect( void ) = 0;

    //-----------------------------------------------------------------------------------
    //! \brief  Disconnects the device from the USB.
    //! This method disables the pull-up resistor on the D+ line, notifying the host
    //! that the device wishes to disconnect from the bus.
    //! \see   CUsbDriver::Connect
    //-----------------------------------------------------------------------------------
    virtual void Disconnect( void ) = 0;

public:
    //-----------------------------------------------------------------------------------
    //       Public Methods
    //-----------------------------------------------------------------------------------
    
    //-----------------------------------------------------------------------------------
    //! \brief  Link device driver to CEventSink instance so CUsbDriver methods
    //!         could send callbacks properly.
    //! \return Pointer to the CEventSink instance
    //-----------------------------------------------------------------------------------
    void LinkTo( CEventSink* pArgEventSink, CBoard* pArgBoard )
    {
        pEventSink = pArgEventSink;
        pBoard = pArgBoard;
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Returns a pointer to the last received SETUP request
    //! \return Pointer to the last received SETUP request
    //-----------------------------------------------------------------------------------
    S_usb_request* GetSetup( void )
    {
        return &sSetup;
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Poll the status of flags in dStatus register
    //! \return bool  true if flag(s) is/are set
    //-----------------------------------------------------------------------------------
    bool IsStateSet( uint flags ) const
    {
        return ( dState & flags ) == flags;
    }
    
    //-----------------------------------------------------------------------------------
    //! \brief  Poll the status of flag(s) in dStatus register
    //! \return bool  true if flag(s) is/are cleared
    //-----------------------------------------------------------------------------------
    bool IsStateCleared( uint flags ) const
    {
        return ( dState & flags ) == 0;
    }
    
    //-----------------------------------------------------------------------------------
    //! \brief  Returns a number of configured endpoints
    //! \return uint   number of configured endpoints
    //-----------------------------------------------------------------------------------
    int GetNumEndpoints( void ) const
    {
        return dNumEndpoints;
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Establish the list of configured endpoints
    //-----------------------------------------------------------------------------------
    void SetEndpointList( CEndpoint* list, int count )
    {
        pEndpoints     = list;
        dNumEndpoints  = count;
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Sends a Zero-Length Packet (ZLP) through the Control endpoint 0.
    //! \details Since sending a ZLP on endpoint 0 is quite common, this function is 
    //! provided as an overload to the CUsbDriver::Write function.
    //! \param  fCallback Optional callback function to invoke when the transfer
    //!                   finishes
    //! \param  pArgument Optional parameter to pass to the callback function
    //! \return Result of operation
    //! \see    EnumStandardReturnValue
    //-----------------------------------------------------------------------------------
    EnumStandardReturnValue SendZLP0( Callback_f fCallback = 0, void* pArgument = 0 ) 
    {
        return Write( 0, 0, 0, fCallback, pArgument );
    }
};

//---------------------------------------------------------------------------------------
//      Macros
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
//      Structures and Classes
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
//! \brief   List of standard descriptors used by the device
//---------------------------------------------------------------------------------------
struct S_std_descriptors
{
    const S_usb_device_descriptor*        pDevice;        //!< Device descriptor
    const S_usb_configuration_descriptor* pConfiguration; //!< Configuration descriptor
    const char**                          pStrings;       //!< List of string descriptors
    const S_usb_endpoint_descriptor**     pEndpoints;     //!< List of endpoint descriptors
    
#if defined(HIGHSPEED)
    //!< Qualifier descriptor (high-speed only)
    const S_usb_device_qualifier_descriptor* pQualifier; 
    //!< Other speed configuration descriptor (high-speed only)
    const S_usb_configuration_descriptor*    pOtherSpeedConfiguration; 
#endif
};

//---------------------------------------------------------------------------------------
//! \brief   USB standard class driver structure.
//! \details Used to provide standard driver information so external modules can
//!          still access an internal driver.
//---------------------------------------------------------------------------------------
class CSTD
{
    //-----------------------------------------------------------------------------------
    //! wDeviceStatus flags: Information Returned by a GetStatus() Request to a Device.
    //! \see_usb20 - Section 9.4.5 - Figure 9-4
    //-----------------------------------------------------------------------------------
    enum // wDeviceStatus flags
    {
        SELF_POWERED          = (1 << 0),
        REMOTE_WAKEUP         = (1 << 1)
    };

protected:

    CUsbDriver*              pDriver;       //!< Pointer to usb driver instance
    const S_std_descriptors* pDescriptors;  //!< Pointer to the list of descriptors 
                                            //!< used by the device
    ushort                   wDeviceStatus; //!< Data buffer used for information returned 
                                            //!< by a GetStatus() request to a Device
                                            //!< \see_usb20 Figure 9-4
    ushort                   wData;         //!< Temporary data buffer used by
                                            //!< pDriver->Write()

private:
    
    //-----------------------------------------------------------------------------------
    //! \brief  Callback for the SetAddress usb request.
    //! \see    CSTD::RequestHandler
    //-----------------------------------------------------------------------------------
    static void OnSetAddress( CSTD* pThis )
    {
        pThis->pDriver->SetAddress ();
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Configures the device and the endpoints
    //-----------------------------------------------------------------------------------
    void ConfigureEndpoints( void )
    {
        // Enter the Configured state
        //
        pDriver->SetConfiguration ();

        // Configure endpoints
        // TODO: Why this works for GetNumEndpoints()-1 and not
        // for GetNumEndpoints() when windows is getting out stand-by mode?
        //
        for ( int i = 0; i < pDriver->GetNumEndpoints () - 1; i++ )
        {
            pDriver->ConfigureEndpoint( pDescriptors->pEndpoints[ i ] );
        }
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Callback for the SetConfiguration usb request.
    //!         Configures the device and the endpoints
    //! \see    CSTD::SetConfiguration, CSTD::ConfigureEndpoints
    //-----------------------------------------------------------------------------------
    static void OnConfigureEndpoints( CSTD* pThis )
    {
        pThis->ConfigureEndpoints ();
    }
    
    //-----------------------------------------------------------------------------------
    //! \brief  Sends a zero-length packet and starts the configuration procedure.
    //! \param  bConfiguration  Newly selected configuration
    //-----------------------------------------------------------------------------------
    void SetConfiguration( int bConfiguration )
    {
        pDriver->SendZLP0( Callback_f( OnConfigureEndpoints ), this );
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Sends the currently selected configuration to the host.
    //-----------------------------------------------------------------------------------
    void GetConfiguration( void )
    {
        wData = pDriver->IsStateSet( USB_STATE_CONFIGURED ) ? 1 : 0; 
        pDriver->Write( 0, &wData, 1 );
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Sends the current device status to the host.
    //-----------------------------------------------------------------------------------
    void GetDeviceStatus( void )
    {
        // Bus or self-powered ?
        //
        if ( ISSET( pDescriptors->pConfiguration->bmAttibutes, USB_CONFIG_SELF_POWERED ) ) 
        {
            wDeviceStatus |= SELF_POWERED;   // Self powered device
        }
        else 
        {
            wDeviceStatus &= ~SELF_POWERED;  // Bus powered device
        }

        // Return the device status
        //
        pDriver->Write( 0, &wDeviceStatus, 2 );
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Sends the current status of specified endpoint to the host.
    //! \param  bEndpoint Endpoint number
    //-----------------------------------------------------------------------------------
    void GetEndpointStatus( int bEndpoint )
    {
        // Retrieve the endpoint current status
        //
        wData = ushort( pDriver->Halt( bEndpoint, USB_GET_STATUS ) );

        // Return the endpoint status
        //
        pDriver->Write( 0, &wData, 2 );
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Sends the device descriptor to the host.
    //! \details The number of bytes actually sent depends on both the length
    //! requested by the host and the actual length of the descriptor.
    //! \param  wLength Number of bytes requested by the host
    //-----------------------------------------------------------------------------------
    void GetDeviceDescriptor( ushort wLength )
    {
        pDriver->Write
        ( 
            0, // endpoint
            pDescriptors->pDevice, // ptr to data
            min( sizeof(S_usb_device_descriptor), wLength ) // data length
            );
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Sends the configuration descriptor to the host.
    //! \details The number of bytes actually sent depends on both the length
    //! requested by the host and the actual length of the descriptor.
    //! \param  wLength Number of bytes requested by the host
    //-----------------------------------------------------------------------------------
    void GetConfigurationDescriptor( ushort wLength )
    {
        pDriver->Write
        (
             0, // endpoint
             pDescriptors->pConfiguration, // ptr to data
             min( pDescriptors->pConfiguration->wTotalLength, wLength ) // data length
             );
    }

    #if defined(HIGHSPEED)
    //-----------------------------------------------------------------------------------
    //! \brief  Sends the qualifier descriptor to the host.
    //! \details The number of bytes actually sent depends on both the length
    //! requested by the host and the actual length of the descriptor.
    //! \param  wLength Number of bytes requested by the host
    //-----------------------------------------------------------------------------------
    void GetQualifierDescriptor( ushort wLength )
    {
        pDriver->Write
        (
            0, // endpoint
            pDescriptors->pQualifier, // ptr to data
            min( pDescriptors->pQualifier->bLength, wLength ) // data length
            );
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Sends the other speed configuration descriptor to the host.
    //! \details The number of bytes actually sent depends on both the length
    //! requested by the host and the actual length of the descriptor.
    //! \param  wLength Number of bytes requested by the host
    //-----------------------------------------------------------------------------------
    void GetOSCDescriptor( ushort wLength )
    {
        pDriver->Write
        (
             0, // end point
             pDescriptors->pOtherSpeedConfiguration, // ptr to data
             min( pDescriptors->pOtherSpeedConfiguration->wTotalLength, wLength ) // len
             );
    }
    #endif

    //-----------------------------------------------------------------------------------
    //! \brief  Sends the specified string descriptor to the host
    //! \details The number of bytes actually sent depends on both the length
    //! requested by the host and the actual length of the descriptor.
    //! \param  wLength Number of bytes requested by the host
    //! \param  bIndex  Index of requested string descriptor
    //-----------------------------------------------------------------------------------
    void GetStringDescriptor( ushort wLength, int bIndex )
    {
        pDriver->Write
        (
            0, // endpoint
            pDescriptors->pStrings[ bIndex ], // ptr to data
            min( *pDescriptors->pStrings[ bIndex ], wLength ) // data length
            );
    }
    
protected:

    //-----------------------------------------------------------------------------------
    //! \brief  Handles standard SETUP requests
    //-----------------------------------------------------------------------------------
    virtual void RequestHandler( void );

    //-----------------------------------------------------------------------------------
    //! \brief  Constructor. Connects object to CUsbDriver instance.
    //! \details Note that this constructor is not public, which means that CSTD objects 
    //! can not be instantiated. However, derived class objects could be instantiated 
    //! normally.
    //-----------------------------------------------------------------------------------
    CSTD( CUsbDriver* usbDriver, CBoard* board, CEventSink* eventSink )
    {
        pDriver = usbDriver;
        pDriver->LinkTo( eventSink, board );
    }

public:

    //-----------------------------------------------------------------------------------
    //! \brief  Low-level usb driver interrupt handler
    //-----------------------------------------------------------------------------------
    void EventHandler( void )
    {
        pDriver->EventHandler ();
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Initializes the USB API and the USB controller.
    //-----------------------------------------------------------------------------------
    void Init( void )
    {
        pDriver->Init ();
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Return true if device is powered 
    //-----------------------------------------------------------------------------------
    bool IsPowered( void ) const
    {
        return pDriver->IsStateSet( USB_STATE_POWERED );
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Connect to device. 
    //-----------------------------------------------------------------------------------
    void Connect( void )
    {
        pDriver->Connect ();
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Attach device to USB bus. 
    //-----------------------------------------------------------------------------------
    void Attach( void )
    {
        pDriver->Attach ();
    }
};

//---------------------------------------------------------------------------------------
//      Exported symbols
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
//! Default USB driver for the current chip
//---------------------------------------------------------------------------------------
extern CUsbDriver& sDefaultUsbDriver;

//---------------------------------------------------------------------------------------
} // namespace USB
//---------------------------------------------------------------------------------------

#endif // _USB_FRAMEWORK_H_INCLUDED

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
$Id: usb.h 121 2006-10-17 12:54:54Z jjoannic $
$Id: standard.h 108 2006-10-16 08:33:33Z jjoannic $
*/
