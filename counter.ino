/*

 Copyright (C) 2018 Vladislav Ross <vladislav dot ross at gmail dot com>
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the version 3 GNU General Public License as
 published by the Free Software Foundation.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//#define SELFTEST 1

#define WAIT_INTERVAL 60000

#define UPDATE_DISPLAY_INTERVAL 500

#include <TM1638.h> 
#include <TM1640.h>

#include <math.h>


TM1640 module(3, 2);

double dX, dT, Xcur = 0;
long Xrcv[2] = {
  0, 0}; 
long Trcv[2] = {
  0, 0}; 
long Tlast, Tnow, Tsleep, Xnew;
byte Nrcv = 0;
double V, a = 0;

int rank = 0;
boolean gotX = false;

#define BUFLEN 17

char sBuf[BUFLEN];
char text[BUFLEN];

void setup()
{
  Serial.begin(9600);
#ifdef SELFTEST
  module.setDisplayToString("POCC BC 2018", 1536); //Vanity
  delay(1000);
  module.setDisplayToString("test", 1536, 12);
  delay(1000);
  module.clearDisplay();
  unsigned long int dots = 1;
  for (int pos = 0; pos < 16; pos++)
  {
    dots = dots * 2;
    module.setDisplayToString("8", 0, pos);  
    delay(200);
  }
  delay(500);
#endif 

  module.clearDisplay();
  Tlast = millis();
  Xcur = 0;
}

void loop()
{

  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read(); 
    if (inChar == '\n') {      
      sBuf[rank++] = 0x00;
      Xnew = atol(sBuf);   

        Serial.print("Xnew="); 
        Serial.println(Xnew);
        Serial.println(sBuf);
        
      if (Xrcv[Nrcv] == Xnew) {
        Serial.println("E_DUPLICATE"); 
      } 
      else if (Xrcv[Nrcv] > Xnew) {
        Serial.println("E_LESS"); 
      } 
      else {
        if (Nrcv == 1) {
          V = ((double)(Xrcv[1] - Xcur))/((double)(Trcv[1] - Trcv[0])); 
          a = 0;
          Xrcv[0] = Xrcv[1];        
          Trcv[0] = Trcv[1];      
        } 
        else {          
          Nrcv = 1;
        }

        Xrcv[1] = Xnew;
        Trcv[1] = millis();   
        Serial.print(";OK"); 
        Serial.println(Nrcv);
      }

      resetBuf();

    } 
    else {
      sBuf[rank++] = inChar;
      if (rank > 10) {
        resetBuf();
        Serial.println("E_OVERFLOW"); 
      }
    }
  }

  Tnow = millis();
  dX = V * (Tnow - Tlast);
  Xcur = Xcur + dX;
  if (Xcur < 0) {
    //?!
    Xcur = 0;
  } 
  else if (Xcur > Xrcv[1]) {
    //cannot tick further
    Xcur = Xrcv[1]; 
    Serial.println("E_NODATA"); 
  } 
  else if (Xcur > Xrcv[0]) {
    //buy some time by descreasing speed
    Serial.println("W_DELAY"); 
    dT = (double)(Trcv[1] - Trcv[0]);
    if (dT < WAIT_INTERVAL) {
      dT = WAIT_INTERVAL;
    }

    Xrcv[0] += ((Xrcv[1] - Xrcv[0]) / 2 + 1);  
    V = ((double)(Xrcv[0] - Xcur))/dT; 
  }

  sprintf(text, "%16ld", (long)Xcur); //<-- "16" is for right space padding
  module.setDisplayToString(text);

#ifdef DEBUG
  Serial.print("X="); 
  Serial.println(Xcur, 3); 
  Serial.print("V="); 
  Serial.println(V * 1000, 3); 
#endif

  Tsleep = UPDATE_DISPLAY_INTERVAL - (Tnow - Tlast);
  Tlast = Tnow;

  if (Tsleep > 0) {
    delay(Tsleep);
  }

}

void resetBuf()
{
  for (rank = 0; rank < BUFLEN - 1; rank++)
  {
    sBuf[rank] = 0x00;
  } 

  rank = 0;
}


