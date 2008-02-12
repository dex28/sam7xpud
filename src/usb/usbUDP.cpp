
//---------------------------------------------------------------------------------------
//      Includes
//---------------------------------------------------------------------------------------

#include "usbFramework.hpp"

#include "device.h"
#include "board.h"
#include "trace.h"

using namespace USB;

//---------------------------------------------------------------------------------------
//      Structures and Classes
//---------------------------------------------------------------------------------------

class CUdpDriver : public CUsbDriver
{
    enum //! Constants
    {
        UDP_STATE_SHOULD_RECONNECT      = 0x10000000,
        UDP_EPTYPE_INDEX                = 8,
        UDP_EPDIR_INDEX                 = 10,

        ISR_MASK                        = 0x00003FFF
    };

    //-----------------------------------------------------------------------------------
    //      Members
    //-----------------------------------------------------------------------------------

    AT91PS_UDP  pInterface;   //!< Pointer to the USB controller peripheral
    uint        dID;          //!< ID of the USB controller peripheral
    uint        dPMC;         //!< ID to enable the USB controller peripheral clock

public:
    //-----------------------------------------------------------------------------------
    //      Public Methods
    //-----------------------------------------------------------------------------------

    //-----------------------------------------------------------------------------------
    //! \brief  UDP driver object constructor.
    //! \param  controller  Pointer to the USB controller peripheral
    //! \param  ctrlID      ID of the USB controller peripheral
    //! \param  ctrlPMC     ID to enable the USB controller peripheral clock
    //-----------------------------------------------------------------------------------
    CUdpDriver( AT91PS_UDP controller, uint ctrlID, uint ctrlPMC )
        : CUsbDriver ()
    {
        pInterface = controller;
        dID        = ctrlID;
        dPMC       = ctrlPMC;
    };
    
private:
    //-----------------------------------------------------------------------------------
    //      Internal Methods
    //-----------------------------------------------------------------------------------

    //-----------------------------------------------------------------------------------
    //! \brief  Clear flags in the UDP_CSR register and waits for synchronization
    //! \param  bEndpoint Index of endpoint
    //! \param  dFlags    Flags to clear
    //-----------------------------------------------------------------------------------
    inline void ClearEndpointFlags( int bEndpoint, uint dFlags )
    {
        AT91_REG& UDP_CSR = pInterface->UDP_CSR[ bEndpoint ];
        
        if ( ( UDP_CSR & dFlags ) == 0 )
            return;

        UDP_CSR &= ~dFlags;

        // WARNING: Due to synchronization between MCK and UDPCK, the software 
        // application must wait for the end of the write operation before executing 
        // another write by polling the bits which must be set/cleared.
        // \see Section 35.6.10 UDP Endpoint CSR in AT91SAM7 datasheet (doc6175.pdf)
        //
        do ; while( ( UDP_CSR & dFlags ) != 0 );

        // Note: In a preemptive environment, set or clear the flag and wait for a time 
        // of 1 UDPCK clock cycle and 1 peripheral clock cycle. However, RX_DATA_BK0, 
        // TXPKTRDY, RX_DATA_BK1 require wait times of 3 UDPCK clock cycles and 3 
        // peripheral clock cycles before accessing DPR.
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Set flags in the UDP_CSR register and waits for synchronization
    //! \param  bEndpoint Index of endpoint
    //! \param  dFlags    Flags to clear
    //-----------------------------------------------------------------------------------
    inline void SetEndpointFlags( int bEndpoint, uint dFlags )
    {
        AT91_REG& UDP_CSR = pInterface->UDP_CSR[ bEndpoint ];

        if ( ( UDP_CSR & dFlags ) == dFlags )
            return;

        UDP_CSR |= dFlags;

        // WARNING: Due to synchronization between MCK and UDPCK, the software 
        // application must wait for the end of the write operation before executing 
        // another write by polling the bits which must be set/cleared.
        // \see Section 35.6.10 UDP Endpoint CSR in AT91SAM7 datasheet (doc6175.pdf)
        //
        do ; while ( ( UDP_CSR & dFlags ) != dFlags );

        // Note: In a preemptive environment, set or clear the flag and wait for a time 
        // of 1 UDPCK clock cycle and 1 peripheral clock cycle. However, RX_DATA_BK0, 
        // TXPKTRDY, RX_DATA_BK1 require wait times of 3 UDPCK clock cycles and 3 
        // peripheral clock cycles before accessing DPR.
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Enables the peripheral clock of the USB controller associated with
    //!         the specified USB driver
    //-----------------------------------------------------------------------------------
    inline void EnableMCK( void )
    {
        AT91C_BASE_PMC->PMC_PCER = 1 << dID;
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Disables the peripheral clock of the USB controller associated with
    //!         the specified USB driver
    //-----------------------------------------------------------------------------------
    inline void DisableMCK( void )
    {
        AT91C_BASE_PMC->PMC_PCDR = 1 << dID;
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Enables the 48MHz clock of the USB controller associated with
    //!         the specified USB driver
    //-----------------------------------------------------------------------------------
    inline void EnableUDPCK( void )
    {
        SET( AT91C_BASE_PMC->PMC_SCER, dPMC );
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Disables the 48MHz clock of the USB controller associated with
    //!         the specified USB driver
    //-----------------------------------------------------------------------------------
    inline void DisableUDPCK( void )
    {
        SET( AT91C_BASE_PMC->PMC_SCDR, dPMC );
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Enables the transceiver of the USB controller associated with
    //!         the specified USB driver
    //-----------------------------------------------------------------------------------
    inline void EnableTransceiver( void )
    {
        CLEAR( pInterface->UDP_TXVC, AT91C_UDP_TXVDIS );
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Disables the transceiver of the USB controller associated with
    //!         the specified USB driver
    //-----------------------------------------------------------------------------------
    inline void DisableTransceiver( void )
    {
        SET( pInterface->UDP_TXVC, AT91C_UDP_TXVDIS );
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Clears the correct RX flag in an endpoint status register
    //! \param  bEndpoint Index of endpoint
    //! \see    CEndpoint
    //-----------------------------------------------------------------------------------
    void ClearRXFlag( int bEndpoint );

    //-----------------------------------------------------------------------------------
    //! \brief  Transfers a data payload from the current tranfer buffer to the
    //!         endpoint FIFO.
    //! \param  bEndpoint Index of endpoint
    //! \return Number of bytes transferred
    //-----------------------------------------------------------------------------------
    uint WritePayload( int bEndpoint )
    {
        CEndpoint* pEndpoint = &pEndpoints[ bEndpoint ];
        AT91_REG& FDR = pInterface->UDP_FDR[ bEndpoint ];

        // Get the number of bytes to send
        //
        uint dBytes = min( pEndpoint->wMaxPacketSize, pEndpoint->dBytesRemaining );

        // Are we doing flat transfer or transfer across circular buffer upper boundary?
        //
        if ( ! pEndpoint->pDataLowerBound 
             || pEndpoint->pData + dBytes < pEndpoint->pDataUpperBound 
            ) 
        {
            // Send data from the flat buffer (or inner part of circular buffer)
            //
            for( uint dCtr = 0; dCtr < dBytes; dCtr++ )
                FDR = *pEndpoint->pData++;
        }
        else 
        {
            // Send data by crossing upper boundary of the circular buffer. Not so often. 
            //
            uint len2 = pEndpoint->pData + dBytes - pEndpoint->pDataUpperBound;
            uint len1 = dBytes - len2;

            for( uint i = 0; i < len1; i++ )
                FDR = *pEndpoint->pData++;

            pEndpoint->pData = pEndpoint->pDataLowerBound;

            for( uint i = 0; i < len2; i++ )
                FDR = *pEndpoint->pData++;
        }

        pEndpoint->dBytesBuffered  += dBytes;
        pEndpoint->dBytesRemaining -= dBytes;

        return dBytes;
    }

    //-----------------------------------------------------------------------------------
    //! \brief  Transfers a data payload from an endpoint FIFO to the current
    //!         transfer buffer.
    //! \param  bEndpoint   Index of endpoint
    //! \param  wPacketSize Size of received data packet
    //! \return Number of bytes transferred
    //-----------------------------------------------------------------------------------
    uint GetPayload( int bEndpoint, ushort wPacketSize )
    {
        CEndpoint* pEndpoint = &pEndpoints[ bEndpoint ];
        AT91_REG& FDR = pInterface->UDP_FDR[ bEndpoint ];

        TRACE_DEBUG_L( "%d ", wPacketSize );

        // Get number of bytes to retrieve
        //
        uint dBytes = min( pEndpoint->dBytesRemaining, wPacketSize );

        // Retrieve packet
        //
        for( uint dCtr = 0; dCtr < dBytes; dCtr++ )
        {
            *pEndpoint->pData++ = uchar( FDR );
        }

        pEndpoint->dBytesRemaining   -= dBytes;
        pEndpoint->dBytesTransferred += dBytes;

        // For read operation dBytesBuffered contains number of bytes remaining 
        // in the FIFO, i.e. bytes in last packet that are received but not read.
        // Next Read() operation should return them first.
        //
        pEndpoint->dBytesBuffered = wPacketSize - dBytes;
        
        TRACE_DEBUG_L( "(fifo %d, remain %d) ", 
                       pEndpoint->dBytesBuffered, pEndpoint->dBytesRemaining );

        return dBytes;
    }

    //-----------------------------------------------------------------------------------
    //! \brief  This function reset all endpoint transfer descriptors
    //-----------------------------------------------------------------------------------
    void ResetEndpoints( void );

    //-----------------------------------------------------------------------------------
    //! \brief  Disable all endpoints (except control endpoint 0), aborting current
    //!         transfers if necessary.
    //-----------------------------------------------------------------------------------
    void DisableEndpoints( void );

    //-----------------------------------------------------------------------------------
    //! \brief  Endpoint interrupt handler.
    //!         Handle IN/OUT transfers, received SETUP packets and STALLing
    //! \param  bEndpoint Index of endpoint
    //-----------------------------------------------------------------------------------
    void EndpointHandler( int bEndpoint );
    
private:
    //-----------------------------------------------------------------------------------
    //      Virtual Methods
    //-----------------------------------------------------------------------------------

    // NOTE: Virtual methods are here declared as private, but they are still accessible 
    // as public through the CUsbDevice base class!
    
    //-----------------------------------------------------------------------------------
    //! \brief  Returns a pointer to the UDP controller interface used by an USB
    //!         driver. The pointer is cast to the correct type (AT91PS_UDP).
    //-----------------------------------------------------------------------------------
    void* GetInterface( void )
    {
        return pInterface;
    }
    
    //-----------------------------------------------------------------------------------
    //! \brief  Returns the USB controller peripheral ID of a CUdpDriver instance.
    //! \return USB controller peripheral ID
    //-----------------------------------------------------------------------------------
    uint GetDriverID( void )
    {
        return dID;
    }
    
    //-----------------------------------------------------------------------------------
    //! \brief  Initializes the USB API and the USB controller.
    //! \details This method must be called prior to using an other USB method. Before
    //! finishing, it invokes the OnInit callback.
    //! \see    OnInit
    //-----------------------------------------------------------------------------------
    void Init( void );

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
    //! \return Result of operation
    //! \see    EnumStandardReturnValue
    //-----------------------------------------------------------------------------------
    EnumStandardReturnValue Write
    ( 
        int bEndpoint,
        const void* pData, uint dLength,
        Callback_f fCallback, void* pArgument, 
        const void* pDataLowerBound, const void* pDataUpperBound
        );

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
    EnumStandardReturnValue Read
    (
        int bEndpoint,
        void* pData, uint dLength,
        Callback_f fCallback, void* pArgument
        );

    //-----------------------------------------------------------------------------------
    //! \brief  Sends a STALL handshake for the next received packet.
    //! \details This function only send one STALL handshake, and only if the next packet
    //! is not a SETUP packet (when using this function on a control endpoint).
    //! \param  bEndpoint Number of endpoint on which to send the STALL
    //! \return Result of operation
    //! \see    EnumStandardReturnValue
    //! \see    CUsbDriver::Halt
    //-----------------------------------------------------------------------------------
    EnumStandardReturnValue Stall( int bEndpoint );

    //-----------------------------------------------------------------------------------
    //! \brief  Clears, sets or retrieves the halt state of the specified endpoint.
    //! \details While an endpoint is in Halt state, it acknowledges every received packet
    //! with a STALL handshake.
    //! \param  bEndpoint Number of the endpoint to alter
    //! \param  bRequest  The operation to perform (set, clear or get)
    //! \return true if the endpoint is halted, false otherwise
    //-----------------------------------------------------------------------------------
    bool Halt( int bEndpoint, int bRequest );

    //-----------------------------------------------------------------------------------
    //! \brief  Starts a remote wakeup procedure
    //-----------------------------------------------------------------------------------
    void RemoteWakeUp( void );

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
    bool ConfigureEndpoint( const S_usb_endpoint_descriptor* pEpDesc );

    //-----------------------------------------------------------------------------------
    //! \brief  Handles the attachment or detachment of the device to or from the USB
    //! \details This method should be called whenever the VBus power line changes state,
    //! i.e. the device becomes powered/unpowered. Alternatively, it can also be
    //! called to poll the status of the device.
    //! When the device is detached from the bus, the OnSuspend callback is
    //! invoked. Conversely, when the device is attached, the OnResume callback
    //! is triggered.
    //! \return true if the device is currently attached, false otherwise
    //! \see    OnSuspend
    //! \see    OnResume
    //! \see    usb_api_callbacks
    //-----------------------------------------------------------------------------------
    bool Attach( void );

    //-----------------------------------------------------------------------------------
    //! \brief  Sets the device address using the last received SETUP packet.
    //! This method must only be called after a SET_ADDRESS standard request has
    //! been received. This is because it uses the last received SETUP packet stored
    //! in the sSetup structure to determine which address the device should use.
    //! \see    EnumStandardDeviceRequests
    //-----------------------------------------------------------------------------------
    void SetAddress( void );

    //-----------------------------------------------------------------------------------
    //! \brief  Sets the device configuration using the last received SETUP packet.
    //! \details This method must only be called after a SET_CONFIGURATION standard 
    //! request has been received. This is necessary because it uses the last received
    //! SETUP packet (stored in the sSetup structure) to determine which
    //! configuration it should adopt.
    //! \see    EnumStandardDeviceRequests
    //-----------------------------------------------------------------------------------
    void SetConfiguration( void );

    //-----------------------------------------------------------------------------------
    //! \brief  Event handler for the USB controller peripheral.
    //! \details This function handles low-level events comming from the USB controller
    //! peripheral. It then dispatches those events through the user-provided
    //! callbacks. The following callbacks can be triggered:
    //!  -#  OnReset
    //!  -#  OnSuspend
    //!  -#  OnResume
    //!  -#  OnNewRequest
    //! \see    usb_api_callbacks
    //-----------------------------------------------------------------------------------
    void EventHandler( void );

    //-----------------------------------------------------------------------------------
    //! \brief  Connects the device to the USB.
    //! \details This method enables the pull-up resistor on the D+ line, notifying the 
    //! host that the device wishes to connect to the bus.
    //! \see    USB_Disconnect
    //-----------------------------------------------------------------------------------
    void Connect( void );

    //-----------------------------------------------------------------------------------
    //! \brief  Disconnects the device from the USB.
    //! \details This method disables the pull-up resistor on the D+ line, notifying the 
    //! host that the device wishes to disconnect from the bus.
    //! \see    USB_Connect
    //-----------------------------------------------------------------------------------
    void Disconnect( void );
};

//---------------------------------------------------------------------------------------
//      Defines
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
//      Internal Functions
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
//! \brief  Clears the correct RX flag in an endpoint status register
//! \param  bEndpoint Index of endpoint
//! \see    CEndpoint
//---------------------------------------------------------------------------------------
void CUdpDriver::ClearRXFlag( int bEndpoint )
{
    CEndpoint* pEndpoint = &pEndpoints[ bEndpoint ];

    // Clear flag
    //
    ClearEndpointFlags( bEndpoint, pEndpoint->dFlag );

    // Swap banks
    //
    if ( pEndpoint->dFlag == AT91C_UDP_RX_DATA_BK0 ) 
    {
        if ( pEndpoint->dNumFIFO > 1 ) 
        {
            // Swap bank if in dual-fifo mode
            //
            pEndpoint->dFlag = AT91C_UDP_RX_DATA_BK1;
            
            TRACE_DEBUG_M( "F%d(2) ", bEndpoint );
        }
    }
    else 
    {
        pEndpoint->dFlag = AT91C_UDP_RX_DATA_BK0;
        TRACE_DEBUG_M( "F%d(1) ", bEndpoint );
    }
}

//---------------------------------------------------------------------------------------
//! \brief  This function reset all endpoint transfer descriptors
//---------------------------------------------------------------------------------------
void CUdpDriver::ResetEndpoints( void )
{
    // Reset the transfer descriptor of every endpoint
    //
    for ( int bEndpoint = 0; bEndpoint < dNumEndpoints; bEndpoint++ )
    {
        CEndpoint* pEndpoint = &pEndpoints[ bEndpoint ];

        // Reset endpoint transfer descriptor
        //
        pEndpoint->pData = 0;
        pEndpoint->pDataLowerBound = 0;
        pEndpoint->pDataUpperBound = 0;
        pEndpoint->dBytesRemaining = 0;
        pEndpoint->dBytesTransferred = 0;
        pEndpoint->dBytesBuffered = 0;
        pEndpoint->fCallback = 0;
        pEndpoint->pArgument = 0;

        // Configure endpoint characteristics
        //
        pEndpoint->dFlag = AT91C_UDP_RX_DATA_BK0;
        pEndpoint->dState = CEndpoint::StateDisabled;
    }
}

//---------------------------------------------------------------------------------------
//! \brief  Disable all endpoints (except control endpoint 0), aborting current
//!         transfers if necessary.
//---------------------------------------------------------------------------------------
void CUdpDriver::DisableEndpoints( void )
{
    // For each endpoint, if it is enabled, disable it and invoke the callback
    // Control endpoint 0 is not disabled
    //
    for( int bEndpoint = 1; bEndpoint < dNumEndpoints; bEndpoint++ ) 
    {
        CEndpoint* pEndpoint = &pEndpoints[ bEndpoint ];
        pEndpoint->EndOfTransfer( USB_STATUS_RESET );
        pEndpoint->dState = CEndpoint::StateDisabled;
    }
}

//---------------------------------------------------------------------------------------
//! \brief  Endpoint interrupt handler.
//!         Handle IN/OUT transfers, received SETUP packets and STALLing
//! \param  bEndpoint Index of endpoint
//---------------------------------------------------------------------------------------
void CUdpDriver::EndpointHandler( int bEndpoint )
{
    CEndpoint* pEndpoint = &pEndpoints[ bEndpoint ];
    uint dCSR = pInterface->UDP_CSR[ bEndpoint ];

    TRACE_DEBUG_L( "Ept%d ", bEndpoint);

    //-----------------------------------------------------------------------------------
    // Handle interrupts
    //-----------------------------------------------------------------------------------
    
    //-----------------------------------------------------------------------------------
    // IN packet sent
    //
    if ( ISSET( dCSR, AT91C_UDP_TXCOMP ) )
    {
        TRACE_DEBUG_L( "Wr " );

        // Check that endpoint was in Write state
        //
        if ( pEndpoint->dState == CEndpoint::StateWrite )
        {
            // End of transfer ?
            //
            if ( pEndpoint->dBytesBuffered < pEndpoint->wMaxPacketSize
                || ( 
                  ! pEndpoint->bCompletePacket
                  && ! ISCLEARED( dCSR, AT91C_UDP_EPTYPE )
                  && pEndpoint->dBytesRemaining == 0
                  && pEndpoint->dBytesBuffered == pEndpoint->wMaxPacketSize
                  )
                )
            {
                TRACE_DEBUG_L( "%d ", pEndpoint->dBytesBuffered );

                pEndpoint->dBytesTransferred += pEndpoint->dBytesBuffered;
                pEndpoint->dBytesBuffered = 0;

                // Disable interrupt if this is not a control endpoint
                //
                if ( ! ISCLEARED( dCSR, AT91C_UDP_EPTYPE ) )
                {
                    SET( pInterface->UDP_IDR, 1 << bEndpoint );
                }

                pEndpoint->EndOfTransfer( USB_STATUS_SUCCESS );
            }
            else 
            {
                // Transfer remaining data
                //
                TRACE_DEBUG_L( "%d ", pEndpoint->wMaxPacketSize );

                pEndpoint->dBytesTransferred += pEndpoint->wMaxPacketSize;
                pEndpoint->dBytesBuffered    -= pEndpoint->wMaxPacketSize;

                // Send next packet
                //
                if( pEndpoint->dNumFIFO == 1 )
                {
                    // No double buffering
                    //
                    WritePayload( bEndpoint );
                    SetEndpointFlags( bEndpoint, AT91C_UDP_TXPKTRDY );
                }
                else
                {
                    // Double buffering
                    //
                    SetEndpointFlags( bEndpoint, AT91C_UDP_TXPKTRDY );
                    WritePayload( bEndpoint );
                }
            }
        }

        // Acknowledge interrupt
        //
        ClearEndpointFlags( bEndpoint, AT91C_UDP_TXCOMP );
    }
    
    //-----------------------------------------------------------------------------------
    // OUT packet received
    //
    if ( ISSET( dCSR, AT91C_UDP_RX_DATA_BK0 ) || ISSET( dCSR, AT91C_UDP_RX_DATA_BK1 ) )
    {
        TRACE_DEBUG_L( "Rd " );

        // Check that the endpoint is in Read state
        //
        if ( pEndpoint->dState != CEndpoint::StateRead )
        {
            // Endpoint is NOT in Read state
            //
            if ( ISCLEARED( dCSR, AT91C_UDP_EPTYPE ) && ISCLEARED( dCSR, 0xFFFF0000 )
                )
            {
                // Control endpoint, 0 bytes received
                // Acknowledge the data and finish the current transfer
                //
                TRACE_DEBUG_L( "Ack " );
                ClearRXFlag( bEndpoint );

                pEndpoint->EndOfTransfer( USB_STATUS_SUCCESS );
            }
            else if ( ISSET( dCSR, AT91C_UDP_FORCESTALL ) )
            {
                // Non-control endpoint
                // Discard stalled data
                //
                TRACE_DEBUG_L( "Disc " );
                ClearRXFlag( bEndpoint );
            }
            else
            {
                // Non-control endpoint
                // Nak data
                //
                TRACE_DEBUG_L( "Nak " );
                SET( pInterface->UDP_IDR, 1 << bEndpoint );
            }
        }
        else
        {
            // Endpoint is in Read state
            // Retrieve data and store it into the current transfer buffer
            //
            ushort wPacketSize = ushort( dCSR >> 16 );

            GetPayload( bEndpoint, wPacketSize );
            
            if ( pEndpoint->dBytesRemaining == 0
                || wPacketSize < pEndpoint->wMaxPacketSize 
                )
            {
                // Disable interrupt if this is not a control endpoint
                //
                if ( ! ISCLEARED( dCSR, AT91C_UDP_EPTYPE ) )
                {
                    SET( pInterface->UDP_IDR, 1 << bEndpoint );
                }
            }

            // Maybe there is still data left in FIFO? Check pEndpoint->dBytesBuffered
            // and clear RX flag (i.e. swap FIFO banks) if there is no data left in FIFO.
            //
            if ( pEndpoint->dBytesBuffered == 0 )
            {
                ClearRXFlag( bEndpoint );
            }

            if ( pEndpoint->dBytesRemaining == 0
                || wPacketSize < pEndpoint->wMaxPacketSize 
                )
            {
                pEndpoint->EndOfTransfer( USB_STATUS_SUCCESS );
            }
        }
    }

    //-----------------------------------------------------------------------------------
    // SETUP packet received
    //
    if ( ISSET( dCSR, AT91C_UDP_RXSETUP ) )
    {
        TRACE_DEBUG_L( "Stp " );

        // If a transfer was pending, complete it
        // Handle the case where during the status phase of a control write
        // transfer, the host receives the device ZLP and ack it, but the ack
        // is not received by the device
        //
        pEndpoint->EndOfTransfer( USB_STATUS_SUCCESS );

        // Transfers a received SETUP packet from endpoint 0 FIFO to the
        // S_usb_request structure of an USB driver
        //
        uchar* pData = (uchar*) &sSetup;
        for ( int dCtr = 0; dCtr < 8; dCtr++ ) 
        {
            *pData++ = uchar( pInterface->UDP_FDR[0] );
        }

        // Set the DIR bit before clearing RXSETUP in Control IN sequence
        //
        if ( USB_REQUEST_DIR( sSetup.bmRequestType ) == USB_DIR_DEVICE2HOST )
        {
            SetEndpointFlags( bEndpoint, AT91C_UDP_DIR );
        }

        ClearEndpointFlags( bEndpoint, AT91C_UDP_RXSETUP );

        // Forward the request to the upper layer
        //
        pEventSink->OnNewRequest ();
    }

    //-----------------------------------------------------------------------------------
    // STALL sent
    //
    if ( ISSET( dCSR, AT91C_UDP_STALLSENT ) )
    {
        TRACE_DEBUG_L( "Sta " );

        // Acknowledge the stall flag
        //
        ClearEndpointFlags( bEndpoint, AT91C_UDP_STALLSENT );

        // If the endpoint is not halted, clear the stall condition
        //
        if ( pEndpoint->dState != CEndpoint::StateHalted )
        {
            ClearEndpointFlags( bEndpoint, AT91C_UDP_FORCESTALL );
        }
    }
}

//---------------------------------------------------------------------------------------
//      Virtual Methods Implementation
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
//! \brief  Configure an endpoint with the provided endpoint descriptor
//! \param  pEpDesc Pointer to the endpoint descriptor
//! \return true if the endpoint is now configured, false otherwise
//! \see    S_usb_endpoint_descriptor
//---------------------------------------------------------------------------------------
bool CUdpDriver::ConfigureEndpoint( const S_usb_endpoint_descriptor* pEpDesc )
{
    // Default for NULL descriptor -> Control endpoint 0
    //
    int  bEndpoint    = 0;
    int  bEpType      = ENDPOINT_TYPE_CONTROL;
    bool isINEndpoint = false;

    if ( pEpDesc )
    {
        bEndpoint    = USB_ENDPOINT_NUMBER( pEpDesc->bEndpointAddress );
        bEpType      = USB_ENDPOINT_TYPE( pEpDesc->bmAttributes );
        isINEndpoint = USB_ENDPOINT_DIRECTION( pEpDesc->bEndpointAddress ) != 0;
    }

    // Get pointer on endpoint
    //
    if ( bEndpoint >= dNumEndpoints || bEndpoint < 0 )
    {
        return false;
    }
    CEndpoint* pEndpoint = &pEndpoints[ bEndpoint ];

    // Configure wMaxPacketSize
    //
    if ( pEpDesc != 0 )
    {
        pEndpoint->wMaxPacketSize = pEpDesc->wMaxPacketSize;
    }
    else
    {
        pEndpoint->wMaxPacketSize = USB_ENDPOINT0_MAXPACKETSIZE;
    }

    // Abort the current transfer is the endpoint was configured and in
    // Write or Read state
    //
    if ( pEndpoint->dState == CEndpoint::StateRead
        || pEndpoint->dState == CEndpoint::StateWrite 
        )
    {
        pEndpoint->EndOfTransfer( USB_STATUS_RESET );
    }

    // Enter IDLE state
    //
    pEndpoint->dState = CEndpoint::StateIdle;

    // Reset Endpoint FIFOs, beware this is a 2 steps operation
    //
    SET( pInterface->UDP_RSTEP, 1 << bEndpoint );
    CLEAR( pInterface->UDP_RSTEP, 1 << bEndpoint );

    // Configure endpoint
    // Do not use SetEndpointFlags() here!
    //
    AT91_REG& UDP_CSR = pInterface->UDP_CSR[ bEndpoint ];
    TRACE_DEBUG_L( "CfgEpt%d(%08X->", bEndpoint, UDP_CSR );
    
    uint dNewCSR = AT91C_UDP_EPEDS;
    SET( dNewCSR, bEpType << UDP_EPTYPE_INDEX );

    if ( isINEndpoint )
    {
        SET( dNewCSR, 1 << UDP_EPDIR_INDEX );
    }

    if ( bEpType == ENDPOINT_TYPE_CONTROL )
    {
        SET( pInterface->UDP_IER, 1 << bEndpoint );
    }

    while ( UDP_CSR != dNewCSR )
        UDP_CSR = dNewCSR;

    TRACE_DEBUG_L( "%08X) ", UDP_CSR );
    
    return true;
}

//---------------------------------------------------------------------------------------
//! \brief   UDP interrupt handler
//! \details Manages device resume, suspend, end of bus reset. Forwards endpoint
//!          interrupts to the appropriate handler.
//---------------------------------------------------------------------------------------
void CUdpDriver::EventHandler( void )
{
#ifdef LED_USB
    if ( ! IsStateSet( USB_STATE_SUSPENDED ) && IsStateSet( USB_STATE_POWERED ) )
    {
        AT91F_PIO_ClearOutput( LED_PIO, LED_USB ); // LED on
    }
#endif

    TRACE_DEBUG_L( "Hlr " );

    // Get interrupts status
    //
    uint dISR = pInterface->UDP_ISR & pInterface->UDP_IMR & ISR_MASK;

    // Handle all UDP interrupts
    //
    while( dISR != 0 )
    {
        //----------------------
        // Start Of Frame (SOF)?
        //----------------------
        if( ISSET( dISR, AT91C_UDP_SOFINT ) )
        {
            TRACE_DEBUG_L( "SOF " );

            // Invoke the SOF callback
            //
            pEventSink->OnStartOfFrame ();

            // Acknowledge interrupt
            //
            SET( pInterface->UDP_ICR, AT91C_UDP_SOFINT );
            CLEAR( dISR, AT91C_UDP_SOFINT );
        }

        //----------
        // Suspend?
        //----------
        if( dISR == AT91C_UDP_RXSUSP )
        {
            TRACE_DEBUG_L( "Susp " );

            if ( ! IsStateSet( USB_STATE_SUSPENDED ) )
            {
                // The device enters the Suspended state
                //      MCK + UDPCK must be off
                //      Pull-Up must be connected
                //      Transceiver must be disabled

                // Enable wakeup
                //
                SET( pInterface->UDP_IER, AT91C_UDP_WAKEUP | AT91C_UDP_RXRSM );

                // Acknowledge interrupt
                //
                SET( pInterface->UDP_ICR, AT91C_UDP_RXSUSP );

                SetState( USB_STATE_SUSPENDED );
                DisableTransceiver ();
                DisableMCK ();
                DisableUDPCK ();

                // Invoke the Suspend callback
                //
                pEventSink->OnSuspend ();
            }
        }
        //---------
        // Resume?
        //---------
        else if ( ISSET( dISR, AT91C_UDP_WAKEUP )
               || ISSET( dISR, AT91C_UDP_RXRSM )
               )
        {
            // Invoke the Resume callback
            //
            pEventSink->OnResume ();

            TRACE_DEBUG_L( "Res " );

            // The device enters Configured state
            //      MCK + UDPCK must be on
            //      Pull-Up must be connected
            //      Transceiver must be enabled

            if ( IsStateSet( USB_STATE_SUSPENDED ) )
            {
                // Powered state
                EnableMCK ();
                EnableUDPCK ();

                // Default state
                if ( IsStateSet( USB_STATE_DEFAULT ) )
                {
                    EnableTransceiver ();
                }

                ClearState( USB_STATE_SUSPENDED );
            }

            SET( pInterface->UDP_ICR,
                AT91C_UDP_WAKEUP | AT91C_UDP_RXRSM | AT91C_UDP_RXSUSP );

            SET( pInterface->UDP_IDR, AT91C_UDP_WAKEUP | AT91C_UDP_RXRSM );
        }
        //-------------------
        // End of bus reset?
        //-------------------
        else if( ISSET( dISR, AT91C_UDP_ENDBUSRES ) )
        {
            TRACE_DEBUG_L( "EoBRes " );

            // The device enters the Default state:
            //
            //   -  MCK + UDPCK are already enabled
            //   -  Pull-Up is already connected
            //   -  Transceiver must be enabled
            //   -  Endpoint 0 must be enabled
            //
            // Note: Each time an ENDBUSRES interrupt is triggered, 
            // the Interrupt Mask Register and UDP_CSR registers have been reset.
            //
            SetState( USB_STATE_DEFAULT );
            EnableTransceiver ();

            // The device leaves the Address and Configured states
            //
            ClearState( USB_STATE_ADDRESS | USB_STATE_CONFIGURED );
            ResetEndpoints ();
            DisableEndpoints ();
            ConfigureEndpoint( 0 );

            // Flush and enable the Suspend interrupt
            //
            SET( pInterface->UDP_ICR,
                AT91C_UDP_WAKEUP | AT91C_UDP_RXRSM | AT91C_UDP_RXSUSP );

            // Enable the Start Of Frame (SOF) interrupt if needed
            //
            if ( useSOFCallback ) 
            {
                SET( pInterface->UDP_IER, AT91C_UDP_SOFINT );
            }

            // Invoke the Reset callback
            //
            pEventSink->OnReset ();

            // Acknowledge end of bus reset interrupt
            //
            SET( pInterface->UDP_ICR, AT91C_UDP_ENDBUSRES );
        }
        //---------------------
        // Endpoint interrupts
        //---------------------
        else
        {
            while ( dISR != 0 )
            {
                // Get endpoint index
                //
                int bEndpoint = lastSetBit( dISR );
                EndpointHandler( bEndpoint );

                CLEAR( dISR, 1 << bEndpoint );

                TRACE_DEBUG_L( dISR != 0 ? "\n  + " : "" );
            }
        }

        // Retrieve new interrupt status
        //
        dISR = pInterface->UDP_ISR & pInterface->UDP_IMR & ISR_MASK;

        // Mask unneeded interrupts
        //
        if ( ! IsStateSet( USB_STATE_DEFAULT ) )
        {
            dISR &= ( AT91C_UDP_ENDBUSRES | AT91C_UDP_SOFINT );
        }

        TRACE_DEBUG_L( dISR != 0 ? "\n  - " : "\n" );
    }

#ifdef LED_USB
    if ( ! IsStateSet( USB_STATE_SUSPENDED ) && IsStateSet( USB_STATE_POWERED ) )
    {
        AT91F_PIO_SetOutput( LED_PIO, LED_USB ); // LED off
    }
#endif
}

//---------------------------------------------------------------------------------------
//! \brief   Sends data through an USB endpoint
//! \details Sets up the transfer descriptor, write one or two data payloads
//!          (depending on the number of FIFO banks for the endpoint) and then
//!          starts the actual transfer. The operation is complete when all
//!          the data has been sent.
//! \param   bEndpoint Index of endpoint
//! \param   pData     Pointer to a buffer containing the data to send
//! \param   dLength   Length of the data buffer
//! \param   fCallback Optional function to invoke when the transfer finishes
//! \param   pArgument Optional argument for the callback function
//! \param   pDataLowerBound Lower bound of the circular buffer
//! \param   pDataUpperBound Upper bound of the circular buffer
//! \return  Operation result code
//! \see     EnumStandardReturnValue
//! \see     Callback_f
//---------------------------------------------------------------------------------------
EnumStandardReturnValue CUdpDriver::Write
(
    int bEndpoint,
    const void* pData, uint dLength,
    Callback_f fCallback, void* pArgument,
    const void* pDataLowerBound, const void* pDataUpperBound
    )
{
    CEndpoint* pEndpoint = &pEndpoints[ bEndpoint ];

    // Check that the endpoint is in Idle state
    //
    if ( pEndpoint->dState != CEndpoint::StateIdle )
    {
        return USB_STATUS_LOCKED;
    }

    TRACE_DEBUG_M( "Write%d%4d ", bEndpoint, dLength );

    // Setup the transfer descriptor
    //
    pEndpoint->pData             = (uchar*) pData;
    pEndpoint->pDataLowerBound   = (uchar*) pDataLowerBound;
    pEndpoint->pDataUpperBound   = (uchar*) pDataUpperBound;
    pEndpoint->dBytesRemaining   = dLength;
    pEndpoint->dBytesBuffered    = 0;
    pEndpoint->dBytesTransferred = 0;
    pEndpoint->bCompletePacket   = true;
    pEndpoint->fCallback         = fCallback;
    pEndpoint->pArgument         = pArgument;

    // Send one packet
    //
    pEndpoint->dState = CEndpoint::StateWrite;
    WritePayload( bEndpoint );
    SetEndpointFlags( bEndpoint, AT91C_UDP_TXPKTRDY );

    // If double buffering is enabled and there is data remaining,
    // prepare another packet
    //
    if ( pEndpoint->dNumFIFO > 1 && pEndpoint->dBytesRemaining > 0 )
    {
        WritePayload( bEndpoint );
    }

    // Enable interrupt on endpoint
    //
    SET( pInterface->UDP_IER, 1 << bEndpoint );

    return USB_STATUS_SUCCESS;
}

//---------------------------------------------------------------------------------------
//! \brief   Reads incoming data on an USB endpoint
//! \details This methods sets the transfer descriptor and activate the endpoint
//!          interrupt. The actual transfer is then carried out by the endpoint
//!          interrupt handler. The Read operation finishes either when the
//!          buffer is full, or a short packet (inferior to endpoint maximum
//!          packet size) is received.
//! \param   bEndpoint Index of endpoint
//! \param   pData     Pointer to a buffer to store the received data
//! \param   dLength   Length of the receive buffer
//! \param   fCallback Optional callback function
//! \param   pArgument Optional callback argument
//! \return  Operation result code
//! \see     Callback_f
//---------------------------------------------------------------------------------------
EnumStandardReturnValue CUdpDriver::Read
(
    int bEndpoint,
    void* pData, uint dLength,
    Callback_f fCallback, void* pArgument
    )
{
    CEndpoint* pEndpoint = &pEndpoints[ bEndpoint ];

    // Return if the endpoint is not in IDLE state
    //
    if ( pEndpoint->dState != CEndpoint::StateIdle )
    {
        return USB_STATUS_LOCKED;
    }

    TRACE_DEBUG_M( "Read%d%5d ", bEndpoint, dLength );

    // Remember if there is some data left in FIFO buffer
    //
    uint dBytesInBuffer = pEndpoint->dBytesBuffered;
    
    // Endpoint enters Read state
    //
    pEndpoint->dState = CEndpoint::StateRead;

    // Set the transfer descriptor
    //
    pEndpoint->pData             = (uchar*) pData;
    pEndpoint->dBytesRemaining   = dLength;
    pEndpoint->dBytesBuffered    = 0;
    pEndpoint->dBytesTransferred = 0;
    pEndpoint->fCallback         = fCallback;
    pEndpoint->pArgument         = pArgument;

    // If there is data left earlier in FIFO buffer, get it and exit immediatelly.
    //
    if ( dBytesInBuffer > 0 )
    {
        TRACE_DEBUG_M( "Immed " );

        GetPayload( bEndpoint, dBytesInBuffer );

        // Maybe there is still data left in FIFO? Check pEndpoint->dBytesBuffered
        // and clear RX flag (i.e. swap FIFO banks) if there is no data left in FIFO.
        //
        if ( pEndpoint->dBytesBuffered == 0 )
        {
            ClearRXFlag( bEndpoint );
        }

        // Note that end-of-read callback will not be called in interrupt context!
        // Callback will receive special bStatus to now that.
        //
        pEndpoint->EndOfTransfer( USB_STATUS_IMMEDREAD );
        return USB_STATUS_SUCCESS;
    }

    // Enable interrupt on endpoint
    //
    SET( pInterface->UDP_IER, 1 << bEndpoint );

    return USB_STATUS_SUCCESS;
}

//---------------------------------------------------------------------------------------
//! \brief   Clears, sets or returns the Halt state on specified endpoint
//! \details When in Halt state, an endpoint acknowledges every received packet
//!          with a STALL handshake. This continues until the endpoint is
//!          manually put out of the Halt state by calling this function.
//! \param   bEndpoint Index of endpoint
//! \param   bRequest  Request to perform
//!                   -> USB_SET_FEATURE, USB_CLEAR_FEATURE, USB_GET_STATUS
//! \return  true if the endpoint is currently Halted, false otherwise
//---------------------------------------------------------------------------------------
bool CUdpDriver::Halt( int bEndpoint, int bRequest )
{
    CEndpoint* pEndpoint = &pEndpoints[ bEndpoint ];

    // Clear the Halt feature of the endpoint if it is enabled
    //
    if ( bRequest == USB_CLEAR_FEATURE 
        && pEndpoint->dState == CEndpoint::StateHalted
        )
    {
        TRACE_DEBUG_L( "Unhalt %02X ", bEndpoint );

        // Return endpoint to Idle state
        //
        pEndpoint->dState = CEndpoint::StateIdle;

        // Clear FORCESTALL flag
        //
        ClearEndpointFlags( bEndpoint, AT91C_UDP_FORCESTALL );

        // Reset Endpoint FIFOs, beware this is a 2 steps operation
        //
        SET( pInterface->UDP_RSTEP, 1 << bEndpoint );
        CLEAR( pInterface->UDP_RSTEP, 1 << bEndpoint );
    }
    //
    // Set the Halt feature on the endpoint if it is not already enabled
    // and the endpoint is not disabled
    //
    else if ( bRequest == USB_SET_FEATURE
             && pEndpoint->dState != CEndpoint::StateHalted
             && pEndpoint->dState != CEndpoint::StateDisabled
             )
    {
        TRACE_DEBUG_L( "Halt %02X ", bEndpoint );

        // Abort the current transfer if necessary
        //
        pEndpoint->EndOfTransfer( USB_STATUS_ABORTED );

        // Put endpoint into Halt state
        //
        SetEndpointFlags( bEndpoint, AT91C_UDP_FORCESTALL );
        pEndpoint->dState = CEndpoint::StateHalted;

        // Enable the endpoint interrupt
        //
        SET( pInterface->UDP_IER, 1 << bEndpoint );
    }

    // Return the endpoint halt status
    //
    return pEndpoint->dState == CEndpoint::StateHalted;
}

//---------------------------------------------------------------------------------------
//! \brief  Causes the endpoint to acknowledge the next received packet with
//!         a STALL handshake.
//!         Further packets are then handled normally.
//! \param  bEndpoint Index of endpoint
//! \return Operation result code
//---------------------------------------------------------------------------------------
EnumStandardReturnValue CUdpDriver::Stall( int bEndpoint )
{
    CEndpoint* pEndpoint = &pEndpoints[ bEndpoint ];

    // Check that endpoint is in Idle state
    //
    if ( pEndpoint->dState != CEndpoint::StateIdle )
    {
        TRACE_WARNING( "W: CUdpDriver::Stall: Endpoint%d locked\n", bEndpoint );
        return USB_STATUS_LOCKED;
    }

    TRACE_DEBUG_L( "Stall%d ", bEndpoint );

    SetEndpointFlags( bEndpoint, AT91C_UDP_FORCESTALL );

    return USB_STATUS_SUCCESS;
}

//---------------------------------------------------------------------------------------
//! \brief  Activates a remote wakeup procedure
//---------------------------------------------------------------------------------------
void CUdpDriver::RemoteWakeUp( void )
{
    EnableMCK ();
    EnableUDPCK ();
    EnableTransceiver ();

    TRACE_DEBUG_L( "Remote WakeUp " );

    // Activates a remote wakeup (edge on ESR)
    //
    SET( pInterface->UDP_GLBSTATE, AT91C_UDP_ESR );

    // Then clear ESR
    //
    CLEAR( pInterface->UDP_GLBSTATE, AT91C_UDP_ESR );
}

//---------------------------------------------------------------------------------------
//! \brief  Handles attachment or detachment from the USB when the VBus power
//!         line status changes.
//! \return true if VBus is present, false otherwise
//---------------------------------------------------------------------------------------
bool CUdpDriver::Attach( void )
{
    TRACE_DEBUG_L( "Attach( " );

    // Check if VBus is present
    //
    if ( ! IsStateSet( USB_STATE_POWERED )
        && pBoard->IsVBusConnected () 
        )
    {
        // Powered state:
        //      MCK + UDPCK must be on
        //      Pull-Up must be connected
        //      Transceiver must be disabled

        // Invoke the Resume callback
        //
        pEventSink->OnResume ();

        EnableMCK ();
        EnableUDPCK ();

        // Reconnect the pull-up if needed
        //
        if ( IsStateSet( UDP_STATE_SHOULD_RECONNECT ) )
        {
            Connect ();
            ClearState( UDP_STATE_SHOULD_RECONNECT );
        }

        // Clear the Suspend and Resume interrupts
        //
        SET( pInterface->UDP_ICR,
             AT91C_UDP_WAKEUP | AT91C_UDP_RXRSM | AT91C_UDP_RXSUSP );

        SET( pInterface->UDP_IER, AT91C_UDP_RXSUSP );

        // The device is in Powered state
        //
        SetState( USB_STATE_POWERED );

    }
    else if ( IsStateSet( USB_STATE_POWERED )
         && ! pBoard->IsVBusConnected ()
         )
    {
        // Attached state:
        //      MCK + UDPCK off
        //      Pull-Up must be disconnected
        //      Transceiver must be disabled

        // Warning: MCK must be enabled to be able to write in UDP registers
        // It may have been disabled by the Suspend interrupt, so re-enable it
        //
        EnableMCK ();

        // Disable interrupts
        //
        SET( pInterface->UDP_IDR, 
            AT91C_UDP_WAKEUP | AT91C_UDP_RXRSM | AT91C_UDP_RXSUSP | AT91C_UDP_SOFINT );

        DisableEndpoints ();
        DisableTransceiver ();

        // Disconnect the pull-up if needed
        //
        if ( IsStateSet( USB_STATE_DEFAULT ) )
        {
            Disconnect ();
            SetState( UDP_STATE_SHOULD_RECONNECT );
        }

        DisableMCK ();
        DisableUDPCK ();

        // The device leaves the all states except Attached
        //
        ClearState( USB_STATE_POWERED | USB_STATE_DEFAULT
              | USB_STATE_ADDRESS | USB_STATE_CONFIGURED | USB_STATE_SUSPENDED );

        // Invoke the Suspend callback
        //
        pEventSink->OnSuspend ();
    }

    TRACE_DEBUG_L( "%d) ", IsStateSet( USB_STATE_POWERED ) );

    return IsStateSet( USB_STATE_POWERED );
}

//---------------------------------------------------------------------------------------
//! \brief   Sets or unsets the device address
//! \details This function directly accesses the S_usb_request instance located
//!          in the sSetup structure to extract its new address.
//---------------------------------------------------------------------------------------
void CUdpDriver::SetAddress( void )
{
    ushort wAddress = sSetup.wValue;

    TRACE_DEBUG_L( "SetAddr(%d) ", wAddress);

    // Set address
    //
    SET( pInterface->UDP_FADDR, AT91C_UDP_FEN | wAddress );

    if ( wAddress == 0 )
    {
        SET( pInterface->UDP_GLBSTATE, 0 );

        // Device enters the Default state
        //
        ClearState( USB_STATE_ADDRESS );
    }
    else
    {
        SET(pInterface->UDP_GLBSTATE, AT91C_UDP_FADDEN );

        // The device enters the Address state
        //
        SetState( USB_STATE_ADDRESS );
    }
}

//---------------------------------------------------------------------------------------
//! \brief   Changes the device state from Address to Configured, or from
//!          Configured to Address.
//! \details This method directly access the last received SETUP packet to
//!          decide on what to do.
//---------------------------------------------------------------------------------------
void CUdpDriver::SetConfiguration( void )
{
    ushort wValue = sSetup.wValue;

    TRACE_DEBUG_L( "SetCfg() " );

    // Check the request
    //
    if ( wValue != 0 ) 
    {
        // Enter Configured state
        //
        SetState( USB_STATE_CONFIGURED );
        SET( pInterface->UDP_GLBSTATE, AT91C_UDP_CONFG );
    }
    else 
    {
        // Go back to Address state
        //
        ClearState( USB_STATE_CONFIGURED );
        SET( pInterface->UDP_GLBSTATE, AT91C_UDP_FADDEN );

        // Abort all transfers
        //
        DisableEndpoints ();
    }
}

//---------------------------------------------------------------------------------------
//! \brief  Enables the pull-up on the D+ line to connect the device to the USB.
//---------------------------------------------------------------------------------------
void CUdpDriver::Connect( void )
{
#if defined(UDP_INTERNAL_PULLUP)
    SET( pInterface->UDP_TXVC, AT91C_UDP_PUON );

#elif defined(UDP_INTERNAL_PULLUP_BY_MATRIX)
    TRACE_DEBUG_L( "PUON 1\n" );
    AT91C_BASE_MATRIX->MATRIX_USBPCR |= AT91C_MATRIX_USBPCR_PUON;

#else
    pBoard->ConnectPullUp ();

#endif
}

//---------------------------------------------------------------------------------------
//! \brief  Disables the pull-up on the D+ line to disconnect the device from
//!         the bus.
//---------------------------------------------------------------------------------------
void CUdpDriver::Disconnect( void )
{
#if defined(UDP_INTERNAL_PULLUP)
    CLEAR( pInterface->UDP_TXVC, AT91C_UDP_PUON );

#elif defined(UDP_INTERNAL_PULLUP_BY_MATRIX)
    TRACE_DEBUG_L( "PUON 0\n" );
    AT91C_BASE_MATRIX->MATRIX_USBPCR &= ~AT91C_MATRIX_USBPCR_PUON;

#else
    pBoard->DisconnectPullUp ();

#endif
    // Device leaves the Default state
    //
    ClearState( USB_STATE_DEFAULT );
}

//---------------------------------------------------------------------------------------
//! \brief   Initializes the specified USB driver
//! \details This function initializes the current FIFO bank of endpoints,
//!          configures the pull-up and VBus lines, disconnects the pull-up and
//!          then trigger the Init callback.
//---------------------------------------------------------------------------------------
void CUdpDriver::Init( void )
{
    TRACE_DEBUG_L( "CUdpDriver::Init()\n" );
    
    if ( ! pEventSink )
    {
        TRACE_ERROR( "CUdpDriver::Init: Event sink instance is not defined\n" );
        return;
    }

    // Init data banks
    //
    for ( int bEndpoint = 0; bEndpoint < dNumEndpoints; bEndpoint++ )
    {
        pEndpoints[ bEndpoint ].dFlag = AT91C_UDP_RX_DATA_BK0;
    }

    // Configure external pull-up on D+
    //
    pBoard->ConfigurePullUp ();
    
    // Configure VBus monitoring
    pBoard->ConfigureVBus ();

    // Disable
    //
    Disconnect ();

    // Device is in the Attached state
    //
    ClearState ();
    SetState( USB_STATE_ATTACHED );

    // Disable the UDP transceiver and interrupts
    //
    EnableMCK ();
    SET( pInterface->UDP_IDR, AT91C_UDP_RXRSM );
    Connect ();
    DisableTransceiver ();
    DisableMCK ();
    Disconnect ();

    // Configure interrupts
    //
    pEventSink->OnInit ();
}

//---------------------------------------------------------------------------------------
//      Global variables
//---------------------------------------------------------------------------------------

//! Default driver when an UDP controller is present on a chip.
//! Note that CUdpDriver implementation is masked i.e. it is private to this
//! module it exports only its usb driver interface as sDefautlUsbDriver.
//
static CUdpDriver udpDriver( AT91C_BASE_UDP, AT91C_ID_UDP, AT91C_PMC_UDP );

namespace USB
{
    //! USB driver instance
    CUsbDriver& sDefaultUsbDriver = udpDriver;
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
$Id: udp.c 196 2006-10-30 09:56:17Z jjoannic $
*/
