// ---------------------------------------------------------------------------------------------
// PROJECT:     TCP Stack for Keil MCB167-NET
// UNIT:        CLOCK.C
// PURPOSE:     Clock and timer procedures, provides default timers for LED control and
//              various custom timers.
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
// timer structure definition
// ---------------------------------------------------------------------------------------------

typedef struct programmable_timer {     // programmable timer type
         procref EventProcedure;        // user definable event procedure
         INT32U   LastTime;             // last time event proc has been called (miliseconds)
         INT32U   Interval;             // interval for next call (miliseconds)
} __attribute__ ((packed)) PROGRAMMABLE_TIMER;

// ---------------------------------------------------------------------------------------------
// clock and ISN variables
// ---------------------------------------------------------------------------------------------

INT32U miliseconds;           // current system time in miliseconds
INT32U ISNnumber;             // ISN generator

// ---------------------------------------------------------------------------------------------
// 10 programmable timers
// ---------------------------------------------------------------------------------------------

static PROGRAMMABLE_TIMER ProgrammedTimer[MAX_SOCKET_NUMBERS];

// ---------------------------------------------------------------------------------------------
// reserved timers
// ---------------------------------------------------------------------------------------------

//static PROGRAMMABLE_TIMER Timer_DataReceivedLED   = {NULL,0,0};   // LED 8
//static PROGRAMMABLE_TIMER Timer_DataSentLED       = {NULL,0,0};   // LED 7
//static PROGRAMMABLE_TIMER Timer_AliveLED          = {NULL,0,0};   // LED 1
//static PROGRAMMABLE_TIMER Timer_ChecksumErrorLED  = {NULL,0,0};   // LED 2
static PROGRAMMABLE_TIMER Timer_TCPRetransmission = {NULL,0,0};   // no display

// ---------------------------------------------------------------------------------------------
// procedure heads for following procedures
// ---------------------------------------------------------------------------------------------

void CLOCK_AliveLEDOn (void);
void CLOCK_AliveLEDOff (void);

// ---------------------------------------------------------------------------------------------
// function:    INT32U CLOCK_GetTime (void)
// parameters:  <none>
// result:      INT32U      system time (ms)
// purpose:            returns the system time in miliseconds
// ---------------------------------------------------------------------------------------------

INT32U  CLOCK_GetTime (void) {
return (miliseconds);         // get milliseconds
}

// ---------------------------------------------------------------------------------------------
// function:    INT32U CLOCK_GetISN (void)
// parameters:  <none>
// result:      INT32U       current ISN generator value
// purpose:            returns the initial sequence number
// ---------------------------------------------------------------------------------------------

INT32U CLOCK_GetISN (void) {
return (ISNnumber);          // get ISN number
}

// ---------------------------------------------------------------------------------------------
// function:     void CLOCK_DataReceivedLEDOff (void)
// parameters:   <none>
// result:       <none>
// purpose:               switches the "data received" LED off and sets the event
//                        routine to NULL
// ---------------------------------------------------------------------------------------------

//void CLOCK_DataReceivedLEDOff (void) {
////UTILS_SwitchOFFGreenLED (2);
//
//Timer_DataReceivedLED.EventProcedure = NULL;   // switch timer off
//}

// ---------------------------------------------------------------------------------------------
// function:     void CLOCK_DataReceivedLEDFlash (void)
// parameters:   <none>
// result:       <none>
// purpose:              switches the "data received" LED on, but sets event
//                       procedure and time offset for clearing.
// ---------------------------------------------------------------------------------------------

//void CLOCK_DataReceivedLEDFlash (void) {
////UTILS_SwitchONGreenLED (2);
//
//Timer_DataReceivedLED.EventProcedure = &CLOCK_DataReceivedLEDOff;
//Timer_DataReceivedLED.LastTime       = CLOCK_GetTime ();
//Timer_DataReceivedLED.Interval       = 100;
//}

// ---------------------------------------------------------------------------------------------
// function:     void CLOCK_DataSentLEDOff (void)
// parameters:   <none>
// result:       <none>
// purpose:              switches the "data sent" LED off and sets the event
//                       routine to NULL
// ---------------------------------------------------------------------------------------------

//void CLOCK_DataSentLEDOff (void) {
////UTILS_SwitchOFFGreenLED (3);
//
//Timer_DataSentLED.EventProcedure = NULL;    // switch timer off
//}

// ---------------------------------------------------------------------------------------------
// function:     void CLOCK_DataSentLEDFlash (void)
// parameters:   <none>
// result:       <none>
// purpose:                switches the "data sent" LED on, but sets event
//                         procedure and time offset for clearing.
// ---------------------------------------------------------------------------------------------

//void CLOCK_DataSentLEDFlash (void) {
////UTILS_SwitchONGreenLED (3);
//
//Timer_DataSentLED.EventProcedure = &CLOCK_DataSentLEDOff;
//Timer_DataSentLED.LastTime       = CLOCK_GetTime ();
//Timer_DataSentLED.Interval       = 100;
//}

// ---------------------------------------------------------------------------------------------
// function:    void CLOCK_ChecksumErrorLEDOff (void)
// parameters:  <none>
// result:      <none>
// purpose:              switches the "checksum error" LED off and sets the event
//                       routine to NULL
// ---------------------------------------------------------------------------------------------

//void CLOCK_ChecksumErrorLEDOff (void) {
////UTILS_SwitchOFFGreenLED (4);
//
//Timer_ChecksumErrorLED.EventProcedure = NULL;      // switch timer off
//}

// ---------------------------------------------------------------------------------------------
// function:     void CLOCK_ChecksumErrorLEDFlash (void)
// parameters:   <none>
// result:       <none>
// purpose:              switches the "checksum error" LED on, but sets event
//                       procedure and time offset for clearing.
// ---------------------------------------------------------------------------------------------

//void CLOCK_ChecksumErrorLEDFlash (void) {
////UTILS_SwitchONGreenLED (4);
//
//Timer_ChecksumErrorLED.EventProcedure = &CLOCK_ChecksumErrorLEDOff;
//Timer_ChecksumErrorLED.LastTime       = CLOCK_GetTime ();
//Timer_ChecksumErrorLED.Interval       = 5000;                      // 5 seconds warning
//}

// ---------------------------------------------------------------------------------------------
// function:     void CLOCK_AliveLEDOn (void)
// parameters:   <none>
// result:       <none>
// purpose:              switches the "i-am-alive" LED on
// ---------------------------------------------------------------------------------------------

//void CLOCK_AliveLEDOn (void) {
////UTILS_SwitchONGreenLED (1);
//
//Timer_AliveLED.EventProcedure = &CLOCK_AliveLEDOff;
//Timer_AliveLED.LastTime       = CLOCK_GetTime ();
//Timer_AliveLED.Interval       = 1000;
//}

// ---------------------------------------------------------------------------------------------
// function:     void CLOCK_AliveLEDOff (void)
// parameters:   <none>
// result:       <none>
// purpose:               switches the "i-am-alive" LED off
// ---------------------------------------------------------------------------------------------

//void CLOCK_AliveLEDOff (void) {
////UTILS_SwitchOFFGreenLED (1);
//
//Timer_AliveLED.EventProcedure = &CLOCK_AliveLEDOn;
//Timer_AliveLED.LastTime       = CLOCK_GetTime ();    // to be precise
//Timer_AliveLED.Interval       = 1000;
//}

// ---------------------------------------------------------------------------------------------
// function:     void CLOCK_ServeProgrammableTimer (PROGRAMMABLE_TIMER  *Timer)
// parameters:   <*Timer>    timer to be frequently checked
// result:       <none>
// purpose:               serves programmable timers, checks if some timers are
//                        defined	in a sufficient way and the moment to call the
//                        event procedure has come, then executes this procedure.
// ---------------------------------------------------------------------------------------------

void CLOCK_ServeProgrammableTimer (PROGRAMMABLE_TIMER  *Timer) {
INT32U NextCallTime;

if (Timer->EventProcedure != NULL)
{
 NextCallTime = Timer->LastTime + Timer->Interval;
 if (CLOCK_GetTime() >= NextCallTime)
 {
  Timer->EventProcedure ();           // call event procedure
  Timer->LastTime = CLOCK_GetTime();  // update last call time
 }
}
}

// ---------------------------------------------------------------------------------------------
// function:    void CLOCK_SetProgrammableTimer (procref EventProcedure, int timerID,
//                                               INT32U lasttime, INT32U interval)
// parameters:  <Eventprocedure>      address of custom event handling proc
//              <timerID>             timer number [0..9]
//              <lasttime>            last time the event proc has been called
//              <interval>            interval for next call
// result:      <none>
// purpose:                    sets all neccessary custom timer parameters for use
// ---------------------------------------------------------------------------------------------

void CLOCK_SetProgrammableTimer (procref EventProcedure,    // event procedure for this timer
                                 int   timerID,             // ID of programmable timer
                                 INT32U lasttime,            // time of last call
                                 INT32U interval) {          // interval of this timer

ProgrammedTimer[timerID].EventProcedure = EventProcedure;
ProgrammedTimer[timerID].LastTime       = lasttime;
if (interval > 50)                                 // don't let the interval be to small!
    ProgrammedTimer[timerID].Interval = interval;
else
    ProgrammedTimer[timerID].Interval = 50;        // 50 msec minimum!
}

// ---------------------------------------------------------------------------------------------
// function:    void CLOCK_InitRegisters (void)
// parameters:  <none>
// result:      <none>
// purpose:                initializes the controller timer/counter registers
// ---------------------------------------------------------------------------------------------

void CLOCK_InitRegisters (void) {

//T2CON = 0x0000;        // T3 timer mode, full period = 26ms.
//T3CON = 0x0000;        // T3 timer mode, full period = 26ms.
//T3UD  = 1;             // Count down full period
//T2IC  = 0x0000;        // IE = 0, ILVL = 2, GLVL = 0
//T3IC  = 0x0044;        // IE = 1, ILVL = 1, GLVL = 0
}

// ---------------------------------------------------------------------------------------------
// function:     void CLOCK_InitStandardTimers (void)
// parameters:   <none>
// result:       <none>
// purpose:               standard timer set-up, set custom ones to NULL
// ---------------------------------------------------------------------------------------------

void CLOCK_InitStandardTimers (void) {
int a;

 // retransmission timer
Timer_TCPRetransmission.EventProcedure = &SOCKET_refresh; // retransmit due to current state
Timer_TCPRetransmission.LastTime       = CLOCK_GetTime ();// last time called is NOW
Timer_TCPRetransmission.Interval       = 26;              // 1 second interval
 // programmable ones
for (a=0; a<MAX_SOCKET_NUMBERS; a++)
{
 ProgrammedTimer[a].EventProcedure = NULL;
 ProgrammedTimer[a].LastTime       = 0;
 ProgrammedTimer[a].Interval       = 0;
}
}

// ---------------------------------------------------------------------------------------------
// function:    void CLOCK_Start (void)
// parameters:  <none>
// result:      <none>
// purpose:             lets the clock run by enabling all neccessary INTs
// ---------------------------------------------------------------------------------------------

void CLOCK_Start (void) {

 miliseconds = 0;
 ISNnumber   = 0;
}

// ---------------------------------------------------------------------------------------------
// function:    void CLOCK_ServeSocketTimers (void)
// parameters:  <none>
// result:      <none>
// purpose:               serves the socket timers
// ---------------------------------------------------------------------------------------------

void CLOCK_ServeSocketTimers (void) {
int a;                         // counter var

for (a=0; a<10; a++)
SOCKET_DecrementAllTimers (a); // decrement all
}

// ---------------------------------------------------------------------------------------------
// function:    void CLOCK_UpdateTime (void)
// parameters:  <none>
// result:      <none>
// purpose:              used to update the system time and to launch timer events
// ---------------------------------------------------------------------------------------------

void CLOCK_UpdateTime (void) {
int a;

miliseconds += CLOCK_CYCLETIME;   // update system time (add 26ms)
ISNnumber   += 1;                 // increase ISN (every 26ms instead of 4ms)

 // call reserved timer procedures
/*CLOCK_ServeProgrammableTimer ((PROGRAMMABLE_TIMER  *)
                               &Timer_DataReceivedLED);
CLOCK_ServeProgrammableTimer ((PROGRAMMABLE_TIMER  *)
                               &Timer_DataSentLED);
CLOCK_ServeProgrammableTimer ((PROGRAMMABLE_TIMER  *)
                               &Timer_AliveLED);
CLOCK_ServeProgrammableTimer ((PROGRAMMABLE_TIMER  *)
                               &Timer_ChecksumErrorLED);*/
CLOCK_ServeProgrammableTimer ((PROGRAMMABLE_TIMER  *)
                               &Timer_TCPRetransmission);
 // decrease all socket timer values
CLOCK_ServeSocketTimers ();        // decrement all socket timer values
 // call
for (a=0; a<MAX_SOCKET_NUMBERS; a++)
 CLOCK_ServeProgrammableTimer ((PROGRAMMABLE_TIMER  *)   // serve all custom timers
                                &ProgrammedTimer[a]);
}

// ---------------------------------------------------------------------------------------------
// END OF  UNIT
// ---------------------------------------------------------------------------------------------

