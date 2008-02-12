//---------------------------------------------------------------------------------------
//      Includes
//---------------------------------------------------------------------------------------

#include "usbFramework.hpp"
#include "usbCDC.hpp"

#include "device.h"
#include "board.h"

using namespace USB;

//---------------------------------------------------------------------------------------
//      External References
//---------------------------------------------------------------------------------------

extern void ISR_Wrapper_USB( void );
extern void ISR_Wrapper_VBus( void );

//---------------------------------------------------------------------------------------
//      Forward reference to exported object
//---------------------------------------------------------------------------------------

extern CCDC sSer;

//---------------------------------------------------------------------------------------
namespace { // UNNAMED
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
// Handler for the VBus state change interrupt
// This method calls the CUsbDriver::Attach function to perform the necessary
// operations.
//---------------------------------------------------------------------------------------

void ISR_VBus(void)
{
    sSer.Attach ();

    // Acknowledge the interrupt
    //
    AT91F_PIO_GetInterruptStatus( AT91C_PIO_VBUS );
    AT91F_AIC_AcknowledgeIt( AT91C_BASE_AIC );
    }

//---------------------------------------------------------------------------------------
//      Structures and Classes
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
// Event sink implementation for the sSer CCDC instance.
//---------------------------------------------------------------------------------------
class CCallbacks: public CEventSink
{
    CUsbDriver* pDriver;

    //-----------------------------------------------------------------------------------
    // \brief Initialization callback function
    // This callback is invoked whenever the USB API is initialized using the
    // USB_Init function. It should perform the following operations:
    //  -  If an OS is being used, install the USB driver
    //  -  Configure the USB controller interrupt
    //  -  Configure the VBus monitoring interrupt
    // Attention: Implementation of this callback is mandatory
    // \see CUsbDriver::Init
    //-----------------------------------------------------------------------------------
    void OnInit( void );

    //-----------------------------------------------------------------------------------
    // \brief Reset callback function
    // Invoked whenever the device is reset by the host. This function should
    // perform initialization or re-initialization of the user application.
    // Attention: Implementation of this callback is optional
    // \see CUsbDriver::EventHandler
    //-----------------------------------------------------------------------------------
    void OnReset( void );

    //-----------------------------------------------------------------------------------
    // \brief Suspend callback function
    // Invoked when the device is suspended by the host or detached from the bus.
    // If the device must enter low-power mode when suspended, then the necessary
    // code must be implemented here.
    // \see CUsbDriver::Attach
    // \see CUsbDriver::EventHandler
    //-----------------------------------------------------------------------------------
    void OnSuspend( void );
    
    //-----------------------------------------------------------------------------------
    // \brief Resume callback function
    // Invoked when the device is resumed by the host or attached to the bus. If
    // the suspend callback has put the device into low-power mode, then this
    // function must perform the necessary actions to return it to a normal mode of
    // operation.
    // \see CEventSink::OnSuspend
    // \see CUsbDriver::Attach
    // \see CUsbDriver::Handler
    //-----------------------------------------------------------------------------------
    void OnResume( void );

    //-----------------------------------------------------------------------------------
    // \brief New Request callback function
    // Invoked when a new SETUP request is received. The request can then be
    // retrieved by using the USB_GetSetup function on the CUsbObject instance.
    // \see EventHandler
    //-----------------------------------------------------------------------------------
    void OnNewRequest( void );

    //-----------------------------------------------------------------------------------
    // \brief  Interrupt SOF callback function
    //         Invoked when a SOF interrupt is received.
    // \see    CUsbDriver::EventHandler
    //-----------------------------------------------------------------------------------
    void OnStartOfFrame( void );
    
public:
    //-----------------------------------------------------------------------------------
    // \brief  Constructor. Just reports initialization.
    //-----------------------------------------------------------------------------------
    CCallbacks( CUsbDriver* pArgDriver )
    {
        pDriver = pArgDriver;

        TRACE_INFO( "CCallback %08x, Driver=%08x\n", uint( this ), pDriver );
    }        
};

//---------------------------------------------------------------------------------------
// Callback invoked during the initialization of the USB driver
// Configures and enables USB controller and VBus monitoring interrupts
//---------------------------------------------------------------------------------------
void CCallbacks::OnInit( void )
{
    TRACE_DEBUG_M( "OnInit\n" );
    
    // Configure and enable the USB controller interrupt
    //
    AT91F_AIC_ConfigureIt
    (
        AT91C_BASE_AIC,
        pDriver->GetDriverID (),
        AT91C_AIC_PRIOR_LOWEST,
        0, //AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL,
        ISR_Wrapper_USB
        );

    AT91F_AIC_EnableIt( AT91C_BASE_AIC, pDriver->GetDriverID () );

#ifdef USB_SELF_POWERED

    // Configure and enable the Vbus detection interrupt
    AT91F_AIC_ConfigureIt
    (
        AT91C_BASE_AIC,
        AT91C_ID_VBUS,
        AT91C_AIC_PRIOR_LOWEST,
        0, //AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL,
        ISR_Wrapper_VBus
        );

    AT91F_PIO_InterruptEnable( AT91C_PIO_VBUS, AT91C_VBUS );
    AT91F_AIC_EnableIt( AT91C_BASE_AIC, AT91C_ID_VBUS );
#else
    // Power up the USB controller
    pDriver->Attach ();
#endif
    }

//---------------------------------------------------------------------------------------
// Callback invoked when the device becomes suspended
// Disables LEDs (if they are used) and then puts the device into
// low-power mode. When traces are used, the device does not enter
// low-power mode to avoid losing some outputs.
//---------------------------------------------------------------------------------------
void CCallbacks::OnSuspend( void )
{
    TRACE_DEBUG_M( "OnSuspend\n" );

    // DEV_Suspend ();
    }

//---------------------------------------------------------------------------------------
// Callback invoked when the device leaves the suspended state
// The device is first returned to a normal operating mode and LEDs are
// re-enabled. When traces are used, the device does not enter
// low-power mode to avoid losing some outputs.
//---------------------------------------------------------------------------------------
void CCallbacks::OnResume( void )
{
    TRACE_DEBUG_M( "OnResume\n" );
    
    // DEV_Resume ();
    }

//---------------------------------------------------------------------------------------
// Callback invoked when a new SETUP request is received
// The new request if forwarded to the standard request handler,
// which performs the enumeration of the device.
//---------------------------------------------------------------------------------------
void CCallbacks::OnNewRequest( void )
{
    sSer.RequestHandler ();
    }

//---------------------------------------------------------------------------------------
// Callback invoked when a Reset request is received
//---------------------------------------------------------------------------------------
void CCallbacks::OnReset( void )
{
    TRACE_DEBUG_M( "OnReset\n" );
}

//---------------------------------------------------------------------------------------
// Callback invoked when a SOF is received
//---------------------------------------------------------------------------------------
void CCallbacks::OnStartOfFrame( void )
{
    }

//---------------------------------------------------------------------------------------
// Class describing how to handle PullUp and VBus of the board.
//---------------------------------------------------------------------------------------
class CXpuBoard : public CBoard
{
    //-----------------------------------------------------------------------------------
    // \brief   Indicates the state of the VBus power line associated with the
    //          specified interface.
    // \return  true if VBus is detected, false otherwise
    //-----------------------------------------------------------------------------------
    bool IsVBusConnected( void )
    {
#ifdef USB_SELF_POWERED
        return ISSET( AT91F_PIO_GetInput( AT91C_PIO_VBUS ), AT91C_VBUS ); 
#else
        return true;
#endif
    }

    //-----------------------------------------------------------------------------------
    // \brief   Enables the external pull-up on D+ associated with the specified
    //          USB controller
    //-----------------------------------------------------------------------------------
    void ConnectPullUp( void )
    {
        AT91F_PIO_ClearOutput( AT91C_PIO_PULLUP, AT91C_PULLUP );
    }

    //-----------------------------------------------------------------------------------
    // \brief   Disables the external pull-up on D+ associated with the specified
    //          USB controller
    //-----------------------------------------------------------------------------------
    void DisconnectPullUp( void )
    {
        AT91F_PIO_SetOutput( AT91C_PIO_PULLUP, AT91C_PULLUP );
    }

    //-----------------------------------------------------------------------------------
    // \brief   Indicates the state of the external pull-up associated with the
    //          specified interface.
    // \return  true if the pull-up is currently connected, false otherwise.
    //-----------------------------------------------------------------------------------
    bool IsPullUpConnected( void )
    {
        return ISSET(AT91F_PIO_GetInput( AT91C_PIO_PULLUP ), AT91C_PULLUP ); 
    }

    //-----------------------------------------------------------------------------------
    // \brief   Configures the external pull-up on the D+ line associated with
    //          the specified USB controller.
    //-----------------------------------------------------------------------------------
    void ConfigurePullUp( void )
    {
        AT91F_PMC_EnablePeriphClock( AT91C_BASE_PMC, 1 << AT91C_ID_PULLUP );
        AT91F_PIO_SetOutput( AT91C_PIO_PULLUP, AT91C_PULLUP ); // disconnect
        AT91F_PIO_CfgOutput( AT91C_PIO_PULLUP, AT91C_PULLUP );
    }
    
    //-----------------------------------------------------------------------------------
    // \brief   Configures the VBus monitoring PIO associated with the specified
    //          USB controller.
    //-----------------------------------------------------------------------------------
    void ConfigureVBus( void )
    {
#ifdef USB_SELF_POWERED
        AT91F_PMC_EnablePeriphClock( AT91C_BASE_PMC, 1 << AT91C_ID_VBUS );
        AT91C_PIO_VBUS->PIO_PPUDR = AT91C_VBUS; // disable pullup
        AT91F_PIO_CfgInput( AT91C_PIO_VBUS, AT91C_VBUS );
#endif
    }
};

//---------------------------------------------------------------------------------------
// One and only object instance. Note that it belongs to unnamed namespace and thus
// it is not visible outside.
//---------------------------------------------------------------------------------------

CXpuBoard sBoard;
CCallbacks sCallbacks( &sDefaultUsbDriver );

//---------------------------------------------------------------------------------------
} // UNNAMED namespace
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
//      Exported Symbols
//---------------------------------------------------------------------------------------

CCDC sSer( &sDefaultUsbDriver, &sBoard, &sCallbacks );
