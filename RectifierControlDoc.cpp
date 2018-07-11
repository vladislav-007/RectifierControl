
// RectifierControlDoc.cpp : реализация класса CRectifierControlDoc
//

#include "stdafx.h"
// SHARED_HANDLERS можно определить в обработчиках фильтров просмотра реализации проекта ATL, эскизов
// и поиска; позволяет совместно использовать код документа в данным проекте.
#ifndef SHARED_HANDLERS
#include "RectifierControl.h"
#endif

#include "RectifierControlDoc.h"
#include "tinyxml2.h"
#include <filesystem>

#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace fs = std::experimental::filesystem;

// CRectifierControlDoc

IMPLEMENT_DYNCREATE(CRectifierControlDoc, CDocument)

BEGIN_MESSAGE_MAP(CRectifierControlDoc, CDocument)
END_MESSAGE_MAP()


// создание/уничтожение CRectifierControlDoc

CRectifierControlDoc::CRectifierControlDoc()
{
	// TODO: добавьте код для одноразового вызова конструктора
	CString filePath = this->GetPathName();
}

CRectifierControlDoc::~CRectifierControlDoc()
{
}

BOOL CRectifierControlDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: добавьте код повторной инициализации
	// (Документы SDI будут повторно использовать этот документ)

	return TRUE;
}




// сериализация CRectifierControlDoc

void CRectifierControlDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: добавьте код сохранения
	}
	else
	{
		// TODO: добавьте код загрузки
	}
}

#ifdef SHARED_HANDLERS

// Поддержка для эскизов
void CRectifierControlDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Измените этот код для отображения данных документа
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Поддержка обработчиков поиска
void CRectifierControlDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Задайте содержимое поиска из данных документа. 
	// Части содержимого должны разделяться точкой с запятой ";"

	// Например:  strSearchContent = _T("точка;прямоугольник;круг;объект ole;");
	SetSearchContent(strSearchContent);
}

void CRectifierControlDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = NULL;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != NULL)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// диагностика CRectifierControlDoc

#ifdef _DEBUG
void CRectifierControlDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CRectifierControlDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}

RectifierInfo & CRectifierControlDoc::getRectifierInfo()
{
	return m_rectifierInfo;
}

void CRectifierControlDoc::parseRectifierCfg(CString & rectifierCfgPath)
{
	//fs::path dir(szPath);
	//fs::path file("RectifierControlConfig.xml");
	//fs::path full_path = dir.parent_path() / file;
	//std::string s = full_path.string();
	CT2A str(rectifierCfgPath, CP_UTF8);
	const char* filePath = str.m_psz;
	tinyxml2::XMLDocument doc;
	doc.LoadFile(filePath);
	if (doc.Error()) {
		const char * msg = doc.ErrorStr();
		DWORD error = GetLastError();
	}
	else {
		tinyxml2::XMLElement * rectifiers = doc.FirstChildElement("RectifierController")->FirstChildElement("Rectifiers");
		tinyxml2::XMLElement * rectifier = rectifiers->FirstChildElement("Rectifier");
		while (rectifier != nullptr) {
			if (tinyxml2::XML_SUCCESS != rectifier->QueryIntAttribute("id", &m_rectifierInfo.id))
				throw std::exception("Can't read 'id' if rectifier");
			const char * str;
			rectifier->QueryStringAttribute("name", &str);
			m_rectifierInfo.name = CA2T(str, CP_UTF8);
			if (tinyxml2::XML_SUCCESS != rectifier->QueryStringAttribute("comport", &str))
				throw std::exception("Can't read 'comport' if rectifier's mode");
			m_rectifierInfo.comport = CA2T(str, CP_UTF8);

			rectifier->QueryIntAttribute("address", &m_rectifierInfo.address);
			tinyxml2::XMLElement * mode = rectifier->FirstChildElement("Mode");
			if (tinyxml2::XML_SUCCESS != mode->QueryIntAttribute("id", &m_rectifierInfo.modeID))
				throw std::exception("Can't read 'id' if rectifier's mode");

			mode->QueryStringAttribute("name", &str);
			m_rectifierInfo.modeName = CA2T(str, CP_UTF8);
			tinyxml2::XMLElement * valueElement = mode->FirstChildElement("BaudRate");
			valueElement->QueryIntAttribute("value", &m_rectifierInfo.modeBoundRate);
			valueElement = mode->FirstChildElement("ByteSize");
			valueElement->QueryIntAttribute("value", &m_rectifierInfo.modeByteSize);
			valueElement = mode->FirstChildElement("Parity");
			valueElement->QueryStringAttribute("value", &str);
			CString valueStr = CA2T(str, CP_UTF8);
			m_rectifierInfo.modeParity = parityFromString(valueStr);
			valueElement = mode->FirstChildElement("StopBits");
			valueElement->QueryStringAttribute("value", &str);
			valueStr = CA2T(str, CP_UTF8);
			m_rectifierInfo.modeStopbits = stopbitsFromString(valueStr);
			break;
		}
	}
}
#endif //_DEBUG


// команды CRectifierControlDoc


BOOL CRectifierControlDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;

	// TODO:  Add your specialized creation code here
	CString path(lpszPathName);
	parseRectifierCfg(path);

	return TRUE;
}
