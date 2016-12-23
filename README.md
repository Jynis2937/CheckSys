# CheckSys
CheckSys is an system information alram service (checker) between windows server and client.
Server (windows) 
## Windows Sender (C++) : 
                 hw_profile_console.exe (Console application)
                 hw_profile_service.exe (Windows service program with attached batch file)
You should register the service program with sc, which is windows utility.
Setting `ConsoleApp` global flag TRUE, it will be built console application. Or not, windows service program. 

## Linux Sender (C) :
                 hw_profile

