// ---------------------------------------------------------------------------------------------
// PROJECT:     TCP Stack for Keil MCB167-NET
// UNIT:        IP.C
// PURPOSE:     Up to now IP and ICMP echo reply messages are supported.
//              Calls functions of higher levels, such as TCP routines.
// AUTHOR:      Martin Zauner, Matr.Nr.9911011114
// COPYRIGHT:   (c)2003, Fachhochschule Technikum Wien, Modul Telekom und
//              Computer & Systemtechnik
// LAST UPDATE: 24-04-2003
// ---------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------
// includes
// ---------------------------------------------------------------------------------------------

#include "net_includes.h"

// ---------------------------------------------------------------------------------------------
// buffer variables
// ---------------------------------------------------------------------------------------------

TxBufferType Level3_checkbuffer;      // buffer for IP checksum control
extern IP_A my_ip;
extern OS_MEM PacketMemArea;
extern MAC_A my_mac;
// ---------------------------------------------------------------------------------------------
// function:    void IP_SendDatagram (IP_A IP_Address, INT16U IP_Protocol, INT16U Identification,
//                                    INT16U FragmentOffset, INT16U  *Data, INT16U Datalen)
// parameters:  <IP_Address>      IP address where to send the datagram
//              <IP_Protocol>     protocol type of the IP payload eg. TCP or UDP type
//              <Identification>  identification of the TCP/UDP fragment
//              <FragmentOffset>  fragment offset (ignored)
//              <*Data>           pointer to IP data area, segments may be placed here
//              <Datalen>         data length
// result:      <none>
// purpose:                  creates a new datagram using the *Data payload pointer
//                           and launches transmission by calling ETHER_Sendframe
//
// note:        1.) fragmentation is not supported with this version!
//              2.) uses IP version 4 only
// ---------------------------------------------------------------------------------------------

void IP_SendDatagram (IP_A IP_Address,        // address where to send datagram
                      INT16U IP_Protocol,     // type of message (ICMP, TCP)
                      INT16U Identification,  // fragment ID
                      INT16U FragmentOffset,  // fragment offset
                      INT16U  *Data,      // Message to be embedded into IP datagram
                      INT16U Datalen,         // Length of IP data
                      void * pTxLevel4) {     // pointer holding dynamic memory block

IP_HEADER  *pIPdest;           // buffer var for building the new datagram

MAC_A TargetAddress;               // hardware addresses
MAC_A OwnAddress;
INT16U NewChecksum,                //hecksum var
      DatagramLen,                 // length of datagram
      IP_Totallength,              // total length of the IP packet
      IP_HeaderLen,                // IP header length
      ETH_HeaderLen,               // Ethernet header length
      ICMP_HeaderLen;              // ICMP header length

OS_ERR ErrVar;

 // ------ call system function to dynamically allocate buffer storage -------
/*TODO: change PacketMemArea?*/
pIPdest = (IP_HEADER  *)OSMemGet(&PacketMemArea, &ErrVar);// get memory block from Lan transmit partition
if ( pIPdest == NULL )
{
  #if DEBUG
  //DEBUG("IP_SendDatagram GetBufBlock");
  #endif
 return;
}

IP_HeaderLen         = sizeof(IP_HEADER) - 2;      // IP_Header length
ICMP_HeaderLen       = sizeof(ICMP_HEADER) - 2;    // ICMP Header length
ETH_HeaderLen        = sizeof(ETHERNET_HEADER) - 2;// ETHERNET Header length
IP_Totallength       = Datalen + IP_HeaderLen;     // total size of IP datagram (bytes toggled)
DatagramLen          = IP_Totallength;             // again, length of datagram
 // set header fields
pIPdest->Vht         = 0x0045;                     // IP4, std. length
pIPdest->Length      = UTILS_ToggleBytes(DatagramLen);// total length (plus header) in bytes
pIPdest->Identification = Identification;          // datagram number
pIPdest->Frag        = UTILS_ToggleBytes (0x4000 |
                       FragmentOffset);            // don't fragment, but accept offset value
pIPdest->ttlProtocol = (IP_Protocol << 8) | 0x0080;// 01 = ICMP protocol, 80H = 128D (TTL)
pIPdest->Checksum    = 0;                          // reset checksum prior to new CRC calculation
pIPdest->dest        = IP_Address;                 // set target address
memcpy(&pIPdest->source,(INT16U  *)&my_ip.d,// use own IP address as source
        IP_LEN);
 // generate checksum
NewChecksum = UTILS_CalcChecksum((INT16U  *)pIPdest,// new checksum for this IP header
                                  IP_HeaderLen);
pIPdest->Checksum = NewChecksum;                       // accept new checksum
 // move payload
memcpy((INT16U  *)&pIPdest->Data[0],
        (INT16U  *)&Data[0], Datalen);  // move data from allocd space to new IP data area
 // ------ return dynamic level 4 buffer to LAN Tx memory partition --------
if ( pTxLevel4 != NULL )
{
 OSMemPut(&PacketMemArea, pTxLevel4,&ErrVar);// put memory block back to Lan transmit partition
 if(ErrVar != OS_ERR_NONE)
	 for(;;);
 #if DEBUG
   //ASSERT("IP_SendDatagram PutBufBlock", check == TRUE);
 #endif
}
 // get own MAC address
memcpy((INT16U *)&OwnAddress, (INT16U *)my_mac.b, ETH_ALEN);// get own MAC address for ETHER_SendFrame
 // get target MAC address
if (ARP_IsNewMACAddress (IP_Address,      // get MAC address for this IP
   (MAC_A *)&TargetAddress) == TRUE)      // MAC not found in ARP list?
{
 ARP_SendEchoRequest (IP_Address);        // launch ARP request
 return;                                  // bailing out, try again next time
}
ETHER_SendFrame(OwnAddress, TargetAddress,// MAC addresses
                8,                        // send ethernet frame using type 8 (10Base)
                (INT16U  *)pIPdest,   // pIPdest is placed here
                IP_Totallength);          // length of datagram
OSMemPut(&PacketMemArea, (void *)pIPdest,&ErrVar);
if(ErrVar != OS_ERR_NONE)
	for(;;);
}

// ---------------------------------------------------------------------------------------------
// function:    void IP_TCPProcess (IP_HEADER  *pIP, INT16U TCPlen)
// parameters:  <*pIP>        IP address where to send the datagram
//              <TCPlen>      protocol type of the IP payload eg. TCP or UDP type
// result:      <none>
// purpose:             continues with datagram, knowing there is a TCP segment
//                      located in the data area. The function will extract the
//                      segment and call an OSI level 4 proc for further investigation
// note:        1.) incoming IP options are ignored.
//              2.) both IP header checksum and TCP checksum control
//                  is done here, even if this unit must not handle OSI
//                  level 4 protocols.
//                  But knowing the IP data is of TCP type and the data area
//                  contains the checksum field somewhere, overall testing
//                  can be performed without extracting the segment itself.
// ---------------------------------------------------------------------------------------------

void IP_TCPProcess (IP_HEADER  *pIP, INT16U TCPlen) {
INT16U  TCP_Dataoffset,             // TCP data offset due to IP option length
       IP_Hdrlen_PlusOptions,      // IP header size including options
       NewChecksum,                // checksum variable
       IP_Totallength,             // total length of the IP packet
       IP_HeaderLen;               // IP header length

TCP_HEADER  *pTCP;             // TCP header var to be overlaid with IP data
TCP_HEADER  *pTCPcheck;        // for checksum control
IP_HEADER   *pIPcheck;         // for checksum control

pIPcheck = (IP_HEADER  *)&Level3_checkbuffer;// overlay array space for mem allocation!
IP_HeaderLen  = sizeof(IP_HEADER) - 2;           // default IP_Header length without options
IP_Hdrlen_PlusOptions = (pIP->Vht) & (0x000F);   // IP header length (in x 32bit, includes options)
 // rebuild IP header checksum
NewChecksum = UTILS_CalcChecksum((INT16U  *)pIP,// rebuild checksum for this IP header
                                  IP_Hdrlen_PlusOptions*4);
if (NewChecksum != 0)              // IP hdr checksum included, so result must be 0!
{
 //CLOCK_ChecksumErrorLEDFlash ();   // flash LED 2
 return;                           // failed IP header checksum test!
}
IP_Totallength = (IP_Hdrlen_PlusOptions*4) + TCPlen;      // total size of IP datagram (bytes toggled)
TCP_Dataoffset = (IP_Hdrlen_PlusOptions*4) - IP_HeaderLen;// offset due to existing options (strip them)
pTCP           = (TCP_HEADER  *)
                  &pIP->Data[TCP_Dataoffset]; // move source TCP pointer to IP data area
memcpy((INT16U  *)&pIPcheck->Vht, (INT16U  *)&pIP->Vht,// copy pIP to pIPcheck (from Vht field on)
         IP_Totallength);
pTCPcheck = (TCP_HEADER  *)
             &pIPcheck->Data[TCP_Dataoffset]; // move TCPcheck pointer to IPcheck data area
 // set fields for checksum control
pIPcheck->ttlProtocol = 0x0600;                    // TCP type assumed
pIPcheck->Checksum    = UTILS_ToggleBytes(TCPlen); // replaced by segment size to simulate pseudo hdr
NewChecksum = UTILS_CalcChecksum ((INT16U *)   // calculate checksum..
                         &pIPcheck->ttlProtocol,   // ...from here
                         (IP_Totallength-8));      // bytes long, include segment checksum!
if (NewChecksum != 0)                        // TCP checksum included, so result must be 0!
{
 //CLOCK_ChecksumErrorLEDFlash ();             // flash LED 2
  return;                                    // TCP checksum error
}
// continue with verified datagram

TCP_Process ((TCP_HEADER  *)pTCP, TCPlen,// TCP continued without IP options
              pIP->source.d);                // sender address

}

// ---------------------------------------------------------------------------------------------
// function:    void IP_ICMPEchoReturn(ICMP_HEADER  *pICMP, INT16U ICMPlen,
//                                     ICMP_HEADER  *pICMPdest, INT16U *ICMPdestlen)
// parameters:  <*pICMP>        pointer to incoming ICMP message
//              <ICMPlen>       length of ICMP message
//              <*pICMPdest>    pointer to destination ICMP message (ardy allocated)
//              <*pICMPdestlen> length of new-built message
// result:      <none>
// purpose:                builds an ICMP echo reply message
// ---------------------------------------------------------------------------------------------

void IP_ICMPEchoReturn(ICMP_HEADER  *pICMP,    // incoming ICMP msg (ardy allocated)
                       INT16U ICMPlen,             // length of message
                       ICMP_HEADER  *pICMPdest,// destination ICMP packet (ardy allocated)
                       INT16U *ICMPdestlen) {      // resulting ICMP message length

INT16U NewChecksum,        // checksum value variable
       ICMP_Data_Length,   // ICMP data length
       ICMP_HeaderLen;     // ICMP header length

 // constants
ICMP_HeaderLen    = sizeof(ICMP_HEADER) - 2;   // ICMP Header length
ICMP_Data_Length  = ICMPlen - ICMP_HeaderLen;  // ICMP data length
 // set ICMP fields
pICMPdest->ICMP_Type  = 0;                   // echo reply type
pICMPdest->Code       = 0;                   // standard value
pICMPdest->Checksum   = 0;                   // reset prior to new checksum calculation
pICMPdest->Identifier = pICMP->Identifier;   // port ID
pICMPdest->Sequence   = pICMP->Sequence;     // number of datagram
 // copy payload from incoming to outgoing message
memcpy ((INT16U  *)&pICMPdest->Data[0],  // copy payload
         (INT16U  *)&pICMP->Data[0],
         ICMP_Data_Length);
 // compute checksum
NewChecksum = UTILS_CalcChecksum((INT16U *)pICMPdest,
                                 ICMPlen);   // new checksum for ICMP msg
pICMPdest->Checksum = NewChecksum;           // accept new checksum
*ICMPdestlen        = ICMPlen;               // length of new ICMP message
}

// ---------------------------------------------------------------------------------------------
// function:     void IP_ICMPProcess (IP_A TargetAddress, INT16U Identification,
//                                    ICMP_HEADER  *pICMP, INT16U ICMPlen)
// parameters:   <TargetAddress>   pointer to incoming ICMP message
//               <Identification>  length of ICMP message
//               <*pICMP>          pointer to received ICMP message (memory allocated)
//               <ICMPlen>         length of ICMP message
// result:       <none>
// purpose:             Forwards the received ICMP message to the IP_ICMPEchoReturn
//                      procedure for creating the appropriate ICMP reply data.
//                      Then the reply message is sent back to the sender of the
//                      ICMP echo request.
// ---------------------------------------------------------------------------------------------

void IP_ICMPProcess (IP_A TargetAddress,
                     INT16U Identification,
                     ICMP_HEADER  *pICMP, // pointer to incoming IP packet (allocated!)
                     INT16U ICMPlen) {        // bytes to be transfered

INT8U Service_Type;          // type of ICMP message
INT16U ICMPdestlen;
ICMP_HEADER  *pICMPdest;
OS_ERR ErrVar;

 // ------ call system function to dynamically allocate buffer storage -------
/*TODO: change PacketMemArea?*/
pICMPdest = (ICMP_HEADER  *)OSMemGet(&PacketMemArea, &ErrVar);// get memory block from Lan transmit partition
if ( pICMPdest == NULL )
{
  #if DEBUG
  //DEBUG("IP_ICMPProcess");
  #endif // DEBUG
 return;
}

Service_Type = pICMP->ICMP_Type;                   // type of ICMP message
switch (Service_Type) {
   case 0x0008:                                    // ICMP Echo request
   // prepare new ICMP
   IP_ICMPEchoReturn((ICMP_HEADER  *)pICMP,    // incoming ICMP msg
                      ICMPlen,                     // length of incoming ICMP
                     (ICMP_HEADER  *)pICMPdest,// resulting ICMP
                      &ICMPdestlen);               // length of the new ICMP
   // send ICMP via IP
   IP_SendDatagram (TargetAddress,                 // address where to send
                    1,                             // message type: ICMP Echo reply
                    Identification,                // datagram identification
                    0,                             // do not fragment
                    (INT16U  *)pICMPdest,      // ICMP to be sent, allocated
                    ICMPdestlen,                   // ICMP total length
                    (void *)pICMPdest );           // pointer to dynamic memory block
   //TODO: ISSUE?
   //ErrVar = OSMemPut(PacketMemArea, pICMPdest);
   break;
    default:
        break;
  }
}

// ---------------------------------------------------------------------------------------------
// function:    void IP_AllTypesProcess (IP_HEADER  *pIP)
// parameters:  <*pIP>     pointer to incoming IP datagram
// result:      <none>
// purpose:            reads the protocol field of the datagram to determine
//                     the transported payload type, then forwards the datagram
//                     to the general TCPIP or ICMP processing routine.
// ---------------------------------------------------------------------------------------------

void IP_AllTypesProcess (IP_HEADER  *pIP) {// Parameter: pointer to incoming IP

INT16U I_Protocol,           // protocol type variable
       IP_Datalength,        // IP data length
       IP_HeaderLen;         // IP header length
ICMP_HEADER  *pICMP;     // ICMP header to overlay with datagram
int   IP_Dataoffset;         // data offset

pICMP = (ICMP_HEADER  *)pIP->Data;   // move ICMP pointer to IP data area
if ((((pIP->Vht) & (0x00F0))>>4) != 4)
    return;                              // supports IP 4 only
I_Protocol = UTILS_ToggleBytes(pIP->ttlProtocol) & 0x00FF;
 // determine length of IP data
if (memcmp(&my_ip.d, &pIP->dest, IP_LEN))
    return;                               // is it my ip address?
IP_HeaderLen  = 4*((pIP->Vht) & (0x000F));// get the IP header length (+options)
IP_Datalength = (UTILS_ToggleBytes(pIP->Length)// data len = total len - header len
                 & 0xFFFF) - IP_HeaderLen;     // length of payload (ICMP length)
IP_Dataoffset = IP_HeaderLen - sizeof(IP_HEADER) +2;// offset due to existing options (strip them)
pICMP = (ICMP_HEADER  *)&pIP->Data[IP_Dataoffset];// move ICMP pointer (IP options now stripped)
 // determine level 3 protocol
switch (I_Protocol) {
    case 0x0001:                                        // ICMP type
    IP_ICMPProcess (pIP->source, pIP->Identification,
            (ICMP_HEADER  *)pICMP, IP_Datalength);  // total ICMP message length
    break;
    case 0x0006:                                        // call TCP routine
    IP_TCPProcess((IP_HEADER  *)pIP, IP_Datalength);// TCP message, continue
        break;
    default:                                            // other (e.g. UDP) not implemented
        break;
  }
}

// ---------------------------------------------------------------------------------------------
// END OF UNIT
// ---------------------------------------------------------------------------------------------

