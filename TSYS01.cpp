#include "TSYS01.h"
#include <Wire.h>

#define TSYS01_ADDR 0x77
#define TSYS01_RESET 0x1E
#define TSYS01_ADC_READ 0x00
#define TSYS01_ADC_TEMP_CONV 0x48
#define TSYS01_PROM_READ 0XA0

TSYS01::TSYS01()
{
}

void TSYS01::init()
{
    // Reset the TSYS01, per datasheet
    Wire.beginTransmission(TSYS01_ADDR);
    Wire.write(TSYS01_RESET);
    Wire.endTransmission();

    delay(10);

    // Read calibration values
    for (uint8_t i = 0; i < 8; i++)
    {
        Wire.beginTransmission(TSYS01_ADDR);
        Wire.write(TSYS01_PROM_READ + (i << 1));
        Wire.endTransmission();

        Wire.requestFrom(TSYS01_ADDR, 2);
        C[i] = (Wire.read() << 8) | Wire.read();
    }
}

void TSYS01::prepareRead()
{
    Wire.beginTransmission(TSYS01_ADDR);
    Wire.write(TSYS01_ADC_TEMP_CONV);
    Wire.endTransmission();
}

void TSYS01::nowRead()
{
    Wire.beginTransmission(TSYS01_ADDR);
    Wire.write(TSYS01_ADC_READ);
    Wire.endTransmission();

    Wire.requestFrom(TSYS01_ADDR, 3);
    D1 = Wire.read();
    D1 = (D1 << 8) | Wire.read();
    D1 = (D1 << 8) | Wire.read();
    calculate();
}

void TSYS01::read()
{
    Wire.beginTransmission(TSYS01_ADDR);
    Wire.write(TSYS01_ADC_TEMP_CONV);
    Wire.endTransmission();

    delay(10); // Max conversion time per datasheet

    Wire.beginTransmission(TSYS01_ADDR);
    Wire.write(TSYS01_ADC_READ);
    Wire.endTransmission();

    Wire.requestFrom(TSYS01_ADDR, 3);
    D1 = Wire.read();
    D1 = (D1 << 8) | Wire.read();
    D1 = (D1 << 8) | Wire.read();
    calculate();
}

void TSYS01::readTestCase()
{
    C[0] = 0;
    C[1] = 28446; //0xA2 K4
    C[2] = 24926; //0XA4 k3
    C[3] = 36016; //0XA6 K2
    C[4] = 32791; //0XA8 K1
    C[5] = 40781; //0XAA K0
    C[6] = 0;
    C[7] = 0;

    D1 = 9378708.0f;
    calculate();
}

uint64_t tsys01_simple_pow(uint32_t input, uint8_t loop)
{
#ifdef TSYS01_USE_STANDARD_POW
    return pow(input, loop);
#else /* TEMPERATURE_FLOAT_WITH_TWO_DECIMAL */
    uint64_t returned = input;
    loop--;
    while (loop)
    {
        returned *= input;
        loop--;
    }
    return returned;
#endif /* TEMPERATURE_FLOAT_WITH_TWO_DECIMAL */
}

void TSYS01::calculate()
{
    adc = D1 >> 8;

    TEMP = (-2) * float(C[1]) / 1000000000000000000000.0f * tsys01_simple_pow(adc, 4) +
           4 * float(C[2]) / 10000000000000000.0f * tsys01_simple_pow(adc, 3) +
           (-2) * float(C[3]) / 100000000000.0f * tsys01_simple_pow(adc, 2) +
           1 * float(C[4]) / 1000000.0f * adc +
           (-1.5) * float(C[5]) / 100;
}

float TSYS01::temperature()
{
#ifdef TEMPERATURE_FLOAT_WITH_TWO_DECIMAL
    return TEMP;
#else  /* TEMPERATURE_FLOAT_AS_ACCURACY */
    float returned = (uint16_t)(TEMP * 10);
    returned /= 10;
    // serial_print_info(F("the_input:"));
    // serial_println_info(the_input);
    // serial_print_info(F("1st returned:"));
    // serial_println_info(returned);
    if ((TEMP - returned) > 0.05)
    {
        returned += 0.1;
    }
    return returned;
#endif /* TEMPERATURE_FLOAT_AS_ACCURACY */
}
