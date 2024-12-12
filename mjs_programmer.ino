// Atmega chip programmer
// Author: Nick Gammon
// Heavily modified by Matthijs Kooijman & Bas Peschier for the
// Meet-je-stad project.

static uint8_t AppEui[] = { 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x00, 0x03, 0xBA };

const int ENTER_PROGRAMMING_ATTEMPTS = 50;

/*

  Copyright 2012 Nick Gammon.
  Copyright 2016 Matthijs Kooijman
  Copyright 2016 Bas Peschier


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

const unsigned long BAUD_RATE = 9600;
const byte CLOCKOUT = 9;
const byte RESET = 10;  // --> goes to reset on the target board

#if ARDUINO < 100
const byte SCK = 13;    // SPI clock
#endif

#define EEPROM_LAYOUT_MAGIC_OLD 0x2a60af86 // Just a random number, stored little-endian
#define EEPROM_LAYOUT_MAGIC 0x2a60af87 // Just a random number, stored little-endian
#define EEPROM_LAYOUT_MAGIC_START 0x00 // 4 bytes
#define EEPROM_OSCCAL_START (EEPROM_LAYOUT_MAGIC_START + 4) // 1 byte
#define EEPROM_APP_EUI_START (EEPROM_OSCCAL_START + 1)
#define EEPROM_APP_EUI_LEN 8
#define EEPROM_DEV_EUI_START (EEPROM_APP_EUI_START + EEPROM_APP_EUI_LEN)
#define EEPROM_DEV_EUI_LEN 8
#define EEPROM_APP_KEY_START (EEPROM_DEV_EUI_START + EEPROM_DEV_EUI_LEN)
#define EEPROM_APP_KEY_LEN 16
#define EEPROM_LENGTH 1024

#include "Signatures.h"
#include "General_Stuff.h"

#include "bootloader_mjs.h"
#include "calibrator_atmega328p_8mhz.h"

// structure to hold signature and other relevant data about each bootloader
typedef struct {
  byte sig [3];
  unsigned long start;
  const byte * data;
  unsigned int length;
  unsigned long osccal_offset;
  byte lowFuse, highFuse, extFuse, lockByte;
} image_t;


const image_t bootloader = {
  { 0x1E, 0x95, 0x0F },
  0x7E00,               // start address
  optiboot_atmega328p_8Mhz_57600_bin,   // loader image
  sizeof optiboot_atmega328p_8Mhz_57600_bin,
  0x7ffd,
  0xE2,         // fuse low byte: RC oscillator 8Mhz, max start-up time (0xA2 with CKOUT)
  0xD6,         // fuse high byte: SPI enable, boot into bootloader, 512 byte bootloader, EESAVE
  0x05,         // fuse extended byte: brown-out detection at 2.7V
  0x2F
};

const image_t calibration = {
  { 0x1E, 0x95, 0x0F },
  0x0,               // start address
  calibrator_atmega328p_8mhz,   // image
  sizeof calibrator_atmega328p_8mhz,
  -1,
  0xE2,         // fuse low byte: RC oscillator 8Mhz, max start-up time (0xA2 with CKOUT)
  0xD7,         // fuse high byte: SPI enable, boot into main code, EESAVE
  0x05,         // fuse extended byte: brown-out detection at 2.7V
  0x2F
};

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
uint16_t idToWrite = 0;

bool writeImage(const image_t* image, uint8_t osccal_value = 0xff) {

  unsigned long i;

  byte lFuse = readFuse (lowFuse);

  byte newlFuse = image->lowFuse;
  byte newhFuse = image->highFuse;
  byte newextFuse = image->extFuse;
  byte newlockByte = image->lockByte;

  unsigned long addr = image->start;
  unsigned long len = image->length;
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
      return false;
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
    if (addr + i == image->osccal_offset)
      writeFlash (addr + i, osccal_value);
    else
      writeFlash (addr + i, pgm_read_byte(image->data + i));

    if (addr + i + 1== image->osccal_offset)
      writeFlash (addr + i + 1, osccal_value);
    else
      writeFlash (addr + i + 1, pgm_read_byte(image->data + i + 1));
  }  // end while doing each word

  // commit final page
  commitPage (oldPage, false);
  Serial.println ();
  Serial.println (F("Written."));

  Serial.println (F("Verifying ..."));

  // count errors
  unsigned int errors = 0;
  // check each byte
  for (i = 0; i < len; i++)
  {
    byte found = readFlash (addr + i);
    byte expected = pgm_read_byte(image->data + i);
    if (addr + i == image->osccal_offset)
      expected = osccal_value;
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
    return false;  // don't change fuses if errors
  }  // end if

  Serial.println (F("Writing fuses ..."));

  writeFuse (newlFuse, lowFuse);
  writeFuse (newhFuse, highFuse);
  writeFuse (newextFuse, extFuse);
  writeFuse (newlockByte, lockByte);

  Serial.println (F("Done."));
  return true;
} // end of writeImage

void dumpEEPROM() {
  // Dump EEPROM written
  Serial.println("Values written to EEPROM:");
  for (byte i = 0; i < EEPROM_APP_KEY_START + EEPROM_APP_KEY_LEN; i++) {
    printHexByte(readEEPROM(i));
    if (i % 16 < 15)
      Serial.write(' ');
    else
      Serial.println();
  }
  Serial.println();
}

void clearEEPROM() {
  for (uint16_t i = 0; i < EEPROM_LENGTH; ++i)
    writeEEPROM(i, 0xff);

  dumpEEPROM();
}

void writeIDsAndKey(uint32_t id) {
  uint8_t DevEui[EEPROM_DEV_EUI_LEN], AppKey[EEPROM_APP_KEY_LEN];

  DevEui[0] = 0x00;
  DevEui[1] = 0x00;
  DevEui[2] = 0x00;
  DevEui[3] = 0x00;
  DevEui[4] = 0x00;
  DevEui[5] = 0x00;
  DevEui[6] = id >> 8 & 0xFF;
  DevEui[7] = id & 0xFF;

  // Write layout dword hash (little endian)
  writeEEPROM(EEPROM_LAYOUT_MAGIC_START + 3, EEPROM_LAYOUT_MAGIC >> 24 & 0xFF);
  writeEEPROM(EEPROM_LAYOUT_MAGIC_START + 2, EEPROM_LAYOUT_MAGIC >> 16 & 0xFF);
  writeEEPROM(EEPROM_LAYOUT_MAGIC_START + 1, EEPROM_LAYOUT_MAGIC >> 8 & 0xFF);
  writeEEPROM(EEPROM_LAYOUT_MAGIC_START, EEPROM_LAYOUT_MAGIC & 0xFF);

  // Write Device EUI and App EUI
  for (byte i = 0; i < EEPROM_DEV_EUI_LEN; i++)
    writeEEPROM(EEPROM_DEV_EUI_START + i, DevEui[i]);

  for (byte i = 0; i < EEPROM_APP_EUI_LEN; i++)
    writeEEPROM(EEPROM_APP_EUI_START + i, AppEui[i]);

  // Generate and write App Key
  for (byte i = 0; i < EEPROM_APP_KEY_LEN; i++)
  {
    while (!Entropy.available()) /* wait */;
    AppKey[i] = Entropy.randomByte();
    writeEEPROM(EEPROM_APP_KEY_START + i, AppKey[i]);
  }

  dumpEEPROM();
  Serial.println();

  if (id != 0) {
    Serial.println(F("Now run:"));
    Serial.print(F("ttnctl devices register meetstation-"));
    Serial.print(id);
    Serial.print(F(" "));
    printHexBytes(DevEui, EEPROM_DEV_EUI_LEN);
    Serial.print(F(" "));
    printHexBytes(AppKey, EEPROM_APP_KEY_LEN);
    Serial.print(F(" --app-eui "));
    printHexBytes(AppEui, EEPROM_APP_EUI_LEN);
    Serial.println();
  }
}

void printHexBytes(const uint8_t *buf, uint8_t len)
{
  while (len--)
    printHexByte(*buf++);
}

void printHexByte(uint8_t b)
{
  if (b < 0x10)
    Serial.write('0');
  Serial.print(b, HEX);
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

  for (uint16_t j = 0; j < NUMITEMS (signatures); j++)
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
}  // end of setup


uint16_t readInt() {
  while (Serial.read() >= 0) /* flush */;

  uint16_t res = 0;
  bool valid = true;
  while (true) {
    int ch = Serial.read();
    if (ch < 0)
      continue;

    Serial.write(ch);

    if (ch == '\r' || ch == '\n')
    {
      if (!valid) {
        Serial.println(F("Invalid number"));
        res = 0;
        valid = true;
        continue;
      } else {
        return res;
      }
    }
    else if (ch >= '0' && ch <= '9')
    {
      res *= 10;
      res += (ch - '0');
    } else {
      valid = false;
    }
  }
}

void loop ()
{
  Serial.println ();
  if (idToWrite == 0) {
    Serial.println (F("Not writing id and keys, just calibration and bootloader."));
    Serial.println (F("Enter a number to write id and keys for the device id entered, or enter to continue without id and keys."));
  } else {
    Serial.print (F("Next device ID is: "));
    Serial.println (idToWrite);
    Serial.println (F("Enter a number to change it, or just press enter to use this one."));
  }

  uint16_t id = readInt();

  if (id != 0)
  {
    idToWrite = id;
    return;
  }

  if (startProgramming ())
  {
    getSignature ();
    getFuseBytes ();

    // if we found a signature try to write a bootloader
    if (foundSig != -1)
    {
      uint32_t magic = (uint32_t)readEEPROM(EEPROM_LAYOUT_MAGIC_START + 3) << 24
                     | (uint32_t)readEEPROM(EEPROM_LAYOUT_MAGIC_START + 2) << 16
                     | (uint32_t)readEEPROM(EEPROM_LAYOUT_MAGIC_START + 1) << 8
                     | (uint32_t)readEEPROM(EEPROM_LAYOUT_MAGIC_START + 0);

      if (magic == EEPROM_LAYOUT_MAGIC_OLD || magic == EEPROM_LAYOUT_MAGIC) {
        uint16_t currentId = (uint16_t)readEEPROM(EEPROM_DEV_EUI_START + EEPROM_DEV_EUI_LEN - 2) << 8
                        | (uint16_t)readEEPROM(EEPROM_DEV_EUI_START + EEPROM_DEV_EUI_LEN - 1);
        Serial.print("Current device id: ");
        Serial.println(currentId);
      }
      if (magic != EEPROM_LAYOUT_MAGIC_OLD) {
        Serial.print("Device id will be set to: ");
        Serial.println(idToWrite);
      }

      if (magic == EEPROM_LAYOUT_MAGIC_OLD) {
        if (idToWrite == 0)
          Serial.println("Detected older EEPROM content, press enter to replace bootloader and CLEAR id and keys.");
        else
          Serial.println("Detected older EEPROM content, press enter to replace bootloader, but not the id and keys.");
      } else if (magic == EEPROM_LAYOUT_MAGIC) {
        if (idToWrite == 0)
          Serial.println("Detected up-to-date EPROM content, press enter to replace bootloader and CLEAR id and keys.");
        else
          Serial.println("Detected up-to-date EEPROM content, press enter to replace everything (include the id and keys).");
      } else {
        if (idToWrite == 0)
          Serial.println("No EEPROM content found, press enter to write bootloader (but not id and keys).");
        else
          Serial.println("No EEPROM content found, press enter to write bootloader and EEPROM.");
      }

      Serial.println("Enter any non-zero number to abort.");
      if (readInt()) {
        stopProgramming();
        return;
      }

      if (!writeImage(&calibration))
        return;

      // Clear any previous calibration value, so we can be really sure
      // that a new one was succesfully written.
      writeEEPROM(EEPROM_OSCCAL_START, 0xff);
      stopProgramming();

      Serial.println(F("Waiting for calibration to complete...."));

      delay(6000); // Wait for calibration

      if (!startProgramming())
        return;

      uint8_t osccal = readEEPROM(EEPROM_OSCCAL_START);
      if (osccal == 0xff) {
        Serial.println(F("!!! Calibration failed !!!"));
        return;
      }

      Serial.print(F("Calibration complete, OSCCAL value: 0x"));
      Serial.println(osccal, HEX);
      Serial.print(F("Factory calibration was: 0x"));
      Serial.println(readFuse(calibrationByte), HEX);

      if (!writeImage(&bootloader, osccal))
        return;

      if (magic && idToWrite == 0) {
        clearEEPROM();
      } else {
        if (magic == EEPROM_LAYOUT_MAGIC_OLD) {
          // Only update the signature so the sketch knows OSCCAL does not
          // need to be loaded.
          writeEEPROM(EEPROM_LAYOUT_MAGIC_START + 3, EEPROM_LAYOUT_MAGIC >> 24 & 0xFF);
          writeEEPROM(EEPROM_LAYOUT_MAGIC_START + 2, EEPROM_LAYOUT_MAGIC >> 16 & 0xFF);
          writeEEPROM(EEPROM_LAYOUT_MAGIC_START + 1, EEPROM_LAYOUT_MAGIC >> 8 & 0xFF);
          writeEEPROM(EEPROM_LAYOUT_MAGIC_START, EEPROM_LAYOUT_MAGIC & 0xFF);
        } else {
          writeIDsAndKey(idToWrite);
        }
      }
    }
    stopProgramming ();

    if (idToWrite) {
      idToWrite += 1;
    }
  }   // end of if entered programming mode OK
}  // end of loop

