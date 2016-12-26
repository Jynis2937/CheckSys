#pragma once
#include <string>

/*
*@brief    Marshalers for string.
*@param    [In] Managed string supported by c++/cli
*@param    [Out] STL ANSI string reference
*/
System::Void MarshalString(System::String^, std::string&);
/*
*@brief    Marshalers for string.
*@param    [In] Managed string supported by c++/cli
*@param    [Out] STL UTF-8 string reference
*/
System::Void MarshalString(System::String^, std::wstring&);