
// ---------------------------------------------------------------------------------------------
// functions
// ---------------------------------------------------------------------------------------------

#include "net_includes.h"

extern OS_MEM PacketMemArea;
extern OS_Q TXNicQ;
extern MAC_A my_mac;

// ---------------------------------------------------------------------------------------------
// functions
// ---------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------
// function:     void ETHER_SendFrame (MAC_A ESender, MAC_A EDestination,
//                    INT16U EProtocol, INT16U huge *EtherData, INT16U DataLength)
// parameters:   <ESender>        own MAC address (sender)
//               <EDestination>   destination MAC address
//               <EProtocol>      protocol type
//               <*Etherdata>     pointer to payload
//               <DataLength>     length of payload
// result:       <none>
// purpose:                   ethernet frame send procedure
// ---------------------------------------------------------------------------------------------

void ETHER_SendFrame (MAC_A ESender,           // MAC sender address (my address)
                      MAC_A EDestination,      // MAC destination
                      INT16U  EProtocol,       // OSI level 3 protocol type
                      INT16U  *EtherData,      // pointer to frame data
                      INT16U  DataLength)      // number of bytes
{

LLC_HEADER        *pTx;
ETHERNET_HEADER   *pEther;
OS_ERR              ErrVar;                    // only for debug purpose

 // ------ call system function to dynamically allocate buffer storage -------
pTx = (LLC_HEADER*)OSMemGet(&PacketMemArea, &ErrVar) ;// get memory block from Frame Buffer Area
if ( ErrVar != OS_ERR_NONE )
{
  #if DEBUG
		uart_puts("ETHER_SendFrame get memory block from frame buffer area failed!\n");
  #endif
  return;
}

pEther = (ETHERNET_HEADER *)&(pTx->Data);
pTx->Bytes   = sizeof(ETHERNET_HEADER)-1 + DataLength;        // length of ether frame
pEther->Destination     = EDestination;                            // destination hw address
pEther->Source   = ESender;                                 // sender (me?)
pEther->Protocol = EProtocol;                               // protocol (IP,ARP)
memcpy(pEther->Data, (INT16U *)EtherData, DataLength);// move data into ether frame

 // ------ return dynamic level 3 buffer to LAN Tx memory partition --------
/*if ( EtherData != NULL )
{
 // put memory block back to Frame Buffer Area
 ErrVar = OSMemPut(FrameBufferArea, (void *)EtherData) ;
 #ifdef DEBUGF
 printf("ETHER_SendFrame put memory block back to frame buffer area failed!\n");
 #endif // DEBUGF
}
*/
OS_ERR os_err;
EMAC_PACKETBUF_Type frame_p;
frame_p.ulDataLen = pTx->Bytes;
frame_p.pbDataBuf = (uint32_t *)(pTx->Data);

/*write frame to dma location -> ready to send*/
EMAC_WritePacketBuffer(&frame_p);
EMAC_UpdateTxProduceIndex();
OSMemPut(&PacketMemArea, (void*)pTx,&os_err);
//OSQPost(&TXNicQ, (void *)pTx, sizeof(LLC_HEADER*), OS_OPT_POST_FIFO | OS_OPT_POST_ALL , &ErrVar);// post packets into transmit queue + OS_OPT_POST_ALL
if(ErrVar != OS_ERR_NONE)
	for(;;);
}


// ---------------------------------------------------------------------------------------------
// function:    void ETHER_IPProcess (ETHERNET_HEADER huge *pETH)
// parameters:  <*pETH>      pointer to ethernet frame
// result:      <none>
// purpose:             extracts IP datagram and calls the desired OSI level 3
//                      procedure
// ---------------------------------------------------------------------------------------------

void ETHER_IPProcess (ETHERNET_HEADER *pETH) {
    IP_HEADER *pIP;

    if (memcmp(my_mac.b, &pETH->Destination, ETH_ALEN)) return; // is it my HW address?
    pIP = (IP_HEADER *) pETH->Data;           // pIP starts at Ethernet data area (recvd frame)
    ARP_CanUpdateTable (pIP->source, pETH->Source);// Parameter: pointer to incoming ARP request
    IP_AllTypesProcess((IP_HEADER *) pIP);    // send data up to OSI layer 3 (IP  Module)
}                                              // uses <Send_Ether_Frame>


// ---------------------------------------------------------------------------------------------
// function:    void ETHER_ARPProcess (ETHERNET_HEADER huge *pETH)
// parameters:  <*pETH>      pointer to ethernet frame
// result:      <none>
// purpose:             extracts ARP message and calls the desired OSI level 3
//                      procedure
// ---------------------------------------------------------------------------------------------

void ETHER_ARPProcess (ETHERNET_HEADER *pETH) {// Moves data up to OSI layer 3 (ARP Module)
ARP_HEADER *pARP;

pARP = (ARP_HEADER *)pETH->Data;           // pARP moved to the data area of recvd frame
ARP_AllTypesProcess ((ARP_HEADER *)pARP);  // moves incoming ARP message up to ARP unit, auto response
}


// ---------------------------------------------------------------------------------------------
// function:    void ETHER_AllTypesProcess (ETHERNET_HEADER huge *pETH)
// parameters:  <*pETH>      pointer to ethernet frame
// result:      <none>
// purpose:             continues with appropriate procedure due to OSI level 3
//                      protocol type
// ---------------------------------------------------------------------------------------------

void ETHER_AllTypesProcess (ETHERNET_HEADER *pETH) {       // pETH is the header of the incoming frame
  switch (pETH->Protocol) {                                     // protocol (reversed bytes)
    case 0x0008:ETHER_IPProcess  ((ETHERNET_HEADER *)pETH);// proceed with IP type, 0x0008 (rev. 0800)=IP
         break;
    case 0x0608:ETHER_ARPProcess ((ETHERNET_HEADER *)pETH);// 0x0608(reversed 0806)=ARP,proceed ARP
         break;
    case 0x3580:break;                               // 0x3580 (reversed 8035) = RARP
    default:    break;                               // Ignore other message types
  }
}


// ---------------------------------------------------------------------------------------------
// EOF
// ---------------------------------------------------------------------------------------------

