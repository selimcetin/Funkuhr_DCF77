#include <mc9s12dp256.h>
#include "buttons.h"


char checkButtons()
{    
    #ifdef SIMULATION
    
    if ( PTH & SW1 )
    {
      return SW1;
    }
    else if ( PTH & SW2)
    {
      return SW2;
    }
    else if ( PTH & SW3 )
    {
      return SW3;
    }
    else if ( PTH & SW4 )
    {
      return SW4;
    }
    
    #else
    
    if ( (PTH & SW1) == 0 )
    {
      return SW1;
    }
    else if ( (PTH & SW2) == 0 )
    {
      return SW2;
    }
    else if ( (PTH & SW3) == 0 )
    {
      return SW3;
    }
    else if ( (PTH & SW4) == 0 )
    {
      return SW4;
    }
    #endif
}