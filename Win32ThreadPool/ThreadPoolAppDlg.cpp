// ThreadPoolAppDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ThreadPoolApp.h"
#include "ThreadPoolAppDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ITERATIONS 10
/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CRunner : public IRunObject
{
public:
	HWND m_hWnd;
	
	void Run()
	{		
		CListBox list;
		list.Attach(m_hWnd);
		list.ResetContent();
		for(int nIndex=0; nIndex < ITERATIONS; nIndex++)
		{
			if(this->pThreadPool->CheckThreadStop())
			{
				break;
			}

			CString sItemText;
			sItemText.Format(_T("Item %d"), nIndex);
			list.AddString(sItemText);
			Sleep(1000);
		}
		list.Detach();	
	}

	bool AutoDelete()
	{
		return true;
	}

};


DWORD WINAPI MyThreadFunc1(LPVOID param)
{
	UserPoolData* poolData = (UserPoolData*)param;
	HWND hWnd = (HWND)(poolData->pData);	
	CListBox list;
	list.Attach(hWnd);
	list.ResetContent();
	for(int nIndex=0; nIndex < ITERATIONS && poolData->pThreadPool->CheckThreadStop() == false; nIndex++)
	{
		CString sItemText;
		sItemText.Format(_T("Item %d"), nIndex);
		list.AddString(sItemText);
		Sleep(1000);
	}
	list.Detach();
	return 0;
}


DWORD WINAPI MyThreadFunc2(LPVOID param)
{
	UserPoolData* poolData = (UserPoolData*)param;
	HWND hWnd = (HWND)(poolData->pData);	
	CListBox list;
	list.Attach(hWnd);
	list.ResetContent();
	for(int nIndex=0; nIndex < 10 && poolData->pThreadPool->CheckThreadStop() == false; nIndex++)
	{
		list.AddString(_T("--Item--"));
		Sleep(1000);
	}
	list.Detach();
	return 0;
}

// pParam - An instance of CPtrArray will be passed
//          Memory has to be released by the called function.
UINT __cdecl DestroyPools(LPVOID pParam)
{
	CPtrArray* pPools = static_cast<CPtrArray*>(pParam);

	for(int n=0; n < pPools->GetCount(); n++)
	{
		CThreadPool* pPool = static_cast<CThreadPool*>(pPools->GetAt(n));
		pPool->Destroy();
	}

	delete pPools;
	pPools = NULL;

	return 0;
}



class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CThreadPoolAppDlg dialog

CThreadPoolAppDlg::CThreadPoolAppDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CThreadPoolAppDlg::IDD, pParent)
	, m_nPoolSize(_T("2")), m_pool1(2)
{
	//{{AFX_DATA_INIT(CThreadPoolAppDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CThreadPoolAppDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CThreadPoolAppDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Text(pDX, IDC_EDIT_POOLSIZE, m_nPoolSize);
}

BEGIN_MESSAGE_MAP(CThreadPoolAppDlg, CDialog)
	//{{AFX_MSG_MAP(CThreadPoolAppDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_CREATE, &CThreadPoolAppDlg::OnBnClickedButtonCreate)
	ON_BN_CLICKED(IDC_BUTTON_ADDWORK1, &CThreadPoolAppDlg::OnBnClickedButtonAddwork1)
	ON_BN_CLICKED(IDC_BUTTON_ADDWORK2, &CThreadPoolAppDlg::OnBnClickedButtonAddwork2)
	ON_BN_CLICKED(IDC_BUTTON_ADDWORK3, &CThreadPoolAppDlg::OnBnClickedButtonAddwork3)
	ON_BN_CLICKED(IDC_BUTTON_DESTROY, &CThreadPoolAppDlg::OnBnClickedButtonDestroy)
	ON_WM_CLOSE()
	ON_WM_DESTROY()	
	ON_BN_CLICKED(ID_CLOSE, &CThreadPoolAppDlg::OnBnClickedClose)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CThreadPoolAppDlg message handlers

BOOL CThreadPoolAppDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	//m_nPoolSize = _T("2");
	//m_pool.SetPoolSize(2);
	UpdateData(FALSE);

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CThreadPoolAppDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CThreadPoolAppDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CThreadPoolAppDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CThreadPoolAppDlg::OnOK() 
{
}

void CThreadPoolAppDlg::OnButton1() 
{	
}

void CThreadPoolAppDlg::OnBnClickedButtonCreate()
{
	UpdateData(TRUE);

	m_pool1.Destroy();
	
	int nPoolsize = _ttoi(m_nPoolSize.GetBuffer());
	
	m_pool1.SetPoolSize(nPoolsize);

	m_pool1.Create();
}

void CThreadPoolAppDlg::OnBnClickedButtonAddwork1()
{
	// Example of using a function pointer as a run object
	HWND hListBox1 = ::GetDlgItem(GetSafeHwnd(), IDC_LIST1);
	m_pool1.Run(MyThreadFunc1, hListBox1, Low);
}

void CThreadPoolAppDlg::OnBnClickedButtonAddwork2()
{
	// Example of using a function pointer as a run object
	HWND hListBox2 = ::GetDlgItem(GetSafeHwnd(), IDC_LIST2);
	m_pool1.Run(MyThreadFunc1, hListBox2, Low);
}

void CThreadPoolAppDlg::OnBnClickedButtonAddwork3()
{
	// Example of using a IRunObject to do some work
	HWND hListBox3 = ::GetDlgItem(GetSafeHwnd(), IDC_LIST3);
		
	CRunner* runner = new CRunner();
	runner->m_hWnd = hListBox3;
	m_pool1.Run(runner);
}

void CThreadPoolAppDlg::OnBnClickedButtonDestroy()
{
	m_pool1.Destroy();
}

void CThreadPoolAppDlg::OnClose()
{
	CDialog::OnClose();
}

void CThreadPoolAppDlg::OnDestroy()
{
	CDialog::OnDestroy();
}

void CThreadPoolAppDlg::OnBnClickedClose()
{
	CPtrArray* pools = new CPtrArray();
	
	pools->Add(&m_pool1);
	
	AfxBeginThread(DestroyPools, pools);

	while(m_pool1.GetState() != Destroyed)
		;
	
	PostQuitMessage(0);
}
