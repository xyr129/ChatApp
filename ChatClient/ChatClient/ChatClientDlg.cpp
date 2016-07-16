
// ChatClientDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ChatClient.h"
#include "ChatClientDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

SOCKET g_clientSock = 0;//标示了和服务器连接

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CChatClientDlg 对话框



CChatClientDlg::CChatClientDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CChatClientDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CChatClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_listUserCtrl);
	DDX_Control(pDX, IDC_MsgText, m_chatText);
}

BEGIN_MESSAGE_MAP(CChatClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//ON_EN_CHANGE(IDC_MsgInfo, &CChatClientDlg::OnEnChangeMsginfo)
	ON_BN_CLICKED(IDC_SendMsg, &CChatClientDlg::OnBnClickedSendmsg)
END_MESSAGE_MAP()


// CChatClientDlg 消息处理程序

BOOL CChatClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO:  在此添加额外的初始化代码

	if (!ConnectServer())
	{
		MessageBox(L"连接服务器失败",L"提示",MB_OK);
	}

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CChatClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CChatClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CChatClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



//void CChatClientDlg::OnEnChangeEdit1()
//{
//	// TODO:  如果该控件是 RICHEDIT 控件，它将不
//	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
//	// 函数并调用 CRichEditCtrl().SetEventMask()，
//	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。
//
//	// TODO:  在此添加控件通知处理程序代码
//}
//
//
//void CChatClientDlg::OnEnChangeMsginfo()
//{
//	// TODO:  如果该控件是 RICHEDIT 控件，它将不
//	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
//	// 函数并调用 CRichEditCtrl().SetEventMask()，
//	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。
//
//	// TODO:  在此添加控件通知处理程序代码
//}


void CChatClientDlg::Receive(void *p)
{
	char buf[2048] = { 0 };
	CChatClientDlg* pDlg = (CChatClientDlg*)p;  //主窗口的指针
	if (pDlg == NULL)
	{
		return;
	}

	while (1)  //一直接收服务器的消息
	{
		int bytes;
		//如果服务器没有发消息，一直等待
		if ((bytes = recv(g_clientSock, buf, sizeof(buf),0)) == SOCKET_ERROR)
		{
			_endthread();
			return;
		}
		buf[bytes] = '\0';
		wchar_t bufTest[1024];
		memcpy(bufTest, buf, bytes);
		bufTest[bytes/2] = '\0';

		pDlg->RecvMessage(bufTest);
	}
	return;
}

bool CChatClientDlg::ConnectServer()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		//cout<<"创建套接字失败"<<endl;
		return false;
	}

	SOCKET clientSocket;
	//申请socket失败
	if ((clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		return false;
	}

	g_clientSock = clientSocket;
	if (!Connect(clientSocket))  //连接服务器失败
	{
		return false;
	}

	_beginthread(&CChatClientDlg::Receive, 0, this);//连接上了   开启一个线程准备接收消息
	
		
	//同时 发送消息给服务器  告诉服务器我的ip(这里用计算机名)
	SendToServer(Msg_Login);

	return true;
}


bool CChatClientDlg::Connect(SOCKET sock)
{
	struct sockaddr_in serverAddress;
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	serverAddress.sin_port = htons(1900);

	//开始连接
	if (connect(sock, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		//和服务器连接失败
		return false;
	}
	return true;
}

void CChatClientDlg::SendToServer(Msg_Type MsgType, wstring str)
{
	wchar_t sendBuf[1024];
	switch (MsgType)
	{
	case Msg_Login:
		{
			wchar_t computerName[MAX_COMPUTERNAME_LENGTH];
			DWORD len = MAX_COMPUTERNAME_LENGTH;
			GetComputerName(computerName, &len);
			wsprintf(sendBuf, L"%d_%s", Msg_Login, computerName);
			break;
		}
	case Msg_Chat:
		{
			wsprintf(sendBuf, L"%d_%s", Msg_Chat, str.c_str());
			m_chatText.SetWindowTextW(L"");  //发送完控件清空
			break;
		}
		
	}
	send(g_clientSock, (char*)sendBuf, lstrlen(sendBuf) * 2, 0);
}

void CChatClientDlg::RecvMessage(wstring strBuf)
{
	//解析发过来的操作类型和消息内容
	int pos = strBuf.find('_');
	int msgType = Msg_Min;
	if (pos != -1)
	{
		wstring bufTemp = strBuf.substr(0, pos);
		strBuf = strBuf.substr(pos + 1);//strbuf里为消息内容
		char* temp = (char*)bufTemp.c_str();
		msgType = atoi(temp); // msgType
	}
	else
	{
		char* temp = (char*)strBuf.c_str();
		msgType = atoi(temp);
	}
	if (msgType <= Msg_Min || msgType >= Msg_Max)
	{
		return;
	}

	switch (msgType)
	{
	case Msg_Login:
		{
			//某个用户登录了  添加到list
			m_listUser.push_back(strBuf);
			//显示到在线列表
			m_listUserCtrl.AddString(strBuf.c_str());

			Sleep(1);
		}
	case Msg_LoginOut:
		{
			wstring loginOutName = strBuf;
			int index = m_listUserCtrl.FindString(-1,loginOutName.c_str());
			if (index != -1)
			{
				m_listUserCtrl.DeleteString(index);
			}
			break;
		}
	case Msg_ChatToMe:
	case Msg_ChatToOther:
		{
			pos = strBuf.find('_');
			if (pos == -1)
			{
				return;
			}
			wstring strNameSend = strBuf.substr(0, pos); //发送人
			strBuf = strBuf.substr(pos + 1);
			pos = strBuf.find('_');
			if (pos == -1){
				return;
			}
			wstring strNameSendTo = strBuf.substr(0,pos);//接收人
			wstring strText = strBuf.substr(pos + 1);//消息内容
			wstring msg;
			if (msgType == Msg_ChatToMe)
			{
				msg = L"【" + strNameSend + L"】对你说：" + strText;
			}
			else
			{
				msg = L"你对【" + strNameSendTo + L"】说：" + strText;
			}
			m_listMsg.push_back(msg);
			UpdateMsg();
			break;
		}
		
	}
}

void CChatClientDlg::UpdateMsg()
{
	if (m_listMsg.size() > 200)
	{
		m_listMsg.pop_front();
	}
	wstring strText;
	for (list<wstring>::iterator it = m_listMsg.begin();
		it != m_listMsg.end(); ++it)
	{
		strText = strText + (*it) + L"\r\n";
	}
	SetDlgItemText(IDC_MsgInfo,strText.c_str());
}

void CChatClientDlg::OnBnClickedSendmsg()
{
	// TODO:  在此添加控件通知处理程序代码
	CString str;
	m_chatText.GetWindowTextW(str);
	if (str.GetLength() == 0)
	{
		MessageBox(L"消息不能为空",L"提示",MB_OK);
		return;
	}
	if (str.GetLength() >= 800)
	{
		MessageBox(L"消息不能超过800", L"提示", MB_OK);
		return;
	}

	//确定要发送的人
	int index = m_listUserCtrl.GetCurSel();
	if (index == -1)
	{
		MessageBox(L"请选择要发送的对象", L"提示", MB_OK);
		return;
	}

	CString name;
	m_listUserCtrl.GetText(index, name);

	wstring strTemp = name;
	wstring strText = str;
	strTemp = strTemp + L"_" + strText;

	//通过服务器转发给所有人
	SendToServer(Msg_Chat, strTemp);

}
