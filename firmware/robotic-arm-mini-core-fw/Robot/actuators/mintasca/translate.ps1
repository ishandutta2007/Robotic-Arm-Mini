$file = "C:\Users\ishan\Documents\Projects\Robotic-Arm-Mini\firmware\robotic-arm-mini-core-fw\Robot\actuators\mintasca\sca_api.c"
$content = Get-Content $file -Raw -Encoding UTF8

$replacements = @{
    "@功`t能" = "@Brief ";
    "@参`t数" = "@Param ";
    "@返`t回" = "@Return ";
    "@注`t意" = "@Note ";
    "获取该ID的信息句柄" = "Get information handle for the ID";
    "非阻塞发送后延时处理，防止总线过载" = "Delay after non-blocking send to prevent bus overload";
    "先清空读取等待标志位" = "Clear read wait flag first";
    "等待执行结果" = "Wait for execution result";
    "封装读取函数，读出值直接保存到句柄中" = "Call read function, read value saved directly to handle";
    "非阻塞" = "Non-blocking";
    "封装写入函数，待写入目标参数" = "Call write function, target parameter to be written";
    "若当前无错误，则无需淸错" = "If no current error, no need to clear";
    "执行淸错命令" = "Execute clear error command";
    "目标参数写入缓存待更新" = "Target parameter written to cache waiting for update";
    "若当前已经处于目标状态，直接返回成功" = "If currently in target state, return success directly";
    "执行写入参数命令" = "Execute write parameter command";
    "无" = "None";
    "操作成功" = "Operation success";
    "其他通信错误参见 SCA_Error 错误列表" = "For other communication errors, see SCA_Error list";
    "id：要操作的执行器id" = "id: ID of actuator to operate";
    "isBlock：Block为阻塞式，Unblock为非阻塞式" = "isBlock: Block for blocking, Unblock for non-blocking";
    "执行器读取" = "Actuator read";
    "执行器保存" = "Actuator save";
    "执行器清除" = "Actuator clear";
    "执行器获取" = "Actuator get";
    "执行器设置" = "Actuator set";
    "执行器修改" = "Actuator modify";
    "电流环" = "current loop";
    "速度环" = "velocity loop";
    "位置环" = "position loop";
    "最大值" = "max value";
    "最小值" = "min value";
    "带宽" = "bandwidth";
    "偏置" = "offset";
    "比例系数" = "proportional coefficient";
    "积分系数" = "integral coefficient";
    "加速度" = "acceleration";
    "减速度" = "deceleration";
    "滤波器" = "filter";
    "使能" = "enable";
    "失能" = "disable";
    "温度" = "temperature";
    "保护" = "protection";
    "恢复" = "recovery";
    "电压" = "voltage";
    "当前" = "current";
    "设定" = "setting";
    "真实值" = "real value";
    "返回值" = "return value";
    "通信超时" = "communication timeout";
    "接收成功" = "receive success";
    "错误代码" = "error code";
    "报警信息" = "warning info";
    "限幅" = "limit";
    "参数" = "parameter";
    "状态" = "status";
    "打开" = "open";
    "关闭" = "close";
    "：要操作的执行器id" = ": ID of actuator to operate";
    "：Block为阻塞式，Unblock为非阻塞式" = ": Block for blocking, Unblock for non-blocking";
    "：操作成功" = ": Operation success";
    "：写入的" = ": written ";
    "执行器" = "Actuator ";
    "读取" = "read ";
    "设置" = "set ";
    "保存" = "save ";
    "清除" = "clear ";
}

foreach ($key in $replacements.Keys) {
    $content = $content.Replace($key, $replacements[$key])
}

Set-Content -Path $file -Value $content -Encoding UTF8
