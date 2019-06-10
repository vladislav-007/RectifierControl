
// RectifierControlDoc.h : интерфейс класса CRectifierControlDoc
//
#include "scomport.h"

#pragma once


class CRectifierControlDoc : public CDocument
{
protected: // создать только из сериализации
	CRectifierControlDoc();
	DECLARE_DYNCREATE(CRectifierControlDoc)

// Атрибуты
public:

// Операции
public:

// Переопределение
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// Реализация
public:
	virtual ~CRectifierControlDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

public:
	RectifierInfo & getRectifierInfo();

protected:
	RectifierInfo m_rectifierInfo;

protected:
	void parseRectifierCfg(CString & rectifierCfgPath);
protected:

	

// Созданные функции схемы сообщений
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// Вспомогательная функция, задающая содержимое поиска для обработчика поиска
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
public:
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	//void Draw(CDC* pDC, CDrawView* pView);
};
