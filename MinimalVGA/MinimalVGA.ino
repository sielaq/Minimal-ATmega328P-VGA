// ****************************************************************************
// *****                                                                  *****
// ***** AtMega328P 40 x 25 Character VGA from a single 74HC166 shift reg *****
// *****                                                                  *****
// *****        written by Carsten Herting 14.03.2021 Version 2.0         *****
// *****                                                                  *****
// ****************************************************************************
// Set AtMega328P FuseA from 0xFF to 0xBF to enable CLOCK output.
// D0-7 (Arduino Pin D0-7): parallel pixel output to 74HC166 shift reg
// B0   (Arduino pin D8):   16MHz clock output
// B2   (Arduino pin D10):  /VSYNC VGA (timer1, every 1/60s)
// B3   (Arduino pin D11):  /PE of 74HC166 (by hand inside ISR, after each character)
// B4   (Arduino pin D12):  /HSYNC VGA (by hand inside ISR, every 32µs)

volatile byte vram[25][40];       // array of video ram data

volatile int vline = 0;           // current vertical position of pixel video output

const volatile byte __attribute__ ((aligned (256))) charset[8][256] PROGMEM =
{
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xC0,0x00,0x18,0x66,0x66,0x18,0x62,0x3C,0x06,0x0C,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x3C,0x18,0x3C,0x3C,0x06,0x7E,0x3C,0x7E,0x3C,0x3C,0x00,0x00,0x0E,0x00,0x70,0x3C,0x3C,0x18,0x7C,0x3C,0x78,0x7E,0x7E,0x3C,0x66,0x3C,0x1E,0x66,0x60,0x63,0x66,0x3C,0x7C,0x3C,0x7C,0x3C,0x7E,0x66,0x66,0x63,0x66,0x66,0x7E,0x3C,0x00,0x3C,0x00,0x00,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0E,0x18,0x70,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x18,0x18,0xFF,0xFF,0x01,0x80,0x0F,0xF0,0xFF,0x00,0x0F,0xF0,0xFF,0x00,0x0F,0xF0,0xFF,0x00,0x0F,0xF0,0xFF,0xE7,0x99,0x99,0xE7,0x9D,0xC3,0xF9,0xF3,0xCF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xC3,0xE7,0xC3,0xC3,0xF9,0x81,0xC3,0x81,0xC3,0xC3,0xFF,0xFF,0xF1,0xFF,0x8F,0xC3,0xC3,0xE7,0x83,0xC3,0x87,0x81,0x81,0xC3,0x99,0xC3,0xE1,0x99,0x9F,0x9C,0x99,0xC3,0x83,0xC3,0x83,0xC3,0x81,0x99,0x99,0x9C,0x99,0x99,0x81,0xC3,0xFF,0xC3,0xFF,0xFF,0xC3,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xF1,0xE7,0x8F,0xFF,0xFF,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0xE0,0x00,0x18,0x66,0x66,0x3E,0x66,0x66,0x0C,0x18,0x18,0x66,0x18,0x00,0x00,0x00,0x03,0x66,0x18,0x66,0x66,0x0E,0x60,0x66,0x66,0x66,0x66,0x00,0x00,0x18,0x00,0x18,0x66,0x66,0x3C,0x66,0x66,0x6C,0x60,0x60,0x66,0x66,0x18,0x0C,0x6C,0x60,0x77,0x76,0x66,0x66,0x66,0x66,0x66,0x18,0x66,0x66,0x63,0x66,0x66,0x06,0x30,0x60,0x0C,0x18,0x00,0x66,0x00,0x60,0x00,0x06,0x00,0x0E,0x00,0x60,0x18,0x06,0x60,0x38,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x18,0x00,0x10,0x00,0x00,0x00,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x18,0x18,0x7F,0xFE,0x03,0xC0,0x0F,0xF0,0xFF,0x00,0x0F,0xF0,0xFF,0x00,0x0F,0xF0,0xFF,0x00,0x0F,0xF0,0xFF,0xE7,0x99,0x99,0xC1,0x99,0x99,0xF3,0xE7,0xE7,0x99,0xE7,0xFF,0xFF,0xFF,0xFC,0x99,0xE7,0x99,0x99,0xF1,0x9F,0x99,0x99,0x99,0x99,0xFF,0xFF,0xE7,0xFF,0xE7,0x99,0x99,0xC3,0x99,0x99,0x93,0x9F,0x9F,0x99,0x99,0xE7,0xF3,0x93,0x9F,0x88,0x89,0x99,0x99,0x99,0x99,0x99,0xE7,0x99,0x99,0x9C,0x99,0x99,0xF9,0xCF,0x9F,0xF3,0xE7,0xFF,0x99,0xFF,0x9F,0xFF,0xF9,0xFF,0xF1,0xFF,0x9F,0xE7,0xF9,0x9F,0xC7,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xE7,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xE7,0xE7,0xE7,0xFF,0xEF,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0E,0x70,0x00,0x18,0x66,0xFF,0x60,0x0C,0x3C,0x18,0x30,0x0C,0x3C,0x18,0x00,0x00,0x00,0x06,0x6E,0x38,0x06,0x06,0x1E,0x7C,0x60,0x0C,0x66,0x66,0x18,0x18,0x30,0x7E,0x0C,0x06,0x6E,0x66,0x66,0x60,0x66,0x60,0x60,0x60,0x66,0x18,0x0C,0x78,0x60,0x7F,0x7E,0x66,0x66,0x66,0x66,0x60,0x18,0x66,0x66,0x63,0x3C,0x66,0x0C,0x30,0x30,0x0C,0x3C,0x00,0x6E,0x3C,0x60,0x3C,0x06,0x3C,0x18,0x3E,0x60,0x00,0x00,0x60,0x18,0x66,0x7C,0x3C,0x7C,0x3E,0x7C,0x3E,0x7E,0x66,0x66,0x63,0x66,0x66,0x7E,0x18,0x18,0x18,0x00,0x30,0x00,0x00,0x00,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x1C,0x38,0x3F,0xFC,0x07,0xE0,0x0F,0xF0,0xFF,0x00,0x0F,0xF0,0xFF,0x00,0x0F,0xF0,0xFF,0x00,0x0F,0xF0,0xFF,0xE7,0x99,0x00,0x9F,0xF3,0xC3,0xE7,0xCF,0xF3,0xC3,0xE7,0xFF,0xFF,0xFF,0xF9,0x91,0xC7,0xF9,0xF9,0xE1,0x83,0x9F,0xF3,0x99,0x99,0xE7,0xE7,0xCF,0x81,0xF3,0xF9,0x91,0x99,0x99,0x9F,0x99,0x9F,0x9F,0x9F,0x99,0xE7,0xF3,0x87,0x9F,0x80,0x81,0x99,0x99,0x99,0x99,0x9F,0xE7,0x99,0x99,0x9C,0xC3,0x99,0xF3,0xCF,0xCF,0xF3,0xC3,0xFF,0x91,0xC3,0x9F,0xC3,0xF9,0xC3,0xE7,0xC1,0x9F,0xFF,0xFF,0x9F,0xE7,0x99,0x83,0xC3,0x83,0xC1,0x83,0xC1,0x81,0x99,0x99,0x9C,0x99,0x99,0x81,0xE7,0xE7,0xE7,0xFF,0xCF,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1C,0x38,0x00,0x18,0x00,0x66,0x3C,0x18,0x38,0x00,0x30,0x0C,0xFF,0x7E,0x00,0x7E,0x00,0x0C,0x76,0x18,0x0C,0x1C,0x66,0x06,0x7C,0x18,0x3C,0x3E,0x00,0x00,0x60,0x00,0x06,0x0C,0x6E,0x7E,0x7C,0x60,0x66,0x78,0x78,0x6E,0x7E,0x18,0x0C,0x70,0x60,0x6B,0x7E,0x66,0x7C,0x66,0x7C,0x3C,0x18,0x66,0x66,0x6B,0x18,0x3C,0x18,0x30,0x18,0x0C,0x7E,0x00,0x6E,0x06,0x7C,0x60,0x3E,0x66,0x3E,0x66,0x7C,0x38,0x06,0x6C,0x18,0x7F,0x66,0x66,0x66,0x66,0x66,0x60,0x18,0x66,0x66,0x6B,0x3C,0x66,0x0C,0x70,0x18,0x0E,0x3B,0x7F,0x1F,0xFF,0xF8,0x1F,0xFF,0xF8,0x1F,0xFF,0xF8,0xFF,0x07,0xE0,0x0F,0xF0,0x1F,0xF8,0x0F,0xF0,0x0F,0xF0,0xFF,0x00,0x0F,0xF0,0xFF,0x00,0x0F,0xF0,0xFF,0x00,0x0F,0xF0,0xFF,0xE7,0xFF,0x99,0xC3,0xE7,0xC7,0xFF,0xCF,0xF3,0x00,0x81,0xFF,0x81,0xFF,0xF3,0x89,0xE7,0xF3,0xE3,0x99,0xF9,0x83,0xE7,0xC3,0xC1,0xFF,0xFF,0x9F,0xFF,0xF9,0xF3,0x91,0x81,0x83,0x9F,0x99,0x87,0x87,0x91,0x81,0xE7,0xF3,0x8F,0x9F,0x94,0x81,0x99,0x83,0x99,0x83,0xC3,0xE7,0x99,0x99,0x94,0xE7,0xC3,0xE7,0xCF,0xE7,0xF3,0x81,0xFF,0x91,0xF9,0x83,0x9F,0xC1,0x99,0xC1,0x99,0x83,0xC7,0xF9,0x93,0xE7,0x80,0x99,0x99,0x99,0x99,0x99,0x9F,0xE7,0x99,0x99,0x94,0xC3,0x99,0xF3,0x8F,0xE7,0xF1,0xC4,0x80,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x38,0x1C,0x00,0x00,0x00,0xFF,0x06,0x30,0x67,0x00,0x30,0x0C,0x3C,0x18,0x00,0x00,0x00,0x18,0x66,0x18,0x30,0x06,0x7F,0x06,0x66,0x18,0x66,0x06,0x00,0x00,0x30,0x7E,0x0C,0x18,0x60,0x66,0x66,0x60,0x66,0x60,0x60,0x66,0x66,0x18,0x0C,0x78,0x60,0x63,0x6E,0x66,0x60,0x66,0x78,0x06,0x18,0x66,0x66,0x7F,0x3C,0x18,0x30,0x30,0x0C,0x0C,0x18,0x00,0x60,0x3E,0x66,0x60,0x66,0x7E,0x18,0x66,0x66,0x18,0x06,0x78,0x18,0x7F,0x66,0x66,0x66,0x66,0x60,0x3C,0x18,0x66,0x66,0x7F,0x18,0x66,0x18,0x18,0x18,0x18,0x6E,0x7F,0x1F,0xFF,0xF8,0x1F,0xFF,0xF8,0x1F,0xFF,0xF8,0xFF,0x0F,0xF0,0x07,0xE0,0x0F,0xF0,0x1F,0xF8,0x00,0x00,0x00,0x0F,0x0F,0x0F,0x0F,0xF0,0xF0,0xF0,0xF0,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0xF9,0xCF,0x98,0xFF,0xCF,0xF3,0xC3,0xE7,0xFF,0xFF,0xFF,0xE7,0x99,0xE7,0xCF,0xF9,0x80,0xF9,0x99,0xE7,0x99,0xF9,0xFF,0xFF,0xCF,0x81,0xF3,0xE7,0x9F,0x99,0x99,0x9F,0x99,0x9F,0x9F,0x99,0x99,0xE7,0xF3,0x87,0x9F,0x9C,0x91,0x99,0x9F,0x99,0x87,0xF9,0xE7,0x99,0x99,0x80,0xC3,0xE7,0xCF,0xCF,0xF3,0xF3,0xE7,0xFF,0x9F,0xC1,0x99,0x9F,0x99,0x81,0xE7,0x99,0x99,0xE7,0xF9,0x87,0xE7,0x80,0x99,0x99,0x99,0x99,0x9F,0xC3,0xE7,0x99,0x99,0x80,0xE7,0x99,0xE7,0xE7,0xE7,0xE7,0x91,0x80,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0x0E,0x00,0x00,0x00,0x66,0x7C,0x66,0x66,0x00,0x18,0x18,0x66,0x18,0x18,0x00,0x18,0x30,0x66,0x18,0x60,0x66,0x06,0x66,0x66,0x18,0x66,0x66,0x18,0x18,0x18,0x00,0x18,0x00,0x66,0x66,0x66,0x66,0x6C,0x60,0x60,0x66,0x66,0x18,0x6C,0x6C,0x60,0x63,0x66,0x66,0x60,0x3C,0x6C,0x66,0x18,0x66,0x3C,0x77,0x66,0x18,0x60,0x30,0x06,0x0C,0x18,0x00,0x66,0x66,0x66,0x60,0x66,0x60,0x18,0x3E,0x66,0x18,0x06,0x6C,0x18,0x6B,0x66,0x66,0x7C,0x3E,0x60,0x06,0x18,0x66,0x3C,0x3E,0x3C,0x3E,0x30,0x18,0x18,0x18,0x00,0x30,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x1C,0x38,0x00,0x00,0x07,0xE0,0x3F,0xFC,0x00,0x00,0x00,0x0F,0x0F,0x0F,0x0F,0xF0,0xF0,0xF0,0xF0,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x99,0x83,0x99,0x99,0xFF,0xE7,0xE7,0x99,0xE7,0xE7,0xFF,0xE7,0xCF,0x99,0xE7,0x9F,0x99,0xF9,0x99,0x99,0xE7,0x99,0x99,0xE7,0xE7,0xE7,0xFF,0xE7,0xFF,0x99,0x99,0x99,0x99,0x93,0x9F,0x9F,0x99,0x99,0xE7,0x93,0x93,0x9F,0x9C,0x99,0x99,0x9F,0xC3,0x93,0x99,0xE7,0x99,0xC3,0x88,0x99,0xE7,0x9F,0xCF,0xF9,0xF3,0xE7,0xFF,0x99,0x99,0x99,0x9F,0x99,0x9F,0xE7,0xC1,0x99,0xE7,0xF9,0x93,0xE7,0x94,0x99,0x99,0x83,0xC1,0x9F,0xF9,0xE7,0x99,0xC3,0xC1,0xC3,0xC1,0xCF,0xE7,0xE7,0xE7,0xFF,0xCF,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xE0,0x07,0x00,0x18,0x00,0x66,0x18,0x46,0x3F,0x00,0x0C,0x30,0x00,0x00,0x18,0x00,0x18,0x60,0x3C,0x7E,0x7E,0x3C,0x06,0x3C,0x3C,0x18,0x3C,0x3C,0x00,0x18,0x0E,0x00,0x70,0x18,0x3C,0x66,0x7C,0x3C,0x78,0x7E,0x60,0x3C,0x66,0x3C,0x38,0x66,0x7E,0x63,0x66,0x3C,0x60,0x0E,0x66,0x3C,0x18,0x3C,0x18,0x63,0x66,0x18,0x7E,0x3C,0x03,0x3C,0x18,0xFF,0x3C,0x3E,0x7C,0x3C,0x3E,0x3C,0x18,0x06,0x66,0x3C,0x06,0x66,0x3C,0x63,0x66,0x3C,0x60,0x06,0x60,0x7C,0x0E,0x3E,0x18,0x36,0x66,0x0C,0x7E,0x0E,0x18,0x70,0x00,0x10,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x03,0xC0,0x7F,0xFE,0x00,0x00,0x00,0x0F,0x0F,0x0F,0x0F,0xF0,0xF0,0xF0,0xF0,0xFF,0xFF,0xFF,0xFF,0xE7,0xFF,0x99,0xE7,0xB9,0xC0,0xFF,0xF3,0xCF,0xFF,0xFF,0xE7,0xFF,0xE7,0x9F,0xC3,0x81,0x81,0xC3,0xF9,0xC3,0xC3,0xE7,0xC3,0xC3,0xFF,0xE7,0xF1,0xFF,0x8F,0xE7,0xC3,0x99,0x83,0xC3,0x87,0x81,0x9F,0xC3,0x99,0xC3,0xC7,0x99,0x81,0x9C,0x99,0xC3,0x9F,0xF1,0x99,0xC3,0xE7,0xC3,0xE7,0x9C,0x99,0xE7,0x81,0xC3,0xFC,0xC3,0xE7,0x00,0xC3,0xC1,0x83,0xC3,0xC1,0xC3,0xE7,0xF9,0x99,0xC3,0xF9,0x99,0xC3,0x9C,0x99,0xC3,0x9F,0xF9,0x9F,0x83,0xF1,0xC1,0xE7,0xC9,0x99,0xF3,0x81,0xF1,0xE7,0x8F,0xFF,0xEF,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC0,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7C,0x00,0x00,0x3C,0x00,0x00,0x00,0x00,0x00,0x60,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x78,0x00,0x00,0x18,0x00,0x00,0x00,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x01,0x80,0xFF,0xFF,0x00,0x00,0x00,0x0F,0x0F,0x0F,0x0F,0xF0,0xF0,0xF0,0xF0,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xCF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xCF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xE7,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x83,0xFF,0xFF,0xC3,0xFF,0xFF,0xFF,0xFF,0xFF,0x9F,0xF9,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x87,0xFF,0xFF,0xE7,0xFF,0xFF,0xFF,
};

void loop()
{
  vram[random(25)][random(40)] = random(30, 256);
}

void setup()
{
  noInterrupts();                   // disable interrupts before messing around with timer registers

  DDRC  = 0b00000000;               // PORTC is always input
  PORTC = 0b00000000;
  DDRD  = 0b11111111;               // PORTD is always output
  PORTD = 0b00000000;
  DDRB  = 0b00111100;               // B0-1: terminal input bits 0-1, B2: VSYNC (timer1), B3: 74165 /PL (timer2 or by hand???), B4: HSync by hand inside ISR
  PORTB = 0b00011000;               // HSYNC=1

  GTCCR = 0b10000011;               // set TSM, PSRSYNC und PSRASY to correlate all 3 timers

  // *****************************
  // ***** Timer0: VGA HSYNC *****
  // *****************************
  TCNT0  = 6;                       // align VSYNC and HSYNC pulses
  TCCR0A = (1 << WGM01) | (0 << WGM00);   // mode 2: Clear Timer on Compare Match (CTC)
  TCCR0B = (0 << WGM02) | (0 << CS02) | (1 << CS01) | (0 << CS00); // x8 prescaler -> 0.5µs
  OCR0A  = 63;                      // compare match register A (TOP) -> 32µs
  TIMSK0 = (1 << OCIE0A);           // Output Compare Match A Interrupt Enable (not working: TOIE1 with ISR TIMER0_TOIE1_vect because it is already defined by timing functions)

  // *****************************
  // ***** Timer1: VGA VSYNC *****
  // *****************************
  TCNT1  = 0;
  TCCR1A = (1 << COM1B1) | (1 << COM1B0) | (1 << WGM11) | (1 << WGM10); // mode 15 (Fast PWM), set OC1B on Compare Match, clear OC1B at BOTTOM, controlling OC1B pin 10
  TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS12) | (0 << CS11) | (1 << CS10); // x1024 prescaler -> 64µs
  OCR1A  = 259;                     // compare match register A (TOP) -> 16.64ms
  OCR1B  = 0;                       // compare match register B -> 64µs
  TIMSK1 = (1 << TOIE1);            // enable timer overflow interrupt setting vlines = 0

  // ************************************************
  // ***** Timer2: only used for jitter control *****
  // ************************************************
  TCNT2  = 0;                       // used for interrupt jitter correction
  TCCR2A = (0 << COM2A1) | (0 << COM2A0) | (0 << WGM21) | (0 << WGM20); // mode 7: Fast PWM, COM2A0=0: normal port HIGH, COM2A0=1: Toggle OC2A pin 11 on Compare Match
  TCCR2B = (0 << WGM22) | (0 << CS22) | (0 << CS21) | (1 << CS20) ;     // set x0 prescaler -> 62.5ns;

  GTCCR = 0;                        // clear TSM => all timers start synchronously
  interrupts();
}

int main() { setup(); while (true) loop(); }      // enforce main() loop w/o serial handler

ISR(TIMER1_OVF_vect) { vline = 0; }               // timer1 overflow interrupt resets vline at HSYNC

/*
-----------
MIT License
-----------
Copyright (c) 2021 Carsten Herting
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
