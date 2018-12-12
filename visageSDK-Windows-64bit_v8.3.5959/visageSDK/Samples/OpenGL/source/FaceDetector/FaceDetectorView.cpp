// FaceDetectorView.cpp : implementation of the CFaceDetectorView class
//

#include "stdafx.h"
#include "FaceDetector.h"

#include "FaceDetectorDoc.h"
#include "FaceDetectorView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CFaceDetectorView

IMPLEMENT_DYNCREATE(CFaceDetectorView, CView)

BEGIN_MESSAGE_MAP(CFaceDetectorView, CView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
END_MESSAGE_MAP()

// CFaceDetectorView construction/destruction

CFaceDetectorView::CFaceDetectorView()
{
	// TODO: add construction code here

}

CFaceDetectorView::~CFaceDetectorView()
{
}

BOOL CFaceDetectorView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// CFaceDetectorView drawing

void CFaceDetectorView::OnDraw(CDC* /*pDC*/)
{
	CFaceDetectorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: add draw code for native data here
}


// CFaceDetectorView printing

BOOL CFaceDetectorView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CFaceDetectorView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CFaceDetectorView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}


// CFaceDetectorView diagnostics

#ifdef _DEBUG
void CFaceDetectorView::AssertValid() const
{
	CView::AssertValid();
}

void CFaceDetectorView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CFaceDetectorDoc* CFaceDetectorView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CFaceDetectorDoc)));
	return (CFaceDetectorDoc*)m_pDocument;
}
#endif //_DEBUG


// CFaceDetectorView message handlers
