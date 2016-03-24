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
// TODO: we should be able to determine this internally using the reference voltage
// in the avr.
float supplyVoltage = 3.3f;
// resistor in the voltage divider. Again, best to actually
// measure since resistor accuracy can be poor (eg.
// if you pick a silver banded resistor, there's a 20% range
// around the target resistance where the real resistance
// can lie).
float balanceResistor = 8960;

// Pin specifiers use arduino nano indexing
// Pin selection
const int pinHeater = 3;
// ADC pin 0
const int pinThermistor = 0;

// PID input/set point/output
double actualTemp = 0.0f;
double targetTemp = 293.15f;
double pulseWidth = 0.0f; // range [0,1]

// Tuning parameters
const double kPfar = 0.1f;
const double kIfar = 0.01f;
const double kDfar = 0.0f;

const double kPnear = 0.1f;
const double kInear = 0.01f;
const double kDnear = 0.0f;

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

// Constants for Steinhart-Hart. These need updated depending on your thermistor.
float tA = 6.1406e-04;
float tB = 2.2759e-04;
float tC = 7.8425e-08;

PID pid(&actualTemp, &pulseWidth, &targetTemp, kPnear, kInear, kDnear, DIRECT);

void setup() {
  // put your setup code here, to run once:
  pinMode(pinHeater, OUTPUT);
  pinMode(pinThermistor, INPUT);
  digitalWrite(pinHeater, 0);

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
    const char *target = cmd + 12;
    targetTemp = (float)atoi(target) / 100.0f;
  
    // Don't allow 100C to be exceeded.
    if(targetTemp > 100.0f) targetTemp = 100.0f;
  
    // Convert to Kelvin
    targetTemp += 273.15;

    // PID SetMode method ignores if we go from automatic
    // to automatic
    pid.SetMode(AUTOMATIC);
      
    Serial.print("AT- SetTempOk\r\n");
  } else if(strncmp(cmd, "AT+ GetActualTemp", 17) == 0) {
    char buf[64];
    snprintf(buf, 64, "AT- ActualTemp %u\r\n", (unsigned int)((actualTemp-273.15) * 100));
    Serial.print(buf);
  } else if(strncmp(cmd, "AT+ GetTargetTemp", 17) == 0) {
    char buf[64];
    snprintf(buf, 64, "AT- TargetTemp %u\r\n", (unsigned int)((targetTemp-273.15) * 100));
    Serial.print(buf);
  } else if(strncmp(cmd, "AT+ TurnOff", 11) == 0) {
    pid.SetMode(MANUAL);
    pulseWidth = 0;
    Serial.print("AT- TurnOffOk\r\n");
  } else if(str.length() > 0) {
    Serial.print("AT- UnknownCmd\r\n");
  }

  // Get actual temp from thermistor
  // Using Steinhart-Hart. Based on
  // http://playground.arduino.cc/ComponentLib/Thermistor2
  int pinValue = analogRead(pinThermistor);
  //float v1 = (float)pinValue / 1024.0f * supplyVoltage;
  // Simple voltage divider.
  // v1 = supplyVoltage * Rt / (balanceResistor + Rt)
  // Solve for Rt.
  float rVal = balanceResistor * (1023.0f/pinValue-1);
  float lnR = log(rVal);
  float tinv = tA + tB * lnR + tC * lnR * lnR * lnR;
  actualTemp = 1.0f / tinv;

  if(fabs(actualTemp - targetTemp) > 5) {
    pid.SetTunings(kPfar, kIfar, kDfar);
  } else {
    pid.SetTunings(kPnear, kInear, kDnear);
  }

  pid.Compute();
  
  // TODO: not sure if analogWrite will work correctly,
  // may turn heater pad on/off too quickly or relay may be 
  // too slow.
  analogWrite(pinHeater, pulseWidth * 255);
}
