EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Programmer-rescue:Arduino_Nano_v3.x-MCU_Module A?
U 1 1 5B2A048C
P 4800 2300
F 0 "A?" H 4450 3250 50  0000 C CNN
F 1 "Arduino_Nano_v3.x" V 4800 2300 50  0000 C CNN
F 2 "Module:Arduino_Nano" H 4950 1250 50  0001 L CNN
F 3 "https://www.arduino.cc/en/Main/arduinoBoardUno" H 4600 3350 50  0001 C CNN
	1    4800 2300
	1    0    0    -1  
$EndComp
$Comp
L Programmer-rescue:ATmega328P-PU-MCU_Microchip_ATmega U?
U 1 1 5B2A08ED
P 1500 2700
F 0 "U?" H 1050 4150 50  0000 C CNN
F 1 "ATMEGA328P-PU" V 1100 2650 50  0000 C CNN
F 2 "Package_DIP:DIP-28_W7.62mm" H 1500 2700 50  0001 C CIN
F 3 "http://ww1.microchip.com/downloads/en/DeviceDoc/atmel-8271-8-bit-avr-microcontroller-atmega48a-48pa-88a-88pa-168a-168pa-328-328p_datasheet.pdf" H 1500 2700 50  0001 C CNN
	1    1500 2700
	1    0    0    -1  
$EndComp
Text GLabel 4300 2700 0    50   Input ~ 0
SS
Text GLabel 4300 2800 0    50   Input ~ 0
MOSI
Text GLabel 4300 2900 0    50   Input ~ 0
MISO
Text GLabel 4300 3000 0    50   Input ~ 0
SCK
Text GLabel 2100 2000 2    50   Input ~ 0
SCK
Text GLabel 2100 1900 2    50   Input ~ 0
MISO
Text GLabel 2100 1800 2    50   Input ~ 0
MOSI
Text GLabel 2100 3000 2    50   Input ~ 0
SS
$Comp
L Device:Crystal Y?
U 1 1 5B2A19A8
P 2800 2200
F 0 "Y?" H 2800 1800 50  0000 C CNN
F 1 "32,768Hz" H 2800 1900 50  0000 C CNN
F 2 "" H 2800 2200 50  0001 C CNN
F 3 "~" H 2800 2200 50  0001 C CNN
	1    2800 2200
	-1   0    0    1   
$EndComp
Text Notes 2550 650  2    50   ~ 0
Chip to be programmed, in a ZIF socket.
Text Notes 4250 1000 0    50   ~ 0
Programmer Arduino. Can be any Arduino, but its voltage\nmust match the voltage the chip will be used with, so the\noscillator is calibrated to that voltage.\n\nNote that the +5V pin is intended as a VCC pin, which should be 3.3V.
$Comp
L power:GND #PWR?
U 1 1 5B2A29F8
P 4850 3450
F 0 "#PWR?" H 4850 3200 50  0001 C CNN
F 1 "GND" H 4855 3277 50  0000 C CNN
F 2 "" H 4850 3450 50  0001 C CNN
F 3 "" H 4850 3450 50  0001 C CNN
	1    4850 3450
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR?
U 1 1 5B2A2A98
P 1500 4300
F 0 "#PWR?" H 1500 4050 50  0001 C CNN
F 1 "GND" H 1505 4127 50  0000 C CNN
F 2 "" H 1500 4300 50  0001 C CNN
F 3 "" H 1500 4300 50  0001 C CNN
	1    1500 4300
	1    0    0    -1  
$EndComp
Wire Wire Line
	1500 4300 1500 4200
$Comp
L power:VCC #PWR?
U 1 1 5B2A2B51
P 1550 1050
F 0 "#PWR?" H 1550 900 50  0001 C CNN
F 1 "VCC" H 1567 1223 50  0000 C CNN
F 2 "" H 1550 1050 50  0001 C CNN
F 3 "" H 1550 1050 50  0001 C CNN
	1    1550 1050
	1    0    0    -1  
$EndComp
$Comp
L power:VCC #PWR?
U 1 1 5B2A2BDD
P 5000 1300
F 0 "#PWR?" H 5000 1150 50  0001 C CNN
F 1 "VCC" H 5017 1473 50  0000 C CNN
F 2 "" H 5000 1300 50  0001 C CNN
F 3 "" H 5000 1300 50  0001 C CNN
	1    5000 1300
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR?
U 1 1 5B2A2CE9
P 2800 2750
F 0 "#PWR?" H 2800 2500 50  0001 C CNN
F 1 "GND" H 2805 2577 50  0000 C CNN
F 2 "" H 2800 2750 50  0001 C CNN
F 3 "" H 2800 2750 50  0001 C CNN
	1    2800 2750
	1    0    0    -1  
$EndComp
Wire Wire Line
	2650 2200 2100 2200
Wire Wire Line
	2100 2100 2650 2100
Wire Wire Line
	2650 2100 2650 2000
Wire Wire Line
	2650 2000 2950 2000
Wire Wire Line
	2950 2000 2950 2200
Wire Wire Line
	2950 2200 2950 2350
Connection ~ 2950 2200
Wire Wire Line
	2650 2200 2650 2350
Connection ~ 2650 2200
$Comp
L Device:C C?
U 1 1 5B2A2F69
P 2950 2500
F 0 "C?" H 3065 2546 50  0000 L CNN
F 1 "C" H 3065 2455 50  0000 L CNN
F 2 "" H 2988 2350 50  0001 C CNN
F 3 "~" H 2950 2500 50  0001 C CNN
	1    2950 2500
	1    0    0    -1  
$EndComp
$Comp
L Device:C C?
U 1 1 5B2A2FD5
P 2650 2500
F 0 "C?" H 2765 2546 50  0000 L CNN
F 1 "C" H 2765 2455 50  0000 L CNN
F 2 "" H 2688 2350 50  0001 C CNN
F 3 "~" H 2650 2500 50  0001 C CNN
	1    2650 2500
	1    0    0    -1  
$EndComp
Wire Wire Line
	2650 2650 2800 2650
Wire Wire Line
	2800 2650 2800 2750
Wire Wire Line
	2800 2650 2950 2650
Connection ~ 2800 2650
NoConn ~ 2100 2400
NoConn ~ 2100 2500
NoConn ~ 2100 2600
NoConn ~ 2100 2700
NoConn ~ 2100 2800
NoConn ~ 2100 2900
NoConn ~ 2100 3200
NoConn ~ 2100 3300
NoConn ~ 2100 3400
NoConn ~ 2100 3500
NoConn ~ 2100 3600
NoConn ~ 2100 3700
NoConn ~ 2100 3800
NoConn ~ 2100 3900
NoConn ~ 4300 1700
NoConn ~ 4300 1800
NoConn ~ 4300 1900
NoConn ~ 4300 2000
NoConn ~ 4300 2100
NoConn ~ 4300 2200
NoConn ~ 4300 2300
NoConn ~ 4300 2400
NoConn ~ 4300 2500
NoConn ~ 4300 2600
NoConn ~ 5300 1700
NoConn ~ 5300 2100
NoConn ~ 5300 2300
NoConn ~ 5300 2400
NoConn ~ 5300 2500
NoConn ~ 5300 2600
NoConn ~ 5300 2700
NoConn ~ 5300 2800
NoConn ~ 5300 3000
NoConn ~ 4900 1300
NoConn ~ 4700 1300
NoConn ~ 5300 1800
NoConn ~ 2100 1700
NoConn ~ 2100 1600
NoConn ~ 2100 1500
Wire Wire Line
	4850 3450 4850 3350
Wire Wire Line
	4850 3350 4900 3350
Wire Wire Line
	4900 3350 4900 3300
Wire Wire Line
	4850 3350 4800 3350
Wire Wire Line
	4800 3350 4800 3300
Connection ~ 4850 3350
Wire Wire Line
	1550 1050 1550 1150
Wire Wire Line
	1550 1150 1600 1150
Wire Wire Line
	1600 1150 1600 1200
Wire Wire Line
	1550 1150 1500 1150
Wire Wire Line
	1500 1150 1500 1200
Connection ~ 1550 1150
NoConn ~ 900  1500
Text Notes 3450 1700 2    50   ~ 0
Crystal, for calibration.
Text Notes 5900 1550 0    50   ~ 0
TODO: Seems the Pro Mini is easier to get in 3.3V edition, perhaps use that? https://github.com/mysensors-kicad has footprints.\n\nOr perhaps use some third-party module, such as the Teensy, or another small M0 ARM-based board? ARM has the advantage of more flash, and being able to contain bigger data objects (e.g. a full 328p flash).
$EndSCHEMATC
