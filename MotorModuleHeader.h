#pragma once

#define FORWARD 0
#define REVERSE 1
#define EXECUTE 2
#define WORKING 3
#define COMPLETE 4
#define PREPARE 5
#define STOP 6

struct motorStatusStruct {

	int commandStatus;
	int revolutionscompleted;
	int currentSteeringAngle;
};

struct motorStruct {
	int setSystemState;
	int motorCommand;
	int motorSpeed;
	int motorRevolutions;
	int steeringAngle;
};

//
//union commandData {
//	motorStruct instructions;
//	char commandBuffer[sizeof(motorStruct)];
//};
//union commandStatus {
//	motorStatusStruct status;
//	char statusBuffer[sizeof(motorStatusStruct)];
//};
//
//struct motorData {
//	int slaveAddress;
//	commandData motorCommand;
//};

