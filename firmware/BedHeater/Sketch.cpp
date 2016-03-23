/*Begining of Auto generated code by Atmel studio */
#include <Arduino.h>

/*End of auto generated code by Atmel studio */


#include "PID_v1.h"
#include <EEPROM.h>
//Beginning of Auto generated function prototypes by Atmel Studio
void setup();
void loop();
//End of Auto generated function prototypes by Atmel Studio


// Configuration
// Set your supply voltage here. Best to
// measure it (i.e. don't say 5.0 because you're
// using 5.0V, actually check the avcc pin with a voltmeter).
float supplyVoltage = 5.0f;
// resistor in the voltage divider. Again, best to actually
// measure since resistor accuracy can be poor (eg.
// if you pick a silver banded resistor, there's a 20% range
// around the target resistance where the real resistance
// can lie).
float balanceResistor = 10000;

// Pin selection
const int pinHeater = 9;
const int pinThermistor = 10;

// PID input/set point/output
double actualTemp = 0.0f;
double targetTemp = 0.0f;
double pulseWidth = 0.0f; // range [0,1]

// Tuning parameters
const double kP = 0.1f;
const double kI = 0.05f;
const double kD = 0.0f;

// Therm step addr in eeprom (to store the calibration)
// Therm step set addr is a byte set to 0 if calibration
// has been done.
const int thermStepSetAddr = 0;
const int thermYIntSetAddr = 1;
const int thermStepAddr = 2;
const int thermYIntAddr = 6;
float thermStep = 1.0f;
float thermYInt = 0.0f;
bool thermCalibrated = false;

// Constants for Steinhart-Hart
float tA = 1.4e-3f;
float tB = 2.37e-4f;
float tC = 9.9e-8f;

PID pid(&actualTemp, &pulseWidth, &targetTemp, kP, kI, kD, DIRECT);

void setup() {
  // put your setup code here, to run once:
  pinMode(pinHeater, OUTPUT);
  pinMode(pinThermistor, INPUT);

  Serial.begin(57600);
  Serial.setTimeout(250); // Set timeout to 250ms

  // Restore calibration if already set.
  thermCalibrated = true;
  if(EEPROM.read(thermStepSetAddr) == 0) {
    unsigned char setPoint[4];
    for(int i = 0; i < 4; i++)
      setPoint[i] = EEPROM.read(thermStepAddr+i);
    thermStep = *((float*)setPoint);
  } else {
    thermCalibrated = false;
  }
  
  if(EEPROM.read(thermYIntSetAddr) == 0) {
    unsigned char setPoint[4];
    for(int i = 0; i < 4; i++)
      setPoint[i] = EEPROM.read(thermYIntAddr+i);
    thermYInt = *((float*)setPoint);
  } else {
    thermCalibrated = false;
  }

  pid.SetOutputLimits(0.0f, 1.0f);
  pid.SetSampleTime(250); // 250ms compute period
  pid.SetMode(MANUAL);
}

void loop() {
  // put your main code here, to run repeatedly:
  String str = Serial.readStringUntil('\n');
  const char *cmd = str.c_str();
  if(strncmp(cmd, "AT+ SetTemp", 11) == 0 && strlen(cmd) > 13) {
    if(thermCalibrated) {
      const char *target = cmd + 12;
      targetTemp = atof(target);
  
      // Don't allow 100C to be exceeded.
      if(targetTemp > 100.0f) targetTemp = 100.0f;
  
      // PID SetMode method ignores if we go from automatic
      // to automatic
      pid.SetMode(AUTOMATIC);
      
      Serial.print("AT- SetTempOk\r\n");
    } else {
      Serial.print("AT- SetTempErr NotCalibrated\r\n");
    }
  } else if(strncmp(cmd, "AT+ SetCalYInt", 14) == 0 && strlen(cmd) > 15) {
    thermYInt = atof(cmd + 15);
    unsigned char *b = (unsigned char*)&thermYInt;
    for(int i = 0; i < 4; i++)
      EEPROM.write(thermYIntAddr+i, b[i]);
    EEPROM.write(thermYIntSetAddr, 0x0);
    if(EEPROM.read(thermStepSetAddr) == 0) 
      thermCalibrated = true;
    char buf[64];
    snprintf(buf, 64, "AT- SetCalYInt %.2f\r\n", thermYInt);
    Serial.print(buf);
  } else if(strncmp(cmd, "AT+ GetCalYInt", 14) == 0) {
    char buf[64];
    snprintf(buf, 64, "AT- CalYInt %.2f\r\n", thermYInt);
    Serial.print(buf);
  } else if(strncmp(cmd, "AT+ SetCalSlope", 10) == 0 && strlen(cmd) > 11) {
    thermStep = atof(cmd + 11);
    unsigned char *b = (unsigned char*)&thermStep;
    for(int i = 0; i < 4; i++)
      EEPROM.write(thermStepAddr+i, b[i]);
    EEPROM.write(thermStepSetAddr, 0x0);
    if(EEPROM.read(thermYIntSetAddr) == 0)
      thermCalibrated = true;
    char buf[64];
    snprintf(buf, 64, "AT- SetCalSlope %.2f\r\n", thermStep);
    Serial.print(buf);
  } else if(strncmp(cmd, "AT+ GetCalSlope", 10) == 0) {
    char buf[64];
    snprintf(buf, 64, "AT- CalSlope %.2f\r\n", thermStep);
    Serial.print(buf);
  } else if(strncmp(cmd, "AT+ GetActualTemp", 17) == 0) {
    char buf[64];
    snprintf(buf, 64, "AT- ActualTemp %.2f\r\n", actualTemp);
    Serial.print(buf);
  } else if(strncmp(cmd, "AT+ GetTargetTemp", 17) == 0) {
    char buf[64];
    snprintf(buf, 64, "AT- TargetTemp %.2f\r\n", targetTemp);
    Serial.print(buf);
  } else if(strncmp(cmd, "AT+ TurnOff", 11) == 0) {
    pid.SetMode(MANUAL);
    pulseWidth = 0;
    Serial.print("AT- TurnOffOk\r\n");
  }

  // Get actual temp from thermistor
  // Using Steinhart-Hart. Based on
  // http://playground.arduino.cc/ComponentLib/Thermistor2
  float v1 = (float)analogRead(pinThermistor) / 1024.0f * supplyVoltage;
  // Simple voltage divider.
  // v1 = supplyVoltage * Rt / (balanceResistor + Rt)
  // Solve for Rt.
  float rVal = balanceResistor * v1 / (supplyVoltage - v1);
  float lnR = log(rVal);
  float tinv = tA + tB * lnR + tC * lnR * lnR * lnR;
  actualTemp = 1.0f / tinv;

  pid.Compute();
  
  // TODO: not sure if analogWrite will work correctly,
  // may turn heater pad on/off too quickly or relay may be 
  // too slow.
  analogWrite(pinHeater, pulseWidth * 255);
}
