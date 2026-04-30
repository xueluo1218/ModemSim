// WaveCtrl.cpp : implementation file
//
// 波形显示类，用于泵压、钩载数据的显示 
//
 
#include "stdafx.h"
#include "WaveCtrl.h"
#include "mymemdc.h"
#include <math.h>
#include "rgbColor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define	draw_line(x0,y0,x1,y1, pDC){ pDC->MoveTo(x0,y0); pDC->LineTo(x1,y1); }

/////////////////////////////////////////////////////////////////////////////
// CWaveCtrl
IMPLEMENT_DYNCREATE(CWaveCtrl, CWnd)

CWaveCtrl::CWaveCtrl():
	m_bAutoVFSFlag(true), m_bDispHookload(FALSE)
{
    RegisterWindowClass();

    _AFX_THREAD_STATE* pState = AfxGetThreadState();
    if (!pState->m_bNeedTerm && !AfxOleInit())
	{
		AfxMessageBox(_T("OLE initialization failed. Make sure that the OLE libraries are the correct version"));
	}

	pdata = NULL;

	m_bLButtonDown = false;
	m_nStartPoint = 0;

	m_crCursor = RGB(255, 255, 255);
	m_crSpec   = RGB(255, 255, 255);
	m_crBkg    = RGB(0, 0, 0);

	//m_ptSelectedLeft = m_ptSelectedRight = CPoint(-999, -999);

	// 字体
//	myfont.SetFaceName("MS Sans Serif");
//	myfont.SetHeight(-11);
	myfont.SetWidth(5);

	m_fHookloadK = 1.0F;
	m_fHookloadB = 0.0F;
	m_fSppK = 1.0F;
	m_fSppB = 0.0F;

	m_bShowPointMarker = FALSE;
	m_bShowSlotGrid = FALSE;
	m_bShowSlotValue = FALSE;
	m_bShowTextMessage = FALSE;
	m_bShowTimeWindow = TRUE;
	m_bShowTimeBar = TRUE;
}

CWaveCtrl::~CWaveCtrl()
{
}


BEGIN_MESSAGE_MAP(CWaveCtrl, CWnd)
	//{{AFX_MSG_MAP(CWaveCtrl)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
//	ON_WM_RBUTTONDOWN()
//	ON_WM_RBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CWaveCtrl message handlers
/////////////////////////////////////////////////////////////////////////////
// CWaveCtrl message handlers
// description :create this window, use like any other window create control
// in parameter: rect       -- window rect
//               pParentWnd -- pointer of parent window 
//               nID        -- resource ID
//               dwStyle    -- style
BOOL CWaveCtrl::Create(const RECT& rect, CWnd* pParentWnd, UINT nID, DWORD dwStyle)
{
    ASSERT(pParentWnd->GetSafeHwnd());

    if (!CWnd::Create(WAVECTRL_CLASSNAME, NULL, dwStyle, rect, pParentWnd, nID))
        return FALSE;

    return TRUE;
}

BOOL CWaveCtrl::RegisterWindowClass()
{
    WNDCLASS wndcls;
    HINSTANCE hInst = AfxGetInstanceHandle();

    if (!(::GetClassInfo(hInst, WAVECTRL_CLASSNAME, &wndcls)))
    {
        // otherwise we need to register a new class
        wndcls.style            = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
        wndcls.lpfnWndProc      = ::DefWindowProc;
        wndcls.cbClsExtra       = wndcls.cbWndExtra = 0;
        wndcls.hInstance        = hInst;
        wndcls.hIcon            = NULL;
        wndcls.hCursor          = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
        wndcls.hbrBackground    = (HBRUSH) (COLOR_3DFACE + 1);
        wndcls.lpszMenuName     = NULL;
        wndcls.lpszClassName    = WAVECTRL_CLASSNAME;

        if (!AfxRegisterClass(&wndcls))
        {
            AfxThrowResourceException();
            return FALSE;
        }
    }

    return TRUE;
}

void CWaveCtrl::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	CMyMemDC memdc(&dc); // use CMemDC to avoid flicker

	CRect rect;
	GetClientRect(&rect);

	// 将显示空间分成三部分
	SeparateWindow(rect);

	// background and margin    
	CBrush bkBrush(m_crBkg);
	CBrush* pOldBrush = memdc.SelectObject(&bkBrush);	
//	memdc.FillRect(rect, &bkBrush);
//	memdc->FillSolidRect(rect, m_crBkg);
	memdc.FillSolidRect(rect, m_crBkg); 
	memdc.SelectObject(pOldBrush);

	if(pdata == NULL)
		return;
	
	m_nPointNum = int(m_nViewWidth * m_nSampleFreq);
	XRatio = m_rectSpec.Width() / (m_x2 - m_x1);
	YRatio = m_rectSpec.Height() / (m_y2 - m_y1);

	// 自动量程时，计算y轴量程
	if(m_bShowAutoRange)
	{
		CalcAutoRange();	
		m_x1=0; m_x2 = m_nViewWidth * m_nSampleFreq;
	}
	
	// 字体
	myfont.SetDC(memdc->m_hDC);
	CFont *oldFont = memdc->SelectObject(&myfont);

	if(m_bShowPointMarker)ShowPointMarker(&memdc);
	if(m_bShowSlotValue)ShowSlotValue(&memdc);
	if(m_bShowTextMessage)ShowTextMessage(&memdc);
	if(m_bShowTimeWindow)ShowTimeWindow(&memdc);
	if(m_bShowTimeBar)ShowTimeBar(&memdc);
	 
	display_spectrum(&memdc);
	
	if(m_bLButtonDown)ShowDataInfo(&memdc);

	ShowLostIndex(&memdc);							// 丢失的数据点
	
	memdc->SelectObject(oldFont);
}

void CWaveCtrl::ChangeSize(CRect rect)
{
	m_rectSpec = rect; 
	MoveWindow(m_rectSpec, true);
	
	Invalidate();
}

void CWaveCtrl::ShowLostIndex(CDC *pDC)
{
	// 数据索引落在[m_nStartPoint, m_nStartPoint+nPointNum]区间时，显示特殊标记。其它范围不显示
	CPen penLostIndex;
	penLostIndex.CreatePen(PS_SOLID, 5, LIGHTGREEN);
	pDC->SelectObject(&penLostIndex);
	for(int i=0; i<m_arLostIndex.GetSize(); i++)
	{
		if( ((int)m_arLostIndex[i] >= m_nStartPoint) && ((int)m_arLostIndex[i] <= (m_nStartPoint + m_nPointNum)) )
		{
			int pos = m_arLostIndex[i] - m_nStartPoint;
			int x = m_rectSpec.left + (int)( pos * XRatio);
			
			draw_line(x, m_rectTop.top, x, m_rectTop.bottom, pDC);
			draw_line(x, m_rectBottom.top, x, m_rectBottom.bottom, pDC);
		}
	}
	penLostIndex.DeleteObject();
}

// 求取最大值，最小值作为纵轴量程
void CWaveCtrl::CalcAutoRange()
{
	double y_min = 10000, y_max = 0;//-100000;
	for(int i=0; i<m_nPointNum; i++)
	{
		double y = *(pdata+i/*+m_nStartPoint*/);
		if(y<y_min)
			y_min=y;
		if(y>y_max)
			y_max=y;
	}
	m_y1 = y_min;
	m_y2 = y_max;
}

BOOL CWaveCtrl::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;	
}

// 在m_rectSpec矩形内显示波形
void CWaveCtrl::display_spectrum( CDC* pDC )
{   
	if(pdata == NULL)
		return;

//	COLORREF m_clrZeroline = RGB(255, 0, 0);
	//COLORREF m_clrVertline = WHITE;
	//COLORREF m_clrSignal   = WHITE;
	//COLORREF m_clrCusor	   = WHITE;

	int x, y, k;

	// 信号线
	CPen penSignal, penInvertSignal;
	penSignal.CreatePen(PS_SOLID, 2, m_crSpec);
	penInvertSignal.CreatePen(PS_SOLID, 2, WHITE - m_crSpec);

	// 第一个点，自左侧开始
	x = m_rectSpec.left + int(0 * XRatio);
	y = m_rectSpec.bottom - int( (*(pdata + 0 /*+ m_nStartPoint*/) - m_y1) * YRatio);
	if(y < m_rectSpec.top) y = m_rectSpec.top;
	if(y > m_rectSpec.bottom) y = m_rectSpec.bottom;
	pDC->MoveTo(x, y);
	pDC->SetPixel(x, y, m_crSpec);	

	for(k = 1; k < m_nPointNum; k++)
	{
		x = m_rectSpec.left + (int)(k * XRatio);
		y = m_rectSpec.bottom - int( ( *(pdata + k/* + m_nStartPoint*/) - m_y1) * YRatio);
		if(y < m_rectSpec.top) y = m_rectSpec.top;
		if(y > m_rectSpec.bottom) y = m_rectSpec.bottom;
		
		pDC->SelectObject(penSignal);

		switch(m_nLineType)
		{
		case 0:
			pDC->LineTo(x, y);
			break;
		case 1:
			pDC->SetPixel(x, y, m_crSpec);
			break;
		}
	}
	pDC->SelectStockObject(BLACK_PEN);
		
	// 写纵轴Y的量程，文字
	CString strY1, strY2;
	CRect rc_tmp;
	{
		myfont.SetFontColor(RGB(200,200,200));

		// 低量程
		if(!m_bDispHookload)						// 显示Spp
		{
			strY1.Format(_T("%.0f"), m_y1 * m_fSppK + m_fSppB);
		}
		else										// 显示Hookload
		{
			strY1.Format(_T("%.0f"), m_y1 * m_fHookloadK + m_fHookloadB);
		}
		CRect rc_tmp(m_rectSpec.left+3, m_rectSpec.bottom+myfont.GetHeight()-5, m_rectSpec.left+50, m_rectSpec.bottom-5);
		pDC->SetBkMode(TRANSPARENT);
		pDC->DrawText(strY1, &rc_tmp,  DT_SINGLELINE | DT_NOCLIP | DT_LEFT);
		
		// 高量程
		if(!m_bDispHookload)						// 显示Spp
		{
			if(m_strSppUnit.IsEmpty())
				strY2.Format(_T("%.0f"), m_y2 * m_fSppK + m_fSppB);
			else
				strY2.Format(_T("%.0f(%s)"), m_y2 * m_fSppK + m_fSppB, m_strSppUnit);
		}
		else										// 显示Hookload
		{
			if(m_strHookloadUnit.IsEmpty())
				strY2.Format(_T("%.0f"), m_y2 * m_fHookloadK + m_fHookloadB);
			else
				strY2.Format(_T("%.0f(%s)"), m_y2 * m_fHookloadK + m_fHookloadB, m_strHookloadUnit);
		}
		rc_tmp = CRect(m_rectSpec.left+3, m_rectSpec.top, m_rectSpec.left+50, m_rectSpec.top-myfont.GetHeight());
		pDC->DrawText(strY2, &rc_tmp,  DT_SINGLELINE | DT_NOCLIP | DT_LEFT);
	}
}


#define INFO_WINDOWS_HEIGHT		15

void CWaveCtrl::SeparateWindow(CRect rect)
{
	m_rectTop = m_rectBottom = m_rectSpec = rect;
	m_rectTop.bottom = rect.top + INFO_WINDOWS_HEIGHT;
	
	m_rectBottom.top = rect.bottom - INFO_WINDOWS_HEIGHT-1;
	
	m_rectSpec.top = 5;		//INFO_WINDOWS_HEIGHT;
	m_rectSpec.bottom = m_rectBottom.top;
}

BOOL CWaveCtrl::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	return CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
}

// 显示不同事件
void CWaveCtrl::ShowPointMarker(CDC* pDC)
{
}

void CWaveCtrl::ShowSlotValue(CDC* pDC)
{

}

// 显示开泵、关泵的光标，文字信息
void CWaveCtrl::ShowTextMessage(CDC* pDC)
{
}
	//if(pump->bFindPumpOn || pump->bFindPumpOff)
	//{
	//	CPen penVertline;
	//	penVertline.CreatePen(PS_SOLID, 1, LIGHTGREEN);
	//	CPen* oldpenVertline = (CPen*)pDC->SelectObject(penVertline);
	//	
	//	CString str;
	//	int x;
	//	myfont.SetFontColor(LIGHTGREEN);
	//	pDC->SetBkMode(TRANSPARENT);
	//	if(pump->bFindPumpOn)		// 开泵
	//	{
	//		// 光标落在[m_nStartPoint, m_nStartPoint+nPointNum]区间时，显示光标。其它范围不显示
	//		if( (pump->nPumpOnTime >= m_nStartPoint) && (pump->nPumpOnTime <= m_nStartPoint + m_nPointNum) )
	//		{
	//			x = m_rectSpec.left + (int)( (pump->nPumpOnTime - m_nStartPoint) * XRatio);
	//			pDC->MoveTo(x, m_rectSpec.top);
	//			pDC->LineTo(x, m_rectSpec.bottom);
	//			
	//			// text message
	//			str.Format(_T("%s"), pump->ucOnMsg);
	//			CRect rc_tmp(m_rectBottom.left + x, m_rectBottom.top, m_rectBottom.left+ x + 100, m_rectBottom.bottom);
	//			pDC->DrawText(str, &rc_tmp,  DT_SINGLELINE | DT_NOCLIP | DT_LEFT);
	//		}
	//	}
	//	
	//	if(pump->bFindPumpOff)		// 关泵
	//	{
	//		// 光标落在[m_nStartPoint, m_nStartPoint+nPointNum]区间时，显示光标。其它范围不显示
	//		if( (pump->nPumpOffTime >= m_nStartPoint) && (pump->nPumpOffTime <= m_nStartPoint + m_nPointNum) )
	//		{
	//			x = m_rectSpec.left + (int)( (pump->nPumpOffTime - m_nStartPoint) * XRatio);
	//			pDC->MoveTo(x, m_rectSpec.top);
	//			pDC->LineTo(x, m_rectSpec.bottom);
	//			
	//			// text message
	//			str.Format(_T("%s"), pump->ucOffMsg);
	//			CRect rc_tmp(m_rectBottom.left + x, m_rectBottom.top, m_rectBottom.left+ x + 100, m_rectBottom.bottom);
	//			pDC->DrawText(str, &rc_tmp,  DT_SINGLELINE | DT_NOCLIP | DT_LEFT);
	//		}	
	//	}
	//	
	//	pDC->SelectObject(&oldpenVertline);
	//	penVertline.DeleteObject();
	//}	
//}

// 显示时间条
void CWaveCtrl::ShowTimeBar(CDC* pDC)
{
	// 时间条位置
	CRect rcTimeBar(m_rectTop);
	if(m_bShowTimeWindow)			// 把显示位置改到图形下方
		rcTimeBar = m_rectBottom;
	
	// 字体
	COLORREF clrTimerBar(RGB(200, 200, 200));
	myfont.SetFontColor(clrTimerBar);
	
	// 时间条
	CPen penTimeBar, penInvertTimeBar;
	penTimeBar.CreatePen(PS_SOLID, 1, clrTimerBar);
	penInvertTimeBar.CreatePen(PS_SOLID, 1, WHITE-clrTimerBar);

	// 垂直刻度线
	CString str;
	for(int kk=m_nStartPoint; kk<m_nStartPoint + m_nPointNum; kk++)
	{
		int x = rcTimeBar.left + (int)((kk-m_nStartPoint) * XRatio);

		{
			myfont.SetFontColor(clrTimerBar);
			pDC->SelectObject(&penTimeBar);
			draw_line(rcTimeBar.left, rcTimeBar.top, rcTimeBar.right, rcTimeBar.top, pDC);
		}

		if( fmod(kk*1.0F, 0.1F*m_nSampleFreq) == 0)		// 每隔0.1s画短刻度线
			draw_line(x, rcTimeBar.top - 3, x, rcTimeBar.top, pDC);
		
		if( fmod(kk*1.0F, 1.0F*m_nSampleFreq) == 0)		// 每隔1s画刻度线
		{
			draw_line(x, rcTimeBar.top - 6, x, rcTimeBar.top, pDC);

			// 写刻度字串
			str.Format(_T("%.0f"), float(kk)/m_nSampleFreq);
			pDC->SetBkMode(TRANSPARENT);
			CRect rc_tmp(x-30, rcTimeBar.top /*+ 6 + myfont.GetHeight()*/, x+30, rcTimeBar.bottom);
			pDC->DrawText(str, &rc_tmp,  DT_SINGLELINE | DT_NOCLIP | DT_CENTER);
		}
	}

	pDC->SelectStockObject(BLACK_PEN);
}

// 显示时间窗口
void CWaveCtrl::ShowTimeWindow(CDC* pDC)
{
}
	// 计算当前显示区间[m_nStartPoint, m_nStartPoint+nPointNum]需要对应那些数据字，只显示这些数据字就够了。
	//int nIndexMin(0), nIndexMax(0);		// 对应显示数组的左右下标
	//int nCount = arDecode.GetSize();
	//
	//for(int i=0; i<nCount; i++)
	//{
	//	if(arDecode[i].dStartPoint > m_nStartPoint)
	//	{
	//		nIndexMin = i - 1;
	//		if(nIndexMin < 0) 
	//			nIndexMin = 0;
	//		break;
	//	}
	//}
	//
	//for(i=nIndexMin; i<nCount; i++)
	//{
	//	if(arDecode[i].dStartPoint > m_nStartPoint + m_nPointNum)
	//	{
	//		nIndexMax = i;
	//		break;
	//	}
	//}
	//if(i == nCount)
	//	nIndexMax = nCount;

	//// 显示该区间内的背景、数据字、格线
	//for(i=nIndexMin; i<nIndexMax; i++)
	//{
	//	// 背景
	//	CBrush bkBrush, bkInvertBrush;
	//	if(arDecode[i].bQuality)
	//		bkBrush.CreateSolidBrush( arDecode[i].crBack );
	//	else
	//		bkBrush.CreateSolidBrush( RED );
	//	CBrush* pOldBrush = pDC->SelectObject(&bkBrush);

	//	int nLeft = int( XRatio * (arDecode[i].dStartPoint - m_nStartPoint) );
	//	int nRight = int( XRatio * (arDecode[i].dStartPoint + Slots2Points(arDecode[i].M) - m_nStartPoint) );
	//	CRect rc_top(nLeft, m_rectTop.top, nRight, m_rectTop.bottom);		// 分成两个矩形，分别填充，目的在于被选择时容易处理
	//	CRect rc_spec(nLeft, m_rectSpec.top, nRight, m_rectSpec.bottom);
	//	pDC->FillRect(rc_top, &bkBrush);
	//	if(!m_bEnableRButton)
	//		pDC->FillRect(rc_spec, &bkBrush);
	//	pDC->SelectObject(pOldBrush);

	//	// 两侧边界，水平线
	//	CPen penLine;
	//	if(arDecode[i].bQuality)
	//		penLine.CreatePen(PS_SOLID, 1, arDecode[i].crLine);
	//	else
	//		penLine.CreatePen(PS_SOLID, 1, LIGHTRED);
	//	pDC->SelectObject(&penLine);
	//	
	//	draw_line(nLeft, m_rectTop.top, nLeft, m_rectSpec.bottom, pDC);
	//	draw_line(nRight, m_rectTop.top, nRight, m_rectSpec.bottom, pDC);
	//	
	//	if( m_bShowSlotGrid )			// 显示槽格线
	//	{
	//		double pos(0.0F), pos1(0.0F);
	//		double sync_slot_width[] = {0, 3, 1.5, 3, 1.5, 1.5, 2, 1.5, 3};
	//		
	//		// 每个槽的网格线
	//		for(int j=0; j<arDecode[i].M; j++)
	//		{
	//			if(arDecode[i].DataType == 3)	// sync window
	//				pos += sync_slot_width[j];
	//			else
	//				pos = j;
	//			int left = int( XRatio * (arDecode[i].dStartPoint + Slots2Points(pos) - m_nStartPoint) );
	//			draw_line(left, m_rectSpec.top, left, m_rectSpec.bottom, pDC);
	//		}
	//		
	//		// 0/1标记
	//		pos = 0.0F;
	//		int M = arDecode[i].M;
	//		if(arDecode[i].DataType == 3)	// sync window
	//			M = 8;
	//		for(j=0; j<M; j++)
	//		{	
	//			int left, right;
	//			if(arDecode[i].DataType == 3)	// sync window
	//			{
	//				pos += sync_slot_width[j];
	//				pos1 = pos + sync_slot_width[j+1];
	//			}
	//			else
	//			{
	//				pos = j;
	//				pos1 = j+1;
	//			}
	//			left = int( XRatio * (arDecode[i].dStartPoint + Slots2Points(pos) - m_nStartPoint) );
	//			right = int( XRatio * (arDecode[i].dStartPoint + Slots2Points(pos1) - m_nStartPoint) );

	//			CRect rect_Ellipse(left, m_rectBottom.top, right, m_rectBottom.bottom);

	//			UINT32 word;
	//			if(arDecode[i].DataType == 3)	// sync window
	//				word = (1<<(8-j-1));
	//			else
	//				word = (1<<(arDecode[i].M-j-1));
	//			if((arDecode[i].ppm & word) == word)
	//			{
	//				myfont.SetFontColor(WHITE);
	//				pDC->DrawText("1", &rect_Ellipse,  DT_SINGLELINE | DT_NOCLIP | DT_CENTER);
	//			}
	//			else
	//			{
	//				myfont.SetFontColor(GREEN);
	//				pDC->DrawText("0", &rect_Ellipse,  DT_SINGLELINE | DT_NOCLIP | DT_CENTER);
	//			}
	//		}
	//	}
	//	
	//	pDC->SelectStockObject(BLACK_PEN);
	//	
	//	// 写字串
	//	CString str;
	//	switch(arDecode[i].DataType)
	//	{
	//	case 3: // sync
	//		str.Format(_T("%s", arDecode[i].Message);
	//		break;
	//	case 4:	// fid
	//		str.Format(_T("%s %.0f", arDecode[i].Message, arDecode[i].Value);
	//		break;
	//	case 5:	// log data
	//		str.Format(_T("%s %.3f", arDecode[i].Message, arDecode[i].Value);
	//		break;
	//	}
	//	if(arDecode[i].bQuality)
	//		myfont.SetFontColor(arDecode[i].crFont);
	//	else
	//		myfont.SetFontColor(LIGHTRED);
	//	pDC->SetBkMode(TRANSPARENT);
	//	pDC->DrawText(str, &rc_top,  DT_SINGLELINE | DT_NOCLIP | DT_CENTER);
	//}
//}

// 显示时间窗口
void CWaveCtrl::ShowDataInfo(CDC* pDC)
{
#define TEXT_HEIGHT 15
	CRect rcInfo(m_rectSpec.right - 120, m_rectSpec.bottom - TEXT_HEIGHT*3, m_rectSpec.right, m_rectSpec.bottom);

	if(m_rectSpec.Height() < TEXT_HEIGHT*3)		// 窗口太小
	{
		rcInfo.top = m_rectSpec.top;
		rcInfo.bottom = m_rectSpec.top + TEXT_HEIGHT*3;
	}

	// 实属无奈，应该有更好的写法——比如把CMemDC定义个全局变量，在MouseMove中通过memdc实现这个函数
	if(m_ptMouseMove.x < 0)m_ptMouseMove.x = 0;
	if(m_ptMouseMove.x > m_rectSpec.right)m_ptMouseMove.x = m_rectSpec.right;
	if(m_ptMouseMove.y < m_rectSpec.top)m_ptMouseMove.y = m_rectSpec.top;
	if(m_ptMouseMove.y > m_rectSpec.bottom)m_ptMouseMove.y = m_rectSpec.bottom;
	
	if(rcInfo.PtInRect(m_ptMouseMove))			// 鼠标指针被遮挡
	{
		rcInfo.top = m_rectSpec.top;
		rcInfo.bottom = m_rectSpec.top + TEXT_HEIGHT*3;
	}
	pDC->FillSolidRect(rcInfo, RGB(80,80,80));

	
	// 显示游丝
	CPen penCross;
	penCross.CreatePen(PS_SOLID, 1, RGB(80,80,80));
	pDC->SelectObject(&penCross);
	draw_line(0, m_ptMouseMove.y, m_rectSpec.right, m_ptMouseMove.y, pDC);
	draw_line(m_ptMouseMove.x, m_rectSpec.top, m_ptMouseMove.x, m_rectSpec.bottom, pDC);
	pDC->SelectStockObject(BLACK_PEN);

//	XRatio = m_rectSpec.Width() / (m_x2 - m_x1);
//	YRatio = m_rectSpec.Height() / (m_y2 - m_y1);
//	TRACE("m_x2=%.5f, m_x1=%.5f\n", m_x2, m_x1);
//	TRACE("XRatio=%.5f, YRatio=%.5f\n", XRatio, YRatio);
//	ASSERT( fabs(XRatio) < 0.0001);
//	ASSERT( fabs(YRatio) < 0.0001);
	int nCurrentPoint_X = int(m_ptMouseMove.x * (m_x2 - m_x1) / m_rectSpec.Width() + 0.5);
	float nCurrentPoint_Y = float(m_y1 + (m_rectSpec.bottom - m_ptMouseMove.y) * (m_y2 - m_y1) / m_rectSpec.Height());

	CString str1, str2, str3, str4;
	if(!m_bDispHookload)						// Spp
	{
		str1.Format(_T("光标: %.0f%s"), nCurrentPoint_Y, m_strSppUnit);
		str2.Format(_T("数值: %.1f%s"), *(pdata + nCurrentPoint_X), m_strSppUnit);
		//str3.Format(_T("Delta: %.2fV"), *(pdata + nCurrentPoint_X) - nCurrentPoint_Y);
	}
	else										// Hookload
	{
		str1.Format(_T("Mouse: %.2fV(%.1f%s)"), nCurrentPoint_Y, nCurrentPoint_Y * m_fHookloadK + m_fHookloadB, m_strHookloadUnit);
		str2.Format(_T("Data:  %.2fV(%.1f%s)"), *(pdata + nCurrentPoint_X), *(pdata + nCurrentPoint_X) * m_fHookloadK + m_fHookloadB, m_strHookloadUnit);
		//str3.Format(_T("Delta: %.2fV"), *(pdata + nCurrentPoint_X) - nCurrentPoint_Y);
	}

	// 光标位置走过的秒数
	CTimeSpan ts = CTimeSpan( 0, 0, 0, (nCurrentPoint_X + m_nStartPoint) / m_nSampleFreq);
	CTime t = m_tmStartTime + ts;
	str4 = _T("时间: ") + t.Format( "%H:%M:%S" );
	//TRACE("m_tmStartTime=%ld, ts=%d\n", m_tmStartTime, ts);

	myfont.SetFontColor(RGB(230,230,230));
	pDC->SetBkMode(TRANSPARENT);

	CRect rcText(rcInfo);
	rcText.left += 5;
	rcText.bottom = rcInfo.top + TEXT_HEIGHT;
	pDC->DrawText(str1, &rcText,  DT_SINGLELINE | DT_NOCLIP | DT_LEFT);
	rcText.top = rcInfo.top + TEXT_HEIGHT;
	rcText.bottom = rcInfo.top + TEXT_HEIGHT*2;
	pDC->DrawText(str2, &rcText,  DT_SINGLELINE | DT_NOCLIP | DT_LEFT);
	//rcText.top = rcInfo.top + TEXT_HEIGHT*2;
	//rcText.bottom = rcInfo.top + TEXT_HEIGHT*3;
	//pDC->DrawText(str3, &rcText,  DT_SINGLELINE | DT_NOCLIP | DT_LEFT);
	rcText.top = rcInfo.top + TEXT_HEIGHT*2;
	rcText.bottom = rcInfo.top + TEXT_HEIGHT*3;
	pDC->DrawText(str4, &rcText,  DT_SINGLELINE | DT_NOCLIP | DT_LEFT);
}


void CWaveCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	if(pdata == NULL)
		return;
	
	m_bLButtonDown = true;

	SetCapture();				// 捕捉鼠标
	ClientToScreen(m_rectSpec);
	m_ptMouseMove = point;		// 点击则显示信息
	Invalidate();
	
	CWnd::OnLButtonDown(nFlags, point);
}

void CWaveCtrl::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if(pdata == NULL)
		return;
	
	m_bLButtonDown = false;

	ReleaseCapture();			// 释放鼠标
	
	CWnd::OnLButtonUp(nFlags, point);
}

void CWaveCtrl::OnMouseMove(UINT nFlags, CPoint point) 
{
	if(pdata == NULL)
		return;
	
	if(m_bLButtonDown)		// 鼠标左键按下，显示光标处的工程数据
	{
		m_ptMouseMove = point;
	}

	//Invalidate();
	
	CWnd::OnMouseMove(nFlags, point);
}

