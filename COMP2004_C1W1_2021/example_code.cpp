#include "BMP280_SPI.h"
#include "FATFileSystem.h"
#include "SDBlockDevice.h"
#include "mbed.h"
#include <iostream>

// Instantiate the SDBlockDevice by specifying the SPI pins connected to the
// SDCard socket. The PINS are: (This can also be done in the JSON file see
// mbed.org Docs API Storage SDBlockDevice) PB_5    MOSI (Master Out Slave In)
// PB_4    MISO (Master In Slave Out)
// PB_3    SCLK (Serial Clock)
// PF_3    CS (Chip Select)
//
// and there is a Card Detect CD on PF_4 ! (NB this is an Interrupt capable
// pin..)

// MATRIX DISPLAY SPI WITH OUTPUT ENABLE
static SPI spi(PC_12, PC_11, PC_10); // MOSI, MISO, SCLK
static DigitalOut cs(PB_6);          // Chip Select ACTIVE LOW
static DigitalOut oe(PB_12);         // Output Enable ACTIVE LOW

// SD Card
static SDBlockDevice sd(PB_5, PB_4, PB_3, PF_3); // SD Card Block Device
static InterruptIn sd_inserted(PF_4); // Interrupt for card insertion events

int write_sdcard() {
  printf("Initialise and write to a file\n");

  // call the SDBlockDevice instance initialisation method.
  if (0 != sd.init()) {
    printf("Init failed \n");
    return -1;
  }

  FATFileSystem fs("sd", &sd);
  FILE *fp = fopen("/sd/test.txt", "w");
  if (fp == NULL) {
    error("Could not open file for write\n");
    sd.deinit();
    return -1;
  } else {
    // Put some text in the file...
    fprintf(fp, "Nick says Hi!\n");
    // Tidy up here
    fclose(fp);
    printf("SD Write done...\n");
    sd.deinit();
    return 0;
  }
}

int read_sdcard() {
  printf("Initialise and read from a file\n");

  // call the SDBlockDevice instance initialisation method.
  if (0 != sd.init()) {
    printf("Init failed \n");
    return -1;
  }

  FATFileSystem fs("sd", &sd);
  FILE *fp = fopen("/sd/test.txt", "r");
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

// LED Matrix
#define SCAN_RATE_MS 25
void matrix_init(void) {
  spi.format(8, 0);       // 8bits, Rising edge, +VE Logic Data
  spi.frequency(1000000); // 1MHz Data Rate
  oe = 0;
}

void matrix_scan(void) {

  for (int j = 0; j <= 7;
       j++) // NB ROW has 3 to 8 decoder so 0 to 7 to Row Select
            // COLUMNS(COL) are 0 to 255 to select each Byte
  {
    for (int i = 1; i < 255; i *= 2) // Increment LEFT Hand Block of 8
    {
      cs = 0;          // Send Data to Matrix
      spi.write(0x00); // COL RHS
      spi.write(i);    // COL LHS
      spi.write(j);    // ROW RHS
      cs = 1; // low to high will effectivelly LATCH the Shift register to
              // output
      thread_sleep_for(SCAN_RATE_MS);
    }
    for (int i = 1; i < 255; i *= 2) // Increment RIGHT Hand Block of 8
    {
      cs = 0;          // Send Data to Matrix
      spi.write(i);    // COL RHS
      spi.write(0x00); // COL LHS
      spi.write(j);    // ROW RHS
      cs = 1; // low to high will effectivelly LATCH the Shift register to
              // output
      thread_sleep_for(SCAN_RATE_MS);
    }
  }
  for (int j = 0; j <= 7; j++) 
    // NB ROW has 3 to 8 decoder so 0 to 7 to Row Select    
    // COLUMNS(COL) are 0 to 255 to select each Byte
  {
    {
      cs = 0;          // Send Data to Matrix
      spi.write(0xFF); // COL RHS
      spi.write(0xFF); // COL LHS
      spi.write(j);    // ROW RHS
      cs = 1; // low to high will effectivelly LATCH the Shift register to
              // output
      thread_sleep_for(SCAN_RATE_MS);
    }
  }
  for (int j = 7; j >= 0; j--) 
       // NB ROW has 3 to 8 decoder so 0 to 7 to Row Select
       // COLUMNS(COL) are 0 to 255 to select each Byte
  {
    {
      cs = 0;          // Send Data to Matrix
      spi.write(0xFF); // COL RHS
      spi.write(0xFF); // COL LHS
      spi.write(j);    // ROW RHS
      cs = 1; // low to high will effectivelly LATCH the Shift register to
              // output
      thread_sleep_for(SCAN_RATE_MS);
    }
  }
}

void clearMatrix(void) {
  cs = 0;          // CLEAR Matrix
  spi.write(0x00); // COL RHS
  spi.write(0x00); // COL LHS
  spi.write(0x00); // ROX RHS
  cs = 1;
}
// Enable Output NB can be more selective than this if you wish
