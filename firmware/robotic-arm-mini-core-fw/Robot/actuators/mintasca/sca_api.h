/**
  ******************************************************************************
  * @File    : SCA_API.h
  * @Author  : INNFOS Software Team
  * @Version : V1.5.3
  * @Date    : 2019.09.10
  * @Summary : SCA Control Interface Layer
  ******************************************************************************/

#ifndef __SCA_API_H
#define __SCA_API_H


#ifdef __cplusplus
extern "C" {
#endif

#include "sca_protocol.h"
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include "time_utils.h"

/* Parameter Configuration */
#define SCA_NUM_USE        2            // Number of SCAs currently in use, 1-255
#define SCA_DEBUGER        0            // Enable debug interface
#define CanOvertime        0xFFFF       // Data blocking timeout (180MHZ)
#define CanPowertime    0xFFFFFF    // Power on/off blocking timeout (180MHZ)
#define SendInterval    200            // Command send interval when non-blocking
#define SCA_Delay(x)    delayMicroseconds(x)    // Delay interface, continuous sending requires delay when non-blocking


#if SCA_DEBUGER
#define SCA_Debug printf
#else
#define SCA_Debug(s,...)
#endif

/* !!! DO NOT MODIFY the macro definitions below !!! */

// SCA Status Definitions
#define Actr_Enable        0x01
#define Actr_Disable    0x00

// Communication Mode Definitions
#define Block            0x01
#define Unblock            0x00

// SCA Operation Mode Definitions
#define SCA_Current_Mode            0x01
#define SCA_Velocity_Mode            0x02
#define SCA_Position_Mode            0x03
#define SCA_Profile_Position_Mode    0X06
#define SCA_Profile_Velocity_Mode    0X07
#define SCA_Homing_Mode                0X08

/* 
FAST type function instructions:
	When calling an API by ID, it first searches for the information handle corresponding to the ID internally. This operation is intuitive but when there are many SCAs in use,
	the execution efficiency of the function is low. If a pointer is defined directly to the corresponding structure, it can skip
	the process of looking up the handle, but care must be taken to prevent accidental modification of internal data of the handle by this pointer during use.
	Such pointers are used for Fast APIs. When there are many SCAs in use or high-frequency read/write operations, this can
	improve the execution efficiency of functions. Other types of functions can also be modified this way if needed.
	
	Example:

		// For instance, if the actuator ID is 0x03, for fast writing of position to this ID
		SCA_Handler_t* pSCA_ID3 = NULL;
		pSCA_ID3 = getInstance(0x03);
		if(pSCA_ID3 == NULL)	return;// Could not find information handle for this ID

		// Use the defined pointer directly in the Fast type position write function
		setPositionFast(pSCA_ID3,100);
	
Instructions for blocking execution of functions:
	Functions with the isBlock parameter support blocking or non-blocking execution modes, which can be selected according to actual
	use cases. The blocking wait time can be changed in the parameter configuration based on the CPU speed. Note that functions like
	power-on must use blocking communication (waiting for the execution result to return), otherwise it may cause data corruption.
	In addition, continuous use of non-blocking functions can easily cause bus overload, which leads to SCA blue light phenomenon. There is a protective delay 
	internally in the function during non-blocking execution; the delay time can be modified through parameter configuration.
*/

/*************** Control Related ******************/
void lookupActuators(CAN_Handler_t *canPort);
void setupActuators(uint8_t id, CAN_Handler_t *can);
void resetController(uint8_t id);
void enableAllActuators(void);
void disableAllActuators(void);
void regainAttrbute(uint8_t id, uint8_t isBlock);
uint8_t isOnline(uint8_t id, uint8_t isBlock);
uint8_t isEnable(uint8_t id, uint8_t isBlock);
uint8_t isUpdate(uint8_t id);
uint8_t enableActuator(uint8_t id);
uint8_t disableActuator(uint8_t id);
uint8_t activateActuatorMode(uint8_t id, uint8_t ActuatorMode, uint8_t isBlock);
uint8_t getActuatorMode(uint8_t id, uint8_t isBlock);
uint8_t getErrorCode(uint8_t id, uint8_t isBlock);
uint8_t clearError(uint8_t id, uint8_t isBlock);
uint8_t saveAllParams(uint8_t id, uint8_t isBlock);
SCA_Handler_t *getInstance(uint8_t id);

/*************** Position Related ******************/
uint8_t setPosition(uint8_t id, float pos);
uint8_t setPositionFast(SCA_Handler_t *pSCA, float pos);
uint8_t getPosition(uint8_t id, uint8_t isBlock);
uint8_t getPositionFast(SCA_Handler_t *pSCA, uint8_t isBlock);
uint8_t setPositionKp(uint8_t id, float Kp, uint8_t isBlock);
uint8_t getPositionKp(uint8_t id, uint8_t isBlock);
uint8_t setPositionKi(uint8_t id, float Ki, uint8_t isBlock);
uint8_t getPositionKi(uint8_t id, uint8_t isBlock);
uint8_t setPositionUmax(uint8_t id, float max, uint8_t isBlock);
uint8_t getPositionUmax(uint8_t id, uint8_t isBlock);
uint8_t setPositionUmin(uint8_t id, float min, uint8_t isBlock);
uint8_t getPositionUmin(uint8_t id, uint8_t isBlock);
uint8_t setPositionOffset(uint8_t id, float offset, uint8_t isBlock);
uint8_t getPositionOffset(uint8_t id, uint8_t isBlock);
uint8_t setMaximumPosition(uint8_t id, float maxPos, uint8_t isBlock);
uint8_t getMaximumPosition(uint8_t id, uint8_t isBlock);
uint8_t setMinimumPosition(uint8_t id, float minPos, uint8_t isBlock);
uint8_t getMinimumPosition(uint8_t id, uint8_t isBlock);
uint8_t enablePositionLimit(uint8_t id, uint8_t enable, uint8_t isBlock);
uint8_t isPositionLimitEnable(uint8_t id, uint8_t isBlock);
uint8_t setHomingPosition(uint8_t id, float homingPos, uint8_t isBlock);
uint8_t enablePositionFilter(uint8_t id, uint8_t enable, uint8_t isBlock);
uint8_t isPositionFilterEnable(uint8_t id, uint8_t isBlock);
uint8_t setPositionCutoffFrequency(uint8_t id, float frequency, uint8_t isBlock);
uint8_t getPositionCutoffFrequency(uint8_t id, uint8_t isBlock);
uint8_t clearHomingInfo(uint8_t id, uint8_t isBlock);
uint8_t setProfilePositionAcceleration(uint8_t id, float acceleration, uint8_t isBlock);
uint8_t getProfilePositionAcceleration(uint8_t id, uint8_t isBlock);
uint8_t setProfilePositionDeceleration(uint8_t id, float deceleration, uint8_t isBlock);
uint8_t getProfilePositionDeceleration(uint8_t id, uint8_t isBlock);
uint8_t setProfilePositionMaxVelocity(uint8_t id, float maxVelocity, uint8_t isBlock);
uint8_t getProfilePositionMaxVelocity(uint8_t id, uint8_t isBlock);

/*************** Velocity Related ******************/
uint8_t setVelocity(uint8_t id, float vel);
uint8_t setVelocityFast(SCA_Handler_t *pSCA, float vel);
uint8_t getVelocity(uint8_t id, uint8_t isBlock);
uint8_t getVelocityFast(SCA_Handler_t *pSCA, uint8_t isBlock);
uint8_t getVelocityKp(uint8_t id, uint8_t isBlock);
uint8_t setVelocityKp(uint8_t id, float Kp, uint8_t isBlock);
uint8_t getVelocityKi(uint8_t id, uint8_t isBlock);
uint8_t setVelocityKi(uint8_t id, float Ki, uint8_t isBlock);
uint8_t getVelocityUmax(uint8_t id, uint8_t isBlock);
uint8_t setVelocityUmax(uint8_t id, float max, uint8_t isBlock);
uint8_t getVelocityUmin(uint8_t id, uint8_t isBlock);
uint8_t setVelocityUmin(uint8_t id, float min, uint8_t isBlock);
uint8_t enableVelocityFilter(uint8_t id, uint8_t enable, uint8_t isBlock);
uint8_t isVelocityFilterEnable(uint8_t id, uint8_t isBlock);
uint8_t getVelocityCutoffFrequency(uint8_t id, uint8_t isBlock);
uint8_t setVelocityCutoffFrequency(uint8_t id, float frequency, uint8_t isBlock);
uint8_t setVelocityLimit(uint8_t id, float limit, uint8_t isBlock);
uint8_t getVelocityLimit(uint8_t id, uint8_t isBlock);
uint8_t setProfileVelocityAcceleration(uint8_t id, float acceleration, uint8_t isBlock);
uint8_t getProfileVelocityAcceleration(uint8_t id, uint8_t isBlock);
uint8_t setProfileVelocityDeceleration(uint8_t id, float deceleration, uint8_t isBlock);
uint8_t getProfileVelocityDeceleration(uint8_t id, uint8_t isBlock);
uint8_t setProfileVelocityMaxVelocity(uint8_t id, float maxVelocity, uint8_t isBlock);
uint8_t getProfileVelocityMaxVelocity(uint8_t id, uint8_t isBlock);
float getVelocityRange(uint8_t id);

/*************** Current Related ******************/
uint8_t setCurrent(uint8_t id, float current);
uint8_t setCurrentFast(SCA_Handler_t *pSCA, float current);
uint8_t getCurrent(uint8_t id, uint8_t isBlock);
uint8_t getCurrentFast(SCA_Handler_t *pSCA, uint8_t isBlock);
uint8_t getCurrentKp(uint8_t id, uint8_t isBlock);
uint8_t getCurrentKi(uint8_t id, uint8_t isBlock);
uint8_t getCurrentRange(uint8_t id, uint8_t isBlock);
uint8_t enableCurrentFilter(uint8_t id, uint8_t enable, uint8_t isBlock);
uint8_t isCurrentFilterEnable(uint8_t id, uint8_t isBlock);
uint8_t getCurrentCutoffFrequency(uint8_t id, uint8_t isBlock);
uint8_t setCurrentCutoffFrequency(uint8_t id, float frequency, uint8_t isBlock);
uint8_t setCurrentLimit(uint8_t id, float limit, uint8_t isBlock);
uint8_t getCurrentLimit(uint8_t id, uint8_t isBlock);

/*************** Other Parameters ******************/
uint8_t getVoltage(uint8_t id, uint8_t isBlock);
uint8_t getLockEnergy(uint8_t id, uint8_t isBlock);
uint8_t setLockEnergy(uint8_t id, float energy, uint8_t isBlock);
uint8_t getActuatorSerialNumber(uint8_t id, uint8_t isBlock);
uint8_t getMotorTemperature(uint8_t id, uint8_t isBlock);
uint8_t getInverterTemperature(uint8_t id, uint8_t isBlock);
uint8_t getMotorProtectedTemperature(uint8_t id, uint8_t isBlock);
uint8_t setMotorProtectedTemperature(uint8_t id, float temp, uint8_t isBlock);
uint8_t getMotorRecoveryTemperature(uint8_t id, uint8_t isBlock);
uint8_t setMotorRecoveryTemperature(uint8_t id, float temp, uint8_t isBlock);
uint8_t getInverterProtectedTemperature(uint8_t id, uint8_t isBlock);
uint8_t setInverterProtectedTemperature(uint8_t id, float temp, uint8_t isBlock);
uint8_t getInverterRecoveryTemperature(uint8_t id, uint8_t isBlock);
uint8_t setInverterRecoveryTemperature(uint8_t id, float temp, uint8_t isBlock);
uint8_t setActuatorID(uint8_t currentID, uint8_t newID, uint8_t isBlock);
uint8_t getActuatorLastState(uint8_t id, uint8_t isBlock);
uint8_t requestCVPValue(uint8_t id, uint8_t isBlock);
uint8_t requestCVPValueFast(SCA_Handler_t *pSCA, uint8_t isBlock);


#ifdef __cplusplus
}

#endif
#endif
