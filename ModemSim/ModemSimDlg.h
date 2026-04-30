
// ModemSimDlg.h : 头文件
//

#pragma once

#include "ModemSim.h"
#include "MyIniFile.h"
#include "ClientSocket.h"
#include "ModemProtocol.h"

#include "Listens.h"    // Added by ClassView
#include "ServerT.h"
#include "ClientT.h"
#include "WaveCtrl.h"

#include <vector>
#include <map>

// CModemSimDlg 对话框
class CModemSimDlg : public CDialogEx
{
// 构造
public:
	CModemSimDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_MODEMSIM_DIALOG };

	std::map<DWORD, CServerThread*> m_threadMap; // 存储线程D 和 CServerThread 指针
	CConnectSoc* GetSocketByThreadId(DWORD threadId);

protected:
	CModemProtocol m_Protocol;
	void BuildShockwaveData(SimInstrumentData& data, WORD wFrameNo,  WORD nFrameLen, std::vector <double> vec_ccl, std::vector <double> vec_volt);

	//CGridCtrl m_gridSend;
	//int m_nCols, m_nRows;
	//int m_nRecvRows;

	struct WaveData
	{
		int nWaveType;			// 模拟输出波形：正弦波、三角波……
		double nAmplitude;		// 信号输出幅度
		double nFreq;			// 信号输出频率
		CWaveCtrl* pWaveCtrl;	// 绘图对象
		int nSampleFreq;		// 窗口数据采样率，Fs，(Sampling Rate, Hz)，每秒采样点数
		int nViewWidth;			// 显示窗口的秒数 
		int nPoints;			// 波形数据点个数，两个波形数据个数不一致
		int nStep;				// 每次刷新时推走的数据点个数
		std::vector <double> data;	// 数据集
	};

#define NUM_WAVEFORM 2
	WaveData m_wave[NUM_WAVEFORM];

	double m_time;					// 当前时间（秒）
	int	m_nViewStartPoint;			// 左边算起，起始第一个点所在缓冲区的位置
	
	CIni m_ini;
	int m_nLocalPort, m_nRemotePort;	// IP地址
	int m_nIpSelect;					// =0，本地IP，=1，物理IP
	ULONG m_nSendCount, m_nRecvCount;	// 发送、接收字节数

	void ShowCmd(unsigned char *indata, LONG inlen, int nList = 0);

	// PC作为监听服务器
	CListensoc m_listensoc;	// socket member that listens for new connections
	BOOL m_running;			// listener has been created
	int m_open;				// a count of the number of open connections
	int m_close;			// a count of the number of close connections
	CDWordArray m_threadIDs;	// 每个连接到本地服务器的线程ID
	BOOL m_MainWndIsClosing; 
	//ULONG m_nUploadCount;	// 上传数据包个数，不是TCP帧数，可能有粘包
	UINT_PTR m_nSendDataTimer;	// 定时器ID

	BOOL CreateListener();
	void StopListener();
	BOOL CleanupThreads();
	void UpdateControls();
	void ShowOpenConnections();
	void ShowClosedConnections();

	void SendDataToClient(CByteArray & data, DWORD threadID);

	// PC作为客户机，连接Modem服务器
	CClientThread* m_pClientThread;
	BOOL m_bModemConnecting;
	BOOL m_bRemoteConnected;
	BOOL m_bSending;
	void SendCmd(byte* cmd, int len);
	void LogMessage2(CString msg);

	void ProcessRecvCmds(byte* cmd, int len);
	void LogMessage(CString msg);

	LRESULT OnInfo(WPARAM, LPARAM);
	LRESULT OnThreadStart(WPARAM, LPARAM);  
	LRESULT OnThreadClose(WPARAM, LPARAM);
	LRESULT OnNetworkData(WPARAM, LPARAM);		// received new message
	LRESULT OnNetworkError(WPARAM, LPARAM);		// a network error occurred
	LRESULT OnSendComplete(WPARAM, LPARAM);

	// 作为客户机连接Modem时的消息
	LRESULT OnConnectionClose(WPARAM, LPARAM);	// a connection has been closed
	LRESULT OnConnectionMade(WPARAM, LPARAM);	// modem连接已建立，等待中：pending connection has been established


	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


	// 解析收到的命令；构建发送的测井数据
	CModemProtocol m_ModemProtocol;

	// 其它
	std::map<UINT, CRect> m_mapCtrlRects; // 保存控件
	CRect m_dlgInitRect;       // 初始对话框大小
	void SaveInitRect();

	CMenu m_contextMenu, m_contextMenu2;
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnCopy();
	afx_msg void OnClear();
	afx_msg void OnCopy2();
	afx_msg void OnClear2();

public:

	// 辅助函数
	//void UpdateLog(CString strMsg);
	//void OnPcConnectCmd();      // 处理PC连接1104
	//void OnPcSendCommand();     // 处理PC发送的命令


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual void OnOK();
	virtual void OnCancel();
public:
	afx_msg void OnClose();
	CString m_strLocalIp;
	CString m_strLocalPort;
	CString m_strRemotePort;
	CString m_strRemoteIp;
	afx_msg void OnRun();
	CString m_strListenerState;
	CString m_strConnectState;
	CString m_strCmdLogging;
	CString m_strCmdCharge;
	CListBox m_listRecv;
	CListBox m_listSend;
	afx_msg void OnConnectRemote();
	afx_msg void OnDisconnectRemote();
	afx_msg void OnBnClickedButtonSendData();
	CButton m_btnRun;
	CStatic m_staticLocalIP;
	CStatic m_staticLocalPort;
	CStatic m_staticRemoteIP;
	CStatic m_staticRemotePort;
	CButton m_btnSendData;
	CButton m_btnConnectRemote;
	CButton m_btnDisconnectRemote;
	CStatic m_staticRecvCount;
	CStatic m_staticSendCount;
//	CEdit m_editCmdModem;
//	CEdit m_editCmdCan87;
//	CEdit m_editCmdCan180;
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	//afx_msg void OnGridSendStartEdit(NMHDR *pNotifyStruct, LRESULT* pResult);
	//afx_msg void OnGridSendEndEdit(NMHDR *pNotifyStruct, LRESULT* pResult);
	CComboBox m_cmbCCL;
	CComboBox m_cmbChargeVolt;
	CEdit m_editChargeVoltAmplitude;
	CEdit m_editCclAmplitude;
	CEdit m_editChargeVoltFreq;
	CEdit m_editCclFreq;
	afx_msg void OnCbnSelchangeComboCclValue();
	afx_msg void OnCbnSelchangeComboChargeVolt();
	BOOL m_bPauseDisplay;
	afx_msg void OnBnClickedCheckPauseDisplay();
	CComboBox m_cmbIPSelect;
	afx_msg void OnCbnSelchangeComboIpSelect();
};
