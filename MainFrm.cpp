
// MainFrm.cpp : реализация класса CMainFrame
//

#include "stdafx.h"
#include "RectifierControl.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	ON_WM_CREATE()
	//ON_MESSAGE(UPDATE_RECTIFIERS, &CMainFrame::OnUpdateRectifiers)
	//ON_REGISTERED_MESSAGE(UPDATE_RECTIFIERS1, &CMainFrame::OnUpdateRectifiers1)
	ON_WM_SHOWWINDOW()
	ON_WM_SIZE()
	ON_WM_EXITSIZEMOVE()
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // индикатор строки состояния
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

// создание/уничтожение CMainFrame

CMainFrame::CMainFrame()
{
	// TODO: добавьте код инициализации члена
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Не удалось создать панель инструментов\n");
		return -1;      // не удалось создать
	}

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Не удалось создать строку состояния\n");
		return -1;      // не удалось создать
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));

	// TODO: Удалите эти три строки, если не собираетесь закреплять панель инструментов
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);


	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CMDIFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: изменить класс Window или стили посредством изменения
	//  CREATESTRUCT cs
	return TRUE;
}

// диагностика CMainFrame

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}
#endif //_DEBUG


// обработчики сообщений CMainFrame



afx_msg LRESULT CMainFrame::OnUpdateRectifiers(WPARAM wParam, LPARAM lParam)
{
	return 0;
}


afx_msg LRESULT CMainFrame::OnUpdateRectifiers1(WPARAM wParam, LPARAM lParam)
{
	return 0;
}


BOOL CMainFrame::OnCommand(WPARAM wParam, LPARAM lParam)
{
	// TODO: Add your specialized code here and/or call the base class
	if (wParam == 7 && lParam == 7) {
		CDocTemplate *pTempl;
		CDocument *pDoc;
		CView *pView;

		POSITION posT = theApp.GetFirstDocTemplatePosition();

		while (posT)
		{
			pTempl = theApp.GetNextDocTemplate(posT);

			POSITION posD = pTempl->GetFirstDocPosition();
			while (posD)
			{

				pDoc = pTempl->GetNextDoc(posD);
				pDoc->UpdateAllViews(NULL);
				POSITION posV = pDoc->GetFirstViewPosition();
				while (posV)
				{
					pView = pDoc->GetNextView(posV);
					pView->Invalidate();
				}
			}
		}
		return TRUE;
	}
	return CMDIFrameWnd::OnCommand(wParam, lParam);
}


BOOL CMainFrame::DestroyWindow()
{
	WINDOWPLACEMENT wp;
	GetWindowPlacement(&wp);
	AfxGetApp()->WriteProfileBinary(L"MainFrame", L"WP", (LPBYTE)&wp, sizeof(wp));

	return CMDIFrameWnd::DestroyWindow();
}


void CMainFrame::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CMDIFrameWnd::OnShowWindow(bShow, nStatus);

	if (bShow && !IsWindowVisible())
	{
		WINDOWPLACEMENT *lwp;
		UINT nl;

		if (AfxGetApp()->GetProfileBinary(L"MainFrame", L"WP", (LPBYTE*)&lwp, &nl))
		{
			SetWindowPlacement(lwp);
			delete[] lwp;
		}
	}

}


void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CMDIFrameWnd::OnSize(nType, cx, cy);
	if (SIZE_MAXIMIZED == nType) {
		MDITile(MDITILE_HORIZONTAL);
	}
	// TODO: Add your message handler code here
}


void CMainFrame::OnExitSizeMove()
{
	// TODO: Add your message handler code here and/or call default

	CMDIFrameWnd::OnExitSizeMove();
	MDITile(MDITILE_HORIZONTAL);
	//CMDIFrameWnd::MDICascade();
}
