using System;
using System.IO;
using System.Text;
using System.Collections.Generic;

class Program
{
    static void Main()
    {
        string filePath = @"C:\Users\ishan\Documents\Projects\Robotic-Arm-Mini\firmware\robotic-arm-mini-core-fw\Robot\actuators\mintasca\sca_api.c";
        string content = File.ReadAllText(filePath, Encoding.UTF8);

        var replacements = new Dictionary<string, string>
        {
            {"isBlock：Block for blocking, Unblock for non-blocking", "isBlock: Block for blocking, Unblock for non-blocking"},
            {"pSCA：target actuator handle address or pointer", "pSCA: target actuator handle address or pointer"},
            {"Kp：Target position loop Kp value, real value", "Kp: Target position loop Kp value, real value"},
            {"Kp value，update to handle", "Kp value, update to handle"},
            {"Ki：Target position loop Ki value, real value", "Ki: Target position loop Ki value, real value"},
            {"Ki value，update to handle", "Ki value, update to handle"},
            {"max：Target position loopoutput upper limit value，real value", "max: Target position loop output upper limit value, real value"},
            {"output upper limit value，update to handle", "output upper limit value, update to handle"},
            {"min：Target position loopoutput lower limit value，real value", "min: Target position loop output lower limit value, real value"},
            {"output lower limit value，update to handle", "output lower limit value, update to handle"},
            {"offset：target position offset value，real value", "offset: target position offset value, real value"},
            {"offset value，update to handle", "offset value, update to handle"},
            {"maxPos：target position max value，real value", "maxPos: target position max value, real value"},
            {"max value，update to handle", "max value, update to handle"},
            {"minPos：target position min value，real value", "minPos: target position min value, real value"},
            {"min value，update to handle", "min value, update to handle"},
            {"enable：enable status, Actr_Enable enable, Actr_Disable disable", "enable: enable status, Actr_Enable enable, Actr_Disable disable"},
            {"enablestatus，update to handle", "enable status, update to handle"},
            {"homingPos：zero position, real value, unit R", "homingPos: zero position, real value, unit R"},
            {"frequency：filter bandwidth, real value, unit hz", "frequency: filter bandwidth, real value, unit hz"},
            {"id：Actuator id", "id: Actuator id"},
            {"acceleration：max acceleration, real value, unit RPM/S^2", "acceleration: max acceleration, real value, unit RPM/S^2"},
            {"max acceleration，update to handle", "max acceleration, update to handle"},
            {"deceleration：max deceleration, real value, unit RPM/S^2", "deceleration: max deceleration, real value, unit RPM/S^2"},
            {"max deceleration，update to handle", "max deceleration, update to handle"},
            {"maxVelocity：max velocity, real value, unit RPM", "maxVelocity: max velocity, real value, unit RPM"},
            {"max velocity，update to handle", "max velocity, update to handle"},
            {"vel：target velocity, real value, unit RPM", "vel: target velocity, real value, unit RPM"},
            {"update to handle,快速", "update to handle, fast"},
            {"Kp：velocity loop proportional，real value", "Kp: velocity loop proportional, real value"},
            {"Ki：velocity loop integral，real value", "Ki: velocity loop integral, real value"},
            {"max：max output limit, real value", "max: max output limit, real value"},
            {"min：min output limit, real value", "min: min output limit, real value"},
            {"velocity range，real value", "velocity range, real value"},
            {"limit：input limit", "limit: input limit"},
            {"input limit，update to handle", "input limit, update to handle"},
            {"acceleration：acceleration, real value", "acceleration: acceleration, real value"},
            {"acceleration，update to handle", "acceleration, update to handle"},
            {"deceleration：deceleration, real value", "deceleration: deceleration, real value"},
            {"deceleration，update to handle", "deceleration, update to handle"},
            {"current：current current value, real value, unit A", "current: current current value, real value, unit A"},
            {"frequency：target cutoff frequency, unit hz", "frequency: target cutoff frequency, unit hz"},
            {"energy：lock energy value，real value，单位 J", "energy: lock energy value, real value, unit J"},
            {"temp：motor protection temperature value，real value，单位 摄氏度", "temp: motor protection temperature value, real value, unit Celsius"},
            {"temp：motor recovery temperature value，real value，单位 摄氏度", "temp: motor recovery temperature value, real value, unit Celsius"},
            {"temp：inverter protection temperature value，real value，单位 摄氏度", "temp: inverter protection temperature value, real value, unit Celsius"},
            {"temp：inverter recovery temperature value，real value，单位 摄氏度", "temp: inverter recovery temperature value, real value, unit Celsius"},
            {"newID：new id", "newID: new id"},
            {"currentID：current id", "currentID: current id"},
            {"，update to handle", ", update to handle"},
            {"，real value", ", real value"}
        };

        foreach (var kvp in replacements)
        {
            content = content.Replace(kvp.Key, kvp.Value);
        }
        
        File.WriteAllText(filePath, content, new UTF8Encoding(false));
    }
}
