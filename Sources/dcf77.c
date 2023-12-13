/*  Radio signal clock - DCF77 Module

    Computerarchitektur 3
    (C) 2018 J. Friedrich, W. Zimmermann Hochschule Esslingen

    Author:   W.Zimmermann, Jun  10, 2016
    Modified: -
*/

/*
; A C H T U N G:  D I E S E  S O F T W A R E  I S T  U N V O L L S T � N D I G
; Dieses Modul enth�lt nur Funktionsrahmen, die von Ihnen ausprogrammiert werden
; sollen.
*/


#include <hidef.h>                                      // Common defines
#include <mc9s12dp256.h>                                // CPU specific defines
#include <stdio.h>

#include "dcf77.h"
#include "led.h"
#include "clock.h"
#include "lcd.h"
#include "buttons.h"


// Global variable holding the last DCF77 event
DCF77EVENT dcf77Event = NODCF77EVENT;

// Exterm global vars
extern char usTimeZoneFlag;
extern char toggleTimeZoneFlag;

// Modul internal global variables
int  dcf77Year = 2017, dcf77Month = 1, dcf77Day = 1, dcf77Hour = 0, dcf77Minute = 0, dcf77DayOfWeek = 0;       //dcf77 Date and time as integer values
int tempYear = 2017, tempMonth = 0, tempDay = 0, tempHour = 0, tempMinute = 0, tempDayOfWeek = 0;
char lastSignal = 0, parityCounter = 0, parityErrorFlag = 0, bitNum = 0, bitVal = 0, currentSignal = 0, edgeStatus = NO_EDGE;
char syncFlag = 0;




// Prototypes of functions simulation DCF77 signals, when testing without
// a DCF77 radio signal receiver
void initializePortSim(void);                   // Use instead of initializePort() for testing
char readPortSim(void);                         // Use instead of readPort() for testing

// ****************************************************************************
// Initalize the hardware port on which the DCF77 signal is connected as input
// Parameter:   -
// Returns:     -
void initializePort(void)
{
    DDRH = 0x00;
}

// ****************************************************************************
// Read the hardware port on which the DCF77 signal is connected as input
// Parameter:   -
// Returns:     0 if signal is Low, >0 if signal is High
char readPort(void)
{
    if(PTH_PTH0)
    {
        return 1;
    }

    return 0;
}

// ****************************************************************************
//  Initialize DCF77 module
//  Called once before using the module
void initDCF77(void)
{
    setLED(0x04); // not synchronized DCF77 signal at startup
    setClock((char) dcf77Hour, (char) dcf77Minute, 0);
    displayDateDcf77();

    initializePort();
}

// ****************************************************************************
// Display the date derived from the DCF77 signal on the LCD display, line 1
// Parameter:   -
// Returns:     -
void displayDateDcf77(void)
{   char datum[32];
    
    (void) sprintf(datum, "%s %02d.%02d.%04d", getDayOfWeekCharArr(dcf77DayOfWeek), dcf77Day, dcf77Month, dcf77Year);
    writeLine(datum, 1);
}

// ****************************************************************************
//  Read and evaluate DCF77 signal and detect events
//  Must be called by user every 10ms
//  Parameter:  Current CPU time base in milliseconds
//  Returns:    DCF77 event, i.e. second pulse, 0 or 1 data bit or minute marker
DCF77EVENT sampleSignalDCF77(int currentTime)
{
    DCF77EVENT event = NODCF77EVENT;
    //currentSignal = readPortSim();
    currentSignal = readPort();
    edgeStatus = getSignalEdge(currentSignal, lastSignal);  
    lastSignal = currentSignal; // get lastSignal ready for next call

    if(currentSignal)
    {
        setLED(0x02);
    }
    else
    {
        clrLED(0x02);
    }

    if(parityErrorFlag)
    {
        parityErrorFlag = 0;
        parityCounter = 0;

        return INVALID;
    }
    
    if(usTimeZoneFlag && toggleTimeZoneFlag)
    {
        adjustDcf77ForUsTimeZone();
        toggleTimeZoneFlag = 0;
        displayDateDcf77();
        setClock(dcf77Hour, dcf77Minute, getSecs());
        syncFlag = 0;
    }
    else if(!usTimeZoneFlag && toggleTimeZoneFlag)
    {
        adjustDcf77ForDeTimeZone();
        toggleTimeZoneFlag = 0;
        displayDateDcf77();
        setClock(dcf77Hour, dcf77Minute, getSecs());
        syncFlag = 0;
    }
    
    switch(edgeStatus)
    {
        case(NO_EDGE):
            return NODCF77EVENT;
            break;
        case(POS_EDGE):
            if(currentTime >= 70 && currentTime <= 130)
            {
                return VALIDZERO;
            }
            else if(currentTime >= 170 && currentTime <= 230)
            {
                return VALIDONE;
            }
            else
            {
                return INVALID;
            }
            break;
        case(NEG_EDGE):
            if(currentTime >= 900 && currentTime <= 1100)
            {
                return VALIDSECOND;
            }
            else if(currentTime >= 1900 && currentTime <= 2100)
            {
                return VALIDMINUTE;
            }
            else
            {
                return INVALID;
            }
            break;
        default:
            // Shouldnt happen --> Error
            return INVALID;
            break;
    }
}

// ****************************************************************************
// Process the DCF77 events
// Contains the DCF77 state machine
// Parameter:   Result of sampleSignalDCF77 as parameter
// Returns:     -
void processEventsDCF77(DCF77EVENT event)
{
    switch(event)
    {
        case NODCF77EVENT:
            break;
        case VALIDZERO:
            bitVal = 0;
            break;
        case VALIDONE:
            bitVal = 1;
            break;
        case VALIDSECOND:
            processBit(bitNum, bitVal);
            bitNum++;
            break;
        case VALIDMINUTE:
            if(syncFlag)
            {
              processBit(bitNum, bitVal);
              clrLED(0x04);
              setLED(0x08);
              setDcf77vars();
              setClock(dcf77Hour, dcf77Minute, 0);
              displayDateDcf77();
            }
            else
            {
              syncFlag = 1;
            }
            
            bitNum = 0;            
            resetTempVars();
            
            break;
        case INVALID:
        default:
            // Shouldnt happen --> Error
            resetTempVars();
            syncFlag = 0;
            clrLED(0x08);
            setLED(0x04);
            bitVal = -1;
            break;
    }
}

// ****************************************************************************
// Process the signal edges
// Parameter:   current signal value and last signal value
// Returns:     defines: POS_EDGE (1), NEG_EDGE (2) or NO_EDGE (4)
char getSignalEdge(char currentSignal, char lastSignal)
{
    if(currentSignal == lastSignal)
    {
        return NO_EDGE;
    }
    
    if((lastSignal - currentSignal) < 0)
    {
        return POS_EDGE;
    }
    else
    {
        return NEG_EDGE;
    }
}

// ****************************************************************************
// Process the bits with according process value
// Parameter:   current signal value and last signal value
// Returns:     -
void processBit(char bitNum, char signalVal)
{
    // Handle parity bits
    //-------------------
    // Parity for minutes
    //-------------------
    if(28 == bitNum)
    {
        checkParity(signalVal);
    }
    else if(35 == bitNum)
    {
        checkParity(signalVal);
    }
    else if(58 == bitNum)
    {
        checkParity(signalVal);
    }

    // Handle data bits
    //-----------------
    if(signalVal)
    {
        parityCounter++;

        if(bitNum >= 21 && bitNum <= 27)                
        {
            bitNum = bitNum - 21;
            handleBit(bitNum, &tempMinute);
        }
        else if(bitNum >= 29 && bitNum <= 34)
        {
            bitNum = bitNum - 29;
            handleBit(bitNum, &tempHour);
        }
        else if(bitNum >= 36 && bitNum <= 41)
        {
            bitNum = bitNum - 36;
            handleBit(bitNum, &tempDay);
        }
        else if(bitNum >= 42 && bitNum <= 44)
        {
            bitNum = bitNum - 42;
            handleBit(bitNum, &tempDayOfWeek);
        }
        else if(bitNum >= 45 && bitNum <= 49)
        {
            bitNum = bitNum - 45;
            handleBit(bitNum, &tempMonth);
        }
        else if(bitNum >= 50 && bitNum <= 57)
        {
            bitNum = bitNum - 50;
            handleBit(bitNum, &tempYear);
        }
    }
}

// ****************************************************************************
// Increment variable by bit order
// Bit Order:   0   1   2   3   4    5    6    7
//              |   |   |   |   |    |    |    |
//              v   v   v   v   v    v    v    v
// Bit Value:   1   2   4   8   10   20   40   80
// Parameter:   bit-order, variable pointer
// Returns:     -
void handleBit(char bitNum, int* var)
{
    switch(bitNum)
    {
        case 0:             // Bit 0
            *var += 1;
            break;
        case 1:             // Bit 1
            *var += 2;
            break;
        case 2:             // Bit 2
            *var += 4;
            break;
        case 3:             // Bit 3
            *var += 8;
            break;
        case 4:             // Bit 4
            *var += 10;
            break;
        case 5:             // Bit 5
            *var += 20;
            break;
        case 6:             // Bit 6
            *var += 40;
            break;
        case 7:             // Bit 7
            *var += 80;
            break;
        default:
            // Shouldnt happen -> Error
            break;
    }
}

// ****************************************************************************
// Resets temp DCF77 global-vars
// Parameter:   -
// Returns:     -
void resetTempVars()
{
    tempDay = 0;
    tempMonth = 0;
    tempYear = 2000;
    tempMinute = 0;
    tempHour = 0;
    tempDayOfWeek = 0;
}

// ****************************************************************************
// Sets DCF77 global vars from temp
// Parameter:   -
// Returns:     -
void setDcf77vars()
{
    dcf77Day = tempDay;
    dcf77Month = tempMonth;
    dcf77Year = tempYear;
    dcf77Minute = tempMinute;
    dcf77Hour = tempHour;
    dcf77DayOfWeek = tempDayOfWeek;
}

// ****************************************************************************
// Sets parity flag to 1 when there is a data error
// Parameter:   signal value
// Returns:     -
void checkParity(char signalVal)
{
    if((parityCounter % 2) != signalVal)
    {
        parityErrorFlag = 1;
    }
}

// ****************************************************************************
// Converts numeric value of dayOfWeek to char-pointer
// Parameter:   signal value for dayOfWeek
// Returns:     char-pointer
char* getDayOfWeekCharArr(char dayOfWeek)
{
    switch(dayOfWeek)
    {
        case 1:
            return "Mon";
        case 2:
            return "Tue";
        case 3:
            return "Wen";
        case 4:
            return "Thu";
        case 5:
            return "Fri";
        case 6:
            return "Sat";
        case 7:
            return "Sun";
        default:
            return "Inv";
    }
}

char* getTimeZoneCharArr()
{
    if(usTimeZoneFlag)
    {
        return "US"; 
    }
    else
    {
        return "DE";
    }
}

void adjustDcf77ForUsTimeZone()
{
    if(dcf77Hour < 6)
    {
        dcf77Hour = dcf77Hour + 18;
        
        if(1 == dcf77DayOfWeek)
        {
            dcf77DayOfWeek = 7;          
        }
        else
        {
            dcf77DayOfWeek = dcf77DayOfWeek - 1;
        }
        
    }
    else
    {
        dcf77Hour = dcf77Hour - 6; 
    }  
       
}

void adjustDcf77ForDeTimeZone()
{
    
    if(dcf77Hour > 17)
    {
        dcf77Hour = dcf77Hour - 18;
        
        if(7 == dcf77DayOfWeek)
        {
            dcf77DayOfWeek = 1;          
        }
        else
        {
            dcf77DayOfWeek = dcf77DayOfWeek + 1;
        }
        
    }
    else
    {
        dcf77Hour = dcf77Hour + 6; 
    }     
}
