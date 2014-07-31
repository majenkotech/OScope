/*
 *
 * Left encoder = CN17/18 + CN7
 * Right encoder = CN5/6 + CN1
 *
 */
#include <DSPI.h>
#include <TFT.h>
#include <PIC32.h>
#include <SPIRAM.h>
#include <ChangeNotification.h>
#include "Encoder.h"

#define BITS 10
#define SAMPLEPOINTS (1<<BITS)
#define TIMESAMPLES 512

TFTPMP pmp;
SSD1289 tft(&pmp);

volatile uint8_t rightMode = 0;
volatile uint32_t cursor = 0;

DSPI0 spi;
TFTDSPI tpSPI(&spi, PIN_DSPI0_SS, 255, 100000);
XPT2046 tp(&tpSPI, 240, 320);

DSPI1 obspi;
SPIRAM sram(&obspi, 131072, PIN_A0, PIN_E8, PIN_E9);

uint8_t buffer[SSD1289::Width * SSD1289::Height];
Framebuffer332Fast fb(SSD1289::Width, SSD1289::Height, buffer);

Encoder e0(49, 50, 20);
Encoder e1(21, 22, 23);

struct _flag {
    union {
        uint32_t value;
        struct {
            unsigned ac: 1;
            unsigned dispave: 1;
        } __attribute__((packed));
    } __attribute__((packed));
} __attribute__((packed));

#define MAXCHAN 2
uint8_t brightness = 10;

volatile uint32_t sampleNumber = 0;
uint8_t triggerChannel = 0;
//float sampleFrequency = 200000;
uint8_t decades = 4;
volatile uint32_t sampleSize = 0;

float stored[SAMPLEPOINTS];

struct channel {
    volatile int16_t data[SAMPLEPOINTS];
    float values[SAMPLEPOINTS];
    int16_t real[SAMPLEPOINTS];
    int16_t imaginary[SAMPLEPOINTS];
    float referenceVoltage;
    uint8_t selectedVoltage;
    uint32_t sampleCount;
    union {
        uint16_t flags;
        struct {
            unsigned acCoupled: 1;
            unsigned displayAverage: 1;
            unsigned enabled: 1;
            unsigned subtract: 1;
        } __attribute__((packed));
    } __attribute__((packed));
};

struct channel channels[MAXCHAN];

struct complex {
    float real;            //Real part of the number
    float imaginary;    //Imaginary part of the number
};

struct selection {
    float value;
    char *name;
    uint8_t extra;
};

volatile uint8_t selectedTimebase = 3;

const struct selection timeSelections[] = {
    { 0.0001, "100us", 0 },
    { 0.0002, "200us", 0 },
    { 0.0005, "500us", 0 },
    { 0.001, "1ms", 0 },
    { 0.002, "2ms", 0 },
    { 0.005, "5ms", 0 },
    { 0.01,  "10ms", 0 },
    { 0.02,  "20ms", 0 },
    { 0.05,  "50ms", 0 },
    { 0.1,  "100ms", 0 },
    { 0.2,  "200ms", 0 },
    { 0.5,  "500ms", 0 },
    { 1.0,  "1s", 0 },
    { 2.0,  "2s", 0 },
    { 5.0,  "5s", 0 },
    { 0, 0, 0}
};

const struct selection voltages[] = {
    { 0.001, "100uV", 10 },
    { 0.002, "200uV", 10 },
    { 0.005, "500uV", 10 },
    { 0.01, "1mV", 10 },
    { 0.02, "2mV", 10 },
    { 0.05, "5mV", 10 },
    { 0.1, "10mV", 10 },
    { 0.2, "20mV", 10 },
    { 0.05, "50mV", 1 },
    { 0.1, "100mV", 1 },
    { 0.2, "200mV", 1 },
    { 0.5, "500mV", 1 },
    { 1, "1V", 1 },
    { 2, "2V", 1 },
    { 5, "5V", 1 },
    { 0, 0, 0 }
};

//struct complex inputSet[SAMPLEPOINTS];

enum {
    Background = Color::Black,
    GridBack = Color::DarkGreen,
    GridFore = Color::Green,
    Chan1 = Color::White,
    Chan2 = Color::Yellow,
    Chan3 = Color::Cyan,
    Chan4 = Color::Magenta,
    TextFG = Color::White,
    TextErr = Color::Red,
    Information = Color::SkyBlue,
    MenuFG = Color::Green,
    MenuBG = Color::DarkGreen,
    MenuHI = Color::Yellow
};

enum {
    Time,
    Frequency
};

uint8_t domain = Time;

void printFloatUnits(float f) {
    if (f < 0) {
        fb.print("-");
        f = 0 - f;
    }

    if (f < 0.000001) {
        fb.print(f * 1000000000);
        fb.print("n");
        return;
    }

    if (f < 0.001) {
        fb.print(f * 1000000.0);
        fb.print("u");
        return;
    }

    if (f < 1) {
        fb.print(f * 1000.0);
        fb.print("m");
        return;
    }

    if (f >= 1000.0) {
        fb.print(f / 1000.0);
        fb.print("K");
        return;
    }

    if (f >= 1000000.0) {
        fb.print(f / 1000000.0);
        fb.print("M");
        return;
    }

    fb.print(f);
}
int16_t segment;
int16_t xmargin;
int16_t ymargin;

void setup() {
    obspi.begin();
    sram.begin();
    pinMode(PIN_G9, OUTPUT);
    digitalWrite(PIN_G9, HIGH);
    analogWrite(PIN_OC3, brightness * 25 + 5);

    pinMode(PIN_C2, OUTPUT);
    pinMode(PIN_C3, OUTPUT);

    fb.initializeDevice();
    tft.initializeDevice();
    tp.initializeDevice();
    fb.setRotation(1);
    fb.fillScreen(0);
    tft.fillScreen(Color::Black);

//	fb.setColor(GridBack, Color::DarkGreen);
//	fb.setColor(GridFore, Color::Green);
//	fb.setColor(Chan1, Color::White);
//	fb.setColor(Chan2, Color::Yellow);
//	fb.setColor(Chan3, Color::Cyan);
//	fb.setColor(Chan4, Color::Magenta);
//	fb.setColor(TextFG, Color::Goldenrod);
//	fb.setColor(TextErr, Color::Red);
//	fb.setColor(Information, Color::SkyBlue);
//	fb.setColor(MenuBG, Color::DarkGreen);
//	fb.setColor(MenuFG, Color::Green);
//	fb.setColor(MenuHI, Color::Yellow);
    for (int i = 0; i < MAXCHAN; i++) {
        channels[i].referenceVoltage = 1.65;
        channels[i].enabled = i == 0 ? 1 : 0;
        channels[i].selectedVoltage = 7;
        channels[i].acCoupled = 1;
        channels[i].displayAverage = 0;
        channels[i].subtract = 0;
        setGain(i, voltages[channels[i].selectedVoltage].extra);
    }

    setOffsetVoltage(0, 1.64);
    segment = fb.getWidth() / 11;
    xmargin = (fb.getWidth() - (segment * 10)) / 2;
    ymargin = (fb.getHeight() - (segment * 8)) / 2;

    fb.setTextWrap(false);
    e0.begin();
    e1.begin();

    e0.attachInterrupt(leftIncrement, Encoder::INCREMENT);
    e0.attachInterrupt(leftDecrement, Encoder::DECREMENT);
    e1.attachInterrupt(rightIncrement, Encoder::INCREMENT);
    e1.attachInterrupt(rightDecrement, Encoder::DECREMENT);

    e1.attachInterrupt(rightPress, Encoder::PRESS);
}

uint32_t sampleStartTime = 0;

void loop() {
    switch (domain) {
    case Time:
        calculateTimeDomain();
        displayTimeDomain();
        break;

    case Frequency:
        calculateFrequencyDomain();
        displayFrequencyDomain();
        break;
    }

    if (millis() - sampleStartTime > 1000) {
        fb.setCursor(20, fb.getHeight() - 10);
        fb.setTextColor(Information, 0);
        fb.print("Sampling: ");
        hbar(100, fb.getHeight() - 10, 200, 8, sampleSize, sampleNumber, Information);
//		fb.print((int)(sampleNumber * 100 / SAMPLEPOINTS));
//		fb.print("%");
    }

#if 0
    fb.setCursor(100, 100);
    fb.setTextColor(Information, 0);
    fb.print(e0.read(), DEC);
    fb.print(" ");
    fb.print(e0.readButton(), DEC);

    fb.setCursor(200, 100);
    fb.setTextColor(Information, 0);
    fb.print(e1.read(), DEC);
    fb.print(" ");
    fb.print(e1.readButton(), DEC);
#endif
    menu();
    fb.update(&tft);
}

void setOffsetVoltage(uint8_t chan, float v) {
    uint16_t vset = v * 2000.0;
    vset &= 0x0FFF;

    if (chan == 1) {
        vset |= 1 << 15;
    }

    vset |= 1 << 13;
    vset |= 1 << 12;
    obspi.setTransferSize(16);
    digitalWrite(PIN_G9, LOW);
    obspi.transfer(vset);
    digitalWrite(PIN_G9, HIGH);
}

void leftIncrement(uint8_t type) {
    switch (domain) {
    case Frequency:
        if (decades < 5) {
            AD1CON1bits.ON = 0;
            decades++;
        }

        for (uint8_t c = 0; c < MAXCHAN; c++) {
            for (uint32_t x = 0; x < sampleSize; x++) {
                channels[c].values[x] = 0;
            }

            channels[c].sampleCount = 0;
        }

        break;

    case Time:
        if (timeSelections[selectedTimebase + 1].name != 0) {
            selectedTimebase++;
        }
    }
}

void leftDecrement(uint8_t type) {
    switch (domain) {
    case Frequency:
        if (decades > 1) {
            AD1CON1bits.ON = 0;
            decades--;
        }

        for (uint8_t c = 0; c < MAXCHAN; c++) {
            for (uint32_t x = 0; x < sampleSize; x++) {
                channels[c].values[x] = 0;
            }

            channels[c].sampleCount = 0;
        }

        break;

    case Time:
        if (selectedTimebase > 0) {
            selectedTimebase--;
        }
    }
}

void rightIncrement(uint8_t type) {
    if (rightMode == 0) {
        if (voltages[channels[0].selectedVoltage + 1].name != 0) {
            channels[0].selectedVoltage++;
        }
    } else {
        cursor++;
    }
}

void rightDecrement(uint8_t type) {
    if (rightMode == 0) {
        if (channels[0].selectedVoltage > 0) {
            channels[0].selectedVoltage--;
        }
    } else {
        if (cursor > 0) {
            cursor--;
        }
    }
}

void rightPress(uint8_t type) {
    rightMode = 1 - rightMode;
}
