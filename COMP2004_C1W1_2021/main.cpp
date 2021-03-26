#include "mbed.h"
#include "uop_msb_2_0_0.h"
#include <iostream>
#include "BMP280_SPI.h"

using namespace uop_msb_200;
using namespace std;

// C Function Prototypes
extern int write_sdcard();
extern int read_sdcard(); 
extern void matrix_init(void);
extern void matrix_scan(void);
extern void clearMatrix(void);

//Digital Inputs (you could use DigitalIn or InterruptIn instead)
Buttons btns;

//Analogue Inputs
AnalogIn ldr(AN_LDR_PIN);

//LED Outputs
LatchedLED ledStrip(LatchedLED::STRIP);
LatchedLED ledDigit(LatchedLED::SEVEN_SEG);

//Buzzer
Buzzer buzz;

//LCD Display
LCD_16X2_DISPLAY lcd_disp;

// Remember that main() runs in its own thread in the OS
int main()
{
    //Play sound
    buzz.playTone("C");
    ThisThread::sleep_for(500ms);
    buzz.rest();

    //Spin on a switch
    cout << "Press the blue button" << endl;
    while (btns.BlueButton == 0);

    //Led Strip Display
    ledStrip.setGroup(LatchedLED::LEDGROUP::BLUE);  //Set mode to BLUE led bar
    ledStrip = 0b10101010;                          //Set data pattern

    ledStrip.setGroup(LatchedLED::LEDGROUP::GREEN); //Set mode to GREEN led bar
    ledStrip = 0b11001100;                          //Set data pattern

    ledStrip.setGroup(LatchedLED::LEDGROUP::RED);  //Set mode to RED led bar
    ledStrip = 0b11100111;                         //Set data pattern

    ledStrip.enable(true);                         //Enable outputs (turn them all on!)

    // LED Digit
    ledDigit.setGroup(LatchedLED::LEDGROUP::TENS);
    ledDigit = 4;
    ledDigit.setGroup(LatchedLED::LEDGROUP::UNITS);
    ledDigit = 2;
    ledDigit.enable(true);                         //Enable outputs

    // LCD Display
    lcd_disp.cls();                                 //Clear screen
    lcd_disp.locate(0, 0);                          //Set cursor (row,col)
    lcd_disp.printf("COMP2004");
    lcd_disp.locate(1, 0);
    lcd_disp.printf("Coursework C1W2");

    //Environmental sensor
    BMP280_SPI bmp280(PB_5, PB_4, PB_3, PB_2);
    bmp280.initialize();

    //SD Card (see example_code.cpp)
    write_sdcard();
    read_sdcard();

    //Quick matrix display (see example_code.cpp)
    matrix_init();      // Do once
    matrix_scan();      // Demo
    clearMatrix();      // Clear the matrix

    // Demo of sensors (not a good sampling method!)
    while (true) {
        // Light Levels
        float light = ldr;
        printf("Light level: %5.3f\n", light);

        //Temperatre and pressure
        float temperature=bmp280.getTemperature();
        float pressure=bmp280.getPressure();
        printf("%.1fC %.1fmBar\n",temperature,pressure);        

        ThisThread::sleep_for(500ms);
    }
}


