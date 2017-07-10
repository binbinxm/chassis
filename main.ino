#include <DHT.h>
#include <TimerOne.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define TL 10
#define TICK_SECONDS 60
#define FAN_MAX 3
#define SENSOR_NUM 3
#define ONE_WIRE_BUS 3
#define DHTPIN 4     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)

DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino
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
volatile unsigned long fan_set[] = {0xFF,0xFF,0xFF,0xff};
volatile float sensor_temp[] = {0.0,0.0,0.0};
volatile unsigned long sensor_temp_stamp = 0;
volatile float humidity = 0;  //Stores humidity value
volatile float temperature = 0; //Stores temperature value
volatile float max_temp = 45.0;
bool self_protect = true;

String inputString = "";                 // a string to hold incoming data

const byte pin_onboard_led = 13;
const byte pin_fan_pwm[] = {8,7,6};
const byte pin_fan_int[] = {18,19,20};
const byte gnd_0 = 2;
const byte gnd_1 = 17;
const byte gnd_2 = 5;

void setup(void) {
    unsigned short i;
    analogWrite(gnd_0,0);
    analogWrite(gnd_1,0);
    analogWrite(gnd_2,0);
    
    TCCR4B &= ~7;
    TCCR4B |= 1;
    Timer1.initialize();
    Timer1.attachInterrupt(tick, 10000000); // tick to run every 10 seconds
    
    for(i=0;i<FAN_MAX;i++){
		pinMode(pin_fan_int[i], INPUT);
		sensors.setResolution(sensor_addr[i], 12);
	}   
    Serial.begin(9600);
    inputString.reserve(200);
    while (!Serial);
	dht.begin();
    sensors.begin();
}

void serialEvent() {
    while (Serial.available()) {
        char inChar = (char)Serial.read();
        if(inChar != '\r' && inChar != '\n')    inputString += inChar;
        else    task_add(2);
    }
}

void tick(void){
	task_add(3);//get speed
	task_add(4);//get env
	task_add(1);//send report
	return;
	}

void loop(void) {
    unsigned short i;
    analogWrite(pin_onboard_led,0);
	if(self_protect){
		for(i=0;i<FAN_MAX;i++)    analogWrite(pin_fan_pwm[i],0xff);
	}
	else{
		for(i=0;i<FAN_MAX;i++)    analogWrite(pin_fan_pwm[i],fan_set[i]);
	}
    
    if(task[0] == 0)    return;

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
            get_env_04();
            break;
        case 5:
            set_ack_05();
            break;
        default: 

        break;
    }
    task_shift();
}

void cmd_02(void){
    unsigned long tmp = 0;
    unsigned long convert = 0;
    unsigned short i;

    if(inputString == "get"){
		task_add(3);
		//task_add(4);
		task_add(1);
        inputString = "";
        return;
    }
    else if(inputString.length() == 10 && inputString.substring(0,4) == "set "){
		for(i=0;i<6;i++){
			convert = convert << 4;
			tmp = is_hex(inputString[i+4]);
			if(tmp < 100)    convert += tmp;
			else{
				inputString = "";
				return;
			}
		}
		
		for(i=0;i<3;i++){
			tmp = (convert & 0xff0000) >> 16;
			convert = convert << 8;
			fan_set[i] = tmp;
		}
		fan_set[i] = millis();
		task_add(5);
		inputString = "";
		return;
	}
	else{
		inputString = "";
		return;
	}
}

void fan_speed_detect_03(void){
    unsigned short i;
    for(i=0;i<FAN_MAX;i++){
        fan_speed_tmp = 0;
        attachInterrupt(digitalPinToInterrupt(pin_fan_int[i]), speed_counter, FALLING);
        delay(1000);
        detachInterrupt(digitalPinToInterrupt(pin_fan_int[i]));
        fan_speed[i] = fan_speed_tmp * 30;
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
    Serial.print("],'temperature':");
	Serial.print(temperature);
	Serial.print(",'humidity':");
	Serial.print(humidity);
	Serial.print(",'protect':");
	Serial.print(self_protect);
	Serial.println("}");
}

void set_ack_05(void) {
    unsigned short i;
    Serial.print("{'type':'set_ack','timestamp':");
    Serial.print(millis());
    Serial.print(",'set':[");
    for(i=0;i<FAN_MAX+1;i++){
        Serial.print(fan_set[i]);
        if(i<FAN_MAX) Serial.print(',');
    }
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
        if(task[i] == 0)	break;
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

void get_env_04(void){ 
    sensors.requestTemperatures(); // Send the command to get temperatures
	self_protect = false;
    for(int i=0;i<SENSOR_NUM; i++){
		sensor_temp[i] = sensors.getTempC(sensor_addr[i]);
		self_protect |= sensor_temp[i] >= max_temp;
	}
    sensor_temp_stamp = millis();

	humidity = dht.readHumidity();
	temperature = dht.readTemperature();
}

// convert a digit from character to hex
// return 100 if illigal
unsigned short is_hex(char c){
    if(c >= '0' && c <= '9')		return c - '0';
    else if(c >= 'a' && c <= 'f')	return c - 'a' + 10;
    else if(c >= 'A' && c <= 'F')	return c - 'A' + 10;
    else    return 100;
}

