// --------------------------------------------------------------------------------------------- 
// PROJECT:     TCP Stack for Keil MCB167-NET
// UNIT:        TCP.C
// PURPOSE:     Basic TCP unit, provides the universal segment send primitive <TCP_SendSegment>
//              and the decoding routine TCP_Process, which forwards incoming TCP information to
//              the state machine.
// AUTHOR:      Martin Zauner, Matr.Nr.9911011114
// COPYRIGHT:   (c)2003, Fachhochschule Technikum Wien, Modul Telekom und 
//              Computer & Systemtechnik
// LAST UPDATE: 24-04-2003
// --------------------------------------------------------------------------------------------- 

// -------------------------------------------------------------------------------------
// includes
// -------------------------------------------------------------------------------------

#include "net_includes.h"
extern IP_A my_ip;
extern OS_MEM PacketMemArea;
extern INT16U tcp_ID;


// --------------------------------------------------------------------------------------------- 
// extern global variables
// --------------------------------------------------------------------------------------------- 

// --------------------------------------------------------------------------------------------- 
// function:    void TCP_SendSegment  (IP_A IP_Address, INT16U Sourceport, INT16U Destport,	
//                                     INT32U SEQnum, INT32U ACKnum, INT16U Flags, INT16U WinSize, 
//                                     int  MaxSegSize, INT16U  *Data, INT16U Datalength)
// parameters:  <IP_Address>        target IP address 
//              <Sourceport>        own port
//              <Destport>          destination port
//              <SEQnum>            sequence number
//              <ACKnum>            acknowledge number
//              <Flags>             flags to be sent
//              <WinSize>           desired window size
//              <MaxSegSize>        maximum segment size 
//              <*Data>             pointer to payload
//              <Datalength>        length of payload
// result:      <none>
// purpose:                 TCP segment send procedure, builds a new packet due to
//                          the desired parameters and forwards it down to IP level 
// --------------------------------------------------------------------------------------------- 

void TCP_SendSegment(IP_A IP_Address, INT16U Sourceport, 
                     INT16U Destport, INT32U SEQnum, INT32U ACKnum,
                     INT16U Flags, INT16U WinSize, int  MaxSegSize, 
                     INT16U *Data, INT16U Datalength) {

INT16U NewChecksum,          // checksum variable 
       TCPlength,            // length of TCP message only
       TCPtotallength;       // length of TCP message and pseudo header, for checksum

OS_ERR ErrVar;
INT32U SegLenOpt;

TCP_PSEUDO_HEADER  *pTCP_ps_dest;      // TCP pseudo header
TCP_HEADER         *pTCP_dest;         // TCP header

 // ------ call system function to dynamically allocate buffer storage -------
/*TODO: change PacketMemArea?*/
pTCP_ps_dest = (TCP_PSEUDO_HEADER  *)OSMemGet(&PacketMemArea, &ErrVar);// get memory block from Lan transmit partition
if ( pTCP_ps_dest == NULL )
{
  #ifdef DEBUGF
  DEBUG("TCP_SendSegment");
  #endif // DEBUGF
  return;
}
 // ------ pointer adjustments ------
pTCP_dest = (TCP_HEADER  *)&pTCP_ps_dest->TCPheader;// append header to pseudo header

// ------ addresses ------
memcpy (&(pTCP_ps_dest->source.d), &my_ip.d, IP_LEN);
pTCP_ps_dest->dest.d = (IP_Address.d);                  // IP target address

 // -- other fields
pTCP_ps_dest->dummy      = 0;         // field must be zero
pTCP_ps_dest->protocol   = 6;         // protocol = TCP
pTCP_ps_dest->TCPlength  = 0;         // length is options dependent, dummy value 0

 // --- TCP header -----------------------------------
pTCP_dest->sourceport   = UTILS_ToggleBytes(Sourceport); // my source port
pTCP_dest->destport     = UTILS_ToggleBytes(Destport);   // destination port
pTCP_dest->seqnum       = UTILS_ToggleWords(SEQnum);     // sequence number
pTCP_dest->acknum       = UTILS_ToggleWords(ACKnum);     // ack number
pTCP_dest->offs_res_flags = UTILS_ToggleBytes(Flags | 0x6000);// data offset(6*32Bit= 24Bytes), reserved (=0), flags
pTCP_dest->window       = UTILS_ToggleBytes(WinSize);    // window size is ... bytes
pTCP_dest->checksum     = 0;     // checksum = 0, for new checksum calculation
pTCP_dest->urgentptr    = 0;     // not urgent

 // --- enter max seg size and copy payload ---
if (MaxSegSize > 0)              // yep, send max segment size
{
 SegLenOpt = 0x02040000 | (0x0FFFF & MaxSegSize); 
 pTCP_dest->options = UTILS_ToggleWords(SegLenOpt);// max segment size is MaxSegSize bytes
 if (Datalength>0)                                 // data to be sent?
 {
  memcpy(&pTCP_dest->data, 
          (INT16U  *)&Data[0], 
           Datalength);                    // copy raw values from socket to TCP data area
 }
 TCPlength = sizeof (TCP_HEADER) - 
                     2 + Datalength;       // size of TCP message (without pseudo header)
 pTCP_ps_dest->TCPlength = UTILS_ToggleBytes(TCPlength);// length of message, options not used
 TCPtotallength = TCPlength + 
             sizeof (TCP_PSEUDO_HEADER) - 2;// length of TCP package + pseudo header in bytes
}
else
{// no, do not use options
 pTCP_dest->offs_res_flags = UTILS_ToggleBytes(Flags|0x5000);// only 20 byte header! no options
 if (Datalength>0)                            // data to be sent?
 {
  pTCP_dest->options = 0;                     // set options to zero
  memcpy(&pTCP_dest->options, 
         (INT16U  *)&Data[0], Datalength);// copy raw values from socket to TCP data area
 }
 TCPlength = sizeof(TCP_HEADER) - 6 + Datalength;// size of TCP message (without pseudo header)			
 pTCP_ps_dest->TCPlength = UTILS_ToggleBytes(TCPlength);// length of message, options not used
 TCPtotallength = TCPlength + 
             sizeof(TCP_PSEUDO_HEADER) - 2;  // length of TCP package + pseudo header in bytes
}

 // --- calculate checksum ---
NewChecksum = UTILS_CalcChecksum ((INT16U  *)pTCP_ps_dest,
              TCPtotallength);               // new checksum - TCP header
pTCP_dest->checksum = (NewChecksum);         // accept checksum

 // --- test checksum, value must be zero now ---
NewChecksum = UTILS_CalcChecksum((INT16U  *)&pTCP_ps_dest->source, 
              TCPtotallength);               // must be 0!
 // --- send ----
tcp_ID++;
IP_SendDatagram (IP_Address,                 // address where to send datagram
                 6,                          // type of message = TCP
                 tcp_ID,                     // fragment ID
                 0,                          // not fragmented
                                             // pointer to first element of TCP segment 
                 (INT16U  *)&pTCP_dest->sourceport,
                 TCPlength,                  // segment length in bytes
                 (void *)pTCP_ps_dest);      // pointer holding dynamic memory block level 4

}

// --------------------------------------------------------------------------------------------- 
// function:     void TCP_Process (TCP_HEADER  *pTCP, INT16U TCPlen, INT32U IPaddress) { 
// parameters:   <*pTCP>        pointer to received segment
//               <TCPlen>       length of segment
//               <IPaddress>    source address of segment
// result:       <none>
// purpose:              extracts fields into readable variables, gets options and
//                       finds out max. seg size if desired, then moves data into
//                       a local buffer and informs the TCP central.
// --------------------------------------------------------------------------------------------- 

void TCP_Process (TCP_HEADER *pTCP, 
                  INT16U TCPlen, 
                  INT32U IPaddress) { 
INT16U Flags,                       // flags of incoming segment
       sourceport, destport,        // port IDs
       Datasize,                    // size of payload in bytes
       Dataoffset,                  // offset of the payload origin to the "->options" field.
                                    // (Having no options means "Dataoffset" = 0) 
       OffsResFlags,                // Offset+Reserved+Flags INT16U
       HdrPlusOptions,              // header length including options
       Optionstype,                 // type of options (if existing)
       Window,                      // window value received
       SegmentMaxSize;              // maximum segment size requested by incoming TCP packet
INT32U SEQnum,                      // sequence number
       Options,                     // "->options" field value
       ACKnum;                      // acknowledge number
INT16U  *Dataptr;               // pointer to local buffer
int    socketno;                    // socket descriptor


 // -- default max. segment size ---
SegmentMaxSize = -1;                             // max segment size not used
 // --- extract fields into readable variables ---
sourceport = UTILS_ToggleBytes(pTCP->sourceport);// foreign port
destport   = UTILS_ToggleBytes(pTCP->destport);  // own port
SEQnum     = UTILS_ToggleWords(pTCP->seqnum);    // SEQ num
ACKnum     = UTILS_ToggleWords(pTCP->acknum);    // ACK num
OffsResFlags = UTILS_ToggleBytes(pTCP->offs_res_flags);// eg. 1405 toggled = 5*4byte hdr, 20Dec = RST/ACK
HdrPlusOptions = ((OffsResFlags) & (0xF000)) >> 12;    // total header size in 32bit groups
HdrPlusOptions = HdrPlusOptions * 4;                   // size in bytes
Flags          = ((OffsResFlags) & (0x00FF));          // flags
Window         = UTILS_ToggleBytes	(pTCP->window);     // foreign window size
Options        = UTILS_ToggleWords	(pTCP->options);// first word of options (or data, if not used)
 // eg.0x02040100 = option 2, length 4, 100H max seg. size
Datasize = TCPlen - HdrPlusOptions;                // payload size
 // determine options type, if existing
Optionstype = (Options & 0xFF000000) >> 24;        // options type field (if used)
 // --- check, if options are used ---
Dataoffset = HdrPlusOptions - 20;  // header size - 20 byte default is offset = options size
if (Dataoffset > 0)                // options used! check type <Optionstype>
{ switch (Optionstype) 
  {         
    case 2:                        // Optionstype = 2, "max segment size" given
     SegmentMaxSize = (Options & 0x000000FF);// accept requested maximum segment size 
      break;
    default:                                 // ignore other options eg. 1= "no-option-list"
      break;
  }
}
 // -- move data into data buffer ---
if (Datasize > 0)                      // if there is payload to receive, copy segment data
{ 
 Dataptr = (INT16U  *)&pTCP->options+Dataoffset;
}
else
{
 Dataptr = NULL;                      // give NULL pointer to central
}
socketno = SOCKET_usedSocketno (destport, IPaddress, sourceport); 

 // -- call central --                  // "UNUSED" error, if UNBOUND or UNAVAILABLE
TCPSTATE_Central(socketno,              // ignore function result
                 IPaddress,             // foreign address
                 sourceport,            // foreign port 
                 destport,              // own port
                 SEQnum,                // incoming SEQ value
                 ACKnum,                // incoming ACK value
                 Flags,                 // incoming flags
                 TCP_CONN_NONE,
                 Window,                // incoming window size limit
                 SegmentMaxSize,        // max segment size desired
                (INT16U  *)Dataptr, // pointer to incoming data
                 Datasize);             // data	size in bytes
}

// -------------------------------------------------------------------------------------------
// END OF UNIT
// -------------------------------------------------------------------------------------------

