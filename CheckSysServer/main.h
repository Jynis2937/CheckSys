#pragma once
#include <string>
#include <unordered_map>
#include <utility>
#include <msclr\marshal_cppstd.h>

#include <mmsystem.h>
#include <Windows.h>
#include <Winsock.h>
#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib,"wsock32.lib")

#using <system.dll>

#include "socket_exception.h"
#include "marshal.h"

#define MAX_CLIENT_NUM 65535
#define BACKLOG 10

using std::string;
using std::wstring;
using std::unordered_map;
using std::pair;

namespace CheckSys
{
	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::Threading;
	using namespace System::IO;

	typedef struct _RESOURCE
	{
		SOCKET ClientSocket;
		UINT CpuCycle;
		UINT MemoryUsage;
		UINT HardDiskFreeSpace;
		BOOL IsConnected;
		unordered_map<string, pair<DOUBLE, BOOL>> MountAvailable;
	} RESOURCE, *PRESOURCE;

	static bool Exit = false;
	static bool IsWarnFilePathDefault = true;
	
	static WSADATA WinSock;
	static SOCKADDR_IN Server;
	static SOCKADDR_IN Client;
	static SOCKET ListenSocket;
	static SOCKET ClientSocket[MAX_CLIENT_NUM];
	static bool IsClientOn[MAX_CLIENT_NUM] = { false };
	static UINT AccumulatedClientNumber = 0;
	static UINT CurrentClientNumber = 0;
	static unordered_map<string, RESOURCE> ClientResourceMap;

	public ref class CheckSysMain : public System::Windows::Forms::Form
	{
	private:
		bool IsServerClosed;
		bool IsThreadBegun;
		Thread^ AcceptSocketThread;
		Thread^ GetThread;
		Thread^ ResourceAddThread;
		Thread^ SelectedItemThread;
		Thread^ RepeatedAlarmThread;

		System::Windows::Forms::Label^  LabelMaximum;
		System::Windows::Forms::Label^  LabelCurrentUserNumber;
		System::Windows::Forms::Label^  LabelCurrentDisconnectedUserNumber;
		System::Windows::Forms::Label^  LabelPort;
		System::Windows::Forms::Label^  LabelBacklog;
		System::Windows::Forms::Label^  LabelServerIp;

		System::Windows::Forms::Button^  ButtonServerStart;
		System::Windows::Forms::Button^  ButtonAlarm;
		System::Windows::Forms::Button^  ButtonIsOn;

		System::Windows::Forms::RadioButton^  RadioButtonAlram1;
		System::Windows::Forms::RadioButton^  RadioButtonAlram2;
		System::Windows::Forms::RadioButton^  RadioButtonAlram3;
		System::Windows::Forms::RadioButton^  RadioButtonAlram4;
		System::Windows::Forms::RadioButton^  RadioButtonAlram5;

		System::Windows::Forms::Label^  LabelCpuWarningRatio;
		System::Windows::Forms::Label^  LabelRamWarningRatio;
		System::Windows::Forms::Label^  LabelHddWarningRatio;
		System::Windows::Forms::Label^  LabelRepeatedCycle;
		System::Windows::Forms::Label^  LabelBeepFilePath;
		System::Windows::Forms::Button^  ButtonOpenBeepFileDialog;
		System::Windows::Forms::TextBox^  TextBoxCpuWarningRatio;
		System::Windows::Forms::TextBox^  TextBoxRamWarningRatio;
		System::Windows::Forms::TextBox^  TextBoxHddWarningRatio;
		System::Windows::Forms::TextBox^  TextBoxRepeatedCycle;
		System::Windows::Forms::TextBox^  TextBoxBeepFilePath;
		System::Windows::Forms::CheckBox^  CheckBoxAlarmToAllUsers;
		System::Windows::Forms::Label^  LabelMapperDescription;
		System::Windows::Forms::Button^  ButtonHowToInquiry;

		System::Windows::Forms::DataGridView^  ClientStatusGridView;
		System::Windows::Forms::DataGridViewTextBoxColumn^  Name;
		System::Windows::Forms::DataGridViewTextBoxColumn^  Cpu;
		System::Windows::Forms::DataGridViewTextBoxColumn^  Ram;
		System::Windows::Forms::DataGridViewTextBoxColumn^  Hdd;
		System::Windows::Forms::DataGridViewTextBoxColumn^  Use;
		System::Windows::Forms::DataGridViewComboBoxColumn^  Mount;
		System::Windows::Forms::DataGridViewTextBoxColumn^  IsConnected;
		System::Windows::Forms::DataGridViewTextBoxColumn^  SocketValue;
		

	public:
		CheckSysMain::CheckSysMain(UINT PortToOpen) :
			IsServerClosed(true),
			IsThreadBegun(false),
			GetThread(gcnew Thread(gcnew ThreadStart(this, &CheckSysMain::DataGridViewUpdate))),
			ResourceAddThread(gcnew Thread(gcnew ThreadStart(this, &CheckSysMain::GetResourceFromClient))),
			AcceptSocketThread(gcnew Thread(gcnew ThreadStart(this, &CheckSysMain::AcceptSocket))),
			SelectedItemThread(gcnew Thread(gcnew ThreadStart(this, &CheckSysMain::UpdateSelectedItem))),
			RepeatedAlarmThread(gcnew Thread(gcnew ThreadStart(this, &CheckSysMain::RepeatAlarm)))
		{
			try
			{
				if (0 != WSAStartup(MAKEWORD(2, 2), &WinSock))
				{
					throw SocketException(SocketException::SOCKET_ISSUE_FOR_SERVER::WSAStartup);
				}

				ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

				ZeroMemory(&Server, sizeof(Server));
				Server.sin_family = AF_INET;
				Server.sin_addr.s_addr = htonl(INADDR_ANY);
				Server.sin_port = htons(PortToOpen);

				if (::bind(ListenSocket, (sockaddr*)&Server, sizeof(Server)) == SOCKET_ERROR)
				{
					throw SocketException(SocketException::SOCKET_ISSUE_FOR_SERVER::Bind);
				}

				if (listen(ListenSocket, BACKLOG) == SOCKET_ERROR)
				{
					throw SocketException(SocketException::SOCKET_ISSUE_FOR_SERVER::Listen);
				}
			}
			catch (SocketException& SocketExceptionForServer)
			{
				MessageBox::Show(gcnew String(SocketExceptionForServer.what()));
				exit(EXIT_FAILURE);
			}

			InitializeComponent();
		}

	protected:
		CheckSysMain::~CheckSysMain(System::Void)
		{
			closesocket(ListenSocket);
			for (auto k = 0; k != AccumulatedClientNumber; ++k)
			{
				closesocket(ClientSocket[k]);
			}
			WSACleanup();

			if (components)
			{
				delete components;
			}
		}

	private:
		System::Void CheckSysMain::RepeatAlarm(System::Void)
		{
			array<System::Windows::Forms::RadioButton^>^ RadioButtonAlarmn =
			{ RadioButtonAlram1 , RadioButtonAlram2, RadioButtonAlram3, RadioButtonAlram4, RadioButtonAlram5 };
			do
			{
				if (0 != this->TextBoxRepeatedCycle->Text->Length)
				{
					System::Threading::Thread::Sleep(Convert::ToInt32(this->TextBoxRepeatedCycle->Text) * 1000);
				}
				else
				{
					System::Threading::Thread::Sleep(100);
				}

				bool Checked = false;
				for each(auto Label in RadioButtonAlarmn)
				{
					if (Label->Checked)
					{
						Checked = true;
					}
				}
				if (true == Checked)
				{
					if (0 == this->TextBoxRepeatedCycle->TextLength)
					{
						System::Threading::Thread::Sleep(1000);
						continue;
					}

					if (true == this->CheckBoxAlarmToAllUsers->Checked)
					{
						for (auto& Resource : ClientResourceMap)
						{
							auto n = 1;
							for each(auto Label in RadioButtonAlarmn)
							{
								if (Label->Checked)
								{
									send(Resource.second.ClientSocket, ("Alarm" + std::to_string(n)).c_str(), 7, 0);
								}
								++n;
							}
						}
					}
					else
					{
						DataGridViewSelectedRowCollection^ SelectedRowCollection = this->ClientStatusGridView->SelectedRows;
						string StlString;

						if (0 == SelectedRowCollection->Count)
						{
							continue;
						}
						for (auto i = 0; i != SelectedRowCollection->Count; ++i)
						{
							auto n = 1;
							for each(auto Label in RadioButtonAlarmn)
							{
								if (Label->Checked)
								{
									MarshalString(SelectedRowCollection[i]->Cells[0]->Value->ToString(), StlString);
									send(ClientResourceMap[StlString].ClientSocket, ("Alarm" + std::to_string(n)).c_str(), 7, 0);
								}
								++n;
							}
						}
					}
				}
			} while (true != Exit);

		}
		System::Void CheckSysMain::AcceptSocket(System::Void)
		{
			while (AccumulatedClientNumber < MAX_CLIENT_NUM)
			{
				if (true == Exit)
				{
					return;
				}
				if ((ClientSocket[AccumulatedClientNumber] = accept(ListenSocket, NULL, NULL)) == INVALID_SOCKET)
				{
					//	throw SocketException(SocketException::SOCKET_ISSUE_FOR_SERVER::Accept);
				}
				IsClientOn[AccumulatedClientNumber] = true;
				++AccumulatedClientNumber;
				++CurrentClientNumber;
			}
		}
		System::Void CheckSysMain::UpdateSelectedItem(System::Void)
		{
			do
			{
				auto i = 0;
				for (auto& Resource : ClientResourceMap)
				{
					try
					{
						if (this->ClientStatusGridView->Rows[i]->Cells[5]->FormattedValue->ToString()->Length > 0)
						{
							string SelectedItem;
							MarshalString(this->ClientStatusGridView->Rows[i]->Cells[5]->FormattedValue->ToString(), SelectedItem);
							for (auto& Item : Resource.second.MountAvailable)
							{
								if (Item.first == SelectedItem)
								{
									this->ClientStatusGridView->Rows[i]->Cells[4]->Value = Item.second.first.ToString() + "%";
								}
							}
						}
						++i;
					}
					catch (System::ArgumentOutOfRangeException^ SystemException)
					{
						;
					}
				}

				System::Threading::Thread::Sleep(100);
			} while (true != Exit);
		}
		System::Void CheckSysMain::DataGridViewUpdate(System::Void)
		{
			auto RowCount = 0;
			do
			{
				this->LabelCurrentUserNumber->Text = "�����ο� : " + CurrentClientNumber.ToString();
				this->LabelCurrentDisconnectedUserNumber->Text = "���� ���� : " + (this->ClientStatusGridView->Rows->Count - CurrentClientNumber - 1).ToString();
				while (RowCount < ClientResourceMap.size())
				{
					++RowCount;
					this->ClientStatusGridView->Rows->Add();
				}

				auto i = 0;
				for (auto& Resource : ClientResourceMap)
				{
					this->ClientStatusGridView->Rows[i]->Cells[0]->Value = gcnew System::String(Resource.first.c_str());
					this->ClientStatusGridView->Rows[i]->Cells[1]->Value = static_cast<INT>(Resource.second.CpuCycle) + "%";
					this->ClientStatusGridView->Rows[i]->Cells[2]->Value = static_cast<INT>(Resource.second.MemoryUsage) + "%";
					this->ClientStatusGridView->Rows[i]->Cells[3]->Value = static_cast<INT>(Resource.second.HardDiskFreeSpace) + "%";
					this->ClientStatusGridView->Rows[i]->Cells[6]->Value = Resource.second.IsConnected ? "O" : "X";
					this->ClientStatusGridView->Rows[i]->Cells[7]->Value = Resource.second.ClientSocket.ToString();
					for (auto& MountDirectory : Resource.second.MountAvailable)
					{
						bool Add = true;
						for (auto k = 0; k < this->Mount->Items->Count; ++k)
						{
							if (this->Mount->Items[k]->ToString()
								== gcnew String(MountDirectory.first.c_str()))
							{
								Add = false;
								break;
							}
						}
						if (Add)
						{
							this->Mount->Items->Add(gcnew String(MountDirectory.first.c_str()));
						}
					}
					++i;
				}

				System::Threading::Thread::Sleep(1000);
			} while (true != Exit);
		}
		System::Void CheckSysMain::GetResourceFromClient(System::Void)
		{
			CHAR ReceivedMessage[0x400];
			UINT CpuCycle, RamUsage, HddFreeRatio;
			string Hdd;
			CHAR* Delimiter = "\r\n";
			do
			{
				for (auto k = 0; k != AccumulatedClientNumber; ++k)
				{
					if (true == IsClientOn[k])
					{
						ZeroMemory(ReceivedMessage, sizeof(ReceivedMessage));
						if (recv(ClientSocket[k], ReceivedMessage, sizeof(ReceivedMessage), 0) > 0)
						{
							bool Add = true;
							BOOL ConnectionFinish = FALSE;
							auto i = 0;
							CHAR* Ptr = strtok(ReceivedMessage, Delimiter);
							string UserName = Ptr;
							unordered_map<string, pair<DOUBLE, BOOL>> HddFreeRatioOnMountedDirectory;
							// unordered_map<string, RESOURCE>::const_iterator FindIterator = ClientResourceMap.find(Ptr);

#ifdef _UNICODE
							wstring StlString;
#else
							string StlString;
#endif
							MarshalString(this->TextBoxBeepFilePath->Text, StlString);
							LPTSTR Sound = IsWarnFilePathDefault ? TEXT("./warn.wav") : StlString.c_str();

							Hdd.clear();
							HddFreeRatioOnMountedDirectory.clear();
							for (const auto& Resource : ClientResourceMap)
							{
								if (Ptr == Resource.first)
								{
									Add = false;
								}
							}

							while (Ptr = strtok(NULL, Delimiter))
							{
								switch (i)
								{
								case 0:
									CpuCycle = atoi(Ptr);
									if (this->TextBoxCpuWarningRatio->TextLength == 0)
									{
										break;
									}
									if (CpuCycle >= Convert::ToInt32(this->TextBoxCpuWarningRatio->Text))
									{
										PlaySound(Sound, nullptr, SND_FILENAME);
									}
									break;

								case 1:
									RamUsage = atoi(Ptr);
									if (this->TextBoxRamWarningRatio->TextLength == 0)
									{
										break;
									}
									if (RamUsage >= Convert::ToInt32(this->TextBoxRamWarningRatio->Text))
									{
										PlaySound(Sound, nullptr, SND_FILENAME);
									}
									break;

								case 2:
									HddFreeRatio = atoi(Ptr);
									if (this->TextBoxHddWarningRatio->TextLength == 0)
									{
										break;
									}
									if (HddFreeRatio >= Convert::ToInt32(this->TextBoxHddWarningRatio->Text))
									{
										PlaySound(Sound, nullptr, SND_FILENAME);
									}
									break;

								case 3:
									if (string(Ptr) == "TRUE")
									{
										ConnectionFinish = FALSE;
									}
									else
									{
										ConnectionFinish = TRUE;
									}
									break;

								default:
								{
									string Remainder = Ptr;
									Hdd += Remainder;
									INT Usage = atoi(Remainder.substr(Remainder.find(":") + 1).c_str());
									Remainder.erase(Remainder.find(":"), Remainder.length());

									HddFreeRatioOnMountedDirectory.insert(
										unordered_map<string, pair<DOUBLE, BOOL>>::value_type((string)Remainder, pair<DOUBLE, BOOL>{Usage, TRUE})
									);
								}
								}

								++i;
							}

							if (Add == true)
							{
								ClientResourceMap.insert
								(
									unordered_map<string, RESOURCE>::value_type
									(
										UserName, RESOURCE{ ClientSocket[k], CpuCycle, RamUsage, HddFreeRatio, !ConnectionFinish, HddFreeRatioOnMountedDirectory }
									)
								);
							}
							else
							{
								ClientResourceMap[UserName] =
									RESOURCE{ ClientSocket[k], CpuCycle, RamUsage, HddFreeRatio, !ConnectionFinish, HddFreeRatioOnMountedDirectory };
								if (!ConnectionFinish == FALSE)
								{
									--CurrentClientNumber;
								}
							}
						}
					}
					System::Threading::Thread::Sleep(100);
				}
			} while (true != Exit);
		}

	protected:
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			System::Windows::Forms::DataGridViewCellStyle^  dataGridViewCellStyle1 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^  dataGridViewCellStyle2 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^  dataGridViewCellStyle6 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^  dataGridViewCellStyle7 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^  dataGridViewCellStyle8 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^  dataGridViewCellStyle3 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^  dataGridViewCellStyle4 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^  dataGridViewCellStyle5 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			this->ClientStatusGridView = (gcnew System::Windows::Forms::DataGridView());
			this->Name = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Cpu = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Ram = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Hdd = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Use = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Mount = (gcnew System::Windows::Forms::DataGridViewComboBoxColumn());
			this->IsConnected = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->SocketValue = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->ButtonServerStart = (gcnew System::Windows::Forms::Button());
			this->LabelCurrentUserNumber = (gcnew System::Windows::Forms::Label());
			this->ButtonAlarm = (gcnew System::Windows::Forms::Button());
			this->LabelPort = (gcnew System::Windows::Forms::Label());
			this->LabelBacklog = (gcnew System::Windows::Forms::Label());
			this->RadioButtonAlram1 = (gcnew System::Windows::Forms::RadioButton());
			this->RadioButtonAlram2 = (gcnew System::Windows::Forms::RadioButton());
			this->RadioButtonAlram3 = (gcnew System::Windows::Forms::RadioButton());
			this->RadioButtonAlram4 = (gcnew System::Windows::Forms::RadioButton());
			this->RadioButtonAlram5 = (gcnew System::Windows::Forms::RadioButton());
			this->ButtonIsOn = (gcnew System::Windows::Forms::Button());
			this->TextBoxCpuWarningRatio = (gcnew System::Windows::Forms::TextBox());
			this->LabelCpuWarningRatio = (gcnew System::Windows::Forms::Label());
			this->LabelMaximum = (gcnew System::Windows::Forms::Label());
			this->LabelRamWarningRatio = (gcnew System::Windows::Forms::Label());
			this->TextBoxRamWarningRatio = (gcnew System::Windows::Forms::TextBox());
			this->LabelHddWarningRatio = (gcnew System::Windows::Forms::Label());
			this->TextBoxHddWarningRatio = (gcnew System::Windows::Forms::TextBox());
			this->LabelBeepFilePath = (gcnew System::Windows::Forms::Label());
			this->TextBoxBeepFilePath = (gcnew System::Windows::Forms::TextBox());
			this->ButtonOpenBeepFileDialog = (gcnew System::Windows::Forms::Button());
			this->TextBoxRepeatedCycle = (gcnew System::Windows::Forms::TextBox());
			this->LabelRepeatedCycle = (gcnew System::Windows::Forms::Label());
			this->CheckBoxAlarmToAllUsers = (gcnew System::Windows::Forms::CheckBox());
			this->LabelServerIp = (gcnew System::Windows::Forms::Label());
			this->LabelMapperDescription = (gcnew System::Windows::Forms::Label());
			this->ButtonHowToInquiry = (gcnew System::Windows::Forms::Button());
			this->LabelCurrentDisconnectedUserNumber = (gcnew System::Windows::Forms::Label());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->ClientStatusGridView))->BeginInit();
			this->SuspendLayout();
			// 
			// ClientStatusGridView
			// 
			this->ClientStatusGridView->AllowUserToOrderColumns = true;
			dataGridViewCellStyle1->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->ClientStatusGridView->AlternatingRowsDefaultCellStyle = dataGridViewCellStyle1;
			this->ClientStatusGridView->BackgroundColor = System::Drawing::SystemColors::ButtonHighlight;
			this->ClientStatusGridView->BorderStyle = System::Windows::Forms::BorderStyle::Fixed3D;
			this->ClientStatusGridView->ColumnHeadersBorderStyle = System::Windows::Forms::DataGridViewHeaderBorderStyle::Single;
			dataGridViewCellStyle2->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			dataGridViewCellStyle2->BackColor = System::Drawing::Color::White;
			dataGridViewCellStyle2->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			dataGridViewCellStyle2->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			dataGridViewCellStyle2->SelectionBackColor = System::Drawing::SystemColors::Highlight;
			dataGridViewCellStyle2->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle2->WrapMode = System::Windows::Forms::DataGridViewTriState::True;
			this->ClientStatusGridView->ColumnHeadersDefaultCellStyle = dataGridViewCellStyle2;
			this->ClientStatusGridView->ColumnHeadersHeight = 36;
			this->ClientStatusGridView->Columns->AddRange(gcnew cli::array< System::Windows::Forms::DataGridViewColumn^  >(8) {
				this->Name,
					this->Cpu, this->Ram, this->Hdd, this->Use, this->Mount, this->IsConnected, this->SocketValue
			});
			dataGridViewCellStyle6->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleRight;
			dataGridViewCellStyle6->BackColor = System::Drawing::SystemColors::Window;
			dataGridViewCellStyle6->Font = (gcnew System::Drawing::Font(L"Gulim", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			dataGridViewCellStyle6->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			dataGridViewCellStyle6->SelectionBackColor = System::Drawing::SystemColors::Highlight;
			dataGridViewCellStyle6->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle6->WrapMode = System::Windows::Forms::DataGridViewTriState::False;
			this->ClientStatusGridView->DefaultCellStyle = dataGridViewCellStyle6;
			this->ClientStatusGridView->EnableHeadersVisualStyles = false;
			this->ClientStatusGridView->GridColor = System::Drawing::SystemColors::ActiveCaption;
			this->ClientStatusGridView->Location = System::Drawing::Point(-1, -1);
			this->ClientStatusGridView->Name = L"ClientStatusGridView";
			this->ClientStatusGridView->RowHeadersBorderStyle = System::Windows::Forms::DataGridViewHeaderBorderStyle::Single;
			dataGridViewCellStyle7->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleRight;
			dataGridViewCellStyle7->BackColor = System::Drawing::SystemColors::Control;
			dataGridViewCellStyle7->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 10.2F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			dataGridViewCellStyle7->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			dataGridViewCellStyle7->SelectionBackColor = System::Drawing::SystemColors::Highlight;
			dataGridViewCellStyle7->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle7->WrapMode = System::Windows::Forms::DataGridViewTriState::True;
			this->ClientStatusGridView->RowHeadersDefaultCellStyle = dataGridViewCellStyle7;
			dataGridViewCellStyle8->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleRight;
			dataGridViewCellStyle8->BackColor = System::Drawing::Color::White;
			dataGridViewCellStyle8->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 10.2F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			dataGridViewCellStyle8->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->ClientStatusGridView->RowsDefaultCellStyle = dataGridViewCellStyle8;
			this->ClientStatusGridView->RowTemplate->DefaultCellStyle->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->ClientStatusGridView->RowTemplate->Height = 27;
			this->ClientStatusGridView->Size = System::Drawing::Size(809, 415);
			this->ClientStatusGridView->TabIndex = 0;
			this->ClientStatusGridView->DataError += gcnew System::Windows::Forms::DataGridViewDataErrorEventHandler(this, &CheckSysMain::ClientStatusGridView_DataError);
			// 
			// Name
			// 
			dataGridViewCellStyle3->ForeColor = System::Drawing::SystemColors::MenuHighlight;
			this->Name->DefaultCellStyle = dataGridViewCellStyle3;
			this->Name->HeaderText = L"����� �̸�";
			this->Name->Name = L"Name";
			// 
			// Cpu
			// 
			this->Cpu->HeaderText = L"CPU";
			this->Cpu->Name = L"Cpu";
			this->Cpu->Width = 75;
			// 
			// Ram
			// 
			this->Ram->HeaderText = L"�޸�";
			this->Ram->Name = L"Ram";
			this->Ram->Width = 75;
			// 
			// Hdd
			// 
			this->Hdd->HeaderText = L"��ũ";
			this->Hdd->Name = L"Hdd";
			this->Hdd->Width = 75;
			// 
			// Use
			// 
			this->Use->HeaderText = L"��뷮";
			this->Use->Name = L"Use";
			this->Use->Width = 75;
			// 
			// Mount
			// 
			dataGridViewCellStyle4->BackColor = System::Drawing::SystemColors::ButtonHighlight;
			dataGridViewCellStyle4->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->Mount->DefaultCellStyle = dataGridViewCellStyle4;
			this->Mount->DisplayStyle = System::Windows::Forms::DataGridViewComboBoxDisplayStyle::ComboBox;
			this->Mount->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
			this->Mount->HeaderText = L"����Ʈ";
			this->Mount->Name = L"Mount";
			this->Mount->Resizable = System::Windows::Forms::DataGridViewTriState::True;
			this->Mount->SortMode = System::Windows::Forms::DataGridViewColumnSortMode::Automatic;
			this->Mount->Width = 150;
			// 
			// IsConnected
			// 
			dataGridViewCellStyle5->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
			this->IsConnected->DefaultCellStyle = dataGridViewCellStyle5;
			this->IsConnected->HeaderText = L"���ӻ���";
			this->IsConnected->Name = L"IsConnected";
			// 
			// SocketValue
			// 
			this->SocketValue->HeaderText = L"���ϰ�";
			this->SocketValue->Name = L"SocketValue";
			this->SocketValue->Width = 115;
			// 
			// ButtonServerStart
			// 
			this->ButtonServerStart->BackColor = System::Drawing::SystemColors::ButtonHighlight;
			this->ButtonServerStart->FlatAppearance->BorderColor = System::Drawing::SystemColors::ActiveCaption;
			this->ButtonServerStart->FlatAppearance->BorderSize = 2;
			this->ButtonServerStart->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
			this->ButtonServerStart->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 10.8F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->ButtonServerStart->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->ButtonServerStart->Location = System::Drawing::Point(35, 435);
			this->ButtonServerStart->Name = L"ButtonServerStart";
			this->ButtonServerStart->Size = System::Drawing::Size(157, 63);
			this->ButtonServerStart->TabIndex = 1;
			this->ButtonServerStart->Text = L"���� ����";
			this->ButtonServerStart->UseVisualStyleBackColor = false;
			this->ButtonServerStart->Click += gcnew System::EventHandler(this, &CheckSysMain::ButtonServerStart_Click);
			// 
			// LabelCurrentUserNumber
			// 
			this->LabelCurrentUserNumber->AutoSize = true;
			this->LabelCurrentUserNumber->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->LabelCurrentUserNumber->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->LabelCurrentUserNumber->Location = System::Drawing::Point(31, 576);
			this->LabelCurrentUserNumber->Name = L"LabelCurrentUserNumber";
			this->LabelCurrentUserNumber->Size = System::Drawing::Size(72, 20);
			this->LabelCurrentUserNumber->TabIndex = 2;
			this->LabelCurrentUserNumber->Text = L"���� �� : ";
			// 
			// ButtonAlarm
			// 
			this->ButtonAlarm->BackColor = System::Drawing::SystemColors::ButtonHighlight;
			this->ButtonAlarm->FlatAppearance->BorderColor = System::Drawing::SystemColors::ActiveCaption;
			this->ButtonAlarm->FlatAppearance->BorderSize = 2;
			this->ButtonAlarm->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
			this->ButtonAlarm->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 10.2F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->ButtonAlarm->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->ButtonAlarm->Location = System::Drawing::Point(224, 436);
			this->ButtonAlarm->Name = L"ButtonAlarm";
			this->ButtonAlarm->Size = System::Drawing::Size(124, 62);
			this->ButtonAlarm->TabIndex = 4;
			this->ButtonAlarm->Text = L"�˶� ������";
			this->ButtonAlarm->UseVisualStyleBackColor = false;
			this->ButtonAlarm->Click += gcnew System::EventHandler(this, &CheckSysMain::ButtonAlarm_Click);
			// 
			// LabelPort
			// 
			this->LabelPort->AutoSize = true;
			this->LabelPort->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->LabelPort->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->LabelPort->Location = System::Drawing::Point(31, 672);
			this->LabelPort->Name = L"LabelPort";
			this->LabelPort->Size = System::Drawing::Size(47, 20);
			this->LabelPort->TabIndex = 2;
			this->LabelPort->Text = L"��Ʈ :";
			// 
			// LabelBacklog
			// 
			this->LabelBacklog->AutoSize = true;
			this->LabelBacklog->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->LabelBacklog->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->LabelBacklog->Location = System::Drawing::Point(32, 702);
			this->LabelBacklog->Name = L"LabelBacklog";
			this->LabelBacklog->Size = System::Drawing::Size(67, 20);
			this->LabelBacklog->TabIndex = 13;
			this->LabelBacklog->Text = L"��α� : ";
			// 
			// RadioButtonAlram1
			// 
			this->RadioButtonAlram1->AutoSize = true;
			this->RadioButtonAlram1->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->RadioButtonAlram1->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->RadioButtonAlram1->Location = System::Drawing::Point(376, 440);
			this->RadioButtonAlram1->Name = L"RadioButtonAlram1";
			this->RadioButtonAlram1->Size = System::Drawing::Size(73, 24);
			this->RadioButtonAlram1->TabIndex = 5;
			this->RadioButtonAlram1->TabStop = true;
			this->RadioButtonAlram1->Text = L"�˶� 1";
			this->RadioButtonAlram1->UseVisualStyleBackColor = true;
			// 
			// RadioButtonAlram2
			// 
			this->RadioButtonAlram2->AutoSize = true;
			this->RadioButtonAlram2->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->RadioButtonAlram2->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->RadioButtonAlram2->Location = System::Drawing::Point(455, 439);
			this->RadioButtonAlram2->Name = L"RadioButtonAlram2";
			this->RadioButtonAlram2->Size = System::Drawing::Size(73, 24);
			this->RadioButtonAlram2->TabIndex = 5;
			this->RadioButtonAlram2->TabStop = true;
			this->RadioButtonAlram2->Text = L"�˶� 2";
			this->RadioButtonAlram2->UseVisualStyleBackColor = true;
			// 
			// RadioButtonAlram3
			// 
			this->RadioButtonAlram3->AutoSize = true;
			this->RadioButtonAlram3->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->RadioButtonAlram3->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->RadioButtonAlram3->Location = System::Drawing::Point(534, 439);
			this->RadioButtonAlram3->Name = L"RadioButtonAlram3";
			this->RadioButtonAlram3->Size = System::Drawing::Size(73, 24);
			this->RadioButtonAlram3->TabIndex = 5;
			this->RadioButtonAlram3->TabStop = true;
			this->RadioButtonAlram3->Text = L"�˶� 3";
			this->RadioButtonAlram3->UseVisualStyleBackColor = true;
			// 
			// RadioButtonAlram4
			// 
			this->RadioButtonAlram4->AutoSize = true;
			this->RadioButtonAlram4->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->RadioButtonAlram4->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->RadioButtonAlram4->Location = System::Drawing::Point(376, 470);
			this->RadioButtonAlram4->Name = L"RadioButtonAlram4";
			this->RadioButtonAlram4->Size = System::Drawing::Size(73, 24);
			this->RadioButtonAlram4->TabIndex = 5;
			this->RadioButtonAlram4->TabStop = true;
			this->RadioButtonAlram4->Text = L"�˶� 4";
			this->RadioButtonAlram4->UseVisualStyleBackColor = true;
			// 
			// RadioButtonAlram5
			// 
			this->RadioButtonAlram5->AutoSize = true;
			this->RadioButtonAlram5->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->RadioButtonAlram5->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->RadioButtonAlram5->Location = System::Drawing::Point(455, 469);
			this->RadioButtonAlram5->Name = L"RadioButtonAlram5";
			this->RadioButtonAlram5->Size = System::Drawing::Size(73, 24);
			this->RadioButtonAlram5->TabIndex = 5;
			this->RadioButtonAlram5->TabStop = true;
			this->RadioButtonAlram5->Text = L"�˶� 5";
			this->RadioButtonAlram5->UseVisualStyleBackColor = true;
			// 
			// ButtonIsOn
			// 
			this->ButtonIsOn->BackColor = System::Drawing::Color::MediumTurquoise;
			this->ButtonIsOn->BackgroundImageLayout = System::Windows::Forms::ImageLayout::Center;
			this->ButtonIsOn->FlatAppearance->BorderColor = System::Drawing::SystemColors::ActiveCaption;
			this->ButtonIsOn->FlatAppearance->BorderSize = 0;
			this->ButtonIsOn->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
			this->ButtonIsOn->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 7.8F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->ButtonIsOn->ForeColor = System::Drawing::Color::White;
			this->ButtonIsOn->Location = System::Drawing::Point(36, 506);
			this->ButtonIsOn->Name = L"ButtonIsOn";
			this->ButtonIsOn->Size = System::Drawing::Size(156, 24);
			this->ButtonIsOn->TabIndex = 1;
			this->ButtonIsOn->Text = L"Off";
			this->ButtonIsOn->UseVisualStyleBackColor = false;
			this->ButtonIsOn->Click += gcnew System::EventHandler(this, &CheckSysMain::ButtonServerStart_Click);
			// 
			// TextBoxCpuWarningRatio
			// 
			this->TextBoxCpuWarningRatio->BackColor = System::Drawing::Color::AliceBlue;
			this->TextBoxCpuWarningRatio->BorderStyle = System::Windows::Forms::BorderStyle::None;
			this->TextBoxCpuWarningRatio->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 10.2F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->TextBoxCpuWarningRatio->ForeColor = System::Drawing::Color::DimGray;
			this->TextBoxCpuWarningRatio->Location = System::Drawing::Point(223, 548);
			this->TextBoxCpuWarningRatio->Name = L"TextBoxCpuWarningRatio";
			this->TextBoxCpuWarningRatio->Size = System::Drawing::Size(140, 23);
			this->TextBoxCpuWarningRatio->TabIndex = 7;
			this->TextBoxCpuWarningRatio->TextAlign = System::Windows::Forms::HorizontalAlignment::Right;
			// 
			// LabelCpuWarningRatio
			// 
			this->LabelCpuWarningRatio->AutoSize = true;
			this->LabelCpuWarningRatio->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->LabelCpuWarningRatio->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->LabelCpuWarningRatio->Location = System::Drawing::Point(222, 521);
			this->LabelCpuWarningRatio->Name = L"LabelCpuWarningRatio";
			this->LabelCpuWarningRatio->Size = System::Drawing::Size(117, 20);
			this->LabelCpuWarningRatio->TabIndex = 8;
			this->LabelCpuWarningRatio->Text = L"CPU ����� (%)";
			// 
			// LabelMaximum
			// 
			this->LabelMaximum->AutoSize = true;
			this->LabelMaximum->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->LabelMaximum->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->LabelMaximum->Location = System::Drawing::Point(32, 546);
			this->LabelMaximum->Name = L"LabelMaximum";
			this->LabelMaximum->Size = System::Drawing::Size(82, 20);
			this->LabelMaximum->TabIndex = 2;
			this->LabelMaximum->Text = L"�ִ� �ο� :";
			// 
			// LabelRamWarningRatio
			// 
			this->LabelRamWarningRatio->AutoSize = true;
			this->LabelRamWarningRatio->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->LabelRamWarningRatio->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->LabelRamWarningRatio->Location = System::Drawing::Point(595, 521);
			this->LabelRamWarningRatio->Name = L"LabelRamWarningRatio";
			this->LabelRamWarningRatio->Size = System::Drawing::Size(132, 20);
			this->LabelRamWarningRatio->TabIndex = 10;
			this->LabelRamWarningRatio->Text = L"�޸� ����� (%)";
			// 
			// TextBoxRamWarningRatio
			// 
			this->TextBoxRamWarningRatio->BackColor = System::Drawing::Color::AliceBlue;
			this->TextBoxRamWarningRatio->BorderStyle = System::Windows::Forms::BorderStyle::None;
			this->TextBoxRamWarningRatio->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 10.2F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->TextBoxRamWarningRatio->ForeColor = System::Drawing::Color::DimGray;
			this->TextBoxRamWarningRatio->Location = System::Drawing::Point(597, 548);
			this->TextBoxRamWarningRatio->Name = L"TextBoxRamWarningRatio";
			this->TextBoxRamWarningRatio->Size = System::Drawing::Size(140, 23);
			this->TextBoxRamWarningRatio->TabIndex = 9;
			this->TextBoxRamWarningRatio->TextAlign = System::Windows::Forms::HorizontalAlignment::Right;
			// 
			// LabelHddWarningRatio
			// 
			this->LabelHddWarningRatio->AutoSize = true;
			this->LabelHddWarningRatio->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->LabelHddWarningRatio->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->LabelHddWarningRatio->Location = System::Drawing::Point(406, 522);
			this->LabelHddWarningRatio->Name = L"LabelHddWarningRatio";
			this->LabelHddWarningRatio->Size = System::Drawing::Size(132, 20);
			this->LabelHddWarningRatio->TabIndex = 12;
			this->LabelHddWarningRatio->Text = L"��ũ ����� (%)";
			// 
			// TextBoxHddWarningRatio
			// 
			this->TextBoxHddWarningRatio->BackColor = System::Drawing::Color::AliceBlue;
			this->TextBoxHddWarningRatio->BorderStyle = System::Windows::Forms::BorderStyle::None;
			this->TextBoxHddWarningRatio->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 10.2F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->TextBoxHddWarningRatio->ForeColor = System::Drawing::Color::DimGray;
			this->TextBoxHddWarningRatio->Location = System::Drawing::Point(408, 549);
			this->TextBoxHddWarningRatio->Name = L"TextBoxHddWarningRatio";
			this->TextBoxHddWarningRatio->Size = System::Drawing::Size(140, 23);
			this->TextBoxHddWarningRatio->TabIndex = 11;
			this->TextBoxHddWarningRatio->TextAlign = System::Windows::Forms::HorizontalAlignment::Right;
			// 
			// LabelBeepFilePath
			// 
			this->LabelBeepFilePath->AutoSize = true;
			this->LabelBeepFilePath->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->LabelBeepFilePath->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->LabelBeepFilePath->Location = System::Drawing::Point(220, 599);
			this->LabelBeepFilePath->Name = L"LabelBeepFilePath";
			this->LabelBeepFilePath->Size = System::Drawing::Size(250, 20);
			this->LabelBeepFilePath->TabIndex = 17;
			this->LabelBeepFilePath->Text = L"����� ���� ��� (�⺻ : warn.wav)";
			// 
			// TextBoxBeepFilePath
			// 
			this->TextBoxBeepFilePath->BackColor = System::Drawing::Color::AliceBlue;
			this->TextBoxBeepFilePath->BorderStyle = System::Windows::Forms::BorderStyle::None;
			this->TextBoxBeepFilePath->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 10.2F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->TextBoxBeepFilePath->ForeColor = System::Drawing::Color::DimGray;
			this->TextBoxBeepFilePath->Location = System::Drawing::Point(221, 627);
			this->TextBoxBeepFilePath->Name = L"TextBoxBeepFilePath";
			this->TextBoxBeepFilePath->ReadOnly = true;
			this->TextBoxBeepFilePath->Size = System::Drawing::Size(326, 23);
			this->TextBoxBeepFilePath->TabIndex = 16;
			// 
			// ButtonOpenBeepFileDialog
			// 
			this->ButtonOpenBeepFileDialog->BackColor = System::Drawing::SystemColors::ButtonHighlight;
			this->ButtonOpenBeepFileDialog->FlatAppearance->BorderColor = System::Drawing::SystemColors::ActiveCaption;
			this->ButtonOpenBeepFileDialog->FlatAppearance->BorderSize = 2;
			this->ButtonOpenBeepFileDialog->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
			this->ButtonOpenBeepFileDialog->Font = (gcnew System::Drawing::Font(L"Malgun Gothic Semilight", 7.8F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(129)));
			this->ButtonOpenBeepFileDialog->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->ButtonOpenBeepFileDialog->ImageAlign = System::Drawing::ContentAlignment::TopCenter;
			this->ButtonOpenBeepFileDialog->Location = System::Drawing::Point(518, 626);
			this->ButtonOpenBeepFileDialog->Name = L"ButtonOpenBeepFileDialog";
			this->ButtonOpenBeepFileDialog->Size = System::Drawing::Size(29, 23);
			this->ButtonOpenBeepFileDialog->TabIndex = 18;
			this->ButtonOpenBeepFileDialog->Text = L"\'\'\'";
			this->ButtonOpenBeepFileDialog->UseVisualStyleBackColor = false;
			this->ButtonOpenBeepFileDialog->Click += gcnew System::EventHandler(this, &CheckSysMain::ButtonOpenBeepFileDialog_Click);
			// 
			// TextBoxRepeatedCycle
			// 
			this->TextBoxRepeatedCycle->BackColor = System::Drawing::Color::AliceBlue;
			this->TextBoxRepeatedCycle->BorderStyle = System::Windows::Forms::BorderStyle::None;
			this->TextBoxRepeatedCycle->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 10.2F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->TextBoxRepeatedCycle->ForeColor = System::Drawing::Color::DimGray;
			this->TextBoxRepeatedCycle->Location = System::Drawing::Point(597, 627);
			this->TextBoxRepeatedCycle->Name = L"TextBoxRepeatedCycle";
			this->TextBoxRepeatedCycle->Size = System::Drawing::Size(140, 23);
			this->TextBoxRepeatedCycle->TabIndex = 11;
			this->TextBoxRepeatedCycle->TextAlign = System::Windows::Forms::HorizontalAlignment::Right;
			// 
			// LabelRepeatedCycle
			// 
			this->LabelRepeatedCycle->AutoSize = true;
			this->LabelRepeatedCycle->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->LabelRepeatedCycle->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->LabelRepeatedCycle->Location = System::Drawing::Point(595, 600);
			this->LabelRepeatedCycle->Name = L"LabelRepeatedCycle";
			this->LabelRepeatedCycle->Size = System::Drawing::Size(139, 20);
			this->LabelRepeatedCycle->TabIndex = 12;
			this->LabelRepeatedCycle->Text = L"�ݺ� �ֱ� (�� ����)";
			// 
			// CheckBoxAlarmToAllUsers
			// 
			this->CheckBoxAlarmToAllUsers->AutoSize = true;
			this->CheckBoxAlarmToAllUsers->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
			this->CheckBoxAlarmToAllUsers->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->CheckBoxAlarmToAllUsers->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->CheckBoxAlarmToAllUsers->Location = System::Drawing::Point(616, 440);
			this->CheckBoxAlarmToAllUsers->Name = L"CheckBoxAlarmToAllUsers";
			this->CheckBoxAlarmToAllUsers->Size = System::Drawing::Size(122, 24);
			this->CheckBoxAlarmToAllUsers->TabIndex = 19;
			this->CheckBoxAlarmToAllUsers->Text = L"��� ��������";
			this->CheckBoxAlarmToAllUsers->UseVisualStyleBackColor = true;
			// 
			// LabelServerIp
			// 
			this->LabelServerIp->AutoSize = true;
			this->LabelServerIp->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->LabelServerIp->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->LabelServerIp->Location = System::Drawing::Point(31, 640);
			this->LabelServerIp->Name = L"LabelServerIp";
			this->LabelServerIp->Size = System::Drawing::Size(65, 20);
			this->LabelServerIp->TabIndex = 2;
			this->LabelServerIp->Text = L"���� IP :";
			// 
			// LabelMapperDescription
			// 
			this->LabelMapperDescription->AutoSize = true;
			this->LabelMapperDescription->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->LabelMapperDescription->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->LabelMapperDescription->Location = System::Drawing::Point(610, 472);
			this->LabelMapperDescription->Name = L"LabelMapperDescription";
			this->LabelMapperDescription->Size = System::Drawing::Size(170, 20);
			this->LabelMapperDescription->TabIndex = 13;
			this->LabelMapperDescription->Text = L"��Ī���� : label.mapper";
			// 
			// ButtonHowToInquiry
			// 
			this->ButtonHowToInquiry->BackColor = System::Drawing::SystemColors::ButtonHighlight;
			this->ButtonHowToInquiry->FlatAppearance->BorderColor = System::Drawing::SystemColors::ActiveCaption;
			this->ButtonHowToInquiry->FlatAppearance->BorderSize = 2;
			this->ButtonHowToInquiry->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
			this->ButtonHowToInquiry->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 13.8F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->ButtonHowToInquiry->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->ButtonHowToInquiry->Location = System::Drawing::Point(735, 725);
			this->ButtonHowToInquiry->Name = L"ButtonHowToInquiry";
			this->ButtonHowToInquiry->Size = System::Drawing::Size(45, 45);
			this->ButtonHowToInquiry->TabIndex = 20;
			this->ButtonHowToInquiry->Text = L"\?";
			this->ButtonHowToInquiry->UseVisualStyleBackColor = false;
			this->ButtonHowToInquiry->Click += gcnew System::EventHandler(this, &CheckSysMain::ButtonHowToInquiry_Click);
			// 
			// LabelCurrentDisconnectedUserNumber
			// 
			this->LabelCurrentDisconnectedUserNumber->AutoSize = true;
			this->LabelCurrentDisconnectedUserNumber->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(129)));
			this->LabelCurrentDisconnectedUserNumber->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->LabelCurrentDisconnectedUserNumber->Location = System::Drawing::Point(31, 608);
			this->LabelCurrentDisconnectedUserNumber->Name = L"LabelCurrentDisconnectedUserNumber";
			this->LabelCurrentDisconnectedUserNumber->Size = System::Drawing::Size(87, 20);
			this->LabelCurrentDisconnectedUserNumber->TabIndex = 2;
			this->LabelCurrentDisconnectedUserNumber->Text = L"���� ���� : ";
			// 
			// CheckSysMain
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(8, 15);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->BackColor = System::Drawing::SystemColors::ButtonHighlight;
			this->ClientSize = System::Drawing::Size(803, 782);
			this->Controls->Add(this->ButtonHowToInquiry);
			this->Controls->Add(this->CheckBoxAlarmToAllUsers);
			this->Controls->Add(this->ButtonOpenBeepFileDialog);
			this->Controls->Add(this->LabelBeepFilePath);
			this->Controls->Add(this->TextBoxBeepFilePath);
			this->Controls->Add(this->LabelMapperDescription);
			this->Controls->Add(this->LabelBacklog);
			this->Controls->Add(this->LabelRepeatedCycle);
			this->Controls->Add(this->TextBoxRepeatedCycle);
			this->Controls->Add(this->LabelHddWarningRatio);
			this->Controls->Add(this->TextBoxHddWarningRatio);
			this->Controls->Add(this->LabelRamWarningRatio);
			this->Controls->Add(this->TextBoxRamWarningRatio);
			this->Controls->Add(this->LabelCpuWarningRatio);
			this->Controls->Add(this->TextBoxCpuWarningRatio);
			this->Controls->Add(this->RadioButtonAlram5);
			this->Controls->Add(this->RadioButtonAlram4);
			this->Controls->Add(this->RadioButtonAlram3);
			this->Controls->Add(this->RadioButtonAlram2);
			this->Controls->Add(this->RadioButtonAlram1);
			this->Controls->Add(this->ButtonAlarm);
			this->Controls->Add(this->LabelMaximum);
			this->Controls->Add(this->LabelPort);
			this->Controls->Add(this->LabelServerIp);
			this->Controls->Add(this->LabelCurrentDisconnectedUserNumber);
			this->Controls->Add(this->LabelCurrentUserNumber);
			this->Controls->Add(this->ButtonIsOn);
			this->Controls->Add(this->ButtonServerStart);
			this->Controls->Add(this->ClientStatusGridView);
			this->Name = L"CheckSysMain";
			this->Text = L"����Ʈ���� �˸� �ý���";
			this->FormClosed += gcnew System::Windows::Forms::FormClosedEventHandler(this, &CheckSysMain::CheckSysMain_FormClosed);
			this->Load += gcnew System::EventHandler(this, &CheckSysMain::CheckSysMain_Load);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->ClientStatusGridView))->EndInit();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	private:
		System::String^ MyOwnIPAddress()
		{
			PHOSTENT HostInformation;
			CHAR HostName[0x80]{ 0 };

			if (0 != gethostname(HostName, sizeof(HostName)))
			{
				return nullptr;
			}
			HostInformation = gethostbyname(HostName);

			return gcnew String(inet_ntoa(*(struct in_addr*)HostInformation->h_addr_list[0]));
		}

#define MAPPER_FILE_NAME "label.mapper"
	private:
		System::Void CheckSysMain_Load(System::Object^  sender, System::EventArgs^  e)
		{
			this->LabelMaximum->Text = "�ִ��ο� : " + MAX_CLIENT_NUM.ToString();
			this->LabelCurrentUserNumber->Text = "���� �� : 0";
			this->LabelCurrentDisconnectedUserNumber->Text = "���� ���� : 0";
			this->LabelServerIp->Text = "���� IP : " + MyOwnIPAddress();
			this->LabelPort->Text = "���� ��Ʈ : 2937";
			this->LabelBacklog->Text = "��α� : " + BACKLOG.ToString();

			try
			{
				array<System::Windows::Forms::RadioButton^>^ RadioButtonAlarmn =
				{ RadioButtonAlram1 , RadioButtonAlram2, RadioButtonAlram3, RadioButtonAlram4, RadioButtonAlram5 };
				StreamReader^ AlarmLabelReader = File::OpenText(MAPPER_FILE_NAME);
				String^ MappedString;
				auto Count = 0;

				while (nullptr != (MappedString = AlarmLabelReader->ReadLine()) && Count < 5)
				{
					RadioButtonAlarmn[Count]->Text = MappedString;
					++Count;
				}

				AlarmLabelReader->Close();
			}
			catch (Exception^ SystemIOException)
			{
				if (dynamic_cast<FileNotFoundException^>(SystemIOException))
				{
					MessageBox::Show("Mapper ������ ã�� �� �����ϴ�.");
				}
				else
				{
					MessageBox::Show("Mapper ������ �д� �� �����Ͽ����ϴ�.");
				}
			}
		}
		System::Void CheckSysMain_FormClosed(System::Object^  sender, System::Windows::Forms::FormClosedEventArgs^  e)
		{
			Exit = true;
		}
		System::Void ButtonServerStart_Click(System::Object^  sender, System::EventArgs^  e)
		{
			if (IsThreadBegun == false)
			{
				IsThreadBegun = true;

				AcceptSocketThread->Start();
				GetThread->Start();
				ResourceAddThread->Start();
				SelectedItemThread->Start();
				RepeatedAlarmThread->Start();

				MessageBox::Show("������ �������ϴ�.");

				this->ButtonIsOn->BackColor = System::Drawing::Color::DeepSkyBlue;
				this->ButtonIsOn->Text = "On";
			}
			else
			{
				MessageBox::Show("�̹� ������ �����ֽ��ϴ�.");
			}
		}
		System::Void ButtonAlarm_Click(System::Object^  sender, System::EventArgs^  e)
		{
			array<System::Windows::Forms::RadioButton^>^ RadioButtonAlarmn =
			{ RadioButtonAlram1 , RadioButtonAlram2, RadioButtonAlram3, RadioButtonAlram4, RadioButtonAlram5 };
			bool Checked = false;
			for each(auto Label in RadioButtonAlarmn)
			{
				if (Label->Checked)
				{
					Checked = true;
				}
			}
			if (false == Checked)
			{
				MessageBox::Show("�ϳ��� �˸� �޼����� üũ���ּ���.");
				return;
			}

			if (true == this->CheckBoxAlarmToAllUsers->Checked)
			{
				for (auto& Resource : ClientResourceMap)
				{
					auto n = 1;
					for each(auto Label in RadioButtonAlarmn)
					{
						if (Label->Checked)
						{
							send(Resource.second.ClientSocket, ("Alarm" + std::to_string(n)).c_str(), 7, 0);
						}
						++n;
					}
				}
				MessageBox::Show("��� �������� �޼����� ���½��ϴ�.");
			}
			else
			{
				DataGridViewSelectedRowCollection^ SelectedRowCollection = this->ClientStatusGridView->SelectedRows;
				string StlString;

				if (0 == SelectedRowCollection->Count)
				{
					MessageBox::Show("�޼����� ���� �� �� �̻��� ������ üũ���ּ���.");
					return;
				}
				for (auto i = 0; i != SelectedRowCollection->Count; ++i)
				{
					auto n = 1;
					for each(auto Label in RadioButtonAlarmn)
					{
						if (Label->Checked)
						{
							MarshalString(SelectedRowCollection[i]->Cells[0]->Value->ToString(), StlString);
							send(ClientResourceMap[StlString].ClientSocket, ("Alarm" + std::to_string(n)).c_str(), 7, 0);
						}
						++n;
					}
				}

				MessageBox::Show(SelectedRowCollection->Count.ToString() + "���� �޼����� ���½��ϴ�.");
			}
		}
		System::Void ButtonHelp_Click(System::Object^  sender, System::EventArgs^  e)
		{
			MessageBox::Show("�ؽ�Ʈ�ڽ��� ����� ��ġ�� ���� ��, �ش� Ŭ���̾�Ʈ���� �˸����� �����մϴ�.");
		}

		System::Void ClientStatusGridView_DataError(System::Object^  sender, System::Windows::Forms::DataGridViewDataErrorEventArgs^  e)
		{
			;
		}
		System::Void ButtonOpenBeepFileDialog_Click(System::Object^  sender, System::EventArgs^  e)
		{
			OpenFileDialog^ BeepOpenFileDialog = gcnew OpenFileDialog();
			BeepOpenFileDialog->Filter = "����� ����|*.wav|All Files|*.*";

			if (BeepOpenFileDialog->ShowDialog() != System::Windows::Forms::DialogResult::OK)
			{
				return;
			}

			this->TextBoxBeepFilePath->Text = BeepOpenFileDialog->FileName;
			IsWarnFilePathDefault = false;
		}
		System::Void ButtonHowToInquiry_Click(System::Object^  sender, System::EventArgs^  e)
		{
			::MessageBox((HWND)this->Handle.ToPointer(), TEXT("������ e-mail : jynis2937@gmail.com"), TEXT("����ó"), MB_OK);
		}
	};
}