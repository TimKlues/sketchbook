#ifndef Lamp_h
#define Lamp_h

#include "Mode.h"

class Lamp: 
public Mode {
public:
  Lamp();
  inline void loop();
};

#endif
