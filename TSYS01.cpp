#include "TSYS01.h"
#include <Wire.h>

#define TSYS01_ADDR 0x77
#define TSYS01_RESET 0x1E
#define TSYS01_ADC_READ 0x00
#define TSYS01_ADC_TEMP_CONV 0x48
#define TSYS01_PROM_READ 0XA0

TSYS01::TSYS01()
{
    nb_of_tries_before_getting_temp=0;
}

bool TSYS01::init()
{
    // Reset the TSYS01, per datasheet
    Wire.beginTransmission(TSYS01_ADDR);
    Wire.write(TSYS01_RESET);
    if(Wire.endTransmission()){
        //Success is 0
        return false;
    }

    delay(10);

    int received_bytes = 0;
    for(uint8_t tries = 1; tries <= 5; tries++){
        // Read calibration values
        for (uint8_t i = 0; i < 8; i++)
        {
            Wire.beginTransmission(TSYS01_ADDR);
            Wire.write(TSYS01_PROM_READ + (i << 1));
            Wire.endTransmission();

            received_bytes += Wire.requestFrom(TSYS01_ADDR, 2);
            C[i] = (Wire.read() << 8) | Wire.read();
        }
        nb_of_tries_before_getting_temp = tries;
        //if(true){
        if(received_bytes && is_crc_ok()){
            return true;
        }
        delay(100);
    }
    return false;
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
    Wire.read();//Drop this data
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

void TSYS01::read()
{
    Wire.beginTransmission(TSYS01_ADDR);
    Wire.write(TSYS01_ADC_TEMP_CONV);
    Wire.endTransmission();

    delay(20); // Max conversion time per datasheet(8.5ms(might be lower with less power))

    Wire.beginTransmission(TSYS01_ADDR);
    Wire.write(TSYS01_ADC_READ);
    Wire.endTransmission();

    Wire.requestFrom(TSYS01_ADDR, 3);
    D1 = Wire.read();
    D1 = (D1 << 8) | Wire.read();
    Wire.read();//We don't care about this one
    calculate();
}

void TSYS01::calculate()
{
	TEMP += (-2.0)*C[1];
	TEMP *= ((float)D1) / 100000;
	TEMP += (4.0)*C[2];
	TEMP *= ((float)D1) / 100000;
	TEMP += (-2.0)*C[3];
	TEMP *= ((float)D1) / 100000;
	TEMP += C[4];
	TEMP *= ((float)D1) / 100000;
	TEMP *= 10;
	TEMP += (-1.5)*C[5];
	TEMP /= 100;
}

uint8_t TSYS01::compute_coefficient_crc(){
    uint8_t loop,returned = 0;
    for(loop=0; loop<8;loop++){
        returned += (C[loop]>>8)&0xFF;
        returned += C[loop]&0xFF;
    }
	return returned;
}

bool TSYS01::is_crc_ok(void) {
	return ((compute_coefficient_crc() & 0xFF)==0);
}

uint32_t TSYS01::serial(void) {
    return ((C[7]<<8)&0xFFFF00)+((C[8]>>8)&0xFF);
}

uint16_t TSYS01::get_coef_at_pos(uint8_t coef_pos) {
    if((0<coef_pos)&&(coef_pos<8)){
        return (C[coef_pos]);
    }
    return 0xFFFF;
}

uint8_t TSYS01::get_nb_of_tries_before_getting_temp(void) {
    return nb_of_tries_before_getting_temp;
}

float TSYS01::temperature()
{
#ifdef TEMPERATURE_FLOAT_WITH_TWO_DECIMAL
    return TEMP;
#else  /* TEMPERATURE_FLOAT_AS_ACCURACY */
    float returned = (int16_t)(TEMP * 10);
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
