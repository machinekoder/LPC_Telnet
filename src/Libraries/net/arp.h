// ---------------------------------------------------------------------------------------------
// arp.h
// ---------------------------------------------------------------------------------------------

#ifndef _ARP_H_
#define _ARP_H_

#include "type.h"
#include "protocols.h"

// ---------------------------------------------------------------------------------------------
// user defined makros
// ---------------------------------------------------------------------------------------------

#define ARPENTRIES 10
#ifndef TIMEOUT
#define TIMEOUT    400            // time out interval 20 min = 1200 sec
#endif
#define DECREMENT  10             // decrement time out value

// --------------------------------------------------------------------------------------------
// ARP table typedef
// --------------------------------------------------------------------------------------------

typedef struct arptab {
 IP_A  IP_Address[ARPENTRIES];
 MAC_A MAC_Address[ARPENTRIES];
 int   Time_Out[ARPENTRIES];
} __attribute__ ((packed)) ARPTAB;

// --------------------------------------------------------------------------------------------
// ARP header structure
// --------------------------------------------------------------------------------------------

typedef struct arp_header {         // total 28 bytes
  INT16U  HardwAdrSpace;        	// type of phys adress, 0x0001 = 10Base
  INT16U  ProtocolAdrSpace;        	// level 3 protocol, 0x0800 = IP
  INT8U   HwLen;                    													// length of MAC address (6 bytes default)
  INT8U   ProtLen;         	         													// length of level 3 address (4 bytes default)
  INT16U  Opcode;         // message type, 1 = request, 2 = reply
  MAC_A   SenderHwAdr;         // 48 bit, sender HW (MAC) address
  IP_A    SenderProtAdr;         // 32 bit, sender IP address
  MAC_A   DestHwAdr;         // 48 bit, target HW (MAC) address
  IP_A    DestProtAdr;         // 32 bit, target IP address
} __attribute__ ((packed)) ARP_HEADER;



// ---------------------------------------------------------------------------------------------
// function prototypes
// ---------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------
// function:    void ARP_TableClear (void)
// parameters:  <none>
// result:      <none>
// purpose:              clears the ARP table
// ---------------------------------------------------------------------------------------------

void ARP_TableClear (void);

void ARP_TableClearEntry(MAC_A address);

// ---------------------------------------------------------------------------------------------
// function:    void ARP_RemoveTimedOutEntries ()
// parameters:  <none>
//
// result:      <none>
//
// purpose:                 remove timed out entries from arp table
// ---------------------------------------------------------------------------------------------

void ARP_RemoveTimedOutEntries (void);

// ---------------------------------------------------------------------------------------------
// function:     int ARP_IsNewMACAddress (IP_A IPAddress, MAC_A *MACAddress)
// parameters:   <IPAddress>      find an Ethernet address for this IP address
//               <*MACAddress>    pointer to a matching MAC address stored in the ARP table
// result:       0 (FALSE)        IP address ardy found in list, *MACAddress provides the
//                                appropriate MAC
//               1 (TRUE)         unknown IP address, must launch an ARP echo request first!
// purpose:                returns the MAC address for an incoming IP address if
//                         ardy stored in ARP list
// ---------------------------------------------------------------------------------------------

int ARP_IsNewMACAddress (IP_A IPAddress, MAC_A *MACAddress);

// ---------------------------------------------------------------------------------------------
// function:     int ARP_CanUpdateTable (IP_A IPAddress, MAC_A MACAddress)
// parameters:   <IPAddress>      IP address
//               <MACAddress>     MAC address to be linked with the IP address
// result:       0 (FALSE)        cannot add this line, IP address is ardy known or ARP map
//                                is full
//               1 (TRUE)         ARP table updated
// purpose:                  adds a new IP-MAC entry to the ARP table
// ---------------------------------------------------------------------------------------------

int ARP_CanUpdateTable (IP_A IPAddress, MAC_A MACAddress);

// ---------------------------------------------------------------------------------------------
// function:     void ARP_SendEchoRequest (IP_A TargetIPAddress)
// parameters:   <TargetIPAddress>     IP address
// result:       <none>
// purpose:             sends an ARP echo request (broadcast) to determine the
//                      appropriate MAC address matching to the given IP address.
//                      This is done by reading incoming ARP echo reply messages
// ---------------------------------------------------------------------------------------------

void ARP_SendEchoRequest (IP_A TargetIPAddress);

// ---------------------------------------------------------------------------------------------
// function:     void ARP_AllTypes_Process (ARP_HEADER huge *pARP)
// parameters:   <*pARP>       pointer to the received ARP message
// result:       <none>
// purpose:              continues with incoming ARP messages and determines
//                       the message type for further investigation
// ---------------------------------------------------------------------------------------------

void ARP_AllTypesProcess (ARP_HEADER *pARP);

// ---------------------------------------------------------------------------------------------
// function:     ARP_EchoData_Return (ARP_HEADER huge *pARP,
//                                    INT16U *ARPlength)
// parameters:   <*pARP>       pointer to incoming ARP message
//               <*ARPlength>  length of ARP header
// result:       <none>
// purpose:                builds the ARP reply message according to an incoming
//                         echo request
// ---------------------------------------------------------------------------------------------

void ARP_EchoDataReturn (ARP_HEADER *pARP, INT16U *ARPlength);

// ---------------------------------------------------------------------------------------------
// function:     void ARP_ProcessEchoRequest (ARP_HEADER huge *pARP)
// parameters:   <*pARP>     pointer to incoming ARP message
// result:       <none>
// purpose:              process an incoming ARP echo request. Build the ARP echo
//                       message using <ARP_EchoDataReturn> and launch <ARP_Send>
//                       for transmission of the resulting message
// ---------------------------------------------------------------------------------------------

void ARP_ProcessEchoRequest (ARP_HEADER *pARP);
void ARP_TablePrint(void);

// ---------------------------------------------------------------------------------------------
// EOF
// ---------------------------------------------------------------------------------------------

#endif // _ARP_H_

