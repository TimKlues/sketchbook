#include "Net.h"
#include "pins.h"
#include "Light.h"

#include <stdlib.h>

#include <Ethernet.h>
#include <EthernetInterrupt.h>

extern Light *light;

Net::Net(){
  // Initialize Ethernet
  byte mac[]={
    0x90, 0xA2, 0xDA, 0x0D, 0x02, 0xD0    };
  IPAddress ip(192,168,1, 77);
  IPAddress dns(192,168,1, 1);
  IPAddress gateway(192,168,1, 1);
  IPAddress subnet(255,255,255,0);
  Ethernet.begin(mac, ip, dns, gateway, subnet);
  // Processors
  processor=NULL;
  processorCount=0;
  // Enable interrupts
  EthernetInterrupt::begin(0);
}

void Net::processAll(){  
  if(EthernetInterrupt::available()){
    analogWrite(PIN_G, light->read(255-20)+20);
    for(int8_t c=0;c<processorCount;c++){
      if(processor[c]()){
        removeProcessor(processor[c]);
        c--;
      }
    }
    EthernetInterrupt::next();
    digitalWrite(PIN_G, LOW);
  }
}

void Net::addProcessor(uint8_t (*l)()){
  processor=(uint8_t (**)())realloc(
    processor,
    sizeof(
        uint8_t (*)
    ) 
    * ++processorCount
  ); // FIXME Error reporting
  processor[processorCount-1]=l;
}

void Net::removeProcessor(uint8_t (*l)()){
  for(uint8_t c=0;c<processorCount;c++){
    if(processor[c]==l){
      for(uint8_t d=c;d<processorCount-1;c++){
        processor[d]=processor[d+1];
      }
      processorCount--;
      processor=(uint8_t (**)())realloc(
        processor,
        sizeof(
            uint8_t (*)
        )
        * processorCount
      ); // FIXME Error reporting
      break;
    }
  }
}

Net::~Net(){
  free(processor);
}
