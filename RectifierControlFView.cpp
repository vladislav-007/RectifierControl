// RectifierControlFView.cpp : implementation file
//

#include "stdafx.h"
#include "RectifierControl.h"
#include "RectifierControlFView.h"


// CRectifierControlFView

IMPLEMENT_DYNCREATE(CRectifierControlFView, CFormView)

CRectifierControlFView::CRectifierControlFView()
	: CFormView(IDD_RECTIFIERCONTROLFVIEW)
{

}

CRectifierControlFView::~CRectifierControlFView()
{
}

void CRectifierControlFView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CRectifierControlFView, CFormView)
END_MESSAGE_MAP()


// CRectifierControlFView diagnostics

#ifdef _DEBUG
void CRectifierControlFView::AssertValid() const
{
	CFormView::AssertValid();
}

#ifndef _WIN32_WCE
void CRectifierControlFView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif
#endif //_DEBUG


// CRectifierControlFView message handlers
