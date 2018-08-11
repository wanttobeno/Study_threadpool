// ThreadPoolAppDlg.h : header file
//
#include "ThreadPool.h"

#if !defined(AFX_THREADPOOLAPPDLG_H__1371D911_4679_4D29_B623_1093F1A2C730__INCLUDED_)
#define AFX_THREADPOOLAPPDLG_H__1371D911_4679_4D29_B623_1093F1A2C730__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CThreadPoolAppDlg dialog

class CThreadPoolAppDlg : public CDialog
{
// Construction
public:
	CThreadPoolAppDlg(CWnd* pParent = NULL);	// standard constructor
	CThreadPool m_pool1;

// Dialog Data
	//{{AFX_DATA(CThreadPoolAppDlg)
	enum { IDD = IDD_THREADPOOLAPP_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CThreadPoolAppDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CThreadPoolAppDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual void OnOK();
	afx_msg void OnButton1();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	CString m_nPoolSize;
	afx_msg void OnBnClickedButtonCreate();
	afx_msg void OnBnClickedButtonAddwork1();
	afx_msg void OnBnClickedButtonAddwork2();
	afx_msg void OnBnClickedButtonAddwork3();
	afx_msg void OnBnClickedButtonDestroy();
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedClose();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_THREADPOOLAPPDLG_H__1371D911_4679_4D29_B623_1093F1A2C730__INCLUDED_)
