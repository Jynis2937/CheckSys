# CheckSys
CheckSys is an system information alram service (checker) between windows server and client. When executing programs, resources in `Resources` folders should exist.

## Windows Server (C++/CLI)
    CheckSysServer.exe (Graphic interface application)  
        warn.wav

## Windows Sender (C++) : 
    hw_profile_console.exe (Console application)
    hw_profile_service.exe (Windows service program with attached batch file)
You should register the service program with sc, which is windows utility.
Setting `ConsoleApp` global flag TRUE, it will be built console application. Or not, windows service program. 

## Linux Sender (C) :
    hw_profile (Console application)
        Alarm1.wav
        Alarm2.wav
        Alarm3.wav
        Alarm4.wav
        Alarm5.wav