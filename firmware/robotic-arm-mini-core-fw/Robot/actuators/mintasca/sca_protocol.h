/**
  ******************************************************************************
  * @File    : SCA_Protocol.h
  * @Author  : INNFOS Software Team
  * @Version : V1.5.2
  * @Date    : 2019.08.20
  * @Summary : INNFOS CAN Communication Protocol Layer
  ******************************************************************************/

#ifndef __SCA_PROTOCOL_H
#define __SCA_PROTOCOL_H


#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* !!! DO NOT MODIFY the macro definitions below !!! */

// INNFOS CAN Communication Protocol Commands
// First category of read commands
#define R1_Heartbeat            0x00
#define R1_Mode                    0x55
#define R1_LastState            0xB0
#define R1_CurrentFilterState    0X71
#define R1_VelocityFilterState    0x75
#define R1_PositionFilterState    0x79
#define R1_PositionLimitState    0x8B
#define R1_PowerState            0x2B

// Second category of read commands
#define R2_Voltage                0x45
#define R2_Current_Max            0x53
#define R2_CurrentFilterValue    0x73
#define R2_VelocityFilterValue    0x77
#define R2_PositionFilterValue    0x7B
#define R2_MotorTemp            0x5F
#define R2_InverterTemp            0x60
#define R2_InverterProtectTemp    0x62
#define R2_InverterRecoverTemp    0x64
#define R2_MotorProtectTemp        0x6C
#define R2_MotorRecoverTemp        0x6E
#define R2_Error                0xFF

// Third category of read commands
#define R3_Current                0x04
#define R3_Velocity                0x05
#define R3_Position                0x06
#define R3_CurrentFilterP        0x15
#define R3_CurrentFilterI        0x16
#define R3_VelocityFilterP        0x17
#define R3_VelocityFilterI        0x18
#define R3_PositionFilterP        0x19
#define R3_PositionFilterI        0x1A
#define R3_PositionFilterD        0X1B
#define R3_PPMaxVelocity        0x1C
#define R3_PPMaxAcceleration    0x1D
#define R3_PPMaxDeceleration    0x1E
#define R3_PVMaxVelocity        0x22
#define R3_PVMaxAcceleration    0x23
#define R3_PVMaxDeceleration    0x24
#define R3_CurrentFilterLimitL    0x34
#define R3_CurrentFilterLimitH    0x35
#define R3_VelocityFilterLimitL    0x36
#define R3_VelocityFilterLimitH    0x37
#define R3_PositionFilterLimitL    0x38
#define R3_PositionFilterLimitH    0x39
#define R3_CurrentLimit            0x59
#define R3_VelocityLimit        0x5B
#define R3_Inertia                0x7D
#define R3_PositionLimitH        0x85
#define R3_PositionLimitL        0x86
#define R3_PositionOffset        0x8A
#define R3_HomingCurrentLimitL    0x92
#define R3_HomingCurrentLimitH    0x93
#define R3_BlockEngy            0x7F

// Fourth category of read commands
#define R4_CVP                    0x94

// Fifth category of read commands
#define R5_ShakeHands            0x02

// First category of write commands
#define W1_Mode                    0x07
#define W1_CurrentFilterState    0X70
#define W1_VelocityFilterState    0x74
#define W1_PositionFilterState    0x78
#define W1_PositionLimitState    0x8C
#define W1_PowerState            0x2A

// Second category of write commands
#define W2_CurrentFilterValue    0x72
#define W2_VelocityFilterValue    0x76
#define W2_PositionFilterValue    0x7A
#define W2_InverterProtectTemp    0x61
#define W2_InverterRecoverTemp    0x63
#define W2_MotorProtectTemp        0x6B
#define W2_MotorRecoverTemp        0x6D

// Third category of write commands
#define W3_Current                0x08
#define W3_Velocity                0x09
#define W3_Position                0x0A
#define W3_CurrentFilterP        0x0E
#define W3_CurrentFilterI        0x0F
#define W3_VelocityFilterP        0x10
#define W3_VelocityFilterI        0x11
#define W3_PositionFilterP        0x12
#define W3_PositionFilterI        0x13
#define W3_PositionFilterD        0X14
#define W3_PPMaxVelocity        0x1F
#define W3_PPMaxAcceleration    0x20
#define W3_PPMaxDeceleration    0x21
#define W3_PVMaxVelocity        0x25
#define W3_PVMaxAcceleration    0x26
#define W3_PVMaxDeceleration    0x27
#define W3_CurrentFilterLimitL    0x2E
#define W3_CurrentFilterLimitH    0x2F
#define W3_VelocityFilterLimitL    0x30
#define W3_VelocityFilterLimitH    0x31
#define W3_PositionFilterLimitL    0x32
#define W3_PositionFilterLimitH    0x33
#define W3_CurrentLimit            0x58
#define W3_VelocityLimit        0x5A
#define W3_PositionLimitH        0x83
#define W3_PositionLimitL        0x84
#define W3_HomingValue            0x87
#define W3_PositionOffset        0x89
#define W3_HomingCurrentLimitL    0x90
#define W3_HomingCurrentLimitH    0x91
#define W3_BlockEngy            0x7E

// Fourth category of write commands
#define W4_ClearError            0xFE
#define W4_ClearHome            0x88
#define W4_Save                    0x0D

// Fifth category of write commands
#define W5_ChangeID                0x3D

// Variable scaling factor definitions
#define Velocity_Max    6000.0f            // Maximum velocity, fixed at 6000RPM (for conversion only)
#define BlkEngy_Scal    75.225f            // Blocked energy scaling factor
#define Profile_Scal    960.0f            // Profile parameters scaling factor
#define IQ8                256.0f            // 2^8
#define IQ10            1024.0f            // 2^10
#define IQ24            16777216.0f        // 2^24
#define IQ30            1073741824.0f    // 2^30

/* ID is CAN transmit frame ID, msg is the data (address) to send
   len is the length of data to send, returns 0 on success, other values on failure */
typedef uint8_t (*Send_t)(uint8_t ID, uint8_t *msg, uint8_t len);

typedef struct                // CAN Port Handle
{
    // SCA Status Information
    uint8_t CanPort;        // CAN port number in use
    uint8_t Retry;            // Number of retries on send failure
    Send_t Send;            // Send function, format see Send_t
} CAN_Handler_t;

typedef struct                        // SCA Warning Information
{
    uint16_t Error_Code;            // Error code

    /* Specific warning info, 0: Normal, 1: Error */
    uint8_t WARN_OVER_VOLT;        // Overvoltage warning
    uint8_t WARN_UNDER_VOLT;        // Undervoltage warning
    uint8_t WARN_LOCK_ROTOR;        // Locked rotor warning
    uint8_t WARN_OVER_TEMP;        // Overtemperature warning
    uint8_t WARN_RW_PARA;            // Parameter read/write error
    uint8_t WARN_MUL_CIRCLE;        // Multi-turn counting error
    uint8_t WARN_TEMP_SENSOR_INV;    // Inverter temp sensor error
    uint8_t WARN_CAN_BUS;            // CAN communication error
    uint8_t WARN_TEMP_SENSOR_MTR;    // Motor temp sensor error
    uint8_t WARN_OVER_STEP;            // Position mode step greater than 1
    uint8_t WARN_DRV_PROTEC;        // DRV protection
    uint8_t WARN_DVICE;            // Device error

} SCA_Warn_t;

/* 
	SCA parameter cache, used to save target parameters when writing, and written into the handle after success
	Read flags are used during blocking execution, variable content can be tailored or added based on project needs
 */
typedef struct
{
    /* Basic information */
    uint8_t ID;                        // SCA ID number

    /* First category data variables */
    uint8_t Mode;                    // Current operation mode
    uint8_t Current_Filter_State;    // Current loop filter state
    uint8_t Velocity_Filter_State;    // Velocity loop filter state
    uint8_t Position_Filter_State;    // Position loop filter state
    uint8_t Position_Limit_State;    // Position limit state
    uint8_t Power_State;            // Power on/off state
    /* Read flags */
    uint8_t R_Mode;                    // Read data return flag, 1 means data returned
    uint8_t R_Last_State;
    uint8_t R_Current_Filter_State;
    uint8_t R_Velocity_Filter_State;
    uint8_t R_Position_Filter_State;
    uint8_t R_Position_Limit_State;
    uint8_t R_Power_State;

    /* Second category data variables */
    float Current_Filter_Value;        // Current loop filter bandwidth
    float Velocity_Filter_Value;    // Velocity loop filter bandwidth
    float Position_Filter_Value;    // Position loop filter bandwidth
    float Inverter_Protect_Temp;    // Inverter protection temperature
    float Inverter_Recover_Temp;    // Inverter recovery temperature
    float Motor_Protect_Temp;        // Motor protection temperature
    float Motor_Recover_Temp;        // Motor recovery temperature
    /* Read flags */
    uint8_t R_Current_Filter_Value;
    uint8_t R_Velocity_Filter_Value;
    uint8_t R_Position_Filter_Value;
    uint8_t R_Inverter_Protect_Temp;
    uint8_t R_Inverter_Recover_Temp;
    uint8_t R_Motor_Protect_Temp;
    uint8_t R_Motor_Recover_Temp;
    uint8_t R_Voltage;
    uint8_t R_Current_Max;
    uint8_t R_Motor_Temp;
    uint8_t R_Inverter_Temp;
    uint8_t R_Error_Code;

    /* Third category data variables */
    float Current_Real;                // Current current (Unit: A)
    float Velocity_Real;            // Current velocity (Unit: RPM)
    float Position_Real;            // Current position, real value (Unit: R)
    float Current_Filter_P;            // Current loop P value
    float Current_Filter_I;            // Current loop I value
    float Velocity_Filter_P;        // Velocity loop P value
    float Velocity_Filter_I;        // Velocity loop I value
    float Position_Filter_P;        // Position loop P value
    float Position_Filter_I;        // Position loop I value
    //float Position_Filter_D;		// Position loop D value
    float PP_Max_Velocity;            // Profile position max velocity
    float PP_Max_Acceleration;        // Profile position max acceleration
    float PP_Max_Deceleration;        // Profile position max deceleration
    float PV_Max_Velocity;            // Profile velocity max velocity
    float PV_Max_Acceleration;        // Profile velocity max acceleration
    float PV_Max_Deceleration;        // Profile velocity max deceleration
    //float Current_Filter_Limit_L;	// Current loop output lower limit
    //float Current_Filter_Limit_H;	// Current loop output upper limit
    float Velocity_Filter_Limit_L;    // Velocity loop output lower limit
    float Velocity_Filter_Limit_H;    // Velocity loop output upper limit
    float Position_Filter_Limit_L;    // Position loop output lower limit
    float Position_Filter_Limit_H;    // Position loop output upper limit
    float Position_Limit_H;            // Actuator position upper limit
    float Position_Limit_L;            // Actuator position lower limit
    float Current_Limit;            // Current input limit
    float Velocity_Limit;            // Velocity input limit
    float Homing_Value;                // Actuator homing value
    float Position_Offset;            // Actuator position offset
    float Homing_Current_Limit_L;    // Auto-homing current lower limit
    float Homing_Current_Limit_H;    // Auto-homing current upper limit
    float Blocked_Energy;            // Blocked rotor locking energy
    /* Read flags */
    uint8_t R_Current_Real;
    uint8_t R_Velocity_Real;
    uint8_t R_Position_Real;
    uint8_t R_Current_Filter_P;
    uint8_t R_Current_Filter_I;
    uint8_t R_Velocity_Filter_P;
    uint8_t R_Velocity_Filter_I;
    uint8_t R_Position_Filter_P;
    uint8_t R_Position_Filter_I;
    //uint8_t R_Position_Filter_D;
    uint8_t R_PP_Max_Velocity;
    uint8_t R_PP_Max_Acceleration;
    uint8_t R_PP_Max_Deceleration;
    uint8_t R_PV_Max_Velocity;
    uint8_t R_PV_Max_Acceleration;
    uint8_t R_PV_Max_Deceleration;
    //uint8_t R_Current_Filter_Limit_L;
    //uint8_t R_Current_Filter_Limit_H;
    uint8_t R_Velocity_Filter_Limit_L;
    uint8_t R_Velocity_Filter_Limit_H;
    uint8_t R_Position_Filter_Limit_L;
    uint8_t R_Position_Filter_Limit_H;
    uint8_t R_Position_Limit_H;
    uint8_t R_Position_Limit_L;
    uint8_t R_Current_Limit;
    uint8_t R_Velocity_Limit;
    uint8_t R_Homing_Value;
    uint8_t R_Position_Offset;
    uint8_t R_Homing_Current_Limit_L;
    uint8_t R_Homing_Current_Limit_H;
    uint8_t R_Blocked_Energy;
    uint8_t R_CVP;
    uint8_t R_Serial_Num;
    uint8_t W_ClearHome;

} Para_Cache_t;

/* 
	SCA information handle, please do not arbitrarily change the values inside,
	Variable content can be tailored or added based on project needs
 */
typedef struct
{
    /* Protocol data variable area */
    uint8_t ID;                        // SCA ID number
    uint8_t Serial_Num[4];            // Serial number
    uint8_t Save_State;                // Parameter save state, 1 means saved
    uint8_t Online_State;            // Current online state, 1 means online
    uint8_t Update_State;            // Whether there is parameter refresh, 1 means refreshed
    CAN_Handler_t *Can;                // CAN port used
    Para_Cache_t paraCache;            // Parameter cache

    /* User data variable area */

    /* First category data variables */
    uint8_t Mode;                    // Current operation mode
    uint8_t Last_State;                // Previous shutdown error state, 1 means normal
    uint8_t Current_Filter_State;    // Current loop filter state
    uint8_t Velocity_Filter_State;    // Velocity loop filter state
    uint8_t Position_Filter_State;    // Position loop filter state
    uint8_t Position_Limit_State;    // Position limit state
    uint8_t Power_State;            // Power on/off state

    /* Second category data variables */
    float Voltage;                    // Current voltage (Unit: V)
    float Current_Max;                // Max current range
    float Current_Filter_Value;        // Current loop filter bandwidth
    float Velocity_Filter_Value;    // Velocity loop filter bandwidth
    float Position_Filter_Value;    // Position loop filter bandwidth
    float Motor_Temp;                // Motor temperature
    float Inverter_Temp;            // Inverter temperature
    float Inverter_Protect_Temp;    // Inverter protection temperature
    float Inverter_Recover_Temp;    // Inverter recovery temperature
    float Motor_Protect_Temp;        // Motor protection temperature
    float Motor_Recover_Temp;        // Motor recovery temperature
    SCA_Warn_t SCA_Warn;            // Motor warning info

    /* Third category data variables */
    float Current_Real;                // Current current (Unit: A)
    float Velocity_Real;            // Current velocity (Unit: RPM)
    float Position_Real;            // Current position, real value (Unit: R)
    float Current_Filter_P;            // Current loop P value
    float Current_Filter_I;            // Current loop I value
    float Velocity_Filter_P;        // Velocity loop P value
    float Velocity_Filter_I;        // Velocity loop I value
    float Position_Filter_P;        // Position loop P value
    float Position_Filter_I;        // Position loop I value
    //float Position_Filter_D;		// Position loop D value
    float PP_Max_Velocity;            // Profile position max velocity
    float PP_Max_Acceleration;        // Profile position max acceleration
    float PP_Max_Deceleration;        // Profile position max deceleration
    float PV_Max_Velocity;            // Profile velocity max velocity
    float PV_Max_Acceleration;        // Profile velocity max acceleration
    float PV_Max_Deceleration;        // Profile velocity max deceleration
    //float Current_Filter_Limit_L;	// Current loop output lower limit
    //float Current_Filter_Limit_H;	// Current loop output upper limit
    float Velocity_Filter_Limit_L;    // Velocity loop output lower limit
    float Velocity_Filter_Limit_H;    // Velocity loop output upper limit
    float Position_Filter_Limit_L;    // Position loop output lower limit
    float Position_Filter_Limit_H;    // Position loop output upper limit
    float Position_Limit_H;            // Actuator position upper limit
    float Position_Limit_L;            // Actuator position lower limit
    float Current_Limit;            // Current input limit
    float Velocity_Limit;            // Velocity input limit
    float Homing_Value;                // Actuator homing value
    float Position_Offset;            // Actuator position offset
    float Homing_Current_Limit_L;    // Auto-homing current lower limit
    float Homing_Current_Limit_H;    // Auto-homing current upper limit
    float Blocked_Energy;            // Blocked rotor locking energy

} SCA_Handler_t;

enum SCA_Error                // SCA communication error type enum
{
    SCA_NoError = 0,        // No error
    SCA_OverTime,            // Communication wait timeout
    SCA_SendError,            // Data send failure
    SCA_OperationFailed,    // Operation failed
    SCA_UnknownID,            // Target ID actuator handle not found
};

/* Data receive interface, called when new CAN data packet arrives
  CanRxMsg is the CAN data packet receive type structure, please 
  define CanRxMsg structure type according to the platform when porting. 
  Here it defaults to the receive structure in STM32 standard library functions */
typedef struct
{
    uint32_t StdId;  /*!< Specifies the standard identifier.
                        This parameter can be a value between 0 to 0x7FF. */

    uint32_t ExtId;  /*!< Specifies the extended identifier.
                        This parameter can be a value between 0 to 0x1FFFFFFF. */

    uint8_t IDE;     /*!< Specifies the type of identifier for the message that
                        will be received. This parameter can be a value of
                        @ref CAN_identifier_type */

    uint8_t RTR;     /*!< Specifies the type of frame for the received message.
                        This parameter can be a value of
                        @ref CAN_remote_transmission_request */

    uint8_t DLC;     /*!< Specifies the length of the frame that will be received.
                        This parameter can be a value between 0 to 8 */

    uint8_t Data[8]; /*!< Contains the data to be received. It ranges from 0 to
                        0xFF. */

    uint8_t FMI;     /*!< Specifies the index of the filter the message stored in
                        the mailbox passes through. This parameter can be a
                        value between 0 to 0xFF */
} CanRxMsg;

void canDispatch(CanRxMsg *RxMsg);

/* Functions below are for API layer call */

/* Read command interface */
uint8_t SCA_Read(SCA_Handler_t *pSCA, uint8_t cmd);

/* Five categories of write commands */
uint8_t SCA_Write_1(SCA_Handler_t *pSCA, uint8_t cmd, uint8_t TxData);
uint8_t SCA_Write_2(SCA_Handler_t *pSCA, uint8_t cmd, float TxData);
uint8_t SCA_Write_3(SCA_Handler_t *pSCA, uint8_t cmd, float TxData);
uint8_t SCA_Write_4(SCA_Handler_t *pSCA, uint8_t cmd);
uint8_t SCA_Write_5(SCA_Handler_t *pSCA, uint8_t cmd, uint8_t TxData);


#ifdef __cplusplus
}

#endif
#endif
