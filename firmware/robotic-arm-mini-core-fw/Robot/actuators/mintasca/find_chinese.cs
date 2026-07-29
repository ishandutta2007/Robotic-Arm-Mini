using System;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Collections.Generic;

class Program
{
    static void Main()
    {
        string filePath = @"C:\Users\ishan\Documents\Projects\Robotic-Arm-Mini\firmware\robotic-arm-mini-core-fw\Robot\actuators\mintasca\sca_api.c";
        string content = File.ReadAllText(filePath, Encoding.UTF8);
        var lines = content.Split('\n');
        var regex = new Regex(@"[^\x00-\x7F]");
        var hash = new HashSet<string>();
        foreach (var line in lines)
        {
            if (regex.IsMatch(line))
            {
                hash.Add(line.Trim());
            }
        }
        File.WriteAllLines("remaining_chinese.txt", hash);
    }
}
