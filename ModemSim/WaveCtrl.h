#if !defined(AFX_SPECTRUMCTRL_H__7DE11F22_1C06_465C_B6E5_77013F95FC8EF__INCLUDED_)
#define AFX_SPECTRUMCTRL_H__7DE11F22_1C06_465C_B6E5_77013F95FC8EF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SpectrumCtrl.h : header file
//

// Use this as the classname when inserting this control as a custom control
// in the MSVC++ dialog editor
// sucn as DDX_Control(pDX, IDC_MYCURVE, m_myCurveCtrl)
#define WAVECTRL_CLASSNAME    _T("MFCWaveCtrl")  // Window class name

#pragma once

#include "rgbColor.h"
#include "AutoFont.h"
#include <afxtempl.h>
#include "MyMemDC.h"

#define DATA_PULSE_WIDTH 1		// 数据脉冲宽度=1s

/////////////////////////////////////////////////////////////////////////////
// CWaveCtrl window

class CWaveCtrl : public CWnd
{
	DECLARE_DYNCREATE(CWaveCtrl)
// Construction
public:
	CWaveCtrl();
    BOOL Create(const RECT& rect, CWnd* parent, UINT nID,
                DWORD dwStyle = WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VISIBLE);

// Attributes
public:
	CString m_strWvfName;
	void	SetWvfName(CString strSpecName){m_strWvfName = strSpecName;}
	CString GetWvfName(){ return m_strWvfName; }

// Operations
public:
	void	SetBuffer(double* buf) { pdata = buf; }
	//void	SetBuffers(USHORT* buf0, USHORT* buf1) { pdata[0] = buf0; pdata[1] = buf1;}

	//coordinates functions
    virtual void SetGraphWorldCoords(double x1, double x2, double y1, double y2, BOOL bRedraw = TRUE)
	{
		m_x1 = x1; m_x2 = x2; m_y1 = y1; m_y2 = y2;
	}
    virtual void SetGraphWorldXCoords(double x1, double x2, BOOL bRedraw = TRUE)
	{
		m_x1 = x1; m_x2 = x2; 
	}
	
    virtual void GetGraphWorldCoords(double* x1, double* x2, double* y1, double* y2)
	{
		*x1 = m_x1; *x2 = m_x2; *y1 = m_y1; *y2 = m_y2;
	}

	void SetViewWidth(int t){ m_nViewWidth = t;}
	void SetSampleFreq(int freq){m_nSampleFreq = freq;}

	void	SetColors(COLORREF cur, COLORREF spec, COLORREF bkg)
	{ m_crCursor = cur; m_crSpec = spec; m_crBkg = bkg; }
	void	GetColors(COLORREF *cur, COLORREF *spec, COLORREF *bkg)
	{ *cur = m_crCursor; *spec = m_crSpec; *bkg = m_crBkg; }

	// 曲线颜色反转
	BOOL	m_bSpecReverse;
	int		m_nPointReverse;
	COLORREF m_crSpecReserve;
	void	SetColorReverse(){return;m_bSpecReverse = TRUE; m_crSpec = WHITE - m_crSpec;}
	void	SetColorNormal(){return;m_crSpec = WHITE - m_crSpec;}

	void SetShowEvents(BOOL bPointMarker, BOOL bSlotGrid, BOOL bSlotValue,
		BOOL bTextMessage, BOOL bTimeBar, BOOL bTimeWindow)
	{
		m_bShowPointMarker = bPointMarker;
		m_bShowSlotGrid = bSlotGrid;
		m_bShowSlotValue = bSlotValue;
		m_bShowTextMessage = bTextMessage;
		m_bShowTimeBar = bTimeBar; 
		m_bShowTimeWindow = bTimeWindow;
	}
	void	SetViewPosition(DWORD statrpoint){m_nStartPoint = statrpoint;}
	void	SetStep(int step){m_nStep = step;}

	void	SetLineType(int type){ m_nLineType = type; }
	void	SetAutoRange(BOOL a){ m_bShowAutoRange = a;}
	void	SetCursor(int cursor){ m_nCursor = cursor; }
	void	SetUnitType(int type){ m_nUnitType = type; }

	int		Second2Points(float sec){ return int(sec * m_nSampleFreq); }
	double	Slots2Seconds(int slots){ return int(slots * DATA_PULSE_WIDTH * 2.0F / 3.0F); }
	int		Slots2Points(int slots){ return int(m_nSampleFreq * slots * DATA_PULSE_WIDTH * 2.0F / 3.0F); }
	int		Slots2Points(double slots){ return int(m_nSampleFreq * slots * DATA_PULSE_WIDTH * 2.0F / 3.0F); }

	void	SetStartTime(CTime t){m_tmStartTime = t;}

	void	ChangeSize(CRect rect);

	void	SetDispHookload(BOOL hookload){m_bDispHookload = hookload;}
	void	SetSppUnit(CString unit){m_strSppUnit = unit;}
	void	SetHookloadUnit(CString unit){m_strHookloadUnit = unit;}
	void	SetSppCalCoefficient(double k, double b){m_fSppK = k; m_fSppB = b;}
	void	SetHookloadCalCoefficient(double k, double b){m_fHookloadK = k; m_fHookloadB = b;}

public:
	//CArray <stDecodeField, stDecodeField> arDecode;
	CUIntArray m_arLostIndex;		// 丢数的数据点序号
	void	ShowLostIndex(CDC* pDC);

private:
	BOOL	RegisterWindowClass();
	
	double	XRatio, YRatio;
	float	m_nVertline;
	int		m_nUnitType;			// 0-电压值，1-工程值
	int		m_nLineType;			// 显示直线、点
	int		m_nViewWidth;			// 显示窗口的宽度，秒
	int		m_nPointNum;			// 当前显示窗口内的数据点个数
	int		m_nSampleFreq;			// 采样频率
	int		m_nStep;				// 每次刷新时，移走的数据点个数
	double	m_x1, m_x2, m_y1, m_y2;			// 横轴、纵轴量程

	int		m_nStartPoint;			// 左边算起，起始第一个点所在缓冲区的位置

	// 刻度系数
	double m_fHookloadK, m_fHookloadB;
	double m_fSppK, m_fSppB;
	
	BOOL m_bDispHookload;			// 0-Spp, 1-Hookload
	CString m_strSppUnit, m_strHookloadUnit;

	// 创建字体
	CAutoFont myfont;

	CRect	m_rectSpec;				// 中间曲线显示
	CRect	m_rectTop;				// 上部信息显示，显示文字，标尺
	CRect	m_rectBottom;			// 下部信息显示，显示解码后的数字位
	CRect	m_rectSelected;
	void	SeparateWindow(CRect rect);		// 把控件窗口分割成三部分
	void	display_spectrum( CDC* pDC );
	
	bool	m_bLButtonDown;
	CPoint	m_ptMouseMove;
	CTime	m_tmStartTime;

	int		m_nCursor;				// 行进中的光标，指示当前处理到的数据点

	// spectra display control
	bool	m_bAutoVFSFlag;			// 自动VFS 标志，A键修改
	float	vfs[2];					// 显示谱的幅度值
	COLORREF m_crCursor, m_crSpec, m_crBkg;

	double* pdata;					// 当前显示的波形曲线指针，可以是原始、滤波等等


	// 托拽光标所用的变量

	// 显示的内容（Events）
	BOOL m_bShowPointMarker;
	BOOL m_bShowAutoRange;
	BOOL m_bShowSlotGrid;
	BOOL m_bShowSlotValue;
	BOOL m_bShowTextMessage;
	BOOL m_bShowTimeBar;
	BOOL m_bShowTimeWindow;
	
	void CalcAutoRange();
	void ShowPointMarker(CDC* pDC);
	void ShowSlotValue(CDC* pDC);
	void ShowTextMessage(CDC* pDC);
	void ShowTimeBar(CDC* pDC);
	void ShowTimeWindow(CDC* pDC);
	void ShowDataInfo(CDC* pDC);		// 鼠标左键按下，显示测量脉冲高度窗口

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaveCtrl)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWaveCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CWaveCtrl)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
//	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
//	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPECTRUMCTRL_H__7DE11F22_1C06_465C_B6E5_77013F95FC8E__INCLUDED_)
