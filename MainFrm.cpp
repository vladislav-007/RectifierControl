
// MainFrm.cpp : ���������� ������ CMainFrame
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
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // ��������� ������ ���������
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

// ��������/����������� CMainFrame

CMainFrame::CMainFrame()
{
	// TODO: �������� ��� ������������� �����
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
		TRACE0("�� ������� ������� ������ ������������\n");
		return -1;      // �� ������� �������
	}

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("�� ������� ������� ������ ���������\n");
		return -1;      // �� ������� �������
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));

	// TODO: ������� ��� ��� ������, ���� �� ����������� ���������� ������ ������������
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);


	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CMDIFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: �������� ����� Window ��� ����� ����������� ���������
	//  CREATESTRUCT cs

	return TRUE;
}

// ����������� CMainFrame

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


// ����������� ��������� CMainFrame



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
