; export symbols
        XDEF delay_0_5sec

; Defines

; RAM: Variable data section
.data: SECTION

; ROM: Constant data
.const: SECTION

; ROM: Code section
.init: SECTION
        
; include derivative specific macros
        INCLUDE 'mc9s12dp256.inc'

IMAX:	  EQU		2048



; Public interface function: delay_0_5sec
; Generate a 0.5-second delay using two nested counter loops.
; Parameters: -
; Returns: -
; Registers: Modified (X and Y registers are modified by the subroutine)

delay_0_5sec:
        LDX  #IMAX					    ; 12mio cycles * 1/24 MHz = 0.5s delay
waitO:  LDY  #IMAX                      ; (Uses two nested counter loops with registers X and Y)
waitI:  DBNE Y, waitI                   ; --- Decrement Y and branch to waitI if not equal to 0
        DBNE X, waitO                   ; --- Decrement X and branch to waitO if not equal to 0
		    RTS
		    
		    