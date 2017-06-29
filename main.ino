#include <TimerOne.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define TL 10
#define TICK_SECONDS 60
#define FAN_MAX 3
#define SENSOR_NUM 3
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

DeviceAddress sensor_addr[] = {\
{0x28,0xEE,0xE2,0xFA,0x2E,0x16,0x02,0x8D},\
{0x28,0xEE,0x25,0xF5,0x2E,0x16,0x02,0x64},\
{0x28,0xEE,0xD3,0xFE,0x2E,0x16,0x02,0xCA}};

volatile unsigned short task[] = {0,0,0,0,0,0,0,0,0,0};
volatile unsigned short tick_loop = TICK_SECONDS;
volatile unsigned long timestamp;
volatile unsigned long fan_speed_tmp = 0;
volatile unsigned long fan_speed[] = {0,0,0,0};
volatile unsigned long fan_set[] = {0x5F,0xaf,0xcf,0xff};
volatile float sensor_temp[] = {0.0,0.0,0.0};
volatile unsigned long sensor_temp_stamp = 0;

String inputString = "";         // a string to hold incoming data

const byte pin_onboard_led = 13;
const byte pin_fan_pwm[] = {8,7,6};
const byte pin_fan_int[] = {18,19,20};

void setup(void) {
  unsigned short i;
  
  TCCR4B &= ~7;
  TCCR4B |= 1;
  Timer1.initialize();
  Timer1.attachInterrupt(tick, 1000000); // tick to run every 0.15 seconds
  
  for(i=0;i<FAN_MAX;i++)  pinMode(pin_fan_int[i], INPUT);
  
  Serial.begin(9600);
  inputString.reserve(200);
  while (!Serial);
  
  sensors.begin();
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar != '\r' && inChar != '\n'){
      inputString += inChar;
    }
    if ((inChar == '\n' || inChar == '\r') && inputString != "") {
      bool done = task_add(2);
      if(!done) Serial.println('Task overflow!');
    }
  }
}

void tick(void) {
  if(tick_loop < 0) tick_loop = TICK_SECONDS;
  
  if(tick_loop%10 == 0)  task_add(3);
  if(tick_loop%10 == 0)  task_add(4);
  if(tick_loop%10 == 0)  task_add(1);
  
  tick_loop -= 1;
}

void loop(void) {
  unsigned short i;
  analogWrite(pin_onboard_led,0);
  for(i=0;i<FAN_MAX;i++)  analogWrite(pin_fan_pwm[i],fan_set[i]);
  fan_set[i] = millis();
  
  if(task[0] == 0)  return;

  switch (task[0]) {
    case 1:
      report_01();
      break;
    case 2:
      cmd_02();
      break;
    case 3:
      fan_speed_detect_03();
      break;
    case 4:
      read_temp_04();
      break;
    default: 

    break;
  }
  task_shift();
}

void cmd_02(void){
  timestamp = millis();
  Serial.print(timestamp);
  Serial.print('\t');
  task_print();
  Serial.print('\t');
  Serial.print(fan_speed[0]);
  Serial.print('\t');
  Serial.print(fan_speed[1]);
  Serial.print('\t');
  Serial.print(fan_speed[2]);
  Serial.print('\t');
  Serial.println(fan_speed[3]);
  //Serial.println(inputString);
  inputString = "";
}

void fan_speed_detect_03(void){
  unsigned short i;
  for(i=0;i<FAN_MAX;i++){
    fan_speed_tmp = 0;
    attachInterrupt(digitalPinToInterrupt(pin_fan_int[i]), speed_counter, FALLING);
    delay(100);
    detachInterrupt(digitalPinToInterrupt(pin_fan_int[i]));
    fan_speed[i] = fan_speed_tmp * 300;
  }
  fan_speed[i] = millis();
}

void report_01(void) {
  unsigned short i;
  Serial.print("{'type':'report','timestamp':");
  Serial.print(millis());
  Serial.print(",'speed':[");
  for(i=0;i<FAN_MAX+1;i++){
    Serial.print(fan_speed[i]);
    if(i<FAN_MAX) Serial.print(',');
  }
  Serial.print("],'set':[");
  for(i=0;i<FAN_MAX+1;i++){
    Serial.print(fan_set[i]);
    if(i<FAN_MAX) Serial.print(',');
  }
  Serial.print("],'temp':[");
  for(i=0;i<SENSOR_NUM;i++){
    Serial.print(sensor_temp[i]);
    Serial.print(',');
  }
  Serial.print(sensor_temp_stamp);
  Serial.println("]}");
}

void speed_counter(void){
  fan_speed_tmp += 1;
}

void task_print(void) {
  unsigned short i;
  for(i=0;i<TL;i++) Serial.print(task[i]);
}

void task_shift(void) {
  unsigned int i;
  noInterrupts();
  for(i=0;i<TL-1;i++){
    task[i] = task[i+1];
  }
  task[i] = 0;
  interrupts();
}

bool task_add(unsigned int num) {
  unsigned int i;
  for(i=0;i<TL;i++)
  {
    if(task[i] == 0)
    {
      break;
    }
  }
  if(i < TL)
  {
    task[i] = num;
    return true;
  }
  else {
    return false;
  }
}

void read_temp_04(void){ 
  sensors.requestTemperatures(); // Send the command to get temperatures
  for(int i=0;i<SENSOR_NUM; i++)
  {
		/*
		Serial.print("Temperature for device: ");
		Serial.println(i,DEC);
		Serial.print("Temp C: ");
    Serial.println(sensors.getTempC(sensor_addr[i]));
    */
    sensor_temp[i] = sensors.getTempC(sensor_addr[i]);
	}
	sensor_temp_stamp = millis();
}


