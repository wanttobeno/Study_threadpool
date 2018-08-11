// ThreadPoolApp.h : main header file for the THREADPOOLAPP application
//

#if !defined(AFX_THREADPOOLAPP_H__CCEE55FA_20B7_4FBA_B5C1_DBDA7AA0C20F__INCLUDED_)
#define AFX_THREADPOOLAPP_H__CCEE55FA_20B7_4FBA_B5C1_DBDA7AA0C20F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CThreadPoolAppApp:
// See ThreadPoolApp.cpp for the implementation of this class
//

class CThreadPoolAppApp : public CWinApp
{
public:
	CThreadPoolAppApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CThreadPoolAppApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CThreadPoolAppApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_THREADPOOLAPP_H__CCEE55FA_20B7_4FBA_B5C1_DBDA7AA0C20F__INCLUDED_)
