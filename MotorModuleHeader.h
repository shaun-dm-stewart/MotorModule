#pragma once

#define FORWARD 0
#define REVERSE 1
#define EXECUTE 2
#define WORKING 3
#define COMPLETE 4
#define PREPARE 5
#define STOP 6

struct motorStatusStruct {
	int motorNumber;
	int commandStatus;
	int revolutionscompleted;
	int currentSteeringAngle;
};

struct motorStatusPacket {
	int slaveAddress;
	motorStatusStruct motorStatus;
};

struct motorStruct {
	int setSystemState;
	int motorCommand;
	int motorSpeed;
	volatile int motorRevolutions;
	int steeringAngle;
};

union commandData {
	motorStruct instructions;
	char commandBuffer[sizeof(motorStruct)];
};

union commandStatus {
	motorStatusStruct status;
	char statusBuffer[sizeof(motorStatusStruct)];
};

struct motorData {
	int slaveAddress;
	commandData motorCommand;
};

