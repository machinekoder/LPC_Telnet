// ---------------------------------------------------------------------------------------------
// PROJECT:     TCP Stack for Keil MCB167-NET
// UNIT:        UTILITY_PROCS.H
// PURPOSE:     provides diverse utility procedures such as display, byte/word shift or
//              checksum computing functions.
// AUTHOR:      Martin Zauner, Matr.Nr.9911011114
// COPYRIGHT:   (c)2003, Fachhochschule Technikum Wien, Modul Telekom und
//              Computer & Systemtechnik
// LAST UPDATE: 24-04-2003
// ---------------------------------------------------------------------------------------------

#ifndef UTILITY_DEFINES
 #define UTILITY_DEFINES

// --------------------------------------------------------------------------------------------
// general read and write buffer sizes and types
// --------------------------------------------------------------------------------------------

#define PORT2MASK         0x00FF     // 8 LEDs on port 2
#define PORT2VALUE        0x00FF     // value used for direction settings
#define BUF_RxMAXWORDS    766        // max. 766 words, equivalent to 1532 bytes
#define BUF_TxMAXWORDS    768        // max. 768 words, equivalent to 1536 bytes
#define BUF_RxMAXBYTES    1532
#define BUF_TxMAXBYTES    1536
#define BYTESTORECEIVE    1400
#define BYTESTOSEND       1400
typedef INT16U TxBufferType[BUF_TxMAXWORDS];  // transmit buffer type

// ---------------------------------------------------------------------------------------------
// function:    INT16U UTILS_ToggleBytes (INT16U InData)
// parameters:  <InData>      incoming INT16U for switching the bytes
// result:      INT16U         resulting INT16U
// purpose:              moves the highbyte to the lowbyte position and vice versa
// ---------------------------------------------------------------------------------------------

INT16U UTILS_ToggleBytes (INT16U InData);

// ---------------------------------------------------------------------------------------------
// function:      INT32U UTILS_ToggleWordss (INT32U InData)
// parameters:    <InData>    incoming INT32U for switching the words
// result:        INT32U       resulting INT32U
// purpose:              moves the highword to the lowword position and vice versa
// ---------------------------------------------------------------------------------------------

INT32U UTILS_ToggleWords (INT32U InData);

// ---------------------------------------------------------------------------------------------
// function:     INT16U UTILS_CalcChecksum (INT16U *ReadBuffer, INT16U NumOfBytes)
// parameters:   <*ReadBuffer>   pointer to data origin
// parameters:   <NumOfBytes>    size of data area for computing the checksum
// result:       INT16U           resulting checksum
// purpose:                generates the IP or TCP checksum
// ---------------------------------------------------------------------------------------------

INT16U UTILS_CalcChecksum (INT16U *ReadBuffer, INT16U NumOfBytes);


#endif

// --------------------------------------------------------------------------------------------
// END OF UNIT
// --------------------------------------------------------------------------------------------

