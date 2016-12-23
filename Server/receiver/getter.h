#pragma once
#include <string>
#include <unordered_map>
#include <utility>
#include <msclr\marshal_cppstd.h>

#include <mmsystem.h>
#include <Winsock.h>
#pragma comment(lib, "Winmm.lib")
#pragma comment(lib,"wsock32.lib")

#define MAX_CLIENT_NUM 65535
#define BACKLOG 10

using std::string;
using std::unordered_map;
using std::pair;

namespace receiver
{
	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::Threading;

	typedef struct _RESOURCE
	{
		UINT CpuCycle;
		UINT MemoryUsage;
		UINT HardDiskFreeSpace;
		unordered_map<string, pair<DOUBLE, BOOL>> MountAvailable;
	} RESOURCE, *PRESOURCE;

	static bool Exit = false;
	static WSADATA WinSock;
	static SOCKADDR_IN Server;
	static SOCKADDR_IN Client;
	static SOCKET ListenSocket;
	static SOCKET ClientSocket[MAX_CLIENT_NUM];
	static bool IsClientOn[MAX_CLIENT_NUM] = { false };
	static UINT AccumulatedClientNumber = 0;
	static UINT CurrentClientNumber = 0;

	static unordered_map<string, RESOURCE> ClientResourceMap;

	public ref class getter : public System::Windows::Forms::Form
	{
	private:
		enum class SOCKET_ISSUE : USHORT
		{
			WSAStartup,
			Connect,
			Bind,
			Listen,
			Accept,
			Full,
			Closed,
		};
		void SocketHandle(SOCKET_ISSUE Issue)
		{
			switch (Issue)
			{
			case SOCKET_ISSUE::WSAStartup:
				break;
			case SOCKET_ISSUE::Connect:
				break;
			case SOCKET_ISSUE::Bind:
				break;
			case SOCKET_ISSUE::Accept:
				break;
			case SOCKET_ISSUE::Listen:
				break;
			}

			exit(EXIT_FAILURE);
		}

		bool IsThreadBegun;
		Thread^ AcceptSocketThread;
		Thread^ GetThread;
		Thread^ ResourceAddThread;
		Thread^ SelectedItemThread;

		System::Windows::Forms::TextBox^  TextBoxCurrentUserNumber;
		System::Windows::Forms::Button^  ButtonAlarm;
		System::Windows::Forms::Button^  ButtonHelp;
		System::Windows::Forms::Button^  ButtonIsOn;
		System::Windows::Forms::Label^  LabelServerIP;
		System::Windows::Forms::Label^  LabelPort;
		System::Windows::Forms::RadioButton^  RadioButtonAlram1;
		System::Windows::Forms::RadioButton^  RadioButtonAlram2;
		System::Windows::Forms::RadioButton^  RadioButtonAlram3;
		System::Windows::Forms::RadioButton^  RadioButtonAlram4;
		System::Windows::Forms::RadioButton^  RadioButtonAlram5;
		System::Windows::Forms::Label^  LabelMaximum;
		System::Windows::Forms::Label^  LabelCpuWarningRatio;
		System::Windows::Forms::Label^  LabelRamWarningRatio;
		System::Windows::Forms::Label^  LabelHddWarningRatio;
		System::Windows::Forms::TextBox^  TextBoxCpuWarningRatio;
		System::Windows::Forms::TextBox^  TextBoxRamWarningRatio;
		System::Windows::Forms::Label^  LabelBacklog;
		System::Windows::Forms::DataGridViewTextBoxColumn^  Name;
		System::Windows::Forms::DataGridViewTextBoxColumn^  Cpu;
		System::Windows::Forms::DataGridViewTextBoxColumn^  Ram;
		System::Windows::Forms::DataGridViewTextBoxColumn^  Hdd;
		System::Windows::Forms::DataGridViewTextBoxColumn^  Use;
		System::Windows::Forms::DataGridViewComboBoxColumn^  Mount;
		System::Windows::Forms::TextBox^  TextBoxHddWarningRatio;

	public:
		void MarshalString(String ^ SystemString, string& RefStlString)
		{
			using namespace Runtime::InteropServices;
			LPCSTR PtrString = (LPCSTR)(Marshal::StringToHGlobalAnsi(SystemString)).ToPointer();
			RefStlString = PtrString;
			Marshal::FreeHGlobal(IntPtr((LPVOID)PtrString));
		}

		getter(UINT PortToOpen) :
			IsThreadBegun(false),
			GetThread(gcnew Thread(gcnew ThreadStart(this, &getter::DataGridViewUpdate))),
			ResourceAddThread(gcnew Thread(gcnew ThreadStart(this, &getter::GetResourceFromClient))),
			AcceptSocketThread(gcnew Thread(gcnew ThreadStart(this, &getter::AcceptSocket))),
			SelectedItemThread(gcnew Thread(gcnew ThreadStart(this, &getter::UpdateSelectedItem)))
		{
			WSAStartup(MAKEWORD(2, 2), &WinSock);
			ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

			ZeroMemory(&Server, sizeof(Server));
			Server.sin_family = AF_INET;
			Server.sin_addr.s_addr = htonl(INADDR_ANY);
			Server.sin_port = htons(PortToOpen);

			if (::bind(ListenSocket, (sockaddr*)&Server, sizeof(Server)) == SOCKET_ERROR)
			{
				SocketHandle(SOCKET_ISSUE::Bind);
			}
			if (listen(ListenSocket, BACKLOG) == SOCKET_ERROR)
			{
				SocketHandle(SOCKET_ISSUE::Listen);
			}

			InitializeComponent();
		}
		void AcceptSocket(void)
		{
			while (AccumulatedClientNumber < MAX_CLIENT_NUM)
			{
				if ((ClientSocket[AccumulatedClientNumber] = accept(ListenSocket, NULL, NULL)) == INVALID_SOCKET)
				{
					SocketHandle(SOCKET_ISSUE::Accept);
				}
				IsClientOn[AccumulatedClientNumber] = true;
				++AccumulatedClientNumber;
				++CurrentClientNumber;

				this->LabelCurrentUserNumber->Text = "현재인원 : " + CurrentClientNumber.ToString();
			}
		}
		void UpdateSelectedItem(void)
		{
			do
			{
				auto i = 0;
				for (auto& Resource : ClientResourceMap)
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

				System::Threading::Thread::Sleep(100);
			} while (true != Exit);
		}
		void DataGridViewUpdate(void)
		{
			auto RowCount = 0;
			do
			{
				while (RowCount < ClientResourceMap.size())
				{
					++RowCount;
					this->TextBoxCurrentUserNumber->Text = RowCount.ToString();
					this->ClientStatusGridView->Rows->Add();
				}

				auto i = 0;
				for (auto& Resource : ClientResourceMap)
				{
					this->ClientStatusGridView->Rows[i]->Cells[0]->Value = gcnew System::String(Resource.first.c_str());
					this->ClientStatusGridView->Rows[i]->Cells[1]->Value = static_cast<INT>(Resource.second.CpuCycle) + "%";
					this->ClientStatusGridView->Rows[i]->Cells[2]->Value = static_cast<INT>(Resource.second.MemoryUsage) + "%";
					this->ClientStatusGridView->Rows[i]->Cells[3]->Value = static_cast<INT>(Resource.second.HardDiskFreeSpace) + "%";

					for (auto& MountDirectory : Resource.second.MountAvailable)
					{
						if (MountDirectory.second.second == TRUE)
						{
							this->Mount->Items->Add(gcnew String(MountDirectory.first.c_str()));
							MountDirectory.second.second = FALSE;
						}
					}
				}

				System::Threading::Thread::Sleep(100);
			} while (true != Exit);
		}
		void GetResourceFromClient(void)
		{
			CHAR ReceivedMessage[0x400];
			UINT CpuCycle, RamUsage, HddFreeRatio;
			string Hdd;
			CHAR* Delimiter = "\r\n";
			do
			{
				for (auto k = 0; k != AccumulatedClientNumber; ++k)
				{
					ZeroMemory(ReceivedMessage, sizeof(ReceivedMessage));
					if (recv(ClientSocket[k], ReceivedMessage, sizeof(ReceivedMessage), 0) > 0)
					{
						bool Add = false;
						auto i = 0;
						CHAR* Ptr = strtok(ReceivedMessage, Delimiter);
						string UserName = Ptr;
						unordered_map<string, pair<DOUBLE, BOOL>> HddFreeRatioOnMountedDirectory;
						Hdd.clear();
						unordered_map<string, RESOURCE>::const_iterator FindIterator = ClientResourceMap.find(Ptr);

						for (const auto& Resource : ClientResourceMap)
						{
							if (Ptr == Resource.first)
							{
								Add = true;
							}
						}

						while (Ptr = strtok(NULL, Delimiter))
						{
							switch (i)
							{
							case 0:
								if (Ptr == "Bye")
								{
									ClientResourceMap.erase(UserName);
									--CurrentClientNumber;
								}
								CpuCycle = atoi(Ptr);
								
								if (this->TextBoxCpuWarningRatio->TextLength == 0)
								{
									break;
								}
								if (CpuCycle >= Convert::ToInt32(this->TextBoxCpuWarningRatio->Text))
								{
									PlaySound(TEXT("warn.wav"), nullptr, SND_FILENAME);
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
									PlaySound(TEXT("warn.wav"), nullptr, SND_FILENAME);
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
									PlaySound(TEXT("warn.wav"), nullptr, SND_FILENAME);
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
							ClientResourceMap.insert(
								unordered_map<string, RESOURCE>::value_type(UserName, RESOURCE{ CpuCycle, RamUsage, HddFreeRatio, HddFreeRatioOnMountedDirectory })
							);
						}
						else
						{
							ClientResourceMap[UserName] = RESOURCE{ CpuCycle, RamUsage, HddFreeRatio, HddFreeRatioOnMountedDirectory };
						}
					}
				}
			} while (true != Exit);
		}

	protected:
		~getter()
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
		System::Windows::Forms::DataGridView^  ClientStatusGridView;
		System::Windows::Forms::Label^  LabelCurrentUserNumber;
		System::Windows::Forms::Button^  ButtonServerStart;

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
			System::Windows::Forms::DataGridViewCellStyle^  dataGridViewCellStyle5 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^  dataGridViewCellStyle6 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^  dataGridViewCellStyle7 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^  dataGridViewCellStyle3 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			System::Windows::Forms::DataGridViewCellStyle^  dataGridViewCellStyle4 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
			this->ClientStatusGridView = (gcnew System::Windows::Forms::DataGridView());
			this->Name = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Cpu = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Ram = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Hdd = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Use = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
			this->Mount = (gcnew System::Windows::Forms::DataGridViewComboBoxColumn());
			this->ButtonServerStart = (gcnew System::Windows::Forms::Button());
			this->LabelCurrentUserNumber = (gcnew System::Windows::Forms::Label());
			this->TextBoxCurrentUserNumber = (gcnew System::Windows::Forms::TextBox());
			this->ButtonAlarm = (gcnew System::Windows::Forms::Button());
			this->LabelServerIP = (gcnew System::Windows::Forms::Label());
			this->LabelPort = (gcnew System::Windows::Forms::Label());
			this->RadioButtonAlram1 = (gcnew System::Windows::Forms::RadioButton());
			this->RadioButtonAlram2 = (gcnew System::Windows::Forms::RadioButton());
			this->RadioButtonAlram3 = (gcnew System::Windows::Forms::RadioButton());
			this->RadioButtonAlram4 = (gcnew System::Windows::Forms::RadioButton());
			this->RadioButtonAlram5 = (gcnew System::Windows::Forms::RadioButton());
			this->ButtonHelp = (gcnew System::Windows::Forms::Button());
			this->ButtonIsOn = (gcnew System::Windows::Forms::Button());
			this->TextBoxCpuWarningRatio = (gcnew System::Windows::Forms::TextBox());
			this->LabelCpuWarningRatio = (gcnew System::Windows::Forms::Label());
			this->LabelMaximum = (gcnew System::Windows::Forms::Label());
			this->LabelRamWarningRatio = (gcnew System::Windows::Forms::Label());
			this->TextBoxRamWarningRatio = (gcnew System::Windows::Forms::TextBox());
			this->LabelHddWarningRatio = (gcnew System::Windows::Forms::Label());
			this->TextBoxHddWarningRatio = (gcnew System::Windows::Forms::TextBox());
			this->LabelBacklog = (gcnew System::Windows::Forms::Label());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->ClientStatusGridView))->BeginInit();
			this->SuspendLayout();
			// 
			// ClientStatusGridView
			// 
			this->ClientStatusGridView->DataError += 
				gcnew System::Windows::Forms::DataGridViewDataErrorEventHandler(this, &getter::ClientStatusGridView_DataError);
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
			this->ClientStatusGridView->Columns->AddRange(gcnew cli::array< System::Windows::Forms::DataGridViewColumn^  >(6) {
				this->Name,
					this->Cpu, this->Ram, this->Hdd, this->Use, this->Mount
			});
			dataGridViewCellStyle5->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleRight;
			dataGridViewCellStyle5->BackColor = System::Drawing::SystemColors::Window;
			dataGridViewCellStyle5->Font = (gcnew System::Drawing::Font(L"Gulim", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			dataGridViewCellStyle5->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			dataGridViewCellStyle5->SelectionBackColor = System::Drawing::SystemColors::Highlight;
			dataGridViewCellStyle5->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle5->WrapMode = System::Windows::Forms::DataGridViewTriState::False;
			this->ClientStatusGridView->DefaultCellStyle = dataGridViewCellStyle5;
			this->ClientStatusGridView->EnableHeadersVisualStyles = false;
			this->ClientStatusGridView->GridColor = System::Drawing::SystemColors::ActiveCaption;
			this->ClientStatusGridView->Location = System::Drawing::Point(-1, -1);
			this->ClientStatusGridView->Name = L"ClientStatusGridView";
			this->ClientStatusGridView->RowHeadersBorderStyle = System::Windows::Forms::DataGridViewHeaderBorderStyle::Single;
			dataGridViewCellStyle6->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleRight;
			dataGridViewCellStyle6->BackColor = System::Drawing::SystemColors::Control;
			dataGridViewCellStyle6->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 10.2F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			dataGridViewCellStyle6->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			dataGridViewCellStyle6->SelectionBackColor = System::Drawing::SystemColors::Highlight;
			dataGridViewCellStyle6->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
			dataGridViewCellStyle6->WrapMode = System::Windows::Forms::DataGridViewTriState::True;
			this->ClientStatusGridView->RowHeadersDefaultCellStyle = dataGridViewCellStyle6;
			dataGridViewCellStyle7->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleRight;
			dataGridViewCellStyle7->BackColor = System::Drawing::Color::White;
			dataGridViewCellStyle7->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 10.2F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			dataGridViewCellStyle7->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->ClientStatusGridView->RowsDefaultCellStyle = dataGridViewCellStyle7;
			this->ClientStatusGridView->RowTemplate->DefaultCellStyle->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->ClientStatusGridView->RowTemplate->Height = 27;
			this->ClientStatusGridView->Size = System::Drawing::Size(652, 415);
			this->ClientStatusGridView->TabIndex = 0;
			// 
			// Name
			// 
			dataGridViewCellStyle3->ForeColor = System::Drawing::SystemColors::MenuHighlight;
			this->Name->DefaultCellStyle = dataGridViewCellStyle3;
			this->Name->HeaderText = L"사용자 이름";
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
			this->Ram->HeaderText = L"메모리";
			this->Ram->Name = L"Ram";
			this->Ram->Width = 75;
			// 
			// Hdd
			// 
			this->Hdd->HeaderText = L"디스크";
			this->Hdd->Name = L"Hdd";
			this->Hdd->Width = 75;
			// 
			// Use
			// 
			this->Use->HeaderText = L"사용량";
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
			this->Mount->HeaderText = L"마운트";
			this->Mount->Name = L"Mount";
			this->Mount->Resizable = System::Windows::Forms::DataGridViewTriState::True;
			this->Mount->SortMode = System::Windows::Forms::DataGridViewColumnSortMode::Automatic;
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
			this->ButtonServerStart->Location = System::Drawing::Point(52, 435);
			this->ButtonServerStart->Name = L"ButtonServerStart";
			this->ButtonServerStart->Size = System::Drawing::Size(124, 63);
			this->ButtonServerStart->TabIndex = 1;
			this->ButtonServerStart->Text = L"서버 열기";
			this->ButtonServerStart->UseVisualStyleBackColor = false;
			this->ButtonServerStart->Click += gcnew System::EventHandler(this, &getter::ButtonServerStart_Click);
			// 
			// LabelCurrentUserNumber
			// 
			this->LabelCurrentUserNumber->AutoSize = true;
			this->LabelCurrentUserNumber->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->LabelCurrentUserNumber->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->LabelCurrentUserNumber->Location = System::Drawing::Point(45, 572);
			this->LabelCurrentUserNumber->Name = L"LabelCurrentUserNumber";
			this->LabelCurrentUserNumber->Size = System::Drawing::Size(82, 20);
			this->LabelCurrentUserNumber->TabIndex = 2;
			this->LabelCurrentUserNumber->Text = L"현재 인원 :";
			// 
			// TextBoxCurrentUserNumber
			// 
			this->TextBoxCurrentUserNumber->BorderStyle = System::Windows::Forms::BorderStyle::None;
			this->TextBoxCurrentUserNumber->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 10.2F, System::Drawing::FontStyle::Regular,
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(129)));
			this->TextBoxCurrentUserNumber->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->TextBoxCurrentUserNumber->Location = System::Drawing::Point(709, 276);
			this->TextBoxCurrentUserNumber->Name = L"TextBoxCurrentUserNumber";
			this->TextBoxCurrentUserNumber->Size = System::Drawing::Size(127, 23);
			this->TextBoxCurrentUserNumber->TabIndex = 3;
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
			this->ButtonAlarm->Location = System::Drawing::Point(211, 435);
			this->ButtonAlarm->Name = L"ButtonAlarm";
			this->ButtonAlarm->Size = System::Drawing::Size(124, 63);
			this->ButtonAlarm->TabIndex = 4;
			this->ButtonAlarm->Text = L"알람 보내기";
			this->ButtonAlarm->UseVisualStyleBackColor = false;
			this->ButtonAlarm->Click += gcnew System::EventHandler(this, &getter::ButtonAlarm_Click);
			// 
			// LabelServerIP
			// 
			this->LabelServerIP->AutoSize = true;
			this->LabelServerIP->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->LabelServerIP->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->LabelServerIP->Location = System::Drawing::Point(45, 598);
			this->LabelServerIP->Name = L"LabelServerIP";
			this->LabelServerIP->Size = System::Drawing::Size(65, 20);
			this->LabelServerIP->TabIndex = 2;
			this->LabelServerIP->Text = L"서버 IP :";
			// 
			// LabelPort
			// 
			this->LabelPort->AutoSize = true;
			this->LabelPort->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->LabelPort->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->LabelPort->Location = System::Drawing::Point(45, 624);
			this->LabelPort->Name = L"LabelPort";
			this->LabelPort->Size = System::Drawing::Size(47, 20);
			this->LabelPort->TabIndex = 2;
			this->LabelPort->Text = L"포트 :";
			// 
			// RadioButtonAlram1
			// 
			this->RadioButtonAlram1->AutoSize = true;
			this->RadioButtonAlram1->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->RadioButtonAlram1->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->RadioButtonAlram1->Location = System::Drawing::Point(211, 511);
			this->RadioButtonAlram1->Name = L"RadioButtonAlram1";
			this->RadioButtonAlram1->Size = System::Drawing::Size(73, 24);
			this->RadioButtonAlram1->TabIndex = 5;
			this->RadioButtonAlram1->TabStop = true;
			this->RadioButtonAlram1->Text = L"알람 1";
			this->RadioButtonAlram1->UseVisualStyleBackColor = true;
			// 
			// RadioButtonAlram2
			// 
			this->RadioButtonAlram2->AutoSize = true;
			this->RadioButtonAlram2->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->RadioButtonAlram2->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->RadioButtonAlram2->Location = System::Drawing::Point(211, 537);
			this->RadioButtonAlram2->Name = L"RadioButtonAlram2";
			this->RadioButtonAlram2->Size = System::Drawing::Size(73, 24);
			this->RadioButtonAlram2->TabIndex = 5;
			this->RadioButtonAlram2->TabStop = true;
			this->RadioButtonAlram2->Text = L"알람 2";
			this->RadioButtonAlram2->UseVisualStyleBackColor = true;
			// 
			// RadioButtonAlram3
			// 
			this->RadioButtonAlram3->AutoSize = true;
			this->RadioButtonAlram3->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->RadioButtonAlram3->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->RadioButtonAlram3->Location = System::Drawing::Point(211, 563);
			this->RadioButtonAlram3->Name = L"RadioButtonAlram3";
			this->RadioButtonAlram3->Size = System::Drawing::Size(73, 24);
			this->RadioButtonAlram3->TabIndex = 5;
			this->RadioButtonAlram3->TabStop = true;
			this->RadioButtonAlram3->Text = L"알람 3";
			this->RadioButtonAlram3->UseVisualStyleBackColor = true;
			// 
			// RadioButtonAlram4
			// 
			this->RadioButtonAlram4->AutoSize = true;
			this->RadioButtonAlram4->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->RadioButtonAlram4->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->RadioButtonAlram4->Location = System::Drawing::Point(211, 588);
			this->RadioButtonAlram4->Name = L"RadioButtonAlram4";
			this->RadioButtonAlram4->Size = System::Drawing::Size(73, 24);
			this->RadioButtonAlram4->TabIndex = 5;
			this->RadioButtonAlram4->TabStop = true;
			this->RadioButtonAlram4->Text = L"알람 4";
			this->RadioButtonAlram4->UseVisualStyleBackColor = true;
			// 
			// RadioButtonAlram5
			// 
			this->RadioButtonAlram5->AutoSize = true;
			this->RadioButtonAlram5->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->RadioButtonAlram5->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->RadioButtonAlram5->Location = System::Drawing::Point(211, 614);
			this->RadioButtonAlram5->Name = L"RadioButtonAlram5";
			this->RadioButtonAlram5->Size = System::Drawing::Size(73, 24);
			this->RadioButtonAlram5->TabIndex = 5;
			this->RadioButtonAlram5->TabStop = true;
			this->RadioButtonAlram5->Text = L"알람 5";
			this->RadioButtonAlram5->UseVisualStyleBackColor = true;
			// 
			// ButtonHelp
			// 
			this->ButtonHelp->BackColor = System::Drawing::SystemColors::ButtonHighlight;
			this->ButtonHelp->FlatAppearance->BorderColor = System::Drawing::SystemColors::ActiveCaption;
			this->ButtonHelp->FlatAppearance->BorderSize = 2;
			this->ButtonHelp->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
			this->ButtonHelp->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 13.8F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->ButtonHelp->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->ButtonHelp->Location = System::Drawing::Point(444, 638);
			this->ButtonHelp->Name = L"ButtonHelp";
			this->ButtonHelp->Size = System::Drawing::Size(44, 45);
			this->ButtonHelp->TabIndex = 6;
			this->ButtonHelp->Text = L"\?";
			this->ButtonHelp->UseVisualStyleBackColor = false;
			this->ButtonHelp->Click += gcnew System::EventHandler(this, &getter::ButtonHelp_Click);
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
			this->ButtonIsOn->Location = System::Drawing::Point(52, 506);
			this->ButtonIsOn->Name = L"ButtonIsOn";
			this->ButtonIsOn->Size = System::Drawing::Size(124, 24);
			this->ButtonIsOn->TabIndex = 1;
			this->ButtonIsOn->Text = L"Off";
			this->ButtonIsOn->UseVisualStyleBackColor = false;
			this->ButtonIsOn->Click += gcnew System::EventHandler(this, &getter::ButtonServerStart_Click);
			// 
			// TextBoxCpuWarningRatio
			// 
			this->TextBoxCpuWarningRatio->BackColor = System::Drawing::Color::AliceBlue;
			this->TextBoxCpuWarningRatio->BorderStyle = System::Windows::Forms::BorderStyle::None;
			this->TextBoxCpuWarningRatio->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 10.2F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->TextBoxCpuWarningRatio->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->TextBoxCpuWarningRatio->Location = System::Drawing::Point(371, 462);
			this->TextBoxCpuWarningRatio->Name = L"TextBoxCpuWarningRatio";
			this->TextBoxCpuWarningRatio->Size = System::Drawing::Size(119, 23);
			this->TextBoxCpuWarningRatio->TabIndex = 7;
			this->TextBoxCpuWarningRatio->TextAlign = System::Windows::Forms::HorizontalAlignment::Right;
			// 
			// LabelCpuWarningRatio
			// 
			this->LabelCpuWarningRatio->AutoSize = true;
			this->LabelCpuWarningRatio->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->LabelCpuWarningRatio->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->LabelCpuWarningRatio->Location = System::Drawing::Point(370, 435);
			this->LabelCpuWarningRatio->Name = L"LabelCpuWarningRatio";
			this->LabelCpuWarningRatio->Size = System::Drawing::Size(89, 20);
			this->LabelCpuWarningRatio->TabIndex = 8;
			this->LabelCpuWarningRatio->Text = L"CPU 경고율";
			// 
			// LabelMaximum
			// 
			this->LabelMaximum->AutoSize = true;
			this->LabelMaximum->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->LabelMaximum->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->LabelMaximum->Location = System::Drawing::Point(46, 544);
			this->LabelMaximum->Name = L"LabelMaximum";
			this->LabelMaximum->Size = System::Drawing::Size(82, 20);
			this->LabelMaximum->TabIndex = 2;
			this->LabelMaximum->Text = L"최대 인원 :";
			// 
			// LabelRamWarningRatio
			// 
			this->LabelRamWarningRatio->AutoSize = true;
			this->LabelRamWarningRatio->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->LabelRamWarningRatio->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->LabelRamWarningRatio->Location = System::Drawing::Point(369, 495);
			this->LabelRamWarningRatio->Name = L"LabelRamWarningRatio";
			this->LabelRamWarningRatio->Size = System::Drawing::Size(104, 20);
			this->LabelRamWarningRatio->TabIndex = 10;
			this->LabelRamWarningRatio->Text = L"메모리 경고율";
			// 
			// TextBoxRamWarningRatio
			// 
			this->TextBoxRamWarningRatio->BackColor = System::Drawing::Color::AliceBlue;
			this->TextBoxRamWarningRatio->BorderStyle = System::Windows::Forms::BorderStyle::None;
			this->TextBoxRamWarningRatio->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 10.2F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->TextBoxRamWarningRatio->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->TextBoxRamWarningRatio->Location = System::Drawing::Point(371, 522);
			this->TextBoxRamWarningRatio->Name = L"TextBoxRamWarningRatio";
			this->TextBoxRamWarningRatio->Size = System::Drawing::Size(119, 23);
			this->TextBoxRamWarningRatio->TabIndex = 9;
			this->TextBoxRamWarningRatio->TextAlign = System::Windows::Forms::HorizontalAlignment::Right;
			// 
			// LabelHddWarningRatio
			// 
			this->LabelHddWarningRatio->AutoSize = true;
			this->LabelHddWarningRatio->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->LabelHddWarningRatio->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->LabelHddWarningRatio->Location = System::Drawing::Point(368, 555);
			this->LabelHddWarningRatio->Name = L"LabelHddWarningRatio";
			this->LabelHddWarningRatio->Size = System::Drawing::Size(104, 20);
			this->LabelHddWarningRatio->TabIndex = 12;
			this->LabelHddWarningRatio->Text = L"디스크 경고율";
			// 
			// TextBoxHddWarningRatio
			// 
			this->TextBoxHddWarningRatio->BackColor = System::Drawing::Color::AliceBlue;
			this->TextBoxHddWarningRatio->BorderStyle = System::Windows::Forms::BorderStyle::None;
			this->TextBoxHddWarningRatio->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 10.2F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->TextBoxHddWarningRatio->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->TextBoxHddWarningRatio->Location = System::Drawing::Point(370, 582);
			this->TextBoxHddWarningRatio->Name = L"TextBoxHddWarningRatio";
			this->TextBoxHddWarningRatio->Size = System::Drawing::Size(119, 23);
			this->TextBoxHddWarningRatio->TabIndex = 11;
			this->TextBoxHddWarningRatio->TextAlign = System::Windows::Forms::HorizontalAlignment::Right;
			// 
			// LabelBacklog
			// 
			this->LabelBacklog->AutoSize = true;
			this->LabelBacklog->Font = (gcnew System::Drawing::Font(L"Malgun Gothic", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(129)));
			this->LabelBacklog->ForeColor = System::Drawing::SystemColors::ActiveCaption;
			this->LabelBacklog->Location = System::Drawing::Point(46, 651);
			this->LabelBacklog->Name = L"LabelBacklog";
			this->LabelBacklog->Size = System::Drawing::Size(67, 20);
			this->LabelBacklog->TabIndex = 13;
			this->LabelBacklog->Text = L"백로그 : ";
			// 
			// getter
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(8, 15);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->BackColor = System::Drawing::SystemColors::ButtonHighlight;
			this->ClientSize = System::Drawing::Size(718, 697);
			this->Controls->Add(this->LabelBacklog);
			this->Controls->Add(this->LabelHddWarningRatio);
			this->Controls->Add(this->TextBoxHddWarningRatio);
			this->Controls->Add(this->LabelRamWarningRatio);
			this->Controls->Add(this->TextBoxRamWarningRatio);
			this->Controls->Add(this->LabelCpuWarningRatio);
			this->Controls->Add(this->TextBoxCpuWarningRatio);
			this->Controls->Add(this->ButtonHelp);
			this->Controls->Add(this->RadioButtonAlram5);
			this->Controls->Add(this->RadioButtonAlram4);
			this->Controls->Add(this->RadioButtonAlram3);
			this->Controls->Add(this->RadioButtonAlram2);
			this->Controls->Add(this->RadioButtonAlram1);
			this->Controls->Add(this->ButtonAlarm);
			this->Controls->Add(this->TextBoxCurrentUserNumber);
			this->Controls->Add(this->LabelMaximum);
			this->Controls->Add(this->LabelPort);
			this->Controls->Add(this->LabelServerIP);
			this->Controls->Add(this->LabelCurrentUserNumber);
			this->Controls->Add(this->ButtonIsOn);
			this->Controls->Add(this->ButtonServerStart);
			this->Controls->Add(this->ClientStatusGridView);
			//			this->Name = L"getter";
			this->Text = L"알람 서버";
			this->FormClosed += gcnew System::Windows::Forms::FormClosedEventHandler(this, &getter::getter_FormClosed);
			this->Load += gcnew System::EventHandler(this, &getter::getter_Load);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->ClientStatusGridView))->EndInit();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
		BOOL GetLocalIPAddres(void)
		{
			WSADATA WSAData;
			IN_ADDR Address;
			CHAR LocalName[256];
			CHAR IPAddress[15];
			auto i = 0;

			if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)
			{
				return FALSE;
			}

			if (gethostname(LocalName, sizeof(LocalName)) == SOCKET_ERROR)
			{
				return FALSE;
			}

			HOSTENT* HostEntry = gethostbyname(LocalName);
			if (nullptr == HostEntry)
			{
				return FALSE;
			}

			while (HostEntry->h_addr_list[i] != nullptr)
			{
				memcpy(&Address, HostEntry->h_addr_list[i], HostEntry->h_length);
			}
		}

	private:
		System::Void getter_Load(System::Object^  sender, System::EventArgs^  e)
		{
			this->LabelMaximum->Text = "최대인원 : " + MAX_CLIENT_NUM.ToString();
			this->LabelCurrentUserNumber->Text = "현재인원 : 0";
			this->LabelServerIP->Text = "서버 IP : ";
			this->LabelPort->Text = "서버 포트 : 2937";
			this->LabelBacklog->Text = "백로그 : " + BACKLOG.ToString();
		}
		System::Void getter_FormClosed(System::Object^  sender, System::Windows::Forms::FormClosedEventArgs^  e)
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

				MessageBox::Show("서버를 열었습니다.");

				this->ButtonIsOn->BackColor = System::Drawing::Color::DeepSkyBlue;
				this->ButtonIsOn->Text = "On";
			}
			else
			{
				MessageBox::Show("이미 서버가 열려있습니다.");
			}
		}
		System::Void ButtonAlarm_Click(System::Object^  sender, System::EventArgs^  e)
		{
			for (auto i = 0; i < MAX_CLIENT_NUM; ++i)
			{
				if (IsClientOn[i] != false)
				{
					if (this->RadioButtonAlram1->Checked)
					{
						send(ClientSocket[i], "Alarm1", 7, 0);
					}
					else if (this->RadioButtonAlram2->Checked)
					{
						send(ClientSocket[i], "Alarm2", 7, 0);
					}
					else if (this->RadioButtonAlram3->Checked)
					{
						send(ClientSocket[i], "Alarm3", 7, 0);
					}
					else if (this->RadioButtonAlram4->Checked)
					{
						send(ClientSocket[i], "Alarm4", 7, 0);
					}
					else if (this->RadioButtonAlram4->Checked)
					{
						send(ClientSocket[i], "Alarm4", 7, 0);
					}
				}
			}
			MessageBox::Show("알람을 보냈습니다.");
		}
		System::Void ButtonHelp_Click(System::Object^  sender, System::EventArgs^  e)
		{
			MessageBox::Show("텍스트박스의 경고율 수치를 넘을 시, 해당 클라이언트에게 알림음을 전송합니다.");
		}

		System::Void ClientStatusGridView_DataError(System::Object^  sender, System::Windows::Forms::DataGridViewDataErrorEventArgs^  e)
		{
			;
		}
	};
}
