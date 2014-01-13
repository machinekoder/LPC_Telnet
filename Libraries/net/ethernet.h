
// ---------------------------------------------------------------------------------------------
// ethernet.h
// ---------------------------------------------------------------------------------------------

#ifndef _ETHERNET_H_
#define _ETHERNET_H_

// ---------------------------------------------------------------------------------------------
// function prototypes
// ---------------------------------------------------------------------------------------------

void ETHER_SendFrame (MAC_A ESender,               // MAC sender address (my address)
                      MAC_A EDestination,          // MAC destination
                      INT16U  EProtocol,           // OSI level 3 protocol type
                      INT16U  *EtherData,     		 // pointer to frame data (from OSI level 3)
                      INT16U  DataLength);         // number of bytes


void ETHER_AllTypesProcess (ETHERNET_HEADER *pETH);

// ---------------------------------------------------------------------------------------------
// EOF
// ---------------------------------------------------------------------------------------------

#endif /* _ETHERNET_H_ */
