#include "main.h"

using namespace System;
using namespace System::Windows::Forms;

[STAThread]
void Main(array<System::String^>^ Args)
{
	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false);

	CheckSys::CheckSysMain MainForm(2937);
	Application::Run(%MainForm);
}