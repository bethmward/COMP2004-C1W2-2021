// COMP2004 Coursework
// All code by Bethany Marie Ward (req 11)
// Student Reference Number : 10605926
// Group: D
// I hope that I have satisfied reqs 1, 2, 3, 5, 6, 7, 10, 11, 12, 13 [worth 72
// marks]. Reqs 5, 7, 11 and 12 are reqs that I have considered passive, and so
// have not been identified in comments. I have not attempted reqs 4, 8 and 9
// [worth 28 marks].

#include "../lib/uopmsb/uop_msb_2_0_0.h"
#include "BMP280_SPI.h"
#include "FATFileSystem.h"
#include "SDBlockDevice.h"
#include "mbed.h"
#include "rtos.h"
#include "time.h"
#include <chrono>
#include <iostream>


using namespace std;
using namespace uop_msb_200;

// Environmental sensor
AnalogIn ldr(PC_0);
BMP280_SPI bmp280(PB_5, PB_4, PB_3, PB_2);

// SD Card
static SDBlockDevice sd(PB_5, PB_4, PB_3, PF_3); // SD Card Block Device
static InterruptIn sd_inserted(PF_4); // Interrupt for card insertion events

// LEDs
static DigitalOut redLED(TRAF_RED1_PIN);
static DigitalOut greenLED(TRAF_GRN1_PIN);

// Buttons
DigitalIn userBtn(BTN1_PIN);

// Threads - I do not have four (as in req 6 because I am not reading input from
// serial terminal or communicating with a network)
Thread t1;
Thread t2;

// FIFO structure for req 2
struct FIFO {
public:
  int start = -1;
  int length = 0;
  int end = -1;
  string *data = new string[16];

  bool isFull(FIFO f) { return length >= 10; }

  bool isEmpty(FIFO f) { return length <= 0; }
};

// Global FIFO buffer (req 3)
FIFO f;

// function prototypes
void measureSensorData();
int writeSD();
int readSD();

int main() {
  printf("Starting...\n");
  t1.start(measureSensorData); // (req 6)
  printf("Insert SD and press blue button to write 16 data to SD\n");
  wait_us(50000000);
  t2.start(writeSD);
  while (sd.init() == 1) {
    if (userBtn != 0) {  // unfortunately the button does not seem to work
      greenLED = 1;      // (req 13)
      t2.start(writeSD); // (req 6)
    }
  }
}

// add function for FIFO buffer (req 3) adds new data unless buffer is full
static void add(FIFO f, string s) {
  if (f.isFull(f)) {
    error("Ring buffer is full - cannot add more data to be written");
    redLED = 1;
  } else {
    if (f.isEmpty(f)) { // If buffer empty, reset start and add data there
      f.start = f.end = 0;
      f.data[f.start] = s;
    } else {
      if (f.end == 15) { // If end of buffer at end of array, loop around
        f.end = -1;
      }
      f.end = f.end + 1; // Add data to end of buffer
      f.data[f.end] = s;
    }
    f.length++; // Keep track of length
  }
}

// remove function for FIFO buffer (req 3) removes the oldest data unless buffer
// is empty
static string remove(FIFO f) {
  if (f.isEmpty(f)) {
    error("Ring buffer is empty - no data to write");
    redLED = 1;
    return f.data[0];
  } else {
    string data = f.data[f.start]; // Oldest data is found at start
    f.start = f.start + 1;
    if (f.start == 16) {
      f.start = 0;
    }
    f.length--; // Keep track of length
    return data;
  }
}

// Measure sensor data (Req 1). I have taken in a FIFO argument to write data to
// the buffer as I measure it.
void measureSensorData() {
  float temperature, pressure;
  float ambience;

  bmp280.initialize();

  printf("Sampling...\n");
  while (true) {
    temperature = bmp280.getTemperature();
    pressure = bmp280.getPressure();
    ambience = ldr;
    auto time =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    string s = ctime(&time);

    printf("Datetime: %s", ctime(&time));
    printf("Temperature (C): %.1f; Pressure (mBar): %.1f ; Light: %.1f\n",
           temperature, pressure, ambience);

    if (f.isFull(f) == true) { // Check for full FIFO and remove old data.
      remove(f);
    }

    add(f, ("Datetime: " + s + " ; Temperature (C): " + to_string(temperature) +
            " ; Pressure (mBar): " + to_string(pressure) +
            " ; Light: " + to_string(ambience))); // Then add new data...

    ThisThread::sleep_for(1000ms); //...and repeat each second
  }
}

// Write sensor data to SD card using FIFO buffer (req 2, req 3)
int writeSD() {
  t1.terminate();
  printf("\nWriting...\n");

  // Ensure SD can be initialised (req 10)
  if (0 != sd.init()) {
    redLED = 1;
    printf("Initialisation failed \n");
    return -1;
  }

  // If SD is mounted, write is attempted
  FATFileSystem fs("sd", &sd);
  FILE *fp = fopen("/sd/data.txt", "w");
  if (fp == NULL) { // Check for ability to write to file on SD
    redLED = 1;
    error("Could not open a file for write\n");
    sd.deinit();
    ThisThread::yield();
    t1.start(measureSensorData);
    return -1;
  } else {
    // Green LED flashes for duration of write (req 13)
    // Copy FIFO buffer to text file (req 2, req 3)
    for (int i = f.start; i < f.length; i++) {
      greenLED = !greenLED;
      string x = remove(f);
      int n = x.length();
      char char_array[n + 1];
      strcpy(char_array, x.c_str());
      fprintf(fp, "%s\n", char_array);
    }

    // Close everything off in order
    fclose(fp);
    printf("SD Write done...\n");
    greenLED = 0;
    sd.deinit();
    ThisThread::yield();
    t1.start(measureSensorData);
    return 0;
  }
}

// Purely for testing purposes.
int readSD() {
  printf("Initialise and read from a file\n");

  // Ensure SD can be initialised SDBlock Instance Instansiation Method
  if (0 != sd.init()) {
    printf("Init failed \n");
    return -1;
  }

  FATFileSystem fs("sd", &sd);
  FILE *fp = fopen("/sd/data.txt", "r");
  if (fp == NULL) {
    error("Could not open or find file for read\n");
    sd.deinit();
    return -1;
  } else {
    // Put some text in the file...
    char buff[64];
    buff[63] = 0;
    while (!feof(fp)) {
      fgets(buff, 63, fp);
      printf("%s\n", buff);
    }
    // Tidy up here
    fclose(fp);
    printf("SD Write done...\n");
    sd.deinit();
    return 0;
  }
}
