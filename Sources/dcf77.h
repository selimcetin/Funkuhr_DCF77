/*  Header for DCF77 module

    Computerarchitektur 3
    (C) 2018 J. Friedrich, W. Zimmermann Hochschule Esslingen

    Author:   W.Zimmermann, Jun  10, 2016
    Modified: -
*/

// Defines
#define POS_EDGE 1
#define NEG_EDGE 2
#define NO_EDGE  4

// Data type for DCF77 signal events
typedef enum { NODCF77EVENT, VALIDZERO, VALIDONE, VALIDSECOND, VALIDMINUTE, INVALID } DCF77EVENT;

// Global variable holding the last DCF77 event
extern DCF77EVENT dcf77Event;

// Public functions, for details see dcf77.c
void initDCF77(void);
void displayDateDcf77(void);
DCF77EVENT sampleSignalDCF77(int currentTime);
void processEventsDCF77(DCF77EVENT event);
char getSignalEdge(char currentSignal, char lastSignal);
void processBit(char bitNum, char signalVal);
void handleBit(char bitNum, int* var);
void resetTempVars(void);
void setDcf77vars(void);
void checkParity(char signalVal);
char* getDayOfWeekCharArr(char dayOfWeek);
void adjustDcf77ForUsTimeZone(void);
void adjustDcf77ForDeTimeZone(void);
char* getTimeZoneCharArr(void);