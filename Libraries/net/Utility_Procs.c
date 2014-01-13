// ---------------------------------------------------------------------------------------------
// PROJECT:     TCP Stack for Keil MCB167-NET
// UNIT:        UTILITY_PROCS.C
// PURPOSE:     provides diverse utility procedures such as display, byte/word shift or
//              checksum computing functions.
// AUTHOR:      Martin Zauner, Matr.Nr.9911011114
// COPYRIGHT:   (c)2003, Fachhochschule Technikum Wien, Modul Telekom und
//              Computer & Systemtechnik
// LAST UPDATE: 24-04-2003
// ---------------------------------------------------------------------------------------------

// ---------------------------------------------------------------------------------------------
// includes
// ---------------------------------------------------------------------------------------------

#include "includes.h"


// ---------------------------------------------------------------------------------------------
// function:    INT16U UTILS_ToggleBytes (INT16U InData)
// parameters:  <InData>     incoming INT16U for switching the bytes
// result:      <INT16U>     resulting INT16U
// purpose:               moves the highbyte to the lowbyte position and vice versa
// ---------------------------------------------------------------------------------------------

INT16U UTILS_ToggleBytes (INT16U InData) {

INT16U ResultingData;                         // buffer for the return value

ResultingData = (((InData) & (0xFF00))>>8) |
                (((InData) & (0x00FF))<<8);   // toggle bytes
return(ResultingData);
}


// ---------------------------------------------------------------------------------------------
// function:    INT32U UTILS_ToggleWordss (INT32U InData)
// parameters:  <InData>    incoming INT32U for switching the words
// result:      <INT32U>    resulting INT32U
// purpose:               moves the highword to the lowword position and vice versa
// ---------------------------------------------------------------------------------------------

INT32U UTILS_ToggleWords (INT32U InData) {

INT32U ResultingData;                              // buffer for the return value

ResultingData = (((InData) & (0xFF000000))>>24) |  // toggle words
                 (((InData) & (0x00FF0000))>>8) |
                 (((InData) & (0x0000FF00))<<8) |
                 (((InData) & (0x000000FF))<<24);
return(ResultingData);
}

// ---------------------------------------------------------------------------------------------
// function:    INT16U UTILS_CalcChecksum (INT16U *ReadBuffer, INT16U NumOfBytes)
// parameters:  <*ReadBuffer>  pointer to data origin
// parameters:  <NumOfBytes>   size of data area for computing the checksum
// result:      <INT16U>       resulting checksum
// purpose:                 generates the IP or TCP checksum
// ---------------------------------------------------------------------------------------------

INT16U UTILS_CalcChecksum (INT16U *ReadBuffer, INT16U NumOfBytes) {

INT16U CurrentWord,                         // the current word is stored herein
       NumOfWords,                          // total number of words (INT16U type)
       answer;                              // resulting value
INT32U CurrentDWord,                        // current word (larger INT32U) for shift
       a,                                   // counter var
       WordSum,                             // accumulator var
       Highword,                            // highword var
       Lowword;                             // lowword var
BOOLEAN   isodd;                               // even/odd byte number indicator
float  dummy;                               // needed for correct ceil() usage

dummy      = NumOfBytes;                    // reload bytesnum to float var
NumOfWords =((dummy+1)/2);                    // need float type for ceil ()!
isodd      = ((NumOfWords*2) !=             // even or odd number of bytes?
                  (NumOfBytes));
a          = 0;                             // reset counter
WordSum    = 0;                             // word sum is zero
while (a < NumOfWords)                      // while not end of data area
{
 CurrentWord = (ReadBuffer[a]);             // current word
 if (isodd && (a == NumOfWords-1))          // last word and odd byte no. ?
     CurrentWord &= 0x00FF;                 // then blank last byte!
 CurrentDWord = CurrentWord;                // copy to INT32U variable
 WordSum      = WordSum + CurrentDWord;     // sum (INT32U)
 a++;
}                                           // Wordsum is INT32U type: 0xABCDEFGH
Highword = (WordSum >> 16) & 0x0000ffff;    // Highword is now 0x0000ABCD
Lowword  =  WordSum & 0x0000ffff;           // Lowword is 0x0000EFGH
WordSum  =  Highword + Lowword;             // New Result is Wordsum: 0x000IJKLM !
Highword = (WordSum >> 16) & 0x0000ffff;    // Highword is now 0x0000000I
Lowword  =  WordSum & 0x0000ffff;           // Lowword is 0x0000JKLM
WordSum  =  Highword + Lowword;             // New Result is Wordsum:0x0000NOMP
answer   = ~WordSum;                        // complement
return (answer);                            // return checksum

}



// --------------------------------------------------------------------------------------------
// END OF UNIT
// --------------------------------------------------------------------------------------------


