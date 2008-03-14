
ENGINEERING CHANGE REQUEST
XPU-D R2A board -> R2B

Mandatory:

	1) Add D+ pull-up resistor control over mosfet to PA17 
	   (as per Page 3 sam7s-ek schematic)
	
	2) Connect RXD0/TXD0 (PA5/PA6) pins over RS232 to external connector
	
	3) Add VBUS detect to PA18

Optional:
  
	4) Connect RXD1/TXD1 (PA21/PA22) pins over RS232 to external connector

    5) Add new SAM7S pin to control FPGA hardware reset


Changes (procedural) done to original AT91 USB Framework (v1.02):

1) Macros to set/clear endpoint flags:

        while( ISSET() ) 
            CLEAR();

    should be:

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
 
 
2) Problem with SetEndpointFlags() in ConfigureEndpoints ()
 
     SetEndpointFlags() as described above should not be used in ConfigureEndpoints(). 
     Instead original Atmel implementation should be used; like:
      
     	while( not set CSR flags ) set CSR flags;

     Todo: Find out why does this work this way and not as recommended!
     
3) Problem with Halt()

	    // Clear the Halt feature of the endpoint if it is enabled
	    if ((bRequest == USB_CLEAR_FEATURE)) {

	should be:
	
	    // Clear the Halt feature of the endpoint if it is enabled
	    if ((bRequest == USB_CLEAR_FEATURE)
	        && (pEndpoint->dState == endpointStateHalted)) {


4) Problem in GetPayload() when received packet size is greater then requested in Read()

	E.g let say that 64 is received but Read() requested e.g. 16, 
	then 48 bytes are left in FIFO thus FIFO switch should not and bytes in FIFO should 
	be immediatelly returned on next Read(). ClearRXflags() must be deffered and new 
	bStatus is created to signal Callback that it is immediate data (i.e. callback is not 
	called inside ISR and must not call release from ISR task routines).

5) Incomplete packet in write()

    Added bCompletePacket flag to CEndpoint structure to control whether Write()
    writes complete or incomplete packet.
