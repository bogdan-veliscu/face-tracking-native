// FaceDetectorView.h : interface of the CFaceDetectorView class
//


#pragma once


class CFaceDetectorView : public CView
{
protected: // create from serialization only
	CFaceDetectorView();
	DECLARE_DYNCREATE(CFaceDetectorView)

// Attributes
public:
	CFaceDetectorDoc* GetDocument() const;

// Operations
public:

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~CFaceDetectorView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in FaceDetectorView.cpp
inline CFaceDetectorDoc* CFaceDetectorView::GetDocument() const
   { return reinterpret_cast<CFaceDetectorDoc*>(m_pDocument); }
#endif

