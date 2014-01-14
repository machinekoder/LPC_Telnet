
// ---------------------------------------------------------------------------------------------
// includes
// ---------------------------------------------------------------------------------------------

#include "net_includes.h"
#include <xprintf.h>

// ---------------------------------------------------------------------------------------------
// globals
// ---------------------------------------------------------------------------------------------

ARPTAB ArpTable;

extern IP_A my_ip;
extern MAC_A my_mac;
extern OS_MEM PacketMemArea;
extern char* commandOutBuffer;
extern uint8 messageReady;

// ---------------------------------------------------------------------------------------------
// functions
// ---------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------
// function:    void ARP_TablePrint (void)
// parameters:  <none>
// result:      <none>
// purpose:            prints the ARP table
// ---------------------------------------------------------------------------------------------

void ARP_TablePrint (void) {
INT16U a;

//uart_puts("                      ARP TABLE\n");
//uart_puts("|--------------------------------------------------|\n");
//uart_puts("|   Nr. |    IP-Address      |    MAC-Address      |\n");
//uart_puts("|--------------------------------------------------|\n");

    for (a=0; a<ARPENTRIES; a++)         // through all 10 positions
    {
    xsnprintf(commandOutBuffer+(a*60u),EMAC_ETH_MAX_FLEN,
         "|   %d   |  %u %u %u %u   |  %x:%x:%x:%x:%x:%x  |\n",a,
         ArpTable.IP_Address[a].b[0], ArpTable.IP_Address[a].b[1],
         ArpTable.IP_Address[a].b[2], ArpTable.IP_Address[a].b[3],
         ArpTable.MAC_Address[a].b[0], ArpTable.MAC_Address[a].b[1],
         ArpTable.MAC_Address[a].b[2], ArpTable.MAC_Address[a].b[3],
         ArpTable.MAC_Address[a].b[4], ArpTable.MAC_Address[a].b[5]);
    }
    
    for (a=0u; a < (ARPENTRIES*61u); a++)
    {
        if (commandOutBuffer[a] == '\0')
        {
            commandOutBuffer[a] = ' ';
        }
    }
    
    commandOutBuffer[ARPENTRIES*61u] = '\0';
    
    messageReady = 1u;
}



// ---------------------------------------------------------------------------------------------
// function:    void ARP_TableClear (void)
// parameters:  <none>
// result:      <none>
// purpose:            clears the ARP table
// ---------------------------------------------------------------------------------------------

void ARP_TableClear (void) {

INT16U a;

for (a=0; a<ARPENTRIES; a++)         // through all positions
{
 ArpTable.IP_Address[a].d     = 0;   // erase IP address field
 ArpTable.MAC_Address[a].w[0] = 0;   // and MAC address fields
 ArpTable.MAC_Address[a].w[1] = 0;
 ArpTable.MAC_Address[a].w[2] = 0;
 ArpTable.Time_Out[a] = 0;           // set time out interval to 0
}
}

void ARP_TableClearEntry(MAC_A address)
{
    INT16U a;

    for (a=0; a<ARPENTRIES; a++)         // through all positions
    {
        if ((ArpTable.MAC_Address[a].w[0] == address.w[0])
            && (ArpTable.MAC_Address[a].w[1] == address.w[1])
            && (ArpTable.MAC_Address[a].w[2] == address.w[2])
        )
        {
        ArpTable.IP_Address[a].d     = 0;   // erase IP address field
        ArpTable.MAC_Address[a].w[0] = 0;   // and MAC address fields
        ArpTable.MAC_Address[a].w[1] = 0;
        ArpTable.MAC_Address[a].w[2] = 0;
        ArpTable.Time_Out[a] = 0;           // set time out interval to 0
        }
    }
}


// ---------------------------------------------------------------------------------------------
// function:    int ARP_IsNewMACAddress (IP_A IPAddress, MAC_A *MACAddress)
// parameters:  <IPAddress>    find an Ethernet address for this IP address
//              <*MACAddress>  pointer to a matching MAC address stored in the ARP table
// result:      0 (FALSE)      IP address ardy found in list, *MACAddress provides the
//                             appropriate MAC
//              1 (TRUE)       unknmy IP address, must launch an ARP echo request first!
// purpose:                returns the MAC address for an incoming IP address if
//                         ardy stored in ARP list
// ---------------------------------------------------------------------------------------------

int ARP_IsNewMACAddress (IP_A IPAddress, MAC_A *MACAddress) {

INT16U a;

a = 0;
MACAddress->w[0] = 0;                         // set MAC address to empty
MACAddress->w[1] = 0;
MACAddress->w[2] = 0;
while (a<ARPENTRIES)
{
 if (ArpTable.IP_Address[a].d == IPAddress.d) // is the IP address found in the list?
 {
  *MACAddress = ArpTable.MAC_Address[a];      // then return the MAC address
  return (FALSE);                             // FALSE, not a new MAC address
 }
 a++;
}
return (TRUE);                                // TRUE, it is a new MAC address
}


// ---------------------------------------------------------------------------------------------
// function:    int ARP_CanUpdateTable (IP_A IPAddress, MAC_A MACAddress)
// parameters:  <IPAddress>     IP address
//              <MACAddress>    MAC address to be linked with the IP address
// result:      0 (FALSE)       cannot add this line, IP address is ardy knmy or ARP map
//                              is full
//              1 (TRUE)        ARP table updated
// purpose:                 adds a new IP-MAC entry to the ARP table
// ---------------------------------------------------------------------------------------------

int ARP_CanUpdateTable (IP_A IPAddress, MAC_A MACAddress) {

INT16U a;                     // counter var
INT8U  dummybuffer[6];        // temporarily buffer
a = 0;

if (ARP_IsNewMACAddress (IPAddress, (MAC_A *)&dummybuffer[0]) == 0)
    return(FALSE);                      // IP address ardy found in list!
while (a<ARPENTRIES)
{
 if (ArpTable.IP_Address[a].d == 0)     // this position at "a" is empty
 {
  ArpTable.IP_Address[a]  = IPAddress;
  ArpTable.MAC_Address[a] = MACAddress;
  ArpTable.Time_Out[a]    = TIMEOUT;
  return(TRUE);                         // OK, new entry in ARP table
 }
a++;
}
return (FALSE);                         // ARP map ardy full!
}



// ---------------------------------------------------------------------------------------------
// function:    void ARP_RemoveTimedOutEntries ()
// parameters:  <none>
//
// result:      <none>
//
// purpose:                 remove timed out entries from arp table
// ---------------------------------------------------------------------------------------------

void ARP_RemoveTimedOutEntries (void)
{
  INT16U nVar;
  
  nVar = 0;

  while ( nVar < ARPENTRIES )
  {
    if ( ArpTable.Time_Out[nVar] > 0 )
    {
      ArpTable.Time_Out[nVar] -= DECREMENT;
    }
    else
    {
					 ArpTable.IP_Address[nVar].d     = 0;   // erase IP address field
					 ArpTable.MAC_Address[nVar].w[0] = 0;   // and MAC address fields
					 ArpTable.MAC_Address[nVar].w[1] = 0;
					 ArpTable.MAC_Address[nVar].w[2] = 0;
					 ArpTable.Time_Out[nVar]         = 0;   // set time out interval to 0
    }
    nVar++;
  }
}



// ---------------------------------------------------------------------------------------------
// function:     int ARP_AreFieldsChecked(ARP_HEADER huge *pARP)
// parameters:   <*pARP>     pointer to incoming ARP header
// result:       0 (FALSE)   at least one field does not meet its desired value
//               1 (TRUE)    fields meet the requirements
// purpose:              checks some header fields of an incoming ARP echo request
// ---------------------------------------------------------------------------------------------

int ARP_AreFieldsChecked(ARP_HEADER *pARP) {

if (pARP->HardwAdrSpace    != 0x0100) return(FALSE);// hardware type: 0001, Ethernet 10MBit
if (pARP->ProtocolAdrSpace != 0x0008) return(FALSE);// protocol type: 0800, standard
if (pARP->HwLen            != 0x0006) return(FALSE);// HW address length = 6
if (pARP->ProtLen          != 0x0004) return(FALSE);// IP address length = 4
return (TRUE);
}


// ---------------------------------------------------------------------------------------------
// function:     ARP_Send (MAC_A Targetaddress, ARP_HEADER huge *pARPdest, INT16U Datalength)
// parameters:   <Targetaddress>   Ethernet address where to send the ARP message
//               <*pARPdest>       pointer to ARP header to be sent
//               <Datalength>      length of ARP header
// result:       <none>
// purpose:                    send an ARP message
// ---------------------------------------------------------------------------------------------

void ARP_Send (MAC_A Targetaddress, ARP_HEADER *pARPdest, INT16U Datalength) {

INT16U   EProtocol;      // protocol type variable
MAC_A    myAddress;     // my MAC address buffer


EProtocol = 0x0608;     // protocol type is 0806 = ARP (toggled)
memcpy((void *)&myAddress, (void *)&my_mac, ETH_ALEN);// my address is the sender address

 // --- send ethernet frame by common send routine ---
ETHER_SendFrame (myAddress,               // my hardware address
                 Targetaddress,            // target hardware address
                 EProtocol,                // protocol type (ARP)
                 (INT16U *)pARPdest,       // pointer to ARP header
                 Datalength);              // length of ARP header
}


// ---------------------------------------------------------------------------------------------
// function:     ARP_EchoData_Return (ARP_HEADER huge *pARP,
//                                    INT16U *ARPlength)
// parameters:   <*pARP>       pointer to incoming ARP message
//               <*ARPlength>  length of ARP header
// result:       <none>
// purpose:                builds the ARP reply message according to an incoming
//                         echo request
// ---------------------------------------------------------------------------------------------

void ARP_EchoDataReturn (ARP_HEADER *pARP,       // pointer to incoming ARP request
                         INT16U *ARPlength) {         // number of bytes for ARP echo

  pARP->Opcode = 0x0200;                              // ARP reply code
  memcpy((void*)&pARP->DestHwAdr, (void*)&pARP->SenderHwAdr, ETH_ALEN);  // target HW address
  memcpy((void*)&pARP->DestProtAdr,(void*) &pARP->SenderProtAdr, IP_LEN);// target IP address
  memcpy((void*)&pARP->SenderHwAdr, (void*)&my_mac, ETH_ALEN); // my HW address
  memcpy((void*)&pARP->SenderProtAdr, (void*)&my_ip, IP_LEN);  // my IP address

  *ARPlength = sizeof(ARP_HEADER);                    // 28 bytes total for ARP header
}



// ---------------------------------------------------------------------------------------------
// function:     void ARP_ProcessEchoRequest (ARP_HEADER huge *pARP)
// parameters:   <*pARP>     pointer to incoming ARP message
// result:       <none>
// purpose:              process an incoming ARP echo request. Build the ARP echo
//                       message using <ARP_EchoDataReturn> and launch <ARP_Send>
//                       for transmission of the resulting message
// ---------------------------------------------------------------------------------------------

void ARP_ProcessEchoRequest (ARP_HEADER *pARP) {

MAC_A  TargetMacAddress;              // address buffer
INT16U  Datalength;                   // length of ARP message
// target:is it me? 0 means addresses are the same!
if (memcmp((void*)&my_ip, (void*)&pARP->DestProtAdr, IP_LEN))
    return;                               // quit if address does not match
 TargetMacAddress = pARP->SenderHwAdr;    // new target is the sender of the original msg
ARP_CanUpdateTable (pARP->SenderProtAdr,
                    pARP->SenderHwAdr);   // update ARP table and ignore function result
ARP_EchoDataReturn (pARP, &Datalength);   // generate ARP echo data

if (Datalength != 0)                      // something returned?
{
 ARP_Send  (TargetMacAddress,             // send ARP reply
            (ARP_HEADER *)pARP,
            Datalength);
}
//TODO:
/*
printf("arp replay %d.%d.%d.%d is at %x:%x:%x:%x:%x:%x\n",
        my_ip.b[0], my_ip.b[1], my_ip.b[2], my_ip.b[3], my_mac.b[0],
        my_mac.b[1], my_mac.b[2], my_mac.b[3], my_mac.b[4], my_mac.b[5]);
ARP_TablePrint ();
*/
}



// ---------------------------------------------------------------------------------------------
// function:     void ARP_ProcessEchoReply (ARP_HEADER huge *pARP)
// parameters:   <*pARP>       pointer to incoming ARP message
// result:       <none>
// purpose:               proceed with an incoming echo reply. This function does
//                        not send an my message, but uses the received one for
//                        an ARP table update.
// ---------------------------------------------------------------------------------------------

void ARP_ProcessEchoReply (ARP_HEADER *pARP) {

if (memcmp((void*)&my_ip, (void*)&pARP->DestProtAdr, IP_LEN)) return;// target:is it me? 0 means adresses are the same
ARP_CanUpdateTable (pARP->SenderProtAdr, pARP->SenderHwAdr); // update the ARP list
//TODO:
/*
printf("arp replay %d.%d.%d.%d is at %x:%x:%x:%x:%x:%x\n",
        pARP->SenderProtAdr.b[0], pARP->SenderProtAdr.b[1], pARP->SenderProtAdr.b[2], pARP->SenderProtAdr.b[3], pARP->SenderHwAdr.b[0],
        pARP->SenderHwAdr.b[1], pARP->SenderHwAdr.b[2], pARP->SenderHwAdr.b[3], pARP->SenderHwAdr.b[4], pARP->SenderHwAdr.b[5]);
*/
}




// ---------------------------------------------------------------------------------------------
// function:     void ARP_SendEchoRequest (IP_A TargetIPAddress)
// parameters:   <TargetIPAddress>    IP address
// result:       <none>
// purpose:               sends an ARP echo request (broadcast) to determine the
//                        appropriate MAC address matching to the given IP address.
//                        This is done by reading incoming ARP echo reply messages
// ---------------------------------------------------------------------------------------------

void ARP_SendEchoRequest (IP_A TargetIPAddress) {

ARP_HEADER *pARPDest;                                   // destination ARP header
MAC_A TargetMacAddress;                                 // buffer for the target MAC address
OS_ERR     ErrVar;

  /*TODO: change PacketMemArea?*/
  pARPDest = (ARP_HEADER *)OSMemGet(&PacketMemArea, &ErrVar);// get memory block from Lan transmit partition
  if ( pARPDest == NULL )
  {
//TODO:
   #if DEBUG
   printf("ARP_SendEchoRequest\n");
   #endif
   return;
  }
  pARPDest->HardwAdrSpace    = 0x0100;       // hardware type = Ethernet
  pARPDest->ProtocolAdrSpace = 0x0008;       // IP type
  pARPDest->HwLen            = 0x0006;       // HW address 6 Byte long
  pARPDest->ProtLen          = 0x0004;       // IP address 4 Byte long
  pARPDest->Opcode           = 0x0100;       // ARP request code
  // set my address fields
  memcpy((void*)&pARPDest->SenderHwAdr,   (void*)&my_mac, ETH_ALEN); // my HW address
  memcpy((void*)&pARPDest->SenderProtAdr, (void*)&my_ip, IP_LEN);    // my IP address
  // set target IP field
  memcpy((void*)&pARPDest->DestProtAdr, (void*)&TargetIPAddress, IP_LEN);    // my IP address
  // set target MAC address to unknmy (ARP header field)
  pARPDest->DestHwAdr.w[0] = 0;                   // unknmy MAC
  pARPDest->DestHwAdr.w[1] = 0;
  pARPDest->DestHwAdr.w[2] = 0;
  // set target MAC address to broadcast (ARP_Send target)
  TargetMacAddress.w[0] = 0xFFFF;                 // broadcast MAC
  TargetMacAddress.w[1] = 0xFFFF;
  TargetMacAddress.w[2] = 0xFFFF;
  // Send ARP echo request
  ARP_Send (TargetMacAddress,                     // send broadcast
            (ARP_HEADER *)pARPDest,
            sizeof(ARP_HEADER));
  //TODO:
  /*
  printf("arp who-has %d.%d.%d.%d tell %d.%d.%d.%d\n",
          TargetIPAddress.b[0], TargetIPAddress.b[1],
          TargetIPAddress.b[2], TargetIPAddress.b[3],
          my_ip.b[0], my_ip.b[1], my_ip.b[2], my_ip.b[3]);
*/
  OSMemPut(&PacketMemArea, pARPDest,&ErrVar);
  if(ErrVar != OS_ERR_NONE)
	  for(;;);
}





// ---------------------------------------------------------------------------------------------
// function:     void ARP_AllTypes_Process (ARP_HEADER huge *pARP)
// parameters:   <*pARP>     pointer to the received ARP message
// result:       <none>
// purpose:              continues with incoming ARP messages and determines
//                       the message type for further investigation
// ---------------------------------------------------------------------------------------------

void ARP_AllTypesProcess (ARP_HEADER *pArp) {

if (ARP_AreFieldsChecked(pArp) != TRUE) return;     // check fields in the ARP header
switch (pArp->Opcode) {                             // message type?
 // ARP echo request received
 case 0x0100:                                       // ARP echo request type
//TODO:
	 /*
          printf("arp who-has %d.%d.%d.%d tell %d.%d.%d.%d\n",
          pArp->DestProtAdr.b[0], pArp->DestProtAdr.b[1],
          pArp->DestProtAdr.b[2], pArp->DestProtAdr.b[3],
          pArp->SenderProtAdr.b[0], pArp->SenderProtAdr.b[1],
          pArp->SenderProtAdr.b[2], pArp->SenderProtAdr.b[3]);
*/

  ARP_ProcessEchoRequest ((ARP_HEADER *)pArp); // continue with echo request
  break;
 // ARP echo reply coming in
 case 0x0200:                                       // ARP echo reply type
  ARP_ProcessEchoReply ((ARP_HEADER *)pArp);   // continue with reply message
  break;
 default:                                           // ignore other commands
  break;
}
}

// ---------------------------------------------------------------------------------------------
// EOF
// ---------------------------------------------------------------------------------------------
