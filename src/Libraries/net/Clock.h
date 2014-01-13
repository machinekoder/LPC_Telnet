// ---------------------------------------------------------------------------------------------
// PROJECT:     TCP Stack for Keil MCB167-NET
// UNIT:        CLOCK.H
// PURPOSE:     Clock and timer procedures, provides default timers for LED control and
//              various custom timers.
// AUTHOR:      Martin Zauner, Matr.Nr.9911011114
// COPYRIGHT:   (c)2003, Fachhochschule Technikum Wien, Modul Telekom und
//              Computer & Systemtechnik
// LAST UPDATE: 24-04-2003
// ---------------------------------------------------------------------------------------------

#ifndef CLOCK_DEFINES
 #define CLOCK_DEFINES

#include "tcp_statehandler.h"    // TCP State Machine

// ---------------------------------------------------------------------------------------------
// function:    INT32U CLOCK_GetTime (void)
// parameters:  <none>
// result:      INT32U     system time (ms)
// purpose:           returns the system time in miliseconds
// ---------------------------------------------------------------------------------------------

INT32U CLOCK_GetTime (void);

// ---------------------------------------------------------------------------------------------
// function:    INT32U CLOCK_GetISN (void)
// parameters:  <none>
// result:      INT32U      current ISN generator value
// purpose:             returns the initial sequence number
// ---------------------------------------------------------------------------------------------

INT32U CLOCK_GetISN (void);

// ---------------------------------------------------------------------------------------------
// function:    void CLOCK_DataReceivedLEDFlash (void)
// parameters:  <none>
// result:      <none>
// purpose:             switches the "data received" LED on, but sets event
//                      procedure and time offset for clearing.
// ---------------------------------------------------------------------------------------------

void CLOCK_DataReceivedLEDFlash (void);

// ---------------------------------------------------------------------------------------------
// function:    void CLOCK_DataSentLEDFlash (void)
// parameters:  <none>
// result:      <none>
// purpose:             switches the "data sent" LED on, but sets event
//                      procedure and time offset for clearing.
// ---------------------------------------------------------------------------------------------

void CLOCK_DataSentLEDFlash (void);

// ---------------------------------------------------------------------------------------------
// function:    void CLOCK_ChecksumErrorLEDOff (void)
// parameters:  <none>
// result:      <none>
// purpose:             switches the "checksum error" LED on, but sets event
//                      procedure and time offset for clearing.
// ---------------------------------------------------------------------------------------------

void CLOCK_ChecksumErrorLEDFlash (void);

// ---------------------------------------------------------------------------------------------
// function:    void CLOCK_AliveLEDOn (void)
// parameters:  <none>
// result:      <none>
// purpose:             switches the "i-am-alive" LED on.
// ---------------------------------------------------------------------------------------------

void CLOCK_AliveLEDOn (void);

// ---------------------------------------------------------------------------------------------
// function:    void CLOCK_AliveLEDOff (void)
// parameters:  <none>
// result:      <none>
// purpose:             switches the "i-am-alive" LED off
// ---------------------------------------------------------------------------------------------

void CLOCK_AliveLEDOff (void);

// ---------------------------------------------------------------------------------------------
// function:    void CLOCK_SetProgrammableTimer (procref EventProcedure, int timerID,
//                                               INT32U lasttime, INT32U interval)
// parameters:  <Eventprocedure>     address of custom event handling proc
//              <timerID>            timer number [0..9]
//              <lasttime>           last time the event proc has been called
//              <interval>           interval for next call
// result:      <none>
// purpose:                   sets all neccessary custom timer parameters for use
// ---------------------------------------------------------------------------------------------

void CLOCK_SetProgrammableTimer (procref EventProcedure,    // event procedure for this timer
                                 int timerID,               // ID of programmable timer
                                 INT32U lasttime,            // time of last call
                                 INT32U interval);

// ---------------------------------------------------------------------------------------------
// function:    void CLOCK_InitRegisters (void)
// parameters:  <none>
// result:      <none>
// purpose:               initializes the controller timer/counter registers
// ---------------------------------------------------------------------------------------------

void CLOCK_InitRegisters (void);

// ---------------------------------------------------------------------------------------------
// function:    void CLOCK_InitStandardTimers (void)
// parameters:  <none>
// result:      <none>
// purpose:               standard timer set-up, set custom ones to NULL
// ---------------------------------------------------------------------------------------------

void CLOCK_InitStandardTimers (void);

// ---------------------------------------------------------------------------------------------
// function:    void CLOCK_Start (void)
// parameters:  <none>
// result:      <none>
// purpose:             lets the clock run by enabling all neccessary INTs
// ---------------------------------------------------------------------------------------------

void CLOCK_Start (void);

// ---------------------------------------------------------------------------------------------
// function:    void CLOCK_UpdateTime (void)
// parameters:  <none>
// result:      <none>
// purpose:             used to update the system time and to launch timer events
// ---------------------------------------------------------------------------------------------

void CLOCK_UpdateTime (void);

#endif

// ---------------------------------------------------------------------------------------------
// END OF  UNIT
// ---------------------------------------------------------------------------------------------

