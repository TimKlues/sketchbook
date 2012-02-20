#include "Plasma.h"
#include <SFRGBLEDMatrix.h>
#include "Button.h"
#include <EEPROM.h>
#include "EEPROM.h"

extern SFRGBLEDMatrix *display;
extern Button *button;

#define SPECTRUM_LEN 90

Color spectrum(byte p){
  // RED
  if(p<15){
    return RGB(15, p+1, 0);
  }
  // YELLOW
  if(p<30){
    p-=15;
    return RGB(14-p, 15, 0);
  }
  // GREEN
  if(p<45){
    p-=30;
    return RGB(0, 15, p+1);
  }
  // CYAN
  if(p<60){
    p-=45;
    return RGB(0, 14-p, 15);
  }
  // BLUE
  if(p<75){
    p-=60;
    return RGB(p+1, 0, 15);
  }
  // MAGENTA
  if(p<90){
    p-=75;
    return RGB(15, 0, 14-p);
  }
  return 0;
}

// Bicubic interpolation code based on code found at:
// http://www.paulinternet.nl/?page=bicubic
double cubicInterpolate (double *p, double x) {
  return *(p+1) + 0.5 * x*(*(p+2) - *p + x*(2.0**(p+0) - 5.0**(p+1) + 4.0**(p+2) - *(p+3) + x*(3.0*(*(p+1) - *(p+2)) + *(p+3) - *p)));
}
double bicubicInterpolate (
double *pixmap,
uint8_t pixmapWidth, 
uint8_t pixmapHeight, 
uint8_t pointX, 
uint8_t pointY, 
double x, 
double y) {
  double arr[4];
  arr[0] = cubicInterpolate(pixmap+pixmapWidth*(pointY-1)+pointX-1, x);
  arr[1] = cubicInterpolate(pixmap+pixmapWidth*(pointY+0)+pointX-1, x);
  arr[2] = cubicInterpolate(pixmap+pixmapWidth*(pointY+1)+pointX-1, x);
  arr[3] = cubicInterpolate(pixmap+pixmapWidth*(pointY+2)+pointX-1, x);
  return cubicInterpolate(arr, y);
}

#define PIXMAP_WIDTH ((display->width>>3)+3)
#define PIXMAP_HEIGHT ((display->height>>3)+3)
#define PIXMAP_MAX 48.0

void Plasma::fillPlasma() {
  double *pixmap;

  pixmap=(double *)malloc(sizeof(double)*(PIXMAP_WIDTH*PIXMAP_HEIGHT));

  randomSeed(millis());
  for(byte x=0;x<PIXMAP_WIDTH;x++)
    for(byte y=0;y<PIXMAP_HEIGHT;y++)
      *(pixmap+PIXMAP_WIDTH*y+x)=random(PIXMAP_MAX);

  for(word x=0;x<display->width;x++)
    for(word y=0;y<display->height;y++)
      *(plasma+y*display->width+x)=bicubicInterpolate(
      pixmap,
      PIXMAP_WIDTH, 
      PIXMAP_HEIGHT,
      (x/8)+1, 
      (y/8)+1, 
      double(x%8)/8.0,
      double(y%8)/8.0
        );

  free(pixmap);
}


void Plasma::pSpeedValidate(){
  if(pSpeed==0)
    pSpeed++;
  if(pSpeed>6)
    pSpeed=-6;
  if(pSpeed<-6)
    pSpeed=-6;
}

void Plasma::enter() {
  display->gamma(true); 
  display->print(WHITE, 0, 6, 4, "BUSY");
  display->show();
  plasma=(byte *)malloc((size_t)(display->pixels)*sizeof(byte));
  fillPlasma();
}

void Plasma::loop() {
  // Update pimap
  if(button->pressed(A)){
    display->clear();
    display->print(WHITE, 0, 6, 4, "BUSY");
    display->show();
    fillPlasma();
  }

  // Change speed
  if(button->pressed(B)) {
   pSpeed++;
   pSpeedValidate();
   // FIXME adjust text placement relative to screen size
   if(pSpeed>=0)
   display->print(BLACK, 7, 6, 4, 48+pSpeed);
   else{
   display->print(BLACK, 5, 6, 4, '-');
   display->print(BLACK, 8, 6, 4, 48+pSpeed*-1);
   }
   display->show();
   delay(200);
   EEPROM.write(EEPROM_PLASMA_SPEED, pSpeed);
   }

  // Update pixmap
  for(byte x=0;x<display->width;x++)
    for(byte y=0;y<display->height;y++)
      *(plasma+y*display->width+x)+=pSpeed;

  // Paint display
  for(word x=0;x<display->width;x++)
    for(word y=0;y<display->height;y++)
      display->paintPixel(spectrum(
      byte(
      double(
      *(plasma+y*word(display->width)+x)
        ) / 255.0 * double(SPECTRUM_LEN-1)
        ))
        , x, y);

  // Show
  display->show();
}

void Plasma::exit() {
  display->fill(BLACK);
  display->show();
  free(plasma);
}







































