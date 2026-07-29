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
            {"Block为阻塞式，Unblock为Non-blocking式", "Block for blocking, Unblock for non-blocking"},
            {"Non-blockingsend 后延 when process ，prevent bus overload", "Delay after non-blocking send to prevent bus overload"},
            {"handle address or pointer of actuator to operate", "target actuator handle address or pointer"},
            {"Kp值", "Kp value"},
            {"Ki值", "Ki value"},
            {"output upper limit值", "output upper limit value"},
            {"output lower limit值", "output lower limit value"},
            {"位置offset值", "position offset value"},
            {"位置max value", "position max value"},
            {"位置min value", "position min value"},
            {"位置限位", "position limit"},
            {"零点位置，重新 calculate 左右限位", "zero position, recalculate left and right limits"},
            {"零点位置，real value，单位 R", "zero position, real value, unit R"},
            {"clear homing information ，包括左右极限和0位，待定", "clear homing information, including left and right limits and 0 position, TBD"},
            {"梯形position loop最大acceleration", "profile position max acceleration"},
            {"最大acceleration，real value，单位 RPM/S^2", "max acceleration, real value, unit RPM/S^2"},
            {"梯形acceleration传输值是real value of IQ20倍，第三类读写接口是以", "profile acceleration transmission value is IQ20 times the real value, class 3 read/write interface is"},
            {"IQ24格式传输 of ，need 做IQ4 of 倍数 process 。另外，改数值 of", "transmitted in IQ24 format, needs IQ4 multiple process. In addition, the unit of"},
            {"单位是RPM，需 will the 数值缩放60变成RPM单位。", "this value is RPM, need to scale the value by 60 to become RPM unit."},
            {"最终缩放值 = 2^4 * 60 = 960", "Final scale value = 2^4 * 60 = 960"},
            {"梯形position loop最大deceleration", "profile position max deceleration"},
            {"最大deceleration，real value，单位 RPM/S^2", "max deceleration, real value, unit RPM/S^2"},
            {"梯形position loop最大速度", "profile position max velocity"},
            {"最大速度，real value，单位 RPM", "max velocity, real value, unit RPM"},
            {"/****************************速度相关*******************************/", "/**************************** Velocity Related *******************************/"},
            {"target 速度，real value，单位 RPM", "target velocity, real value, unit RPM"},
            {"currentvelocity value,快速", "current velocity value, fast"},
            {"current速度，update to handle", "current velocity, update to handle"},
            {"current速度，update to handle,快速", "current velocity, update to handle, fast"},
            {"velocity loop比例，update to handle", "velocity loop proportional, update to handle"},
            {"velocity loop比例", "velocity loop proportional"},
            {"velocity loop比例，real value", "velocity loop proportional, real value"},
            {"velocity loop积分，update to handle", "velocity loop integral, update to handle"},
            {"velocity loop积分", "velocity loop integral"},
            {"velocity loop积分，real value", "velocity loop integral, real value"},
            {"velocity loop最大输出limit，update to handle", "velocity loop max output limit, update to handle"},
            {"velocity loop最大输出limit", "velocity loop max output limit"},
            {"最大输出limit，real value", "max output limit, real value"},
            {"velocity loop最小输出limit，update to handle", "velocity loop min output limit, update to handle"},
            {"velocity loop最小输出limit", "velocity loop min output limit"},
            {"最小输出limit，real value", "min output limit, real value"},
            {"velocity loop速度量程", "velocity loop velocity range"},
            {"velocity loop速度量程，real value", "velocity loop velocity range, real value"},
            {"velocity loopfilterenablestatus，update to handle", "velocity loop filter enable status, update to handle"},
            {"velocity loopfilterbandwidth，update to handle", "velocity loop filter bandwidth, update to handle"},
            {"velocity loop输入limit", "velocity loop input limit"},
            {"输入limit", "input limit"},
            {"velocity loop输入limit，update to handle", "velocity loop input limit, update to handle"},
            {"梯形velocity loopacceleration", "profile velocity loop acceleration"},
            {"acceleration，real value", "acceleration, real value"},
            {"梯形velocity loopacceleration，update to handle", "profile velocity loop acceleration, update to handle"},
            {"梯形velocity loopdeceleration", "profile velocity loop deceleration"},
            {"deceleration，real value", "deceleration, real value"},
            {"梯形velocity loopdeceleration，update to handle", "profile velocity loop deceleration, update to handle"},
            {"梯形velocity loop最大速度", "profile velocity loop max velocity"},
            {"梯形velocity loop最大速度，update to handle", "profile velocity loop max velocity, update to handle"},
            {"/****************************电流相关*******************************/", "/**************************** Current Related *******************************/"},
            {"currentcurrent value，real value，单位 A", "current current value, real value, unit A"},
            {"currentcurrent value，update to handle", "current current value, update to handle"},
            {"currentcurrent value，update to handle,快速", "current current value, update to handle, fast"},
            {"current loop比例值，update to handle", "current loop proportional value, update to handle"},
            {"current loop积分，update to handle", "current loop integral, update to handle"},
            {"电流量程，update to handle", "current range, update to handle"},
            {"current loopfilterenablestatus，update to handle", "current loop filter enable status, update to handle"},
            {"current loopfilterbandwidth，update to handle", "current loop filter bandwidth, update to handle"},
            {"target 截止频率，单位 hz", "target cutoff frequency, unit hz"},
            {"current loop输入limit", "current loop input limit"},
            {"current loop输入limit，update to handle", "current loop input limit, update to handle"},
            {"/****************************其他parameter*******************************/", "/**************************** Other parameters *******************************/"},
            {"voltage，update to handle", "voltage, update to handle"},
            {"堵转能量，update to handle", "lock energy, update to handle"},
            {"堵转能量值", "lock energy value"},
            {"堵转能量值，real value，单位 J", "lock energy value, real value, unit J"},
            {"电机temperature值，update to handle", "motor temperature value, update to handle"},
            {"逆变器temperature值，update to handle", "inverter temperature value, update to handle"},
            {"电机protectiontemperature值，update to handle", "motor protection temperature value, update to handle"},
            {"电机protectiontemperature值", "motor protection temperature value"},
            {"电机protectiontemperature值，real value，单位 摄氏度", "motor protection temperature value, real value, unit Celsius"},
            {"电机recoverytemperature值，update to handle", "motor recovery temperature value, update to handle"},
            {"电机recoverytemperature值", "motor recovery temperature value"},
            {"电机recoverytemperature值，real value，单位 摄氏度", "motor recovery temperature value, real value, unit Celsius"},
            {"逆变器protectiontemperature值，update to handle", "inverter protection temperature value, update to handle"},
            {"逆变器protectiontemperature值", "inverter protection temperature value"},
            {"逆变器protectiontemperature值，real value，单位 摄氏度", "inverter protection temperature value, real value, unit Celsius"},
            {"逆变器recoverytemperature值，update to handle", "inverter recovery temperature value, update to handle"},
            {"逆变器recoverytemperature值", "inverter recovery temperature value"},
            {"逆变器recoverytemperature值，real value，单位 摄氏度", "inverter recovery temperature value, real value, unit Celsius"},
            {"新id", "new id"},
            {"currentid", "current id"},
            {"check target ID是否已存 at", "check if target ID already exists"},
            {"of 序列号，save to 句柄中", "serial number, save to handle"},
            {"上次 of 关机status，save to 句柄中", "last shutdown status, save to handle"},
            {"获取电流速度位置 of 值，update to handle，效率高", "get current velocity position values, update to handle, high efficiency"},
            {"获取电流速度位置 of 值，update to handle，效率高, fast", "get current velocity position values, update to handle, high efficiency, fast"},
            {"filterbandwidth，real value，单位 hz", "filter bandwidth, real value, unit hz"},
            {"position loopfilterenablestatus，update to handle", "position loop filter enable status, update to handle"},
            {"position loopfilterbandwidth，update to handle", "position loop filter bandwidth, update to handle"},
            {"target parameterwrite in cache，wait for update", "target parameter written to cache, waiting for update"},
            {"enableor disableActuator", "enable or disable Actuator"},
            {"enablestatus，Actr_Enableenable，Actr_Disabledisable", "enable status, Actr_Enable enable, Actr_Disable disable"},
            {"Actuator id", "Actuator id"}
        };

        foreach (var kvp in replacements)
        {
            content = content.Replace(kvp.Key, kvp.Value);
        }
        
        File.WriteAllText(filePath, content, new UTF8Encoding(false));
    }
}
