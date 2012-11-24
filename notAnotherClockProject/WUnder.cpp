#include "WUnder.h"

#include <Arduino.h>
#include "Net.h"
#include "Light.h"
#include "pins.h"
#include <DS1307.h>
#include <HIH4030.h>
#include <DS18S20.h>
#include <BMP085.h>
#include <Temperature.h>
#include <math.h>
#include "WUnderPass.h"

#define UPDATE_MS (300L*1000L)

#define HOST_PREFIX "Host: "
#define WUNDER_HOST "weatherstation.wunderground.com"
#define WUNDER_PORT 80
#define GET_PREFIX "GET /weatherstation/updateweatherstation.php?ID=ISPSAOPA12&PASSWORD="
#define GET_SUFFIX " HTTP/1.1"
#define SEND_BUFF_LEN 218
#define SEND_BUFF_END &sendBuff[strlen(sendBuff)]
#define APPEND_INT(i) itoa(i, SEND_BUFF_END, 10);
#define APPEND_STR(s) strcpy_P(SEND_BUFF_END, PSTR(s));
#define APPEND_DOUBLE(d) dtostrf(d, -2, 3, SEND_BUFF_END);

extern Net *net;
extern Light *light;
extern DS18S20 *temperatureOutside;
extern BMP085 *pressure;

uint8_t WUnder::inProgress;
uint8_t WUnder::skipHeaders;
char WUnder::lastChar;
char WUnder::recvBuff[SUCCESS_LENGTH];
EthernetClient WUnder::client;

WUnder::WUnder(){
  lastUpdate=millis();
  inProgress=0;
  net->addProcessor(loop);
  
}

void WUnder::update(){
  if(!inProgress){
    if(millis()-lastUpdate>UPDATE_MS){
      char sendBuff[SEND_BUFF_LEN];
Serial.println("New connection.");
      lastUpdate=millis();
      // new connection
      digitalWrite(PIN_G, LOW);
      analogWrite(PIN_B, light->read(255-20)+20);
      if(client.connect(strcpy_P(sendBuff, PSTR(WUNDER_HOST)), WUNDER_PORT)){
        DS1307 time(UTC);
        btime_t bt;
        bt=time.getBTime();
        inProgress=1;
        sendBuff[0]='\0';
        double tInC;
        double h;
        double tOutC;
        double dpb;
        // GET
        APPEND_STR(GET_PREFIX);
        // Password
        APPEND_STR(WUNDER_PASS);
        // Hour
        APPEND_STR("&dateutc=");
        APPEND_INT(bt.year);
        APPEND_STR("-");
        APPEND_INT(bt.mon);
        APPEND_STR("-");
        APPEND_INT(bt.mday);
        APPEND_STR(" ");
        APPEND_INT(bt.hour);
        APPEND_STR(":");
        APPEND_INT(bt.min);
        APPEND_STR(":");
        APPEND_INT(bt.sec);
        APPEND_STR("&indoortempf=");
        APPEND_DOUBLE(Temperature::convC2F(tInC=pressure->readC()));
        APPEND_STR("&indoorhumidity=");
        APPEND_DOUBLE(h=HIH4030::read(PIN_HUMIDITY, tInC));
        APPEND_STR("&tempf=");
        APPEND_DOUBLE(Temperature::convC2F(tOutC=temperatureOutside->readC()));
        APPEND_STR("&humidity=");
        APPEND_DOUBLE(h);
        // http://www.srh.noaa.gov/images/epz/wxcalc/rhTdFromWetBulb.pdf
        APPEND_STR("&dewptf=");
        double E=rh*(6.112*exp(17.67*temp/(temp+243.5)))/100;
        dpb=(243.5 * log(E/6.112))/(17.67-log(E/6.112));
        APPEND_DOUBLE(Temperature::convC2F((237.3 * dpb) / (1 - dpb)));
        APPEND_STR("&baromin=");
        APPEND_DOUBLE(pressure->readInHg());
        APPEND_STR(GET_SUFFIX);
Serial.println(sendBuff);
        client.println(sendBuff);
        // Host
        strcpy_P(sendBuff, PSTR(HOST_PREFIX));
        strcpy_P(&sendBuff[strlen(sendBuff)], PSTR(WUNDER_HOST));
//Serial.println(sendBuff);
        client.println(sendBuff);
        // Headers ending
        client.println();
        skipHeaders=1;
        lastChar=0;
        recvBuff[0]='\0';
      }else{
        // FIXME better error report
Serial.println("Failed to open new connection.");
        lastUpdate=millis();
      }
      analogWrite(PIN_G, light->read(255-20)+20);
      digitalWrite(PIN_B, LOW);
    }
  }else{
    loop();
  }
}

uint8_t WUnder::loop(){
  if(inProgress){
//Serial.println("inProgress");
    if(client.connected()){
//Serial.println("connected");
      digitalWrite(PIN_G, LOW);
      analogWrite(PIN_B, light->read(255-20)+20);
      // Headers
      if(skipHeaders){
//Serial.println("skipHeaders");
        while(client.available()){
          char c;
          c=client.read();
          //Serial.print("New byte: '");
          //Serial.print(c);
          //Serial.println("'.");
          // eat all CR
          if('\r'==c)
            continue;
          if('\n'==c&&'\n'==lastChar){
//Serial.println("End of headers.");
            skipHeaders=0;
            break;
          }
          lastChar=c;
        }
      }
      // Body
      if(!skipHeaders){
//Serial.println("Body");
        while(client.available()){
          char c;
          c=client.read();
          //Serial.print("New byte: '");
          //Serial.print(c);
          //Serial.println("'.");
          // fill buffer
          uint8_t l;
          l=strlen(recvBuff);
          if(l<SUCCESS_LENGTH){
            recvBuff[l]=c;
            recvBuff[l+1]='\0';
            //Serial.print("Buffer: '");
            //Serial.print(recvBuff);
            //Serial.println("'");
            if(l+2==SUCCESS_LENGTH) {
              // buffer full
//Serial.print("Buffer full: ");
//Serial.println(recvBuff);
              char success[SUCCESS_LENGTH];
              strcpy_P(success, PSTR("success"));
              // processing OK
              if(!strcmp(recvBuff, success)){
Serial.println("Processing OK.");
              // Error response
              }else{
                // FIXME Better error reporting
Serial.println("Unexpected body:");
Serial.println(recvBuff);
              }
              inProgress=0;
              client.stop();
            }
          }
        }
      }
      analogWrite(PIN_G, light->read(255-20)+20);
      digitalWrite(PIN_B, LOW);
    }else{
      //Serial.println("Connection lost before end of transaciton.");
      // connection lost before end of transaciton.
      // FIXME better error reporting
      inProgress=0;
      client.stop();
    }
  }
}
