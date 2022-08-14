/*
 Name:		MotorModule.ino
 Created:	8/13/2022 11:11:29 AM
 Author:	Shaun
*/
#include <EEPROM.h>
#include <Servo.h>
#include <uNet485.h>
#include <uNet.h>
#include <Frame.h>
#include <Definitions.h>
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
#include "MotorModuleHeader.h"

#define MOTOR_SPEED_PIN 5		// This is a PWM output for speed control
#define STEERING_SERVO_PIN 6	// Steering angle control
#define FORWARD_PIN 7
#define REVERSE_PIN 8
#define PULSES_PER_REVOLUTION 75
#define INTERRUPT_PIN 2			// This is the pin to which an external interrupt is attached
#define INTERRUPT 0				// There are two external interrupts on a Nano, 0 and 1.  We are using the first interrupt and identify it here
#define XMIT_ENABLE 4

int _moduleAddress;
int _steeringAngle;
int _revolutions;
int _commandOrigin = 0;
volatile int _channelACounter;
volatile int _commandStatus;
char _inBuffer[150];
char _cmdBuffer[150];
bool _commandAvailable = false;
bool _navigationLoaded = false;
bool _executeCommand = false;
unsigned char* _dataBuffer;
motorStruct _motorCommands;
motorStatusStruct _motorStatus;
uNetSocket _socket;
Servo _steeringServo;

//#define INTEGRATION

#ifdef INTEGRATION
void transmitDebug(char* output)
{
	Serial1.println(output);
}
#endif

void dataReceived(unsigned char* data)
{
	_dataBuffer = data;
	_commandAvailable = true;
#ifdef INTEGRATION
	Serial1.print("uNet.ino received: ");
	Serial1.println((char*)data);
#endif
}

void dataSent()
{
}

void transmitResponse(char* output)
{
	_socket.writePacket(_commandOrigin, (unsigned char*)output);
#ifdef INTEGRATION
	transmitDebug(output);
#endif
}

void channelATriggered() {
	_channelACounter++;
}

void forwardSelected() {
	analogWrite(MOTOR_SPEED_PIN, _motorCommands.motorSpeed);
	_steeringServo.write(_steeringAngle);
	digitalWrite(REVERSE_PIN, LOW);
	digitalWrite(FORWARD_PIN, HIGH);
#ifdef INTEGRATION
	transmitDebug("Forward selected");
#endif
}

void reverseSelected() {
	analogWrite(MOTOR_SPEED_PIN, _motorCommands.motorSpeed);
	_steeringServo.write(_steeringAngle);
	digitalWrite(FORWARD_PIN, LOW);
	digitalWrite(REVERSE_PIN, HIGH);
#ifdef INTEGRATION
	transmitDebug("Reverse selected");
#endif
}

void commandBuffer()
{
	char _outBuffer[150];
	sprintf(_outBuffer, "{\"origin\":%d,\"type\":\"cbuf\",\"packet\":{\"speed\":%d,\"cmd\":%d,\"steering\":%d,\"revs\":%d}}"
		, _moduleAddress
		, _motorCommands.motorSpeed
		, _motorCommands.motorCommand
		, _motorCommands.steeringAngle
		, _motorCommands.motorRevolutions);
	transmitResponse(_outBuffer);
	_commandAvailable = false;
}

void identification()
{
#ifdef INTEGRATION
	transmitDebug("Identification selected");
#endif
	char id[40];
	sprintf(id, "{\"origin\":%d,\"id\":\"motor\"}", _moduleAddress);
	transmitResponse(id);
	_commandAvailable = false;
}

void stopSelected() {
	digitalWrite(FORWARD_PIN, LOW);
	digitalWrite(REVERSE_PIN, LOW);
#ifdef INTEGRATION
	transmitDebug("Stop selected");
#endif
}

void statusRequest() {
	char _outBuffer[150];
	sprintf(_outBuffer, "{\"origin\":%d,\"type\":\"motor data\",\"packet\":{\"speed\":%d,\"cmd\":%d,\"steering\":%d,\"revs\":%d}}"
		, _moduleAddress
		, _motorCommands.motorSpeed
		, _motorCommands.motorCommand
		, _steeringAngle
		, _revolutions);
	transmitResponse(_outBuffer);
	_commandAvailable = false;
}

int decodeCommand()
{
	int result = 0;
	StaticJsonDocument<200> jsonBuffer;
	DeserializationError err;
	strcpy(_inBuffer, (const char*)_dataBuffer);
	const char* x = _inBuffer;  // I have to do this to make ArduinoJson work.  It only seems to like const char*
	err = deserializeJson(jsonBuffer, x);

	if (err)
	{
		DynamicJsonDocument errPacket(50);
		int bufferSize = sizeof(errPacket);
		char errData[sizeof(errPacket)];
		serializeJson(errPacket, errData, bufferSize);
		errPacket["type"] = "error";
		errPacket["packet"] = err.c_str();
#ifdef INTEGRATION
		transmitDebug("Error in decode");
#endif
	}
	else
	{
		// Get the root object in the document
		JsonObject root = jsonBuffer.as<JsonObject>();
		_commandOrigin = root["origin"];
		//It's for this device
		const char* messageType = root["type"];
		if (strcmp(messageType, "nav") == 0)
		{
#ifdef INTEGRATION
			transmitDebug("Navigate selected");
#endif
			// The device has just received instructions and is passing them to the run memory
			result = 1;
			_motorCommands.setSystemState = root["state"];
			_motorCommands.motorCommand = root["command"];
			_motorCommands.motorRevolutions = root["revs"];
			_motorCommands.motorSpeed = root["speed"];
			_motorCommands.steeringAngle = root["steering"];
		}
		else if (strcmp(messageType, "id") == 0)
		{
			result = 2;
		}
		else if (strcmp(messageType, "stat") == 0) {
			result = 3;
		}
		else if (strcmp(messageType, "exec") == 0) {
			result = 4;
		}
		else if (strcmp(messageType, "cbuf") == 0) {
			result = 5;
		}
	}
	return result;
}

void executeCommand()
{
	if (_executeCommand)
	{
		//noInterrupts();
		//moveSteeringHead();
		_motorStatus.revolutionscompleted = _channelACounter / PULSES_PER_REVOLUTION;
		if (_motorStatus.revolutionscompleted >= _motorCommands.motorRevolutions) {
			stopSelected();
			_commandStatus = COMPLETE;
			_navigationLoaded = false;
			_commandAvailable = false;
			_executeCommand = false;
			_revolutions = _motorStatus.revolutionscompleted;
		}
		//interrupts();
	}
	else if (_commandAvailable)
	{
		switch (decodeCommand())
		{
		case 1:
			_commandAvailable = false;
			_navigationLoaded = true;
			break;
		case 2:
			identification();
			break;
		case 3:
			statusRequest();
			break;
		case 4:
			_motorStatus.revolutionscompleted = 0;
			_channelACounter = 0;
			_executeCommand = true;
			switch (_motorCommands.motorCommand)
			{
			case FORWARD:
				_commandStatus == WORKING;
				forwardSelected();
				break;
			case REVERSE:
				_commandStatus == WORKING;
				reverseSelected();
				break;
			case STOP:
				stopSelected();
				break;
#ifdef INTEGRATION
			default:
				transmitDebug("Command misunderstood: ");
#endif
			}
			break;
		case 5:
			commandBuffer();
			break;
		case -1:
		default:
			_commandStatus = COMPLETE;
			_navigationLoaded = false;
			_commandAvailable = false;
#ifdef INTEGRATION
			transmitDebug("Something went wrong! Resetting.");
#endif
			break;
		}
	}
}

// the setup function runs once when you press reset or power the board
void setup() {
	// Get the preprogrammed module address
	_moduleAddress = EEPROM.read(0);
	Serial.begin(9600);
	_socket.begin(&Serial, dataReceived, dataSent, _moduleAddress, 1);
#ifdef INTEGRATION
	Serial1.begin(9600);
#endif
	_motorStatus.motorNumber = _moduleAddress;
	_commandStatus = STOP;
	_motorStatus.currentSteeringAngle = 90;
	_motorStatus.revolutionscompleted = 0;

	_motorCommands.setSystemState = PREPARE;
	_motorCommands.motorCommand = STOP;
	_motorCommands.motorSpeed = 0;
	_motorCommands.motorRevolutions = 0;
	_motorCommands.steeringAngle = 90;

	pinMode(MOTOR_SPEED_PIN, OUTPUT);
	pinMode(FORWARD_PIN, OUTPUT);
	pinMode(REVERSE_PIN, OUTPUT);
	pinMode(INTERRUPT_PIN, INPUT);
	pinMode(XMIT_ENABLE, OUTPUT);

	attachInterrupt(INTERRUPT, channelATriggered, RISING);
	_steeringServo.attach(STEERING_SERVO_PIN);
#ifdef INTEGRATION
	transmitDebug("Motor control initialised");
#endif

}

// the loop function runs over and over again until power down or reset
void loop() {
	_socket.readPacket();
	executeCommand();
}
