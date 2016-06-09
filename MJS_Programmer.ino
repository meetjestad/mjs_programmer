// Atmega chip programmer
// Author: Nick Gammon
// Date: 22nd May 2012
// Version: 1.36

// IMPORTANT: If you get a compile or verification error, due to the sketch size,
// make some of these false to reduce compile size (the ones you don't want).
// The Atmega328 is always included (Both Uno and Lilypad versions).

#define APPLICATION_ID 1234567

const int ENTER_PROGRAMMING_ATTEMPTS = 50;

/*

  Copyright 2012 Nick Gammon.


  PERMISSION TO DISTRIBUTE

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software
  and associated documentation files (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.


  LIMITATION OF LIABILITY

  The software is provided "as is", without warranty of any kind, express or implied,
  including but not limited to the warranties of merchantability, fitness for a particular
  purpose and noninfringement. In no event shall the authors or copyright holders be liable
  for any claim, damages or other liability, whether in an action of contract,
  tort or otherwise, arising from, out of or in connection with the software
  or the use or other dealings in the software.

*/

#include <SPI.h>
#include <avr/pgmspace.h>
#include <Entropy.h>


const unsigned long BAUD_RATE = 115200;

const byte CLOCKOUT = 9;


const byte RESET = 10;  // --> goes to reset on the target board

#if ARDUINO < 100
const byte SCK = 13;    // SPI clock
#endif



#include "Signatures.h"
#include "General_Stuff.h"


// structure to hold signature and other relevant data about each bootloader
typedef struct {
  byte sig [3];                // chip signature
  unsigned long loaderStart;   // start address of bootloader (bytes)
  const byte * bootloader;     // address of bootloader hex data
  unsigned int loaderLength;   // length of bootloader hex data (bytes)
  const byte * calibration;     // address of calibration hex data
  unsigned int calibrationLength;   // length of calibration hex data (bytes)
  byte lowFuse, highFuse, extFuse, lockByte;  // what to set the fuses, lock bits to.
} image_t;


// hex bootloader data

// For simplicity later one, we always include these two
#include "bootloader_lilypad328.h"

// see Atmega328 datasheet page 298
const image_t bootloaders [] PROGMEM =
{

  { { 0x1E, 0x95, 0x0F },
    0x7E00,               // start address
    ATmegaBOOT_168_atmega328_pro_8MHz_hex,   // loader image
    sizeof ATmegaBOOT_168_atmega328_pro_8MHz_hex,
    ATmegaBOOT_168_atmega328_pro_8MHz_hex, sizeof ATmegaBOOT_168_atmega328_pro_8MHz_hex,         // calibration
    0xE2,         // fuse low byte: RC oscillator 8Mhz, max start-up time (0xA2 with CKOUT)
    0xD6,         // fuse high byte: SPI enable, boot into bootloader, 512 byte bootloader, EESAVE (0xDE without EESAVE)
    0x05,         // fuse extended byte: brown-out detection at 2.7V
    0x2F
  },       // lock bits: SPM is not allowed to write to the Boot Loader section.

 
};  // end of bootloaders



void getFuseBytes ()
{
  Serial.print (F("LFuse = "));
  showHex (readFuse (lowFuse), true);
  Serial.print (F("HFuse = "));
  showHex (readFuse (highFuse), true);
  Serial.print (F("EFuse = "));
  showHex (readFuse (extFuse), true);
  Serial.print (F("Lock byte = "));
  showHex (readFuse (lockByte), true);
  Serial.print (F("Clock calibration = "));
  showHex (readFuse (calibrationByte), true);
}  // end of getFuseBytes

image_t currentImage;
unsigned int currentId = 0;



void writeImage(const byte* data, unsigned int len) {

  int i;

  byte lFuse = readFuse (lowFuse);

  byte newlFuse = currentImage.lowFuse;
  byte newhFuse = currentImage.highFuse;
  byte newextFuse = currentImage.extFuse;
  byte newlockByte = currentImage.lockByte;

  unsigned long addr = currentImage.loaderStart;
  unsigned long pagesize = currentSignature.pageSize;
  unsigned long pagemask = ~(pagesize - 1);

  Serial.print (F("Image address = 0x"));
  Serial.println (addr, HEX);
  Serial.print (F("Image length = "));
  Serial.print (len);
  Serial.println (F(" bytes."));

  unsigned long oldPage = addr & pagemask;

  // Automatically fix up fuse to run faster, then write to device
  if (lFuse != newlFuse)
  {
    if ((lFuse & 0x80) == 0)
      Serial.println (F("Clearing 'Divide clock by 8' fuse bit."));

    Serial.println (F("Fixing low fuse setting ..."));
    writeFuse (newlFuse, lowFuse);
    delay (1000);
    stopProgramming ();  // latch fuse
    if (!startProgramming ())
      return;
    delay (1000);
  }

  Serial.println (F("Erasing chip ..."));
  eraseMemory ();
  Serial.println (F("Writing data ..."));
  for (i = 0; i < len; i += 2)
  {
    unsigned long thisPage = (addr + i) & pagemask;
    // page changed? commit old one
    if (thisPage != oldPage)
    {
      commitPage (oldPage, false);
      oldPage = thisPage;
    }
    writeFlash (addr + i, pgm_read_byte(data + i));
    writeFlash (addr + i + 1, pgm_read_byte(data + i + 1));
  }  // end while doing each word

  // commit final page
  commitPage (oldPage, false);
  Serial.println (F("Written."));

  Serial.println (F("Verifying ..."));

  // count errors
  unsigned int errors = 0;
  // check each byte
  for (i = 0; i < len; i++)
  {
    byte found = readFlash (addr + i);
    byte expected = pgm_read_byte(data + i);
    if (found != expected)
    {
      if (errors <= 100)
      {
        Serial.print (F("Verification error at address "));
        Serial.print (addr + i, HEX);
        Serial.print (F(". Got: "));
        showHex (found);
        Serial.print (F(" Expected: "));
        showHex (expected, true);
      }  // end of haven't shown 100 errors yet
      errors++;
    }  // end if error
  }  // end of for

  if (errors == 0)
    Serial.println (F("No errors found."));
  else
  {
    Serial.print (errors, DEC);
    Serial.println (F(" verification error(s)."));
    if (errors > 100)
      Serial.println (F("First 100 shown."));
    return;  // don't change fuses if errors
  }  // end if

  Serial.println (F("Writing fuses ..."));

  writeFuse (newlFuse, lowFuse);
  writeFuse (newhFuse, highFuse);
  writeFuse (newextFuse, extFuse);
  writeFuse (newlockByte, lockByte);

  Serial.println (F("Done."));

} // end of writeImage


// burn the bootloader to the target device
void program ()
{
  bool foundImage = false;

  for (int j = 0; j < NUMITEMS (bootloaders); j++)
  {

    memcpy_P (&currentImage, &bootloaders [j], sizeof currentImage);

    if (memcmp (currentSignature.sig, currentImage.sig, sizeof currentSignature.sig) == 0)
    {
      foundImage = true;
      break;
    }  // end of signature found
  }  // end of for each signature

  if (!foundImage)
  {
    Serial.println (F("No bootloader support for this device."));
    return;
  }

  // if in the table, but with zero length, we need to enable a #define to use it.
  if (currentImage.loaderLength == 0)
  {
    Serial.println (F("Image for this device is disabled, edit " __FILE__ " to enable it."));
    return;
  }

  if (currentImage.calibration != NULL) {
    writeImage(currentImage.calibration, currentImage.calibrationLength);
    delay(1000); // XXX
  } else {
    Serial.println (F("No calibration set for image, skipping."));
  }

  writeImage(currentImage.bootloader, currentImage.loaderLength);

}

void writeIDsAndKey(unsigned int id, unsigned long applicationId) {


  String key = "";
  for (int i=0; i< 32; i++)
  {
    key += String(random(16), HEX);
  }

  Serial.println ();
  Serial.print (F("Now run: ttnctl devices register "));
  Serial.print (id);
  Serial.print (F(" "));
  Serial.print (key);
  Serial.print (F(" --app-eui "));
  Serial.println (APPLICATION_ID);

}

void getSignature ()
{
  foundSig = -1;

  byte sig [3];
  Serial.print (F("Signature = "));
  readSignature (sig);
  for (byte i = 0; i < 3; i++)
    showHex (sig [i]);

  Serial.println ();

  for (int j = 0; j < NUMITEMS (signatures); j++)
  {

    memcpy_P (&currentSignature, &signatures [j], sizeof currentSignature);

    if (memcmp (sig, currentSignature.sig, sizeof sig) == 0)
    {
      foundSig = j;
      Serial.print (F("Processor = "));
      Serial.println (currentSignature.desc);
      Serial.print (F("Flash memory size = "));
      Serial.print (currentSignature.flashSize, DEC);
      Serial.println (F(" bytes."));
      if (currentSignature.timedWrites)
        Serial.println (F("Writes are timed, not polled."));
      return;
    }  // end of signature found
  }  // end of for each signature

  Serial.println (F("Unrecogized signature."));
}  // end of getSignature

void setup ()
{
  Serial.begin (BAUD_RATE);
  while (!Serial) ;  // for Leonardo, Micro etc.
  Serial.println ();
  Serial.println (F("MJS chip programmer."));
  Serial.println (F("Written Bas Peschier & Matthijs Kooijman"));
  Serial.println (F("Based on Atmega_Board_Programmer by Nick Gammon"));
  Serial.println (F("Compiled on " __DATE__ " at " __TIME__ " with Arduino IDE " xstr(ARDUINO) "."));

  initPins ();

  // Get a random seed
  Entropy.initialize();
  randomSeed(Entropy.random());

}  // end of setup

void loop ()
{
  currentId += 1;
  Serial.println ();
  Serial.print (F("Current device ID is: "));
  Serial.println (currentId);
  Serial.println (F("Enter device ID and end with # or just # to use suggested ID"));

  char intBuffer[12];
  String intData = "";
  while (true) {
    int ch = Serial.read ();
    if (ch == -1) 
    {
      // Skip
    }
    else if (ch == (int)'#')
    {
      break;
    }
    else
    {
      intData += (char) ch;
    }
  }
  int intLength = intData.length () + 1;
  intData.toCharArray (intBuffer, intLength);
  int id = atoi (intBuffer);
  
  if (id != 0)
  {
    currentId = id;
    Serial.print (F("Current device ID is now: "));
    Serial.println (currentId);
  }

  if (startProgramming ())
  {
    getSignature ();
    getFuseBytes ();

    // if we found a signature try to write a bootloader
    if (foundSig != -1)
    {
      program ();
      writeIDsAndKey(currentId, APPLICATION_ID);
    }
    stopProgramming ();
  }   // end of if entered programming mode OK

}  // end of loop

