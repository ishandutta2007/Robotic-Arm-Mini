/**
 ******************************************************************************
 * @File  : SCA_API.c
 * @Author : INNFOS Software Team
 * @Version : V1.5.3
 * @Date  : 2019.09.10
 * @Summary : SCA Control Interface Layer
 ******************************************************************************/
/* Update log --------------------------------------------------------------------*/
//V1.1.0 2019.08.05 All API call interfaces changed to ID, consistent with PC SDK, added read/write API for all parameters
//V1.5.0 2019.08.16 Changed data receive method (interrupt receive), added non-blocking communication function to adapt to slow data return.
//         Added API to get last shutdown state, optimized startup process.
//V1.5.1 2019.09.10 Added polling function
//V1.5.3 2019.11.15 Optimized power on/off process

/* Includes ----------------------------------------------------------------------*/
#include "sca_api.h"
/* Variable defines --------------------------------------------------------------*/

/* Each SCA needs a handle to save corresponding info, define SCA_NUM_USE based on actual usage */
SCA_Handler_t SCA_Handler_List[SCA_NUM_USE];

/* Funcation declaration ---------------------------------------------------------*/
extern void warnBitAnaly(SCA_Handler_t* pSCA);

/* Funcation defines -------------------------------------------------------------*/

/**************************** Control Related *******************************/

/**
 * @Brief Search for existing SCAs on CAN bus and print found IDs
 * @Param canPort: bus to poll
 * @Return None
 * @Note  Each actuator has its own ID, if unknown during first use,
 *     this function can be used to find it
 */
void lookupActuators(CAN_Handler_t* canPort)
{
  uint16_t ID;
  uint8_t Found = 0;
  SCA_Handler_t temp;

  /* Save original content of the list item */
  temp = SCA_Handler_List[0];

  /* Use one list item for querying */
  SCA_Handler_List[0].Can = canPort;

  for(ID = 1; ID <= 0xFF; ID++)
  {
    /* Load new ID */
    SCA_Handler_List[0].ID = ID;

    /* If heartbeat received, the ID exists */
    if(isOnline(ID,Block) == SCA_NoError)
    {
      /* Record number found, print found ID */
      Found++;
      SCA_Debug("Found ID %d in canPort %d\r\n",ID,canPort->CanPort);
    }
  }
  /* Restore changed content */
  SCA_Handler_List[0] = temp;

  /* Output prompt information */
  SCA_Debug("canPort %d polling done ! Found %d Actuators altogether!\r\n\r\n",canPort->CanPort,Found);
}

/**
 * @Brief Initialize controller, for ID and CAN port information
 * @Param id: ID of actuator to initialize
 *     pCan: used CAN port address
 * @Return None
 * @Note  Definition count must not exceed SCA_NUM_USE
 */
void setupActuators(uint8_t id, CAN_Handler_t* pCan)
{
  static uint32_t i = 0;

  /* Definition count exceeds usage count */
  if(i >= SCA_NUM_USE)	return;

  /* Handle bind information */
  SCA_Handler_List[i].ID = id;
  SCA_Handler_List[i].Can = pCan;

  /* Increase list item */
  i++;
}

/**
 * @Brief Reset controller, used when SCA crashes and reboots due to error
 * @Param id: 0 means reset all, non-zero resets controller with specified ID
 * @Return None
 * @Note  If an SCA crashes with red or blue light state, first power cycle
 *     the SCA to restore to yellow light state, then execute this function,
 *     and execute power-on function to complete crash reboot
 */
void resetController(uint8_t id)
{
  uint8_t i,id_temp;
  CAN_Handler_t* pCan_temp = NULL;

  if(id == 0)
  {
    /* Clear all information handles */
    for(i = 0; i < SCA_NUM_USE; i++)
    {
      /* Preserve ID and CAN port address */
      id_temp = SCA_Handler_List[i].ID;
      pCan_temp = SCA_Handler_List[i].Can;

      /* Clear structure */
      memset(&SCA_Handler_List[i], 0, sizeof(SCA_Handler_List[i]));

      /* Restore ID and CAN port address */
      SCA_Handler_List[i].ID = id_temp;
      SCA_Handler_List[i].Can = pCan_temp;
    }
  }else
  {
    /* Get information handle for the ID */
    SCA_Handler_t* pSCA = getInstance(id);
    if(pSCA == NULL)	return;

    /* Preserve CAN port address */
    pCan_temp = pSCA->Can;

    /* Clear structure */
    memset(pSCA, 0, sizeof(SCA_Handler_List[0]));

    /* Restore ID and CAN port address */
    pSCA->ID = id;
    pSCA->Can = pCan_temp;
  }
}

/**
 * @Brief Get SCA information handle of specified ID
 * @Param id: Actuator ID to get information for
 * @Return NULL: Information handle for this ID not found
 *     Other: Found information handle
 */
SCA_Handler_t* getInstance(uint8_t id)
{
  uint8_t i;

  for(i = 0; i < SCA_NUM_USE; i++)
    if(SCA_Handler_List[i].ID == id)
      return &SCA_Handler_List[i];

  return NULL;
}

/**
 * @Brief Check heartbeat (online) status of actuator
 * @Param id: Actuator ID to check
 *     isBlock: Block for blocking, Unblock for non-blocking
 * @Return SCA_NoError: Actuator is online
 *     SCA_OverTime: Actuator is offline
 *     For other communication errors, see SCA_Error list
 */
uint8_t isOnline(uint8_t id, uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = getInstance(id);

  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear online status first */
  pSCA->Online_State = Actr_Disable;

  /* Call read command to communicate with SCA, result goes to corresponding handle */
  Error = SCA_Read(pSCA, R1_Heartbeat);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Blocking communication */
  while((pSCA->Online_State != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief Check enable state of actuator
 * @Param id: Actuator ID to check
 *     isBlock: Block for blocking, Unblock for non-blocking
 * @Return Actr_Enable: Actuator is enabled
 *     Actr_Disable: Actuator is disabled
 *
 */
uint8_t isEnable(uint8_t id, uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear read flag first */
  pSCA->paraCache.R_Power_State = Actr_Disable;

  /* Call read command to communicate with SCA, result goes to corresponding handle */
  Error = SCA_Read(pSCA, R1_PowerState);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Blocking communication */
  while((pSCA->paraCache.R_Power_State != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief Check parameter update state of actuator
 * @Param id: Actuator ID to check
 * @Return Actr_Enable: Parameter updated
 *     Actr_Disable: Parameter not updated
 */
uint8_t isUpdate(uint8_t id)
{
  uint8_t State;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Save update state, and reset */
  State = pSCA->Update_State;
  pSCA->Update_State = Actr_Disable;

  return State;
}

/**
 * @Brief Enable all actuators, blocking
 * @Param None
 * @Return None
 */
void enableAllActuators()
{
  uint8_t i;

  for(i = 0; i < SCA_NUM_USE; i++)
    enableActuator(SCA_Handler_List[i].ID);
}

/**
 * @Brief Disable all actuators, blocking
 * @Param None
 * @Return None
 */
void disableAllActuators()
{
  uint8_t i;

  for(i = 0; i < SCA_NUM_USE; i++)
    disableActuator(SCA_Handler_List[i].ID);
}

/**
 * @Brief Enable actuator, blocking
 * @Param id: ID of actuator to enable
 * @Return SCA_NoError: Operation success
 *     For other communication errors, see SCA_Error list
 */
uint8_t enableActuator(uint8_t id)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Query current enable state once */
  Error = isEnable(id, Block);
  if(Error)	return Error;

  /* If currently in target state, return success directly */
  if(pSCA->Power_State == Actr_Enable)	goto PowerOn;

  /* Target parameter written to cache waiting for update */
  pSCA->paraCache.Power_State = Actr_Enable;

  /* Execute power-on command */
  Error = SCA_Write_1(pSCA, W1_PowerState, Actr_Enable);
  if(Error)	return Error;

  /* Wait for power-on success, update handle info */
  while((pSCA->Power_State != Actr_Enable) && (waitime++ < CanPowertime));
  if(waitime >= CanPowertime)	return SCA_OperationFailed;

  PowerOn:
  /* Update online status */
  pSCA->Online_State = Actr_Enable;

  /* Read device serial number, for ID change */
  getActuatorSerialNumber(id,Block);

  /* Read last shutdown abnormal state once */
  getActuatorLastState(id,Block);
  if(pSCA->Last_State == 0)    // Prompt last shutdown state abnormal
    SCA_Debug("ID:%d Last_State Error\r\n",pSCA->ID);

  /* Read actuator's full-scale current value, used when reading/writing current loop parameters,
    This value varies by SCA model, can also be manually updated to handle info.
    This parameter value must be obtained. */
  getCurrentRange(id,Block);
  if(pSCA->Current_Max == 0)  // Did not get current full scale value, cannot write current value
    SCA_Debug("ID:%d Current_Max Error\r\n",pSCA->ID);

  /* Update all parameters to handle once, use non-blocking to shorten boot time */
  regainAttrbute(id,Unblock);

  return Error;
}

/**
 * @Brief Disable actuator, blocking
 * @Param id: ID of actuator to disable
 * @Return SCA_NoError: Operation success
 *     For other communication errors, see SCA_Error list
 */
uint8_t disableActuator(uint8_t id)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Query current enable state once */
  Error = isEnable(id, Block);
  if(Error)	return Error;

  /* If currently in target state, return success directly */
  if(pSCA->Power_State == Actr_Disable)	return SCA_NoError;

  /* Target parameter written to cache waiting for update */
  pSCA->paraCache.Power_State = Actr_Disable;

  /* Execute power-off command */
  Error = SCA_Write_1(pSCA, W1_PowerState, Actr_Disable);
  if(Error)	return Error;

  /* Wait for power-off success */
  while((pSCA->Power_State != Actr_Disable) && (waitime++ < CanPowertime));
  if(waitime >= CanPowertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief Actuator switch operation mode
 * @Param id: ID of actuator to operate
 *     ActuatorMode: Operation mode, see SCA_Protocol.h
 *     isBlock: Block for blocking, Unblock for non-blocking
 * @Return SCA_NoError: Operation success
 *     For other communication errors, see SCA_Error list
 */
uint8_t activateActuatorMode(uint8_t id, uint8_t ActuatorMode, uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* If currently in target state, return success directly */
  if(pSCA->Mode == ActuatorMode)	return SCA_NoError;

  /* Target parameter written to cache waiting for update */
  pSCA->paraCache.Mode = ActuatorMode;

  /* Execute mode switch command */
  Error = SCA_Write_1(pSCA, W1_Mode, ActuatorMode);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Mode != ActuatorMode) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief Actuator read current operation mode
 * @Param id: ID of actuator to operate
 *     isBlock: Block for blocking, Unblock for non-blocking
 * @Return SCA_NoError: Operation success
 *     For other communication errors, see SCA_Error list
 */
uint8_t getActuatorMode(uint8_t id, uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear read wait flag first */
  pSCA->paraCache.R_Mode = Actr_Disable;

  /* Call read function, read value saved directly to handle */
  Error = SCA_Read(pSCA, R1_Mode);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Mode != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief Actuator read warning information, update to handle
 * @Param id: ID of actuator to operate
 *     isBlock: Block for blocking, Unblock for non-blocking
 * @Return SCA_NoError: Operation success
 *     For other communication errors, see SCA_Error list
 */
uint8_t getErrorCode(uint8_t id, uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear read wait flag first */
  pSCA->paraCache.R_Error_Code = Actr_Disable;

  /* Execute read error info command */
  Error = SCA_Read(pSCA, R2_Error);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Error_Code != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief Actuator clear warning information
 * @Param id: ID of actuator to operate
 *     isBlock: Block for blocking, Unblock for non-blocking
 * @Return SCA_NoError: Operation success
 *     For other communication errors, see SCA_Error list
 */
uint8_t clearError(uint8_t id, uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* If no current error, no need to clear */
  if(pSCA->SCA_Warn.Error_Code == 0)	return SCA_NoError;

  /* Execute clear error command */
  Error = SCA_Write_4(pSCA, W4_ClearError);

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->SCA_Warn.Error_Code != 0) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief Actuator get all current parameters
 * @Param id: ID of actuator to operate
 *     isBlock: Block for blocking, Unblock for non-blocking
 * @Return None
 */
void regainAttrbute(uint8_t id,uint8_t isBlock)
{
  getErrorCode(id,isBlock);
  requestCVPValue(id,isBlock);
  getActuatorMode(id,isBlock);
  getPositionKp(id,isBlock);
  getPositionKi(id,isBlock);
  getPositionUmax(id,isBlock);
  getPositionUmin(id,isBlock);
  getPositionOffset(id,isBlock);
  getMaximumPosition(id,isBlock);
  getMinimumPosition(id,isBlock);
  isPositionLimitEnable(id,isBlock);
  isPositionFilterEnable(id,isBlock);
  getPositionCutoffFrequency(id,isBlock);
  getProfilePositionAcceleration(id,isBlock);
  getProfilePositionDeceleration(id,isBlock);
  getProfilePositionMaxVelocity(id,isBlock);
  getVelocityKp(id,isBlock);
  getVelocityKi(id,isBlock);
  getVelocityUmax(id,isBlock);
  getVelocityUmin(id,isBlock);
  isVelocityFilterEnable(id,isBlock);
  getVelocityCutoffFrequency(id,isBlock);
  getVelocityLimit(id,isBlock);
  getProfileVelocityAcceleration(id,isBlock);
  getProfileVelocityDeceleration(id,isBlock);
  getProfileVelocityMaxVelocity(id,isBlock);
  getCurrentKp(id,isBlock);
  getCurrentKi(id,isBlock);
  isCurrentFilterEnable(id,isBlock);
  getCurrentCutoffFrequency(id,isBlock);
  getCurrentLimit(id,isBlock);
  getVoltage(id,isBlock);
  getLockEnergy(id,isBlock);
  getMotorTemperature(id,isBlock);
  getInverterTemperature(id,isBlock);
  getMotorProtectedTemperature(id,isBlock);
  getMotorRecoveryTemperature(id,isBlock);
  getInverterProtectedTemperature(id,isBlock);
  getInverterRecoveryTemperature(id,isBlock);
}
/**
 * @Brief Actuator save all current parameters
 * @Param id: ID of actuator to operate
 *     isBlock: Block for blocking, Unblock for non-blocking
 * @Return SCA_NoError: Operation success
 *     For other communication errors, see SCA_Error list
 */
uint8_t saveAllParams(uint8_t id, uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear save status bit */
  pSCA->Save_State = Actr_Disable;

  Error = SCA_Write_4(pSCA, W4_Save);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution success */
  while((pSCA->Save_State != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanPowertime)	return SCA_OperationFailed;

  return Error;
}


/**************************** Position Related *******************************/

/**
 * @Brief 	Actuator set current position value
 * @Param 	id: ID of actuator to operate
 *			pos: Target position value, real value, range -127.0R ~ +127.0R
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setPosition(uint8_t id, float pos)
{
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  return SCA_Write_3(pSCA, W3_Position, pos);
}

/**
 * @Brief 	Actuator set current position value, fast
 * @Param 	pSCA: target actuator handle pointer or address
 *			pos: Target position value, real value, range -127.0R ~ +127.0R
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setPositionFast(SCA_Handler_t* pSCA, float pos)
{
  return SCA_Write_3(pSCA, W3_Position, pos);
}

/**
 * @Brief 	Actuator read currentposition value,update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getPosition(uint8_t id, uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Position_Real = Actr_Disable;

  Error = SCA_Read(pSCA, R3_Position);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* wait for execute success */
  while((pSCA->paraCache.R_Position_Real != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanPowertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Actuator read currentposition value,update to handle, fast
 * @Param 	pSCA: target actuator handle address or pointer
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getPositionFast(SCA_Handler_t* pSCA, uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;

  /* Clear status bit */
  pSCA->paraCache.R_Position_Real = Actr_Disable;

  Error = SCA_Read(pSCA, R3_Position);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* wait for execute success */
  while((pSCA->paraCache.R_Position_Real != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanPowertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	set Actuator position loop Kp value
 * @Param 	id: ID of actuator to operate
 *			Kp: Target position loop Kp value, real value
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setPositionKp(uint8_t id,float Kp, uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Position_Filter_P = Kp;

  Error = SCA_Write_3(pSCA, W3_PositionFilterP, Kp);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Position_Filter_P != Kp) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator position loop Kp value, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getPositionKp(uint8_t id, uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Position_Filter_P = Actr_Disable;

  Error = SCA_Read(pSCA, R3_PositionFilterP);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Position_Filter_P != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	set Actuator position loop Ki value
 * @Param 	id: ID of actuator to operate
 *			Ki: Target position loop Ki value, real value
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setPositionKi(uint8_t id,float Ki, uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Position_Filter_I = Ki;

  Error = SCA_Write_3(pSCA, W3_PositionFilterI, Ki);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Position_Filter_I != Ki) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator position loop Ki value, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getPositionKi(uint8_t id, uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Position_Filter_I = Actr_Disable;

  Error = SCA_Read(pSCA, R3_PositionFilterI);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Position_Filter_I != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	set Actuator position loopoutput upper limit value
 * @Param 	id: ID of actuator to operate
 *			max: Target position loop output upper limit value, real value
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setPositionUmax(uint8_t id,float max,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Position_Filter_Limit_H = max;

  Error = SCA_Write_3(pSCA, W3_PositionFilterLimitH, max);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Position_Filter_Limit_H != max) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator position loopoutput upper limit value, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getPositionUmax(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Position_Filter_Limit_H = Actr_Disable;

  Error = SCA_Read(pSCA, R3_PositionFilterLimitH);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Position_Filter_Limit_H != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	set Actuator position loopoutput lower limit value
 * @Param 	id: ID of actuator to operate
 *			min: Target position loop output lower limit value, real value
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setPositionUmin(uint8_t id,float min,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Position_Filter_Limit_L = min;

  Error = SCA_Write_3(pSCA, W3_PositionFilterLimitL, min);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Position_Filter_Limit_L != min) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator position loopoutput lower limit value, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getPositionUmin(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Position_Filter_Limit_L = Actr_Disable;

  Error = SCA_Read(pSCA, R3_PositionFilterLimitL);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Position_Filter_Limit_L != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	set Actuator position offset value
 * @Param 	id: ID of actuator to operate
 *			offset: target position offset value, real value
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setPositionOffset(uint8_t id, float offset,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Position_Offset = offset;

  Error = SCA_Write_3(pSCA, W3_PositionOffset, offset);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Position_Offset != offset) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator position offset value, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getPositionOffset(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Position_Offset = Actr_Disable;

  Error = SCA_Read(pSCA, R3_PositionOffset);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Position_Offset != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	set Actuator position max value
 * @Param 	id: ID of actuator to operate
 *			maxPos: target position max value, real value
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setMaximumPosition(uint8_t id,float maxPos,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Position_Limit_H = maxPos;

  Error = SCA_Write_3(pSCA, W3_PositionLimitH, maxPos);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Position_Limit_H != maxPos) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator position max value, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getMaximumPosition(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Position_Limit_H = Actr_Disable;

  Error = SCA_Read(pSCA, R3_PositionLimitH);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Position_Limit_H != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	set Actuator position min value
 * @Param 	id: ID of actuator to operate
 *			minPos: target position min value, real value
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setMinimumPosition(uint8_t id,float minPos,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Position_Limit_L = minPos;

  Error = SCA_Write_3(pSCA, W3_PositionLimitL, minPos);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Position_Limit_L != minPos) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator position min value, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getMinimumPosition(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Position_Limit_L = Actr_Disable;

  Error = SCA_Read(pSCA, R3_PositionLimitL);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Position_Limit_L != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	enable or disable Actuator position limit
 * @Param 	id: ID of actuator to operate
 *			enable: enable status, Actr_Enable enable, Actr_Disable disable
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t enablePositionLimit(uint8_t id, uint8_t enable,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Position_Limit_State = enable;

  Error = SCA_Write_1(pSCA, W1_PositionLimitState, enable);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Position_Limit_State != enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator position limitenable status, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t isPositionLimitEnable(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Position_Limit_State = Actr_Disable;

  Error = SCA_Read(pSCA, R1_PositionLimitState);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Position_Limit_State != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	set Actuator zero position, recalculate left and right limits
 * @Param 	id: ID of actuator to operate
 *			homingPos: zero position, real value, unit R
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setHomingPosition(uint8_t id,float homingPos,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Homing_Value = homingPos;

  Error = SCA_Write_3(pSCA, W3_HomingValue, homingPos);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Homing_Value != homingPos) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	enableActuator position loopfilter
 * @Param 	id: ID of actuator to operate
 *			enable: enable status, Actr_Enable enable, Actr_Disable disable
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t enablePositionFilter(uint8_t id,uint8_t enable,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Position_Filter_State = enable;

  Error = SCA_Write_1(pSCA, W1_PositionFilterState, enable);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Position_Filter_State != enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator position loop filter enable status, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t isPositionFilterEnable(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Position_Filter_State = Actr_Disable;

  Error = SCA_Read(pSCA, R1_PositionFilterState);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Position_Filter_State != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	set Actuator position loopfilterbandwidth
 * @Param 	id: ID of actuator to operate
 *			frequency: filter bandwidth, real value, unit hz
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setPositionCutoffFrequency(uint8_t id, float frequency,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Position_Filter_Value = frequency;

  Error = SCA_Write_2(pSCA, W2_PositionFilterValue, frequency);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Position_Filter_Value != frequency) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator position loop filter bandwidth, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getPositionCutoffFrequency(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Position_Filter_Value = Actr_Disable;

  Error = SCA_Read(pSCA, R2_PositionFilterValue);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Position_Filter_Value != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	clear homing information, including left and right limits and 0 position, TBD
 * @Param 	id: Actuator id
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t clearHomingInfo(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.W_ClearHome = Actr_Disable;

  Error = SCA_Write_4(pSCA, W4_ClearHome);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.W_ClearHome != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	set Actuator profile position max acceleration
 * @Param 	id: ID of actuator to operate
 *			acceleration: max acceleration, real value, unit RPM/S^2
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setProfilePositionAcceleration(uint8_t id, float acceleration,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.PP_Max_Acceleration = acceleration;

  /* profile acceleration transmission value is IQ20 times the real value, class 3 read/write interface is
    transmitted in IQ24 format, needs IQ4 multiple process. In addition, the unit of 
    this value is RPM, need to scale the value by 60 to become RPM unit.
    Final scale value = 2^4 * 60 = 960
  */
  acceleration /= Profile_Scal;

  Error = SCA_Write_3(pSCA, W3_PPMaxAcceleration, acceleration);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->PP_Max_Acceleration != acceleration) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator profile position max acceleration, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getProfilePositionAcceleration(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_PP_Max_Acceleration = Actr_Disable;

  Error = SCA_Read(pSCA, R3_PPMaxAcceleration);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_PP_Max_Acceleration != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	set Actuator profile position max deceleration
 * @Param 	id: ID of actuator to operate
 *			deceleration: max deceleration, real value, unit RPM/S^2
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setProfilePositionDeceleration(uint8_t id, float deceleration,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.PP_Max_Deceleration = deceleration;

  deceleration /= Profile_Scal;

  Error = SCA_Write_3(pSCA, W3_PPMaxDeceleration, deceleration);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->PP_Max_Deceleration != deceleration) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator profile position max deceleration, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getProfilePositionDeceleration(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_PP_Max_Deceleration = Actr_Disable;

  Error = SCA_Read(pSCA, R3_PPMaxDeceleration);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_PP_Max_Deceleration != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	set Actuator profile position max velocity
 * @Param 	id: ID of actuator to operate
 *			maxVelocity: max velocity, real value, unit RPM
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setProfilePositionMaxVelocity(uint8_t id, float maxVelocity,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.PP_Max_Velocity = maxVelocity;

  maxVelocity /= Profile_Scal;

  Error = SCA_Write_3(pSCA, W3_PPMaxVelocity, maxVelocity);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->PP_Max_Velocity != maxVelocity) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator profile position max velocity, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getProfilePositionMaxVelocity(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_PP_Max_Velocity = Actr_Disable;

  Error = SCA_Read(pSCA, R3_PPMaxVelocity);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_PP_Max_Velocity != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}


/**************************** Velocity Related *******************************/

/**
 * @Brief 	set Actuator currentvelocity value
 * @Param 	id: ID of actuator to operate
 *			vel: target velocity, real value, unit RPM
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setVelocity(uint8_t id,float vel)
{
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  return SCA_Write_3(pSCA, W3_Velocity, vel);
}

/**
 * @Brief 	set Actuator current velocity value, fast
 * @Param 	pSCA: target actuator handle address or pointer
 *			vel: target velocity, real value, unit RPM
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setVelocityFast(SCA_Handler_t* pSCA,float vel)
{
  return SCA_Write_3(pSCA, W3_Velocity, vel);
}


/**
 * @Brief 	Get Actuator current velocity, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getVelocity(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Velocity_Real = Actr_Disable;

  Error = SCA_Read(pSCA, R3_Velocity);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Velocity_Real != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator current velocity, update to handle, fast
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getVelocityFast(SCA_Handler_t* pSCA,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;

  /* Clear status bit */
  pSCA->paraCache.R_Velocity_Real = Actr_Disable;

  Error = SCA_Read(pSCA, R3_Velocity);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Velocity_Real != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator velocity loop proportional, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getVelocityKp(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Velocity_Filter_P = Actr_Disable;

  Error = SCA_Read(pSCA, R3_VelocityFilterP);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Velocity_Filter_P != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	set Actuator velocity loop proportional
 * @Param 	id: ID of actuator to operate
 *			Kp: velocity loop proportional, real value
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setVelocityKp(uint8_t id,float Kp,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Velocity_Filter_P = Kp;

  Error = SCA_Write_3(pSCA, W3_VelocityFilterP, Kp);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Velocity_Filter_P != Kp) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator velocity loop integral, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getVelocityKi(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Velocity_Filter_I = Actr_Disable;

  Error = SCA_Read(pSCA, R3_VelocityFilterI);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Velocity_Filter_I != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	set Actuator velocity loop integral
 * @Param 	id: ID of actuator to operate
 *			Ki: velocity loop integral, real value
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setVelocityKi(uint8_t id, float Ki,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Velocity_Filter_I = Ki;

  Error = SCA_Write_3(pSCA, W3_VelocityFilterI, Ki);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Velocity_Filter_I != Ki) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator velocity loop max output limit, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getVelocityUmax(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Velocity_Filter_Limit_H = Actr_Disable;

  Error = SCA_Read(pSCA, R3_VelocityFilterLimitH);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Velocity_Filter_Limit_H != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	set Actuator velocity loop max output limit
 * @Param 	id: ID of actuator to operate
 *			max: max output limit, real value
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setVelocityUmax(uint8_t id, float max,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Velocity_Filter_Limit_H = max;

  Error = SCA_Write_3(pSCA, W3_VelocityFilterLimitH, max);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Velocity_Filter_Limit_H != max) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator velocity loop min output limit, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getVelocityUmin(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Velocity_Filter_Limit_L = Actr_Disable;

  Error = SCA_Read(pSCA, R3_VelocityFilterLimitL);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Velocity_Filter_Limit_L != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	set Actuator velocity loop min output limit
 * @Param 	id: ID of actuator to operate
 *			min: min output limit, real value
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setVelocityUmin(uint8_t id, float min,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Velocity_Filter_Limit_L = min;

  Error = SCA_Write_3(pSCA, W3_VelocityFilterLimitL, min);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Velocity_Filter_Limit_L != min) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator velocity loop velocity range
 * @Param 	id: ID of actuator to operate
 * @Return 	velocity loop velocity range, real value
 */
float getVelocityRange(uint8_t id)
{
  return Velocity_Max;
}

/**
 * @Brief 	enableActuator velocity loopfilter
 * @Param 	id: ID of actuator to operate
 *			enable: enable status, Actr_Enable enable, Actr_Disable disable
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t enableVelocityFilter(uint8_t id,uint8_t enable,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Velocity_Filter_State = enable;

  Error = SCA_Write_1(pSCA, W1_VelocityFilterState, enable);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Velocity_Filter_State != enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator velocity loop filter enable status, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t isVelocityFilterEnable(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Velocity_Filter_State = Actr_Disable;

  Error = SCA_Read(pSCA, R1_VelocityFilterState);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Velocity_Filter_State != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator velocity loop filter bandwidth, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getVelocityCutoffFrequency(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Velocity_Filter_Value = Actr_Disable;

  Error = SCA_Read(pSCA, R2_VelocityFilterValue);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Velocity_Filter_Value != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	set Actuator velocity loopfilterbandwidth
 * @Param 	id: ID of actuator to operate
 *			frequency: filter bandwidth, real value, unit hz
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setVelocityCutoffFrequency(uint8_t id, float frequency,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Velocity_Filter_Value = frequency;

  Error = SCA_Write_2(pSCA, W2_VelocityFilterValue, frequency);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Velocity_Filter_Value != frequency) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	set Actuator velocity loop input limit
 * @Param 	id: ID of actuator to operate
 *			limit: input limit
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setVelocityLimit(uint8_t id,float limit,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Velocity_Limit = limit;

  Error = SCA_Write_3(pSCA, W3_VelocityLimit, limit);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Velocity_Limit != limit) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator velocity loop input limit, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getVelocityLimit(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Velocity_Limit = Actr_Disable;

  Error = SCA_Read(pSCA, R3_VelocityLimit);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Velocity_Limit != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	set Actuator profile velocity loop acceleration
 * @Param 	id: ID of actuator to operate
 *			acceleration: acceleration, real value
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setProfileVelocityAcceleration(uint8_t id,float acceleration,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.PV_Max_Acceleration = acceleration;

  acceleration /= Profile_Scal;

  Error = SCA_Write_3(pSCA, W3_PVMaxAcceleration, acceleration);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->PV_Max_Acceleration != acceleration) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator profile velocity loop acceleration, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getProfileVelocityAcceleration(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_PV_Max_Acceleration = Actr_Disable;

  Error = SCA_Read(pSCA, R3_PVMaxAcceleration);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_PV_Max_Acceleration != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	set Actuator profile velocity loop deceleration
 * @Param 	id: ID of actuator to operate
 *			deceleration: deceleration, real value
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setProfileVelocityDeceleration(uint8_t id,float deceleration,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.PV_Max_Deceleration = deceleration;

  deceleration /= Profile_Scal;

  Error = SCA_Write_3(pSCA, W3_PVMaxDeceleration, deceleration);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->PV_Max_Deceleration != deceleration) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator profile velocity loop deceleration, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getProfileVelocityDeceleration(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_PV_Max_Deceleration = Actr_Disable;

  Error = SCA_Read(pSCA, R3_PVMaxDeceleration);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_PV_Max_Deceleration != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	set Actuator profile velocity loop max velocity
 * @Param 	id: ID of actuator to operate
 *			maxVelocity: max velocity, real value, unit RPM
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setProfileVelocityMaxVelocity(uint8_t id, float maxVelocity,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.PV_Max_Velocity = maxVelocity;

  maxVelocity /= Profile_Scal;

  Error = SCA_Write_3(pSCA, W3_PVMaxVelocity, maxVelocity);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->PV_Max_Velocity != maxVelocity) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator profile velocity loop max velocity, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getProfileVelocityMaxVelocity(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_PV_Max_Velocity = Actr_Disable;

  Error = SCA_Read(pSCA, R3_PVMaxVelocity);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_PV_Max_Velocity != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}


/**************************** Current Related *******************************/

/**
 * @Brief 	set Actuator currentcurrent value
 * @Param 	id: ID of actuator to operate
 *			current: current current value, real value, unit A
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setCurrent(uint8_t id,float current)
{
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  return SCA_Write_3(pSCA, W3_Current, current);
}

/**
 * @Brief 	set Actuator currentcurrent value, fast
 * @Param 	pSCA: target actuator handle address or pointer
 *			current: current current value, real value, unit A
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setCurrentFast(SCA_Handler_t* pSCA,float current)
{
  return SCA_Write_3(pSCA, W3_Current, current);
}

/**
 * @Brief 	Get Actuator current current value, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getCurrent(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Current_Real = Actr_Disable;

  Error = SCA_Read(pSCA, R3_Current);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Current_Real != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator current current value, update to handle, fast
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getCurrentFast(SCA_Handler_t* pSCA,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;

  /* Clear status bit */
  pSCA->paraCache.R_Current_Real = Actr_Disable;

  Error = SCA_Read(pSCA, R3_Current);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Current_Real != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator current loop proportional value, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getCurrentKp(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Clear status bit */
  pSCA->paraCache.R_Current_Filter_P = Actr_Disable;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  Error = SCA_Read(pSCA, R3_CurrentFilterP);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Current_Filter_P != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator current loop integral, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getCurrentKi(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Current_Filter_I = Actr_Disable;

  Error = SCA_Read(pSCA, R3_CurrentFilterI);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Current_Filter_I != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;

}

/**
 * @Brief 	Get Actuator current range, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getCurrentRange(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Current_Max = Actr_Disable;

  Error = SCA_Read(pSCA, R2_Current_Max);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Current_Max != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;

}

/**
 * @Brief 	enableActuator current loopfilter
 * @Param 	id: ID of actuator to operate
 *			enable: enable status, Actr_Enable enable, Actr_Disable disable
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t enableCurrentFilter(uint8_t id,uint8_t enable,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Current_Filter_State = enable;

  Error = SCA_Write_1(pSCA, W1_CurrentFilterState, enable);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Current_Filter_State != enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator current loop filter enable status, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t isCurrentFilterEnable(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Current_Filter_State = Actr_Disable;

  Error = SCA_Read(pSCA, R1_CurrentFilterState);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Current_Filter_State != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;

}

/**
 * @Brief 	Get Actuator current loop filter bandwidth, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getCurrentCutoffFrequency(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Current_Filter_Value = Actr_Disable;

  Error = SCA_Read(pSCA, R2_CurrentFilterValue);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Current_Filter_Value != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;

}

/**
 * @Brief 	set Actuator current loopfilterbandwidth
 * @Param 	id: ID of actuator to operate
 *			frequency: target cutoff frequency, unit hz
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setCurrentCutoffFrequency(uint8_t id, float frequency,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Current_Filter_Value = frequency;

  Error = SCA_Write_2(pSCA, W2_CurrentFilterValue, frequency);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Current_Filter_Value != frequency) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	set Actuator current loopinput limit
 * @Param 	id: ID of actuator to operate
 *			limit: input limit
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setCurrentLimit(uint8_t id,float limit,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Current_Limit = limit;

  Error = SCA_Write_3(pSCA, W3_CurrentLimit, limit);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Current_Limit != limit) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator current loopinput limit, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getCurrentLimit(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Current_Limit = Actr_Disable;

  Error = SCA_Read(pSCA, R3_CurrentLimit);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Current_Limit != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;

}

/**************************** Other parameters *******************************/

/**
 * @Brief 	Get Actuator voltage, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getVoltage(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Voltage = Actr_Disable;

  Error = SCA_Read(pSCA, R2_Voltage);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Voltage != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator lock energy, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getLockEnergy(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Blocked_Energy = Actr_Disable;

  Error = SCA_Read(pSCA, R3_BlockEngy);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Blocked_Energy != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;

}

/**
 * @Brief 	set Actuator lock energy value
 * @Param 	id: ID of actuator to operate
 *			energy: lock energy value, real value, unit J
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setLockEnergy(uint8_t id,float energy,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Blocked_Energy = energy;

  Error = SCA_Write_3(pSCA, W3_BlockEngy, energy);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Blocked_Energy != energy) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator motor temperature value, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getMotorTemperature(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Motor_Temp = Actr_Disable;

  Error = SCA_Read(pSCA, R2_MotorTemp);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Motor_Temp != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;

}

/**
 * @Brief 	Get Actuator inverter temperature value, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getInverterTemperature(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Inverter_Temp = Actr_Disable;

  Error = SCA_Read(pSCA, R2_InverterTemp);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Inverter_Temp != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;

}

/**
 * @Brief 	Get Actuator motor protection temperature value, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getMotorProtectedTemperature(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Inverter_Protect_Temp = Actr_Disable;

  Error = SCA_Read(pSCA, R2_MotorProtectTemp);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Inverter_Protect_Temp != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;

}

/**
 * @Brief 	set Actuator motor protection temperature value
 * @Param 	id: ID of actuator to operate
 *			temp: motor protection temperature value, real value, unit Celsius
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setMotorProtectedTemperature(uint8_t id,float temp,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Motor_Protect_Temp = temp;

  Error = SCA_Write_2(pSCA, W2_MotorProtectTemp, temp);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Motor_Protect_Temp != temp) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator motor recovery temperature value, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getMotorRecoveryTemperature(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Motor_Recover_Temp = Actr_Disable;

  Error = SCA_Read(pSCA, R2_MotorRecoverTemp);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Motor_Recover_Temp != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	set Actuator motor recovery temperature value
 * @Param 	id: ID of actuator to operate
 *			temp: motor recovery temperature value, real value, unit Celsius
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setMotorRecoveryTemperature(uint8_t id,float temp,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Motor_Recover_Temp = temp;

  Error = SCA_Write_2(pSCA, W2_MotorRecoverTemp, temp);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Motor_Recover_Temp != temp) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator inverter protection temperature value, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getInverterProtectedTemperature(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Inverter_Protect_Temp = Actr_Disable;

  Error = SCA_Read(pSCA, R2_InverterProtectTemp);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Inverter_Protect_Temp != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;

}

/**
 * @Brief 	set Actuator inverter protection temperature value
 * @Param 	id: ID of actuator to operate
 *			temp: inverter protection temperature value, real value, unit Celsius
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setInverterProtectedTemperature(uint8_t id,float temp,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Inverter_Protect_Temp = temp;

  Error = SCA_Write_2(pSCA, W2_InverterProtectTemp, temp);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Inverter_Protect_Temp != temp) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator inverter recovery temperature value, update to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getInverterRecoveryTemperature(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Inverter_Recover_Temp = Actr_Disable;

  Error = SCA_Read(pSCA, R2_InverterRecoverTemp);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Inverter_Recover_Temp != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;

}

/**
 * @Brief 	set Actuator inverter recovery temperature value
 * @Param 	id: ID of actuator to operate
 *			temp: inverter recovery temperature value, real value, unit Celsius
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setInverterRecoveryTemperature(uint8_t id,float temp,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.Inverter_Recover_Temp = temp;

  Error = SCA_Write_2(pSCA, W2_InverterRecoverTemp, temp);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->Inverter_Recover_Temp != temp) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	set Actuator of id
 * @Param 	newID: new id
 *			currentID: current id
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t setActuatorID(uint8_t currentID, uint8_t newID,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* check if target ID already exists */
  pSCA = getInstance(newID);
  if(pSCA != NULL)	return SCA_OperationFailed;

  /* Get information handle for the ID */
  pSCA = getInstance(currentID);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* target parameter written to cache, waiting for update */
  pSCA->paraCache.ID = newID;

  Error = SCA_Write_5(pSCA, W5_ChangeID, newID);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->ID != newID) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	Get Actuator serial number, save to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getActuatorSerialNumber(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Serial_Num = Actr_Disable;

  Error = SCA_Read(pSCA, R5_ShakeHands);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Serial_Num != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;

}

/**
 * @Brief 	Get Actuator last shutdown status, save to handle
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t getActuatorLastState(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_Last_State = Actr_Disable;

  Error = SCA_Read(pSCA, R1_LastState);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_Last_State != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;

}

/**
 * @Brief 	get current velocity position values, update to handle, high efficiency
 * @Param 	id: ID of actuator to operate
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t requestCVPValue(uint8_t id,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;
  SCA_Handler_t* pSCA = NULL;

  /* Get information handle for the ID */
  pSCA = getInstance(id);
  if(pSCA == NULL)	return SCA_UnknownID;

  /* Clear status bit */
  pSCA->paraCache.R_CVP = Actr_Disable;

  Error = SCA_Read(pSCA, R4_CVP);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_CVP != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

/**
 * @Brief 	get current velocity position values, update to handle, high efficiency, fast
 * @Param 	pSCA: target actuator handle address or pointer
 *			isBlock: Block for blocking, Unblock for non-blocking
 * @Return 	SCA_NoError: Operation success
 *			For other communication errors, see SCA_Error list
 */
uint8_t requestCVPValueFast(SCA_Handler_t* pSCA,uint8_t isBlock)
{
  uint8_t Error;
  uint32_t waitime = 0;

  /* Clear status bit */
  pSCA->paraCache.R_CVP = Actr_Disable;

  Error = SCA_Read(pSCA, R4_CVP);
  if(Error)	return Error;

  /* Non-blocking */
  if(isBlock == Unblock)
  {
    /* Delay after non-blocking send to prevent bus overload */
    SCA_Delay(SendInterval);
    return Error;
  }

  /* Wait for execution result */
  while((pSCA->paraCache.R_CVP != Actr_Enable) && (waitime++ < CanOvertime));
  if(waitime >= CanOvertime)	return SCA_OperationFailed;

  return Error;
}

