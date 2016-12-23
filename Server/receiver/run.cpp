#include "getter.h"

using namespace System;
using namespace System::Windows::Forms;

[STAThread]
void Main(array<System::String^>^ Args)
{
	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false);

	receiver::getter Getter(2937);
	Application::Run(%Getter);
}