
// ModemSimDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ModemSim.h"
#include "ModemSimDlg.h"
#include "afxdialogex.h"
#include "globals.h"
#include "messages.h"
#include "port.h"
#include "ErrorString.h"
#include <locale.h>
#include <cmath>    // sin, M_PI

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define  TIMER_SEND_DATA 666

#define ID_WAVE_CONTROL1 8888
#define ID_WAVE_CONTROL2 9999

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
public:
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


// CModemSimDlg 对话框




CModemSimDlg::CModemSimDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CModemSimDlg::IDD, pParent)
	, m_open(0)
	, m_close(0)
	, m_running(FALSE)
	, m_MainWndIsClosing(FALSE)
	, m_pClientThread(NULL)
	, m_bRemoteConnected(FALSE)
	, m_bModemConnecting(FALSE)
	, m_bSending(FALSE)
	, m_time(0.0F)
	, m_nViewStartPoint(0)
	, m_nSendDataTimer(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_nSendCount = m_nRecvCount = 0;

	for(int i=0; i<NUM_WAVEFORM; i++)
		m_wave[i].pWaveCtrl = NULL;

	m_strLocalIp = _T("");
	m_strLocalPort = _T("");
	m_strRemotePort = _T("");
	m_strRemoteIp = _T("");
	m_strListenerState = _T("");
	m_strConnectState = _T("");
	m_strCmdLogging = _T("");
	m_strCmdCharge = _T("");
}

void CModemSimDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_LOCAL_IP, m_strLocalIp);
	DDX_Text(pDX, IDC_EDIT_LOCAL_PORT, m_strLocalPort);
	DDX_Text(pDX, IDC_EDIT_REMOTE_PORT, m_strRemotePort);
	DDX_Text(pDX, IDC_EDIT_REMOTE_IP, m_strRemoteIp);
	DDX_Text(pDX, IDC_STATIC_LISTENER_STATE, m_strListenerState);
	DDX_Text(pDX, IDC_STATIC_CONNECT_STATE, m_strConnectState);
	DDX_Text(pDX, IDC_STATIC_CMD_LOGGING, m_strCmdLogging);
	DDX_Text(pDX, IDC_STATIC_CMD_CHARGE, m_strCmdCharge);
	DDX_Control(pDX, IDC_LIST_RECEIVE, m_listRecv);
	DDX_Control(pDX, IDC_LIST_SEND, m_listSend);
	DDX_Control(pDX, IDC_BUTTON_LISTENER, m_btnRun);
	DDX_Control(pDX, IDC_STATIC_LOCAL_IP, m_staticLocalIP);
	DDX_Control(pDX, IDC_STATIC_LOCAL_PORT, m_staticLocalPort);
	DDX_Control(pDX, IDC_STATIC_REMOTE_IP, m_staticRemoteIP);
	DDX_Control(pDX, IDC_STATIC_REMOTE_PORT, m_staticRemotePort);
	DDX_Control(pDX, IDC_BUTTON_SEND_DATA, m_btnSendData);
	DDX_Control(pDX, IDC_BUTTON_CONNECT_REMOTE, m_btnConnectRemote);
	DDX_Control(pDX, IDC_BUTTON_DISCONNECT_REMOTE, m_btnDisconnectRemote);
	DDX_Control(pDX, IDC_STATIC_RECV_COUNT, m_staticRecvCount);
	DDX_Control(pDX, IDC_STATIC_SEND_COUNT, m_staticSendCount);
	DDX_Control(pDX, IDC_COMBO_CCL_VALUE, m_cmbCCL);
	DDX_Control(pDX, IDC_COMBO_CHARGE_VOLT, m_cmbChargeVolt);
	DDX_Control(pDX, IDC_EDIT_CHARGEVOLT_AMPLITUDE, m_editChargeVoltAmplitude);
	DDX_Control(pDX, IDC_EDIT_CCL_AMPLITUDE, m_editCclAmplitude);
	DDX_Control(pDX, IDC_EDIT_CHARGEVOLT_FREQ, m_editChargeVoltFreq);
	DDX_Control(pDX, IDC_EDIT_CCL_FREQ, m_editCclFreq);
	DDX_Check(pDX, IDC_CHECK_PAUSE_DISPLAY, m_bPauseDisplay);
	DDX_Control(pDX, IDC_COMBO_IP_SELECT, m_cmbIPSelect);
}

BEGIN_MESSAGE_MAP(CModemSimDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BUTTON_LISTENER, &CModemSimDlg::OnRun)
	ON_BN_CLICKED(IDC_BUTTON_CONNECT_REMOTE, &CModemSimDlg::OnConnectRemote)
	ON_BN_CLICKED(IDC_BUTTON_DISCONNECT_REMOTE, &CModemSimDlg::OnDisconnectRemote)
	ON_BN_CLICKED(IDC_BUTTON_SEND_DATA, &CModemSimDlg::OnBnClickedButtonSendData)
	ON_WM_SIZE()

	ON_MESSAGE(UWM_THREADSTART, OnThreadStart)
	ON_MESSAGE(UWM_THREADCLOSE, OnThreadClose)
	ON_MESSAGE(UWM_NETWORK_DATA, OnNetworkData)    
	ON_MESSAGE(UWM_INFO, OnInfo)
	ON_MESSAGE(UWM_NETWORK_ERROR, OnNetworkError)
	ON_MESSAGE(UWM_SEND_COMPLETE, OnSendComplete)
	ON_MESSAGE(UWM_CONNECTIONMADE, OnConnectionMade)
	ON_MESSAGE(UWM_CONNECTIONCLOSE, OnConnectionClose)

	ON_WM_TIMER()
	//ON_NOTIFY(GVN_BEGINLABELEDIT, IDC_GRID_SEND, &CModemSimDlg::OnGridSendStartEdit)
	//ON_NOTIFY(GVN_ENDLABELEDIT, IDC_GRID_SEND, &CModemSimDlg::OnGridSendEndEdit)
	ON_CBN_SELCHANGE(IDC_COMBO_CCL_VALUE, &CModemSimDlg::OnCbnSelchangeComboCclValue)
	ON_CBN_SELCHANGE(IDC_COMBO_CHARGE_VOLT, &CModemSimDlg::OnCbnSelchangeComboChargeVolt)

	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_LOG_COPY, &CModemSimDlg::OnCopy)
	ON_COMMAND(ID_LOG_COPY2, &CModemSimDlg::OnCopy2)
	ON_COMMAND(ID_LOG_CLEAR, &CModemSimDlg::OnClear)
	ON_COMMAND(ID_LOG_CLEAR2, &CModemSimDlg::OnClear2)

	ON_BN_CLICKED(IDC_CHECK_PAUSE_DISPLAY, &CModemSimDlg::OnBnClickedCheckPauseDisplay)
	ON_CBN_SELCHANGE(IDC_COMBO_IP_SELECT, &CModemSimDlg::OnCbnSelchangeComboIpSelect)
END_MESSAGE_MAP()


// CModemSimDlg 消息处理程序

BOOL CModemSimDlg::OnInitDialog()
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

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TRACE支持中文
	setlocale(LC_ALL, "chs");

	// 读出配置信息
	CString strIniFile = GetExeName() + _T(".ini");
	m_ini.SetPathName(strIniFile);
	m_strLocalIp = m_ini.GetString(_T("Setup"), _T("ServerIp"), _T("192.168.0.70"));
	m_strLocalPort = m_ini.GetString(_T("Setup"), _T("ServerPort"), _T("1104"));
	m_strRemoteIp = m_ini.GetString(_T("Setup"), _T("ModemIp"), _T("192.168.0.89"));
	m_strRemotePort = m_ini.GetString(_T("Setup"), _T("ModemPort"), _T("1234"));
	m_nLocalPort  = _wtoi(m_strLocalPort);
	m_nRemotePort = _wtoi(m_strRemotePort);
	m_bPauseDisplay = m_ini.GetBool(_T("Setup"), _T("PauseDisplay"), FALSE);
	m_nIpSelect = m_ini.GetInt(_T("Setup"), _T("IPSelect"), 0);

	m_wave[0].nWaveType = m_ini.GetInt(_T("Setup"), _T("CClWaveType"), 0);
	m_wave[1].nWaveType = m_ini.GetInt(_T("Setup"), _T("ChargeVoltWaveType"), 0);

	CString str;
	str = m_ini.GetString(_T("Setup"), _T("CclFreq"), _T("1"));					m_wave[0].nFreq = _ttof(str);		m_editCclFreq.SetWindowTextW(str);
	str = m_ini.GetString(_T("Setup"), _T("CclAmplitude"), _T("200"));			m_wave[0].nAmplitude = _ttof(str);	m_editCclAmplitude.SetWindowTextW(str);
	str = m_ini.GetString(_T("Setup"), _T("ChargeVoltFreq"), _T("1"));			m_wave[1].nFreq = _ttof(str);		m_editChargeVoltFreq.SetWindowTextW(str);
	str = m_ini.GetString(_T("Setup"), _T("ChargeVoltAmplitude"), _T("200"));	m_wave[1].nAmplitude = _ttof(str);	m_editChargeVoltAmplitude.SetWindowTextW(str);

	UpdateData(FALSE);

	m_cmbCCL.AddString(_T("正弦波"));
	m_cmbCCL.AddString(_T("方波"));
	m_cmbCCL.AddString(_T("三角波"));
	m_cmbCCL.AddString(_T("锯齿波"));
	m_cmbCCL.AddString(_T("直流100"));
	m_cmbCCL.SetCurSel(m_wave[0].nWaveType);

	m_cmbChargeVolt.AddString(_T("正弦波"));
	m_cmbChargeVolt.AddString(_T("方波"));
	m_cmbChargeVolt.AddString(_T("三角波"));
	m_cmbChargeVolt.AddString(_T("锯齿波"));
	m_cmbChargeVolt.AddString(_T("直流0"));
	m_cmbChargeVolt.SetCurSel(m_wave[1].nWaveType);

	// 初始化波形显示控件，V/A图
	if (m_wave[0].pWaveCtrl == NULL)
	{
		CWnd* pWnd = GetDlgItem(IDC_STATIC_CCL);
		CDC* pDC = pWnd->GetDC();

		CRect rcDraw;
		pWnd->GetWindowRect(&rcDraw);
		ScreenToClient(&rcDraw);

		m_wave[0].pWaveCtrl = new CWaveCtrl;
		m_wave[0].pWaveCtrl->Create(rcDraw, this, ID_WAVE_CONTROL1);

		m_wave[0].pWaveCtrl->SetColors(WHITE, YELLOW, BLACK);
		m_wave[0].pWaveCtrl->SetLineType( 0 );			// 连线
		m_wave[0].pWaveCtrl->SetUnitType( 0 );			// 无单位
		m_wave[0].pWaveCtrl->SetAutoRange(FALSE);		// 非自动量程
		m_wave[0].pWaveCtrl->SetShowEvents(FALSE, FALSE, FALSE, FALSE, TRUE, TRUE);

		m_wave[0].nStep = 1;
		m_wave[0].nSampleFreq = 1000 / 40;		// 采样率，上传时间间隔40ms，则每秒25帧上传数据
		m_wave[0].nPoints = 100;				// 显示窗口共100点
		m_wave[0].nViewWidth = m_wave[0].nPoints / m_wave[0].nSampleFreq;		// 绘图点个数/采样率=显示窗口的秒数=4s
		
		m_wave[0].data.resize(m_wave[0].nPoints, 0.0F);

		m_wave[0].pWaveCtrl->SetWvfName( _T("CCL" ));
		m_wave[0].pWaveCtrl->SetSppUnit( _T("mV" ));
		m_wave[0].pWaveCtrl->SetViewWidth( m_wave[0].nViewWidth );
		m_wave[0].pWaveCtrl->SetSampleFreq( m_wave[0].nSampleFreq );
		m_wave[0].pWaveCtrl->SetStep( m_wave[0].nStep );
	}

	if (m_wave[1].pWaveCtrl == NULL)
	{
		CWnd* pWnd = GetDlgItem(IDC_STATIC_CHARGE_VOLT);
		CDC* pDC = pWnd->GetDC();

		CRect rcDraw;
		pWnd->GetWindowRect(&rcDraw);
		ScreenToClient(&rcDraw);

		m_wave[1].pWaveCtrl = new CWaveCtrl;
		m_wave[1].pWaveCtrl->Create(rcDraw, this, ID_WAVE_CONTROL2);

		m_wave[1].pWaveCtrl->SetColors(WHITE, YELLOW, BLACK);
		m_wave[1].pWaveCtrl->SetLineType( 0 );			// 连线
		m_wave[1].pWaveCtrl->SetUnitType( 0 );			// 无单位
		m_wave[1].pWaveCtrl->SetAutoRange(FALSE);		// 非自动量程
		m_wave[1].pWaveCtrl->SetShowEvents(FALSE, FALSE, FALSE, FALSE, TRUE, TRUE);

		m_wave[1].nStep = 100;
		m_wave[1].nSampleFreq = 1000 / 40;		// 采样率，上传时间间隔40ms，则每秒25帧上传数据
		m_wave[1].nPoints = 100*100;			// 显示窗口共100小段，每段100个数据点
		m_wave[1].nViewWidth = m_wave[1].nPoints / m_wave[1].nSampleFreq;		// 绘图点个数/采样率=显示窗口的秒数

		m_wave[1].data.resize(m_wave[1].nPoints, 0.0F);

		m_wave[1].pWaveCtrl->SetWvfName( _T("充电电压" ));
		m_wave[1].pWaveCtrl->SetSppUnit( _T("mV" ));
		m_wave[1].pWaveCtrl->SetViewWidth( m_wave[1].nViewWidth );
		m_wave[1].pWaveCtrl->SetSampleFreq( m_wave[1].nSampleFreq );
		m_wave[1].pWaveCtrl->SetStep( m_wave[1].nStep );
	}

	// 表格属性
	//{
	//	m_nCols = 1+3*2;
	//	m_nRecvRows = 20;
	//	m_nRows = m_nRecvRows + 2;

	//	//m_gridSend.SetAutoSizeStyle();
	//	m_gridSend.ShowScrollBar(GVL_HORZ, FALSE);

	//	m_gridSend.SetRowCount(m_nRows);
	//	m_gridSend.SetColumnCount(m_nCols);
	//	m_gridSend.SetFixedRowCount(2);
	//	m_gridSend.SetFixedColumnCount(1);

	//	// 2. 设置默认单元格属性：文本居中，自动换行
	//	for (int row = 0; row < m_gridSend.GetRowCount(); ++row)
	//	{
	//		for (int col = 0; col < m_gridSend.GetColumnCount(); ++col)
	//		{
	//			CGridCell* pCell = (CGridCell*)m_gridSend.GetCell(row, col);
	//			if (pCell)
	//			{
	//				pCell->SetFormat(DT_CENTER | DT_VCENTER | DT_WORDBREAK);
	//			}
	//		}
	//	}

	//	CRect cr;
	//	m_gridSend.GetClientRect(&cr);

	//	CString  strHeader[] = { L"序号", L"数传短节",  L"CAN 87", L"CAN 180" };
	//	CString  strHeader2[] = { L"", _T("初始数据"), _T("变化方式"), _T("初始数据"), _T("变化方式"), _T("初始数据"), _T("变化方式") };
	//	for (int col = 0; col < 4; col++)
	//		m_gridSend.SetItemText(0, col<2 ? col : col*2-1, strHeader[col]);		// 列标题
	//	for (int col = 0; col < m_nCols; col++)
	//	{
	//		m_gridSend.SetColumnWidth(col, cr.Width() / m_nCols);		// 设置列宽
	//		m_gridSend.SetItemText(1, col, strHeader2[col]);		// 列标题
	//	}

	//	// 合并单元格


	//	CString strIndex;
	//	for (int i = 0; i < m_nRecvRows; i++)
	//	{
	//		strIndex.Format(L"%d", i+1);

	//		m_gridSend.SetItemText(i+2, 0, strIndex);						// 序号
	//		//m_gridSend.SetItemText(i+1, 2, pMain->m_arGunString[i].strGunId);			// ID

	//		//m_gridSend.SetCellType(i+1, 1, RUNTIME_CLASS(CGridCellCombo));

	//		//CGridCellCombo *pCell = (CGridCellCombo*)m_gridSend.GetCell(i+1, 1);
	//		//pCell->SetOptions(arGunType);
	//		//pCell->SetStyle(CBS_DROPDOWNLIST); // CBS_DROPDOWN, CBS_DROPDOWNLIST, CBS_SIMPLE

	//		//m_gridSend.SetItemText(i+1, 1, pMain->m_arGunString[i].strGunType);		// 枪型设置值
	//	}
	//	m_gridSend.ShowScrollBar(GVL_HORZ, FALSE);
	//	m_gridSend.ExpandLastColumn();
	//}

	// IP选择，并启动监听
	m_cmbIPSelect.AddString(_T("本地回环"));
	m_cmbIPSelect.AddString(_T("实际IP"));
	m_cmbIPSelect.SetCurSel(m_nIpSelect);
	OnCbnSelchangeComboIpSelect();

	// 为list列表框加载菜单资源
	m_contextMenu.LoadMenu(IDR_MENU_LIST1);
	m_contextMenu2.LoadMenu(IDR_MENU_LIST2);

	// 保存控件初始位置
	SaveInitRect();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CModemSimDlg::OnSysCommand(UINT nID, LPARAM lParam)
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
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CModemSimDlg::OnPaint()
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
HCURSOR CModemSimDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CModemSimDlg::OnOK()
{
}


void CModemSimDlg::OnCancel()
{
}

// 点“连接”按钮，本机Modem作为客户机连接服务器远程PC
void CModemSimDlg::OnConnectRemote()
{
	UpdateData(TRUE);
	TRACE(_TEXT("%s: CModemSimDlg::OnConnect()\n"), AfxGetApp()->m_pszAppName);

	// Create a thread to handle the connection. The thread is created suspended so that we can
	// set variables in CClientThread before it starts executing.
	m_pClientThread = (CClientThread*)AfxBeginThread(RUNTIME_CLASS(CClientThread), THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	if (m_pClientThread == NULL)
	{ /* failed to start */
		CString fmt;
		fmt.LoadString(IDS_THREAD_CREATE_FAILED);
		CString s;
		s.Format(fmt, m_strRemoteIp);
		//c_ConnectionStatus.SetWindowText(s);
		LogMessage(s);
		TRACE(_T("%s: %s\n"), AfxGetApp()->m_pszAppName, s);
		return;
	} /* failed to start */

	m_pClientThread->SetTarget(this);
	m_pClientThread->SetServerName(m_strRemoteIp);

	//c_Port.GetWindowText(s);
	//UINT port = _tcstoul(s, NULL, 0);
	m_nRemotePort = _wtoi(m_strRemotePort);
	m_pClientThread->SetPort(m_nRemotePort);

	// Now start the thread.
	m_bModemConnecting = TRUE;
	CString s;
	s.LoadString(IDS_CONNECTING);
	//c_ConnectionStatus.SetWindowText(s);
	LogMessage(s);

	m_pClientThread->ResumeThread();
	UpdateControls();
}

// 点“断开”按钮，本机作为Modem断开与PC的连接
void CModemSimDlg::OnDisconnectRemote()
{
	// If we have a client thread in a tcp connection, post a user message
	// to the client thread to signal a tear down of the connection is requested.
	// The client thread will close the socket and terminates itself
	TRACE(_T("%s: CModemSimDlg::OnDisconnect\n"), AfxGetApp()->m_pszAppName);

	if (m_pClientThread != NULL)
	{ /* shut it down */
		TRACE(_T("%s: CModemSimDlg::OnDisconnect: Sent UWM_TERM_THREAD\n"), AfxGetApp()->m_pszAppName);

		if (!m_pClientThread->PostThreadMessage(UWM_TERM_THREAD, 0, 0))
			TRACE(_T("%s: Thread 0x%02x possibly already terminated\n"), AfxGetApp()->m_pszAppName, m_pClientThread->m_nThreadID);
	} /* shut it down */	
}


//void CModemSimDlg::OnPcConnectCmd()
//{
	// PC 尝试连接 1104，我们需要接受它
	// 注意：我们用 m_sktCmd 来接管这个连接，m_sktListen 继续监听
	//if (m_sktListen.Accept(m_sktCmd))
	//{
	//	UpdateLog(_T("PC 已连接到本机 1104 接口！"));
	//}
//}

// 处理 PC 命令的逻辑 (回调) 这是 CCmdSocket::OnReceive 调用的函数。
//void CModemSimDlg::OnPcSendCommand()
//{
//	BYTE buffer[4096];	
//	int nRead = m_sktCmd.Receive(buffer, 4096);
//
//	if (nRead > 0)
//	{
//		// 解析收到的命令
//		int nCount = m_ModemProtocol.ParseRecvData(buffer, nRead);
//		if (nCount > 0) {
//			// Log: 解析到命令...
//		}
//
//		buffer[nRead] = 0; // 加上结束符方便显示
//
//		CString strLog;
//		strLog.Format(_T("收到命令: %S"), buffer); // %S 用于将 char* 转为 CString
//		UpdateLog(strLog);
//
//		// TODO: 在这里解析命令，比如解析 buffer 内容
//		if (memcmp(buffer, "START", 5) == 0)
//		{
//			// 开始测井...
//		}
//	}
//}


#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

// 获得本地IP，支持IPv6
std::vector<CString> GetLocalIPsEx() {
	std::vector<CString> ips;
	ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
	ULONG family = AF_UNSPEC; // 同时获取IPv4和IPv6

	PIP_ADAPTER_ADDRESSES pAddresses = NULL;
	ULONG outBufLen = 0;

	// 第一次调用获取缓冲区大小
	if (GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen) == ERROR_BUFFER_OVERFLOW) {
		pAddresses = (PIP_ADAPTER_ADDRESSES)new BYTE[outBufLen];
	}

	// 第二次调用获取实际数据
	if (GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen) == ERROR_SUCCESS) {
		PIP_ADAPTER_ADDRESSES pCurr = pAddresses;
		while (pCurr) {
			PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurr->FirstUnicastAddress;
			while (pUnicast) {
				char ipStr[INET6_ADDRSTRLEN] = { 0 };
				DWORD ipStrSize = sizeof(ipStr);
				if (WSAAddressToStringA(pUnicast->Address.lpSockaddr,
					pUnicast->Address.iSockaddrLength,
					NULL,
					ipStr,
					&ipStrSize) == 0) {
						ips.push_back(CString(ipStr));
				}
				pUnicast = pUnicast->Next;
			}
			pCurr = pCurr->Next;
		}
	}

	if (pAddresses) delete[] pAddresses;
	return ips;
}

// 本机作为客户机向远程服务器发送数据(连在1234上的那个)
void CModemSimDlg::OnBnClickedButtonSendData()
{
	// 2. 生成数据包
	int nPacketLen = 0;

	if(m_bRemoteConnected && !m_bModemConnecting)
	{
		unsigned char cmd[1024] = {1};

		ShowCmd(cmd, sizeof(cmd), 1);
		SendCmd(cmd, sizeof(cmd));
	}

}


/****************************************************************************
*                       CModemSimDlg::CreateListener
* Result: BOOL
*       TRUE if socket created
*       FALSE if error
* Effect: 
*       Creates a listener socket
****************************************************************************/

BOOL CModemSimDlg::CreateListener()
{
	// 判断IP地址是否存在？
	BOOL bIpExist(FALSE);
	vector <CString> ips = GetLocalIPsEx();
	for (size_t i=0; i<ips.size(); i++) {
		if (ips[i].CompareNoCase(m_strLocalIp) == 0)
		{
			bIpExist = TRUE;
			break;
		}
	}

	if(!bIpExist)
	{
		CString str;
		str.Format(_T("本地网卡地址不包括前端机IP地址 %s！请修改本地网卡地址，并正确连接网线！"), m_strLocalIp);
		LogMessage(str);
		return FALSE;
	}

	// Create the listener socket
	CString port(m_strLocalPort);
	//m_editServerPort.GetWindowText(port);
	m_nLocalPort = _tcstoul(port, NULL, 0);

	if(m_nLocalPort == 0)
		m_nLocalPort = LOCAL_PORT_NUM;

	if(!m_listensoc.Create(m_nLocalPort))
	{ // failed to create
		DWORD err = ::GetLastError();
		CString fmt;
		fmt.LoadString(IDS_LISTEN_CREATE_FAILED);
		CString * s = new CString;
		s->Format(fmt, m_nLocalPort);
		PostMessage(UWM_INFO, (WPARAM)s, ::GetCurrentThreadId());
		PostMessage(UWM_NETWORK_ERROR, (WPARAM)err, ::GetCurrentThreadId());
		return FALSE;
	} 

	{ // success 
		CString * s = new CString;
		s->Format(_T("开始监听 %s:%d"), m_strLocalIp, m_nLocalPort);
		//PostMessage(UWM_INFO, (WPARAM)s,::GetCurrentThreadId());
		LogMessage2(*s);
		delete s;
	} 

	m_listensoc.SetTarget(this);

	m_listensoc.Listen();
	return TRUE;
} 

void CModemSimDlg::OnRun()
{
	m_running = CreateListener();
	//if(m_running){
	//	OnConnectModem();
	//}
	UpdateControls();
}

void CModemSimDlg::UpdateControls()
{
	UpdateData(TRUE);

	UINT portnum = _ttoi(m_strLocalPort);
	m_btnRun.EnableWindow(!m_running && portnum > 1023 && portnum <= 65535);

	m_staticLocalIP.EnableWindow(!m_running);
	GetDlgItem(IDC_EDIT_LOCAL_IP)->EnableWindow(!m_running);
	m_staticLocalPort.EnableWindow(!m_running);
	GetDlgItem(IDC_EDIT_LOCAL_PORT)->EnableWindow(!m_running);

	// If we are shutting down, disable the entire dialog!
	EnableWindow(!m_MainWndIsClosing);

	// Modem ip/port状态
	BOOL changeable = !m_bRemoteConnected && !m_bModemConnecting;
	m_staticRemoteIP.EnableWindow(changeable); 
	GetDlgItem(IDC_EDIT_REMOTE_IP)->EnableWindow(changeable);
	m_staticLocalPort.EnableWindow(changeable); 
	GetDlgItem(IDC_EDIT_REMOTE_PORT)->EnableWindow(changeable);

	// 发送数据按钮
	BOOL bSend = m_bRemoteConnected;// && m_bSending;
	//TRACE(_T("UpdateControl: m_bRemoteConnected=%d, m_bSending=%d\n"), m_bRemoteConnected, m_bSending);
	//m_btnSendData.EnableWindow(bSend);

	// 连接/断开按钮
	m_btnConnectRemote.EnableWindow(!m_bModemConnecting && !m_bRemoteConnected && !m_strLocalIp.IsEmpty() && portnum > 1023 && portnum <= 65535);
	m_btnDisconnectRemote.EnableWindow(m_bRemoteConnected);

	// 服务器端发送按钮
	//BOOL isExistServerSocket(FALSE);	// 如果没有客户机连接，则未加
	//for(int i=0; i<m_threadIDs.GetSize(); i++){
	//	CConnectSoc* pScoket = GetSocketByThreadId(m_threadIDs[i]);
	//	if(pScoket)isExistServerSocket = TRUE;
	//}
	//m_btnSend2Clients.EnableWindow(isExistServerSocket);
}


void CModemSimDlg::ShowOpenConnections()
{
	CString s;
	s.Format(_T("%d"), m_open);
	//m_editOpenConnections.SetWindowText(s);
} 

void CModemSimDlg::ShowClosedConnections()
{
	CString s;
	s.Format(_T("%d"), m_close);
	//m_editClosedConnections.SetWindowText(s);
}

void CModemSimDlg::LogMessage(CString msg)
{
	int idx = m_listRecv.AddString(msg);
	m_listRecv.SetTopIndex(idx);	// 自动跳转到最后一行
}

void CModemSimDlg::LogMessage2(CString msg)
{
	int idx = m_listSend.AddString(msg);

	m_listSend.SetTopIndex(idx);	// 自动跳转到最后一行
}

/****************************************************************************
*                       CModemSimDlg::OnThreadStart
* Inputs:
*       WPARAM unused
*       LPARAM lParam: (LPARAM)(DWORD) the thread ID
* Result: LRESULT
*       Logically void, 0, always
* Effect: 
*       Increments the open-thread count.  Adds the thread ID to the
*       array of thread IDs
****************************************************************************/

LRESULT CModemSimDlg::OnThreadStart(WPARAM, LPARAM lParam)
{
	m_open++;

	DWORD ThreadID = (DWORD)lParam;
	m_threadIDs.Add(ThreadID);  // save thread refs so we can post msg to quit later

	// Update the display
	ShowOpenConnections();

	// yubsh add, enable send button
	UpdateControls();
	return 0;
}


/****************************************************************************
*                       CModemSimDlg::OnThreadClose
* Inputs:
*       WPARAM unused
*       LPARAM: lParam: (LPARAM)(DWORD) thread ID
* Result: LRESULT
*       Logically void, 0, always
* Effect: 
*       Records that a thread has closed.  If a thread has closed and
*       a close operation has been deferred, completes the close transaction
****************************************************************************/

LRESULT CModemSimDlg::OnThreadClose(WPARAM, LPARAM lParam)
{
	m_open--;
	m_close++;

	ShowOpenConnections();

	ShowClosedConnections();

	// remove dwThreadID from m_threadIDs
	DWORD dwThreadID = (DWORD)lParam;

	INT_PTR size = m_threadIDs.GetSize();
	for (INT_PTR i = 0; i < size; i++)
	{ /* scan threads */
		if (m_threadIDs[i] ==  dwThreadID)
		{
			m_threadIDs.RemoveAt(i);
			TRACE(_T("%s: Thread 0x%02x is removed\n"), AfxGetApp()->m_pszAppName, dwThreadID);
			break;
		}
	} /* scan threads */

	// If we got a WM_CLOSE and there were open threads,  
	// we deferred the close until all the threads terminated
	// Now we try again  
	if(m_MainWndIsClosing && m_open == 0) 
		PostMessage(WM_CLOSE);  // try the close again

	// yubsh add
	UpdateControls();
	return 0;
}

/****************************************************************************
*                        CModemSimDlg::OnNetworkData
* Inputs:
*       WPARAM: (WPARAM)(CByteArray *) The data to display
*       LPARAM: (LPARAM)(DWORD) the thread ID of the thread that sent it
* Result: LRESULT
*       Logically void, 0, always
* Effect: 
*       Displays the string received
* Notes:
*       Deletes the string
****************************************************************************/

LRESULT CModemSimDlg::OnNetworkData(WPARAM wParam, LPARAM lParam)
{
	DWORD threadid = (DWORD)lParam;
	auto it = m_threadMap.find(threadid);
	if (it != m_threadMap.end()) {	// 发送给list1
		// 上传次数
		//m_nUploadCount++;
		//CString str;
		//str.Format(_T("%ld"), m_nUploadCount);
		//m_editUploadCount.SetWindowTextW(str);
	}

	// a new message has been received. Update the UI
	CByteArray * data = (CByteArray *)wParam;

	int len = data->GetCount();
	byte* cmd = new byte[len+1];
	memcpy(cmd, data->GetData(), len);

	//CString s = ConvertReceivedDataToString(*data);
	//c_LastString.SetWindowText(s);

	//	CString msg;
	//#define SERVER_STRING_INFO_LIMIT 63
	//msg.Format(_T("%x: => \"%-*s\""), (DWORD)lParam, SERVER_STRING_INFO_LIMIT, s);
	//msg.Format(_T("%x: => \"%s\""), (DWORD)lParam, s);
	//	c_Record.AddString(msg);

	ProcessRecvCmds(cmd, len);
	delete[] cmd;

	delete data;

	UpdateControls();
	return 0;
}


/****************************************************************************
*                           CModemSimDlg::OnInfo
* Inputs:
*       WPARAM: (WPARAM)(CString *) String to display
*       LPARAM: (LPARAM)(DWORD) thread id of thread providing info
* Result: LRESULT
*       Logically void, 0, always
* Effect: 
*       Logs the info string
****************************************************************************/

LRESULT CModemSimDlg::OnInfo(WPARAM wParam, LPARAM lParam)
{
	CString * msg = (CString *)wParam;
	DWORD threadid = (DWORD)lParam;
	CString id;
	id.Format(_T("%x: "), threadid);
	*msg = id + *msg;

	// 根据信息来源，决定显示在哪个list
	// 其实只能显示服务器监听到的信息，显示在list2；
	// 作为客户机时，显示在下边list1
	//auto it = m_threadMap.find(threadid);
	//if (it != m_threadMap.end()) {	// 发送给list2
	//	// 勾选暂停显示
	//	//UpdateData(TRUE);
	//	//if(m_bPauseDisplay)
	//	//	return 0;

	//	LogMessage2(*msg);
	//}
	//else {		// 发送给list2
		LogMessage(*msg);
	//}

	delete msg;

	return 0;
}


/****************************************************************************
*                       CModemSimDlg::OnNetworkError
* Inputs:
*       WPARAM: (WPARAM)(DWORD) error code
*       LPARAM: (LPARAM)(DWORD) thread that issued the error
* Result: LRESULT
*       Logically void, 0, always
* Effect: 
*       Records the error
****************************************************************************/

LRESULT CModemSimDlg::OnNetworkError(WPARAM wParam, LPARAM lParam)
{
	DWORD err = (DWORD)wParam;
	CString msg = ErrorString(err);
	CString id;
	id.Format(_T("%x: "), (DWORD)lParam);
	LogMessage(id + msg);
	return 0;
}

/****************************************************************************
*                       CModemSimDlg::OnSendComplete
* Inputs:
*       WPARAM: unused
*       LPARAM: (WPARAM)(DWORD) Thread ID of thread that sent data
* Result: LRESULT
*       Logically void, 0, always
* Effect: 
*       Logs the data-sent event
****************************************************************************/

LRESULT CModemSimDlg::OnSendComplete(WPARAM, LPARAM lParam)
{
	return 0;

	// 命令发送完毕，显示信息。根据线城id判断是主机监听到的客户机连接（显示在list1），还是本机发送给服务器的线程（显示在list2）
	DWORD threadid = (DWORD)lParam;

	CString fmt;
	fmt.LoadString(IDS_DATA_SENT);

	CString msg;
	auto it = m_threadMap.find(threadid);
	if (it != m_threadMap.end()) {	// 发送给list1
		msg.Format(fmt, threadid);
		LogMessage(msg);
	}
	else {		// 发送给list2
		msg.Format(fmt, threadid);
		LogMessage2(msg);
	}
	return 0;
} 

/****************************************************************************
*                     CModemSimDlg::OnConnectionClose
* Inputs:
*       WPARAM: ignored
*       LPARAM: (LPARAM)(DWORD) thread ID of thread that closed
* Result: LRESULT
*       Logically void, 0, always
* Effect: 
*       Reports that the connection has closed and the client thread has
*       terminated.
****************************************************************************/

LRESULT CModemSimDlg::OnConnectionClose(WPARAM, LPARAM lParam)
{
	TRACE(_T("%s: CModemSimDlg::OnConnectionClose\n"), AfxGetApp()->m_pszAppName);
	m_pClientThread = NULL;
	m_bRemoteConnected = FALSE;
	m_bModemConnecting = FALSE;

	// 客户机的连接被断开，显示信息。根据线城id判断是主机监听到的客户机连接（显示在list1），还是本机发送给服务器的线程（显示在list2）
	DWORD threadid = (DWORD)lParam;

	CString fmt;
	fmt.LoadString(IDS_CLOSED);
	//c_ConnectionStatus.SetWindowText(s);

	CString msg;
	auto it = m_threadMap.find(threadid);
	if (it != m_threadMap.end()) {	// 发送给list2
		msg.Format(_T("%x: %s"), threadid, fmt);
		//msg.Format(fmt, threadid);
		LogMessage2(msg);
	}
	else	// 发送给list2
	{
		msg.Format(_T("%x: %s"), threadid, fmt);
		//msg.Format(fmt, threadid);
		LogMessage(msg);
	}

	UpdateControls();
	if(m_MainWndIsClosing)
		PostMessage(WM_CLOSE);
	return 0;
}
/****************************************************************************
*                      CModemSimDlg::OnConnectionMade
* Inputs:
*       WPARAM: ignored
*       LPARAM: Thread ID of thread making connection (ignored)
* Result: LRESULT
*       LOgically void, 0, always
* Effect: 
*       Notes that the connection has been closed by the server
****************************************************************************/

LRESULT CModemSimDlg::OnConnectionMade(WPARAM, LPARAM)
{
	// callback from the client socket thread to signify a connection has
	// been established. 

	TRACE(_T("%s: CModemSimDlg::OnConnectionMade\n"), AfxGetApp()->m_pszAppName);
	m_bRemoteConnected = TRUE;
	m_bModemConnecting = FALSE;
	m_bSending = FALSE;
	CString s;
	s.LoadString(IDS_CONNECTED_TO);
	CString msg;
	msg.Format(_T("%s \"%s\""), s, m_strRemoteIp);
	//c_ConnectionStatus.SetWindowText(msg);
	LogMessage(msg);

	UpdateControls();
	return 0;
}

BOOL CModemSimDlg::CleanupThreads() 
{
	INT_PTR size = m_threadIDs.GetSize();
	if(size == 0)                      
		return TRUE;                    

	for (INT_PTR i = 0; i < size; i++)
	{ /* scan threads */
		if (!::PostThreadMessage(m_threadIDs[i], UWM_TERM_THREAD, 0, 0))
		{ /* failed */
			TRACE(_T("%s: Thread 0x%02x possibly already terminated\n"), AfxGetApp()->m_pszAppName, m_threadIDs[i]);
			m_threadIDs.RemoveAt(i);
		} /* failed */
	} /* scan threads */

	// Note that if PostThreadMessage has failed and all the target threads have
	// been removed from the array, we are done

	if(m_threadIDs.GetSize() == 0)
		return TRUE;

	return FALSE;
}

// 显示发送命令，nList=0/1，分别显示在上下两个list中
void CModemSimDlg::ShowCmd(unsigned char *indata, LONG inlen, int nList)
{
	if(m_bPauseDisplay)return;

	CString str, str1;
	str.Format(_T("[TX%03d]"), inlen);

	int nBytesPerLine = 40;
	int j, lines = inlen / nBytesPerLine;
	if( (inlen - inlen/nBytesPerLine * nBytesPerLine) != 0)lines++;
	for(int i=0; i<lines; i++)
	{
		if( i == lines - 1)				// 最末一行
		{
			for(j=0; j<(inlen - i*nBytesPerLine); j++)
			{
				if(j%10==0)
					str1.Format(_T("  %02X"), indata[i*nBytesPerLine+j]);
				else
					str1.Format(_T(" %02X"), indata[i*nBytesPerLine+j]);
				str += str1;
			}
			int blank = nBytesPerLine - (inlen - i*nBytesPerLine);
			for(int k=0; k<blank; k++)
				str += "   ";
			if(j<10)str += "  ";		// 中间的横线
			if(j%10==0)str += "  ";		// 刚好10个字节，则增加一个空格
		}
		else			// 不是最末一行
		{	
			for(int j=0; j<nBytesPerLine; j++)
			{
				if(j%10==0)
					str1.Format(_T("  %02X"), indata[i*nBytesPerLine+j]);
				else
					str1.Format(_T(" %02X"), indata[i*nBytesPerLine+j]);
				str += str1;
			}
		}

		nList == 0 ? m_listRecv.AddString( str ) : m_listSend.AddString( str );// + strLine);
		str.Format(_T("       "));
	}

	nList == 0 ? m_listRecv.SetTopIndex(m_listRecv.GetCount() - 1) : m_listSend.SetTopIndex(m_listSend.GetCount() - 1);
}

// 处理监听接收到的命令
void CModemSimDlg::ProcessRecvCmds(byte *data, int len)
{
	m_Protocol.ParseRecvData(data, len);
	BOOL bLogging = m_Protocol.GetLoggingState();
	m_strCmdLogging = bLogging  ? _T("开始上传") :  _T("结束上传");

	if(bLogging){
		if(m_bRemoteConnected && !m_bModemConnecting)		// 连接后才能发送数据
		{
			m_nSendDataTimer = SetTimer(TIMER_SEND_DATA, 40, NULL);		// 40ms间隔

			// 设置波形初始时间
			{
				CTime t = CTime::GetCurrentTime();  
				for(int i=0; i<NUM_WAVEFORM; i++)
				{
					if(m_wave[i].pWaveCtrl)
						m_wave[i].pWaveCtrl->SetStartTime(t);
				}
			}

			m_nViewStartPoint = 0;		// 图形控件的起点为0
		}
	}else {
		KillTimer(m_nSendDataTimer);   
	}

	UpdateData(FALSE);
}

void CModemSimDlg::OnClose()
{
	TRACE(_T("%s: CAsyncServerDlg::OnClose\n"), AfxGetApp()->m_pszAppName);

	UpdateData(TRUE);
	m_ini.WriteString(_T("Setup"), _T("ServerIp"), m_strLocalIp);
	m_ini.WriteString(_T("Setup"), _T("ServerPort"), m_strLocalPort);
	m_ini.WriteString(_T("Setup"), _T("ModemIp"), m_strRemoteIp);
	m_ini.WriteString(_T("Setup"), _T("ModemPort"), m_strRemotePort);
	m_ini.WriteBool(_T("Setup"), _T("PauseDisplay"), m_bPauseDisplay);

	CString str;
	m_editCclAmplitude.GetWindowText(str);		 
	m_ini.WriteString(_T("Setup"), _T("CclAmplitude"), str);
	m_editCclFreq.GetWindowText(str);
	m_ini.WriteString(_T("Setup"), _T("CclFreq"), str);
	m_editChargeVoltAmplitude.GetWindowText(str);
	m_ini.WriteString(_T("Setup"), _T("ChargeVoltAmplitude"), str);
	m_editChargeVoltFreq.GetWindowText(str);
	m_ini.WriteString(_T("Setup"), _T("ChargeVoltFreq"), str);

	// 断开Modem连接
	if(m_bRemoteConnected)
		OnDisconnectRemote();

	m_MainWndIsClosing = TRUE;
	UpdateControls();

	if(!CleanupThreads())	// defer shutdow
	{
		TRACE(_T("%s: CiWasPowerDlg::OnClose: deferring close\n"), AfxGetApp()->m_pszAppName);
		return;
	}

	if(m_nSendDataTimer)
		KillTimer(m_nSendDataTimer);

	for(int i=0; i<NUM_WAVEFORM; i++)
	{
		if(m_wave[i].pWaveCtrl){
			delete m_wave[i].pWaveCtrl;
			m_wave[i].pWaveCtrl = NULL;
		}
	}

	CDialogEx::OnOK();
}


void CModemSimDlg::SendCmd(byte* cmd, int len)
{
	TRACE(_T("%s: CModemSimDlg::OnSend\n"), AfxGetApp()->m_pszAppName);
	int count = len;

	// 转换成字节数组
	CByteArray byteArray;
	byteArray.SetSize(len);
	memcpy(byteArray.GetData(), cmd, count);

	// 显示
	//CString log;
	//CString str = ConvertReceivedDataToString(byteArray);
	//log.Format(_T("=> [%d] \"%-*s\""), count, 100, str);
	//m_list2.AddString(log);

	// 发送数据：直接给线程发消息
	m_pClientThread->Send(byteArray);

	m_nSendCount += count;
	CString str;
	str.Format(_T("%d"), m_nSendCount);
	m_staticSendCount.SetWindowTextW(str);

	//UpdateData(FALSE);
	m_bSending = TRUE;
	UpdateControls();
}

enum WAVE_TYPE
{
	Wave_Sine = 0,		// 正弦波
	Wave_Square,		// 方波
	Wave_Triangle,		// 三角波
	Wave_Sawtooth,		// 锯齿波
	Wave_DC				// 直流
};

/**
 * @brief 计算各种波形的有效值(RMS)
 * @param waveform_type 波形类型：0=正弦波, 1=方波, 2=三角波, 3=锯齿波, 4=直流
 * @param value 输入值：对于瞬时值模式是当前采样值，对于峰值模式是峰值电压
 * @param mode 计算模式：0=使用瞬时值计算，1=使用峰值计算
 * @param samples 瞬时值采样数组(仅mode=0时使用)
 * @param num_samples 采样点数(仅mode=0时使用)
 * @return 计算得到的有效值(RMS)
 */
double calculate_rms(int waveform_type, double value, int mode, 
                   const double* samples, int num_samples) 
{
    double rms = 0.0;
    
    if (mode == 0) {        // 模式0：基于瞬时值采样计算RMS
        if (num_samples <= 0 || samples == NULL) {
            return 0.0;
        }
        
        double sum_of_squares = 0.0;
        for (int i = 0; i < num_samples; i++) {
            sum_of_squares += samples[i] * samples[i];
        }
        rms = sqrt(sum_of_squares / num_samples);
    } 
    else {					// 模式1：基于峰值计算RMS
        switch (waveform_type) {
            case Wave_Sine: // 正弦波
                rms = value / sqrt(2.0); // U_rms = U_peak / √2
                break;
                
            case Wave_Square: // 方波
                rms = value; // 方波的有效值等于峰值
                break;
                
            case Wave_Triangle: // 三角波
                rms = value / sqrt(3.0); // U_rms = U_peak / √3
                break;
                
            case Wave_Sawtooth: // 锯齿波
                rms = value / sqrt(3.0); // 同三角波
                break;
                
            case Wave_DC: // 直流
                rms = value; // 直流有效值就是其本身
                break;
                
            default:
                rms = 0.0;
                break;
        }
    }
    
    return rms;
}

#define SAMPLE_INTERVAL 0.040	// 40ms

//static void LeftShift(std::vector <double>& array, int k)
//{
//	int N = static_cast<int>(array.size());
//	if (N <= 0 || k <= 0) return;
//
//	if (k >= N) {
//		std::fill(array.begin(), array.end(), 0.0);
//		return;
//	}
//
//	// 使用 std::copy 移动有效数据
//	std::copy(array.begin() + k, array.end(), array.begin());
//
//	// 使用 std::fill 填充末尾零
//	std::fill(array.end() - k, array.end(), 0.0);
//} 
#include <algorithm> // 用于 std::copy, std::fill

/**
 * @brief 将array简单左移new_array.size()位，并用new_array填充空出的末尾位置
 * @param array 原始数组，将被原地修改
 * @param new_array 用于填充的新数据数组
 * @note 这是非循环移动，前shift个元素被丢弃，末尾填充新数据
 */
void LeftShift(std::vector<double>& array, const std::vector<double>& new_array) {
    // 使用size_t避免符号转换警告
    std::size_t shift = new_array.size();
    std::size_t N = array.size();
    
    // 1. 边界情况处理
    if (N == 0) return; // 空数组
    if (shift == 0) return; // 不移位
    
    // 2. 如果shift >= N，整个数组被新数据替换
    if (shift >= N) {
        std::size_t copyCount = (N < new_array.size()) ? N : new_array.size();
        if (copyCount > 0) {
            // 使用std::copy，这是C++98就有的标准算法
            std::copy(new_array.begin(), 
                     new_array.begin() + copyCount, 
                     array.begin());
        }
        // 如果需要用0填充剩余位置，取消注释：
        // if (copyCount < N) {
        //     std::fill(array.begin() + copyCount, array.end(), 0.0);
        // }
        return;
    }
    
    // 3. 核心操作：简单左移shift位
    // 方法1：使用std::copy（推荐，清晰且通常有优化）
    std::copy(array.begin() + shift, array.end(), array.begin());
    
    // 方法2：使用手动循环（C++11的range-based for不适用，因为需要索引）
    // for (std::size_t i = 0; i < N - shift; ++i) {
    //     array[i] = array[i + shift];
    // }
    
    // 4. 用new_array填充末尾的shift个位置
    std::size_t copyCount = (shift < new_array.size()) ? shift : new_array.size();
    if (copyCount > 0) {
        std::copy(new_array.begin(), 
                 new_array.begin() + copyCount, 
                 array.end() - shift);
    }
    
    // 5. 如果new_array比shift短，可选择填充策略
    // 选项A：保留移动来的旧值（默认行为）
    // 选项B：用0填充剩余位置（取消注释）
    // if (copyCount < shift) {
    //     std::fill(array.end() - (shift - copyCount), array.end(), 0.0);
    // }
    // 选项C：用特定值填充
    // if (copyCount < shift) {
    //     double fillValue = 0.0; // 或其他值
    //     std::fill(array.end() - (shift - copyCount), array.end(), fillValue);
    // }
}

// 定时器每次生成一个新的数据值
// dt - 采样间隔0.04s
// dtime - 累计dt，当前时间
double CalcNewValue_1(int nWaveType, double dAmplitude, double dFreq)
{
	double value;
	double AMPLITUDE(dAmplitude);
	double FREQUENCY(dFreq);

	double dt_timer = 0.04;  // 定时器间隔 (s)
	static int m_current_segment = 0;
	double t_start = m_current_segment * dt_timer;
	double m_time(0);

	// 更新时间
	m_time = t_start;		// 采样间隔0.04s
	switch(nWaveType)
	{	
	case Wave_Sine:		// 计算正弦波值：y = A * sin(2πft)
		value = AMPLITUDE * sin(2.0 * M_PI * FREQUENCY * m_time);
		break;
	case Wave_Square:	// 计算方波值（周期 = 1/26 ≈ 38.46ms）
		{
			double period = 1.0 / FREQUENCY; // 周期
			double phase = fmod(m_time, period) / period; // 归一化相位 [0, 1)
			value = (phase < 0.5) ? AMPLITUDE : -AMPLITUDE; // 方波：前半周期 +190，后半周期 -190
		}
		break;
	case Wave_Triangle: // 三角波
		{
			double period = 1.0 / FREQUENCY; // 周期 ≈ 38.46ms
			double phase = fmod(m_time, period) / period; // 归一化相位 [0, 1)
			if (phase < 0.5) {
				// 线性上升：-190 到 +190
				value = AMPLITUDE * (-1.0 + 4.0 * phase); // y = -190 + 4*190*phase
			} else {
				// 线性下降：+190 到 -190
				value = AMPLITUDE * (3.0 - 4.0 * phase); // y = 190 - 4*190*(phase-0.5)
			}
		}
		break;

	case Wave_Sawtooth: // 锯齿波
		{
			// 计算锯齿波值
			double period = 1.0 / FREQUENCY; // 周期 ≈ 38.46ms
			double phase = fmod(m_time, period) / period; // 归一化相位 [0, 1)
			value = AMPLITUDE * (-1.0 + 2.0 * phase); // 线性上升：-190 到 +190
		}
		break;

	case Wave_DC: // 直流
		value = AMPLITUDE;
		break;
	default:
		break;
	}
	m_current_segment++;

	return value;
}


// 定时器每次生成100个新的数据值
// dt - 采样间隔0.04s
// dtime - 累计dt，当前时间
vector <double> CalcNewValue_100(int nWaveType, double dAmplitude, double dFreq)
{
	vector <double> vec_value;
	double AMPLITUDE(dAmplitude);
	double FREQUENCY(dFreq);
	const double dt_timer = 0.04;
	const int N_sub = 100;
	const double dt_sub = dt_timer / (N_sub - 1.0);

	static int m_current_segment = 0;
	double t_start = m_current_segment * dt_timer;
	double m_time(0);
	double value;

	// 计算本段 100 点
	for (int j = 0; j < N_sub; ++j) 
	{
		double t_sub = t_start + j * dt_sub;

		// 更新时间
		m_time = t_sub;		// 子间隔 ≈0.000404 s
		switch(nWaveType)
		{	
		case Wave_Sine:		// 计算正弦波值：y = A * sin(2πft)
			value = AMPLITUDE * sin(2.0 * M_PI * FREQUENCY * m_time);
			break;
		case Wave_Square:	// 计算方波值（周期 = 1/26 ≈ 38.46ms）
			{
				double period = 1.0 / FREQUENCY; // 周期
				double phase = fmod(m_time, period) / period; // 归一化相位 [0, 1)
				value = (phase < 0.5) ? AMPLITUDE : -AMPLITUDE; // 方波：前半周期 +190，后半周期 -190
			}
			break;
		case Wave_Triangle: // 三角波
			{
				double period = 1.0 / FREQUENCY; // 周期 ≈ 38.46ms
				double phase = fmod(m_time, period) / period; // 归一化相位 [0, 1)
				if (phase < 0.5) {
					// 线性上升：-190 到 +190
					value = AMPLITUDE * (-1.0 + 4.0 * phase); // y = -190 + 4*190*phase
				} else {
					// 线性下降：+190 到 -190
					value = AMPLITUDE * (3.0 - 4.0 * phase); // y = 190 - 4*190*(phase-0.5)
				}
			}
			break;

		case Wave_Sawtooth: // 锯齿波
			{
				// 计算锯齿波值
				double period = 1.0 / FREQUENCY; // 周期 ≈ 38.46ms
				double phase = fmod(m_time, period) / period; // 归一化相位 [0, 1)
				value = AMPLITUDE * (-1.0 + 2.0 * phase); // 线性上升：-190 到 +190
			}
			break;

		case Wave_DC: // 直流
			value = AMPLITUDE;
			break;
		default:
			break;
		}

		vec_value.push_back(value);
	}
	m_current_segment++;

	return vec_value;
}

// 40ms间隔发送一帧数据
void CModemSimDlg::OnTimer(UINT_PTR nIDEvent)
{
	// 从UI获得频率和幅度
	CString str;
	m_editCclAmplitude.GetWindowTextW(str);				m_wave[0].nAmplitude = _ttof(str);
	m_editCclFreq.GetWindowTextW(str);					m_wave[0].nFreq = _ttof(str);
	m_editChargeVoltAmplitude.GetWindowTextW(str);		m_wave[1].nAmplitude = _ttof(str);
	m_editChargeVoltFreq.GetWindowTextW(str);			m_wave[1].nFreq = _ttof(str);

	switch(nIDEvent)  
	{
	case TIMER_SEND_DATA:
		if(m_bRemoteConnected && !m_bModemConnecting)
		{
			// 上方ccl波形，每次产生一个点
			vector <double> vec_ccl;
			if(NULL != m_wave[0].pWaveCtrl)
			{
				double FREQUENCY = m_wave[0].nFreq;
				double AMPLITUDE = m_wave[0].nAmplitude;

				double dt = 1.0F/m_wave[0].nSampleFreq;		// 采样间隔，1s生成25个数据点，= 0.04s
				double value = CalcNewValue_1(m_wave[0].nWaveType, AMPLITUDE, FREQUENCY);	
				
				// 不要负数
				value += AMPLITUDE;

				// 现有数据左移
				vec_ccl.push_back(value);
				LeftShift(m_wave[0].data, vec_ccl);

				double* pdata = m_wave[0].data.data();
				m_wave[0].pWaveCtrl->SetBuffer(pdata);

				m_wave[0].pWaveCtrl->SetViewPosition( m_nViewStartPoint);		// 让波形标注的时间走起来
				m_wave[0].pWaveCtrl->SetGraphWorldCoords(0, m_wave[0].nPoints, 0, AMPLITUDE*2 /*-AMPLITUDE, AMPLITUDE*/);
				m_wave[0].pWaveCtrl->Invalidate();
			}
	
			// 下方ChargeVolt波形，每次产生100个点
			vector <double> vec_volt;
			if(NULL != m_wave[1].pWaveCtrl)
			{
				double FREQUENCY = m_wave[1].nFreq;
				double AMPLITUDE = m_wave[1].nAmplitude;

				vec_volt = CalcNewValue_100(m_wave[1].nWaveType, AMPLITUDE, FREQUENCY);

				// 不要负数
				for (size_t i=0; i<vec_volt.size(); i++)
				{
					vec_volt[i] += AMPLITUDE;
				}

				// 现有数据左移
				LeftShift(m_wave[1].data, vec_volt);

				double* pdata = m_wave[1].data.data();
				m_wave[1].pWaveCtrl->SetBuffer(pdata);

				m_wave[1].pWaveCtrl->SetViewPosition(m_nViewStartPoint * m_wave[1].nStep);		// 让波形标注的时间走起来，每次走100个点
				m_wave[1].pWaveCtrl->SetGraphWorldCoords(0, m_wave[1].nPoints, 0, AMPLITUDE*2 /*-AMPLITUDE, AMPLITUDE*/);
				m_wave[1].pWaveCtrl->Invalidate();
			}

			m_nViewStartPoint++;

			{
				// 分析接收到的命令参数
				ToolCommandManager vecCmds = m_Protocol.GetToolCommands();
				SimInstrumentData m_CanMaster(0x87 | 0x100);				// 改为0x187	
				SimInstrumentData m_CanShockwave(0x180);

				static WORD wFrameNo(0);			// 帧号
				WORD buffer[100] = {0};
				int nOffset = 0;	// 数据长度偏移量，单位：字
				WORD wFrameLen(0);

				// 通讯短节（MODEM上传数据）不在这里准备

				// 发送帧里设置的每只仪器接收长度
				WORD wFrameLen87(0), wFrameLen180(0);		
				for (size_t i=0; i<vecCmds.size(); i++)
				{
					ToolSubCommand vecCmd = vecCmds.GetSubCommand(i);
					std::vector<WORD> vecParam = vecCmd.getParams();
					for (size_t j=0; j<vecParam.size(); j++)
					{
						if(vecParam[0] == 0x87)
						{
							wFrameLen87 = vecParam[1];
							break;
						}
						if(vecParam[0] == 0x180)
						{
							wFrameLen180 = vecParam[1];

							// 仪器参数
							if(vecParam.size() > 2)
								m_strCmdCharge = vecParam[2] ?  _T("打开充电") : _T("关闭充电");
							else
								m_strCmdCharge = _T("关闭充电");
							UpdateData(FALSE);
							break;
						}
					}
				}

				// CAN节点0x87，冲击波仪器
				nOffset = 0;						// 数据长度偏移量，单位：字
				wFrameLen = wFrameLen87 - 8;		// 长度，字节长，=接收到的长度-8=34字节=17字=0x11
				buffer[nOffset++] = wFrameNo;		// 帧号
				buffer[nOffset++] = 0x00;			// 状态标识字，标识仪器工作状态
				buffer[nOffset++] = 0x00;			// 返回仪器命令，仪器接收到的最近命令字
				WORD wUpdataDataLen = wFrameLen/2 - 5;	// 12个字的数据
				for(int i=0; i<wUpdataDataLen; i++)
					buffer[nOffset++] = i;
				buffer[nOffset++] = 0x2020;			// 校验预留字1，ERC1
				buffer[nOffset++] = 0x3020;			// 校验预留字2，ERC2
				m_CanMaster.AddBytes((BYTE*)buffer, wFrameLen);	// 字节长度=34字节
			
				// 仪器帧尾不在仪器长度以内，这里不理睬

				// CAN节点0x180，冲击波仪器
				wFrameLen = wFrameLen180 - 3;			// 长度，字数，=发送的字数-3
				BuildShockwaveData(m_CanShockwave, wFrameNo, wFrameLen, vec_ccl, vec_volt);
				wFrameNo++;

				std::vector<SimInstrumentData> vecInstList;
				vecInstList.push_back(m_CanMaster);
				vecInstList.push_back(m_CanShockwave);

				// 与MODEM数据组合并发送
				{
					BYTE sendBuf[512];
					int nLen = 0;

					// 生成完美合规的 156 字节数据包
					m_Protocol.GenerateSimPacket(sendBuf, nLen, vecInstList);

					// 发送
					ShowCmd(sendBuf, nLen, 1);
					SendCmd(sendBuf, nLen);
					TRACE("Send Len=%d\n", nLen);
				}
			}
		}

		break;
	}

	CDialogEx::OnTimer(nIDEvent);
}

// nFrameNo  帧同步，每次+1
// nFrameLen 返回的数据帧长度
void CModemSimDlg::BuildShockwaveData(SimInstrumentData& data, WORD wFrameNo, WORD wFrameLen, std::vector <double> vec_ccl, std::vector <double> vec_volt)
{
	// 冲击波数据
	WORD ShockwaveData[150] = {0};
	int nOffset = 0;	// 数据长度偏移量，单位：字

	// 帧头55AA、仪器ID、长度最后添加，这里不理睬
	ShockwaveData[nOffset++] = wFrameNo;		// 帧号，自动加1
	ShockwaveData[nOffset++] = 0x00;						// 仪器工作状态字1，保留
	ShockwaveData[nOffset++] = 0x00;						// 仪器工作状态字2，保留
	ShockwaveData[nOffset++] = (WORD)(vec_ccl[0]+0.5);		// CCL值

	for(int i=0; i<wFrameLen - 7; i++)			// 100个充电电压值
		ShockwaveData[nOffset++] = (WORD)(vec_volt[i]+0.5);

	ShockwaveData[nOffset++] = 0x00;						// 保留
	ShockwaveData[nOffset++] = 0x00;						// 保留
	ShockwaveData[nOffset++] = CalcChecksum(ShockwaveData, 105);	// 校验字

	data.AddBytes((BYTE*)ShockwaveData, nOffset * 2);		// 此处长度为字节
}

void CModemSimDlg::OnCbnSelchangeComboCclValue()
{
	m_wave[0].nWaveType = m_cmbCCL.GetCurSel();
	m_ini.WriteInt(_T("Setup"), _T("CClWaveType"), m_wave[0].nWaveType);
	m_wave[0].pWaveCtrl->Invalidate();
}

void CModemSimDlg::OnCbnSelchangeComboChargeVolt()
{
	m_wave[1].nWaveType = m_cmbChargeVolt.GetCurSel();
	m_ini.WriteInt(_T("Setup"), _T("ChargeVoltWaveType"), m_wave[1].nWaveType);
	m_wave[1].pWaveCtrl->Invalidate();
}

// 列表框的右键菜单
void CModemSimDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
	if (pWnd == &m_listRecv)
	{
		CMenu* pPopup = m_contextMenu.GetSubMenu(0);
		if (pPopup != NULL)
		{
			// 转换为屏幕坐标
			if (point.x == -1 && point.y == -1)
			{
				// 键盘触发的上下文菜单
				CRect rect;
				m_listRecv.GetWindowRect(&rect);
				point = rect.TopLeft();
				point.y += rect.Height() / 2;
			}

			pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, 
				point.x, point.y, this);
		}
	}
	else if (pWnd == &m_listSend)
	{
		CMenu* pPopup = m_contextMenu2.GetSubMenu(0);
		if (pPopup != NULL)
		{
			// 转换为屏幕坐标
			if (point.x == -1 && point.y == -1)
			{
				// 键盘触发的上下文菜单
				CRect rect;
				m_listSend.GetWindowRect(&rect);
				point = rect.TopLeft();
				point.y += rect.Height() / 2;
			}

			pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, 
				point.x, point.y, this);
		}
	}
}

// 拷贝到剪裁板
void CModemSimDlg::OnCopy()
{
	CString str, str2;
	CString source;
	int nCount = m_listRecv.GetCount();
	if (nCount > 0) {
		for (int i = 0; i < nCount; i++)
		{
			int n = m_listRecv.GetTextLen(i);
			m_listRecv.GetText(i, str.GetBuffer(n));
			str.ReleaseBuffer();

			// 以\n结尾的不再加\n，否则会多出一个空白行
			if(n > 0 && str.GetAt(str.GetLength() - 1) == _T('\n'))
				str2.Format(_T("%s"), str.GetBuffer(0));
			else
				str2.Format(_T("%s\n"), str.GetBuffer(0));
			source += str2;
		}

		USES_CONVERSION;

		// put your text in source
		if (OpenClipboard())
		{
			LPTSTR lptstrCopy;
			HGLOBAL hglbCopy;
			int cch;

			// Open the clipboard, and empty it. 
			EmptyClipboard();

			// Allocate a global memory object for the text. 
			cch = source.GetLength() + 1;
			hglbCopy = GlobalAlloc(GMEM_DDESHARE, (cch + 1) * sizeof(TCHAR));

			// Lock the handle and copy the text to the buffer.
			lptstrCopy = (LPTSTR)GlobalLock(hglbCopy);
			memcpy(lptstrCopy, W2A(source), cch * sizeof(TCHAR));
			GlobalUnlock(hglbCopy);
			SetClipboardData(CF_TEXT, lptstrCopy);
			CloseClipboard();
		}
	}
}

// 拷贝到剪裁板
void CModemSimDlg::OnCopy2()
{
	CString str, str2;
	CString source;
	int nCount = m_listSend.GetCount();
	if (nCount > 0) {
		for (int i = 0; i < nCount; i++)
		{
			int n = m_listSend.GetTextLen(i);
			m_listSend.GetText(i, str.GetBuffer(n));
			str.ReleaseBuffer();

			// 以\n结尾的不再加\n，否则会多出一个空白行
			if(n > 0 && str.GetAt(str.GetLength() - 1) == _T('\n'))
				str2.Format(_T("%s"), str.GetBuffer(0));
			else
				str2.Format(_T("%s\n"), str.GetBuffer(0));
			source += str2;
		}

		USES_CONVERSION;

		// put your text in source
		if (OpenClipboard())
		{
			LPTSTR lptstrCopy;
			HGLOBAL hglbCopy;
			int cch;

			// Open the clipboard, and empty it. 
			EmptyClipboard();

			// Allocate a global memory object for the text. 
			cch = source.GetLength() + 1;
			hglbCopy = GlobalAlloc(GMEM_DDESHARE, (cch + 1) * sizeof(TCHAR));

			// Lock the handle and copy the text to the buffer.
			lptstrCopy = (LPTSTR)GlobalLock(hglbCopy);
			memcpy(lptstrCopy, W2A(source), cch * sizeof(TCHAR));
			GlobalUnlock(hglbCopy);
			SetClipboardData(CF_TEXT, lptstrCopy);
			CloseClipboard();
		}
	}
}

// 清列表显示
void CModemSimDlg::OnClear()
{
	m_listRecv.ResetContent();
}

// 清列表显示
void CModemSimDlg::OnClear2()
{
	m_listSend.ResetContent();
}


void CModemSimDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	if (m_dlgInitRect.Width() == 0 || m_dlgInitRect.Height() == 0)
		return;

	float fx = (float)cx / m_dlgInitRect.Width();
	float fy = (float)cy / m_dlgInitRect.Height();
	float fscale = min(fx, fy); // 字体保持比例一致

	int fontSize = (int)(-20 * fscale); // 假设原始字体是10pt
	if (fontSize > -4) fontSize = -4;   // 防止太小

	for (std::map<UINT, CRect>::iterator it = m_mapCtrlRects.begin(); it != m_mapCtrlRects.end(); ++it)
	{
		UINT id = it->first;
		CRect origRect = it->second;


		CRect newRect(origRect);
		int height = origRect.Height();
		newRect.left   = (LONG)(origRect.left * fx);
		newRect.top    = (LONG)(origRect.top * fy);
		newRect.right  = (LONG)(origRect.right * fx);
		newRect.bottom = (LONG)(origRect.bottom * fy);

		CWnd* pCtrl = GetDlgItem(id);
		if(!pCtrl)
			return;

		// 编辑框和文本控件不改变高度，注意，文本控件需改名字
		if(pCtrl->GetRuntimeClass() == RUNTIME_CLASS(CEdit) || pCtrl->GetRuntimeClass() == RUNTIME_CLASS(CStatic))
		{
			newRect.bottom = newRect.top + height;
		}

		if (pCtrl)
		{
			pCtrl->MoveWindow(&newRect);
			//pCtrl->SetFont(&m_scaledFont); // 设置新字体
		}
	}
	Invalidate();

}


void CModemSimDlg::SaveInitRect()
{
	// 保存对话框初始大小
	GetClientRect(&m_dlgInitRect);

	CWnd* pChild = GetWindow(GW_CHILD);
	while(pChild)
	{
		UINT id = pChild->GetDlgCtrlID();
		if (id != 0)
		{
			CRect rc;
			pChild->GetWindowRect(&rc);
			ScreenToClient(&rc);
			m_mapCtrlRects[id] = rc;
		}

		pChild = pChild->GetNextWindow(); // 继续下一个控件
	}
}


void CModemSimDlg::OnBnClickedCheckPauseDisplay()
{
	UpdateData(TRUE);
	TRACE("m_bPauseDisplay=%d\n", m_bPauseDisplay);
}


void CModemSimDlg::OnCbnSelchangeComboIpSelect()
{
	StopListener();

	m_nIpSelect = m_cmbIPSelect.GetCurSel();
	if(m_nIpSelect == 0)		// 本地IP
	{
		m_strLocalIp = _T("127.0.0.1");
		m_strRemoteIp = _T("127.0.0.1");
	}
	else
	{
		m_strLocalIp = m_ini.GetString(_T("Setup"), _T("ServerIp_Orig"), _T("192.168.0.70"));
		m_strRemoteIp = m_ini.GetString(_T("Setup"), _T("ModemIp_Orig"), _T("192.168.0.89"));
	}
	UpdateData(FALSE);

	m_ini.WriteInt(_T("Setup"), _T("IpSelect"), m_nIpSelect);
	m_ini.WriteString(_T("Setup"), _T("ServerIp"), m_strLocalIp);
	m_ini.WriteString(_T("Setup"), _T("ModemIp"), m_strRemoteIp);

	OnRun();
}

// 停止监听
void CModemSimDlg::StopListener()
{
	// 1. 停止接受新连接
	if (m_listensoc.m_hSocket != INVALID_SOCKET)
	{
		// 关闭监听 socket
		m_listensoc.Close();

		CString * s = new CString;
		s->Format(_T("停止监听 %s:%d"), m_strLocalIp, m_nLocalPort);
		LogMessage2(*s);
		delete s;
	}

	CleanupThreads();

	m_running = FALSE;
	UpdateControls();
}
