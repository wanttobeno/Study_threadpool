// Filename		: ThreadPool.cpp
// Author		: Siddharth Barman
// Date			: 18 Sept 2005
// Description	: Implementation file for CThreadPool class. 
//------------------------------------------------------------------------------
#include "stdafx.h"
#include "ThreadPool.h"
#include <process.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
//------------------------------------------------------------------------------

volatile long CThreadPool::m_lInstance = 0;

/* Parameters	: Pointer to a _threadData structure.
   Description	: This is the internal thread function which will run 
				  continuously till the Thread Pool is deleted. Any user thread
				  functions will run from within this function.
*/
#ifdef USE_WIN32API_THREAD
DWORD WINAPI CThreadPool::_ThreadProc(LPVOID pParam)
#else
UINT __stdcall CThreadPool::_ThreadProc(LPVOID pParam)
#endif
{
	DWORD					dwWait;
	CThreadPool*			pool;
  	HANDLE					hThread = GetCurrentThread();
	LPTHREAD_START_ROUTINE  proc;
	LPVOID					data;
	DWORD					dwThreadId = GetCurrentThreadId();
	HANDLE					hWaits[2];
	IRunObject*				runObject;
	bool					bAutoDelete;

	ASSERT(pParam != NULL);
	if(NULL == pParam)
	{
		return -1;
	}

	pool = static_cast<CThreadPool*>(pParam);
	hWaits[0] = pool->GetWaitHandle(dwThreadId);
	hWaits[1] = pool->GetShutdownHandle();
	
	loop_here:

	dwWait = WaitForMultipleObjects(2, hWaits, FALSE, INFINITE);

	if(dwWait - WAIT_OBJECT_0 == 0)
	{
		if(pool->CheckThreadStop())
		{
			return 0;
		}

		// a new function was added, go and get it
		if(pool->GetThreadProc(dwThreadId, proc, &data, &runObject))
		{	
			pool->BusyNotify(dwThreadId);
			
			if(proc == NULL)
			{
				// a function object is being used instead of 
				// a function pointer.
				bAutoDelete = runObject->AutoDelete();
				
				runObject->Run();

				// see if we need to free this object
				if(bAutoDelete)
				{
					#ifdef _DEBUG
					TCHAR szMessage[256];
					_stprintf(szMessage, _T("Deleting Run Object of thread , handle = %d, id = %d\n"), 
					 hThread, dwThreadId);
					TRACE(szMessage);
					#endif

					delete runObject;
				}
				else
				{
					#ifdef _DEBUG
					TCHAR szMessage[256];
					_stprintf(szMessage, _T("Not Deleted Run Object of thread , handle = %d, id = %d\n"), 
					 hThread, dwThreadId);
					TRACE(szMessage);
					#endif
				}
			}
			else
			{
				// note: the user's data is wrapped inside UserPoolData object
				proc(data);
				
				// Note: data is created by the pool and is deleted by the pool.
				//       The internal user data is not deleted by the pool.
				UserPoolData* pPoolData = static_cast<UserPoolData*>(data);
				delete pPoolData;	
			}

			pool->FinishNotify(dwThreadId); // tell the pool, i am now free
		}

		goto loop_here;
	}
			
	return 0;
}

//------------------------------------------------------------------------------
/* Parameters	: Pool size, indicates the number of threads that will be 
				  available in the queue.
*******************************************************************************/
CThreadPool::CThreadPool(int nPoolSize, bool bCreateNow, int nWaitTimeForThreadsToComplete)
{
	m_state = Destroyed;
	m_nPoolSize = nPoolSize;
	m_nWaitForThreadsToDieMS = nWaitTimeForThreadsToComplete;

	// this is used to protect the shared data like the list and map
	InitializeCriticalSection(&m_csFuncList); 
	InitializeCriticalSection(&m_csThreads); 

	if(bCreateNow)
	{
		if(!Create())
		{
			throw 1;
		}
	}
}

//------------------------------------------------------------------------------

/* Description	: Use this method to create the thread pool. The constructor of
				  this class by default will create the pool. Make sure you 
				  do not call this method without first calling the Destroy() 
				  method to release the existing pool.
   Returns		: true if everything went fine else returns false.
  *****************************************************************************/
bool CThreadPool::Create()
{
	if(m_state != Destroyed)
	{
		// To create a new pool, destory the existing one first
		return false;
	}

	InterlockedIncrement(&CThreadPool::m_lInstance);

	_stprintf(m_szPoolName, _T("Pool%d"), m_lInstance);

	HANDLE		hThread;
	DWORD		dwThreadId;
	_ThreadData ThreadData;
	TCHAR		szEvtName[30];
	UINT		uThreadId;	

	// create the event which will signal the threads to stop
	m_hNotifyShutdown = CreateEvent(NULL, TRUE, FALSE, NULL);
	ASSERT(m_hNotifyShutdown != NULL);
	if(!m_hNotifyShutdown)
	{
		return false;
	}

	// create the threads
	for(int nIndex = 0; nIndex < m_nPoolSize; nIndex++)
	{
		_stprintf(szEvtName, _T("PID:%d IID:%d TDX:%d"), GetCurrentProcessId(), CThreadPool::m_lInstance, nIndex /* thread index*/);
				
		#ifdef USE_WIN32API_THREAD
		hThread = CreateThread(NULL, 0, CThreadPool::_ThreadProc, 
							   this, CREATE_SUSPENDED, &dwThreadId);
		ASSERT(NULL != hThread);

		if(NULL == hThread)
		{
			return false;
		}
		#else
		hThread = (HANDLE)_beginthreadex(NULL, 0, CThreadPool::_ThreadProc, this,  
								 CREATE_SUSPENDED, (UINT*)&uThreadId);
		ASSERT(INVALID_HANDLE_VALUE != hThread);

		if(INVALID_HANDLE_VALUE == hThread)
		{
			return false;
		}

		dwThreadId = uThreadId;
		#endif
		
		if(hThread)
		{
			// add the entry to the map of threads
			ThreadData.bFree		= true;
			ThreadData.WaitHandle	= CreateEvent(NULL, TRUE, FALSE, szEvtName);
			ThreadData.hThread		= hThread;
			ThreadData.dwThreadId	= dwThreadId;
		
			m_threads.insert(ThreadMap::value_type(dwThreadId, ThreadData));		

			ResumeThread(hThread); 
		
			#ifdef _DEBUG
			TCHAR szMessage[256];
			_stprintf(szMessage, _T("Thread created, handle = %d, id = %d\n"), 
					  hThread, dwThreadId);
			TRACE(szMessage);
			#endif
		}
		else
		{
			return false;
		}
	}

	m_state = Ready;
	return true;
}
//------------------------------------------------------------------------------

CThreadPool::~CThreadPool()
{
	Destroy();
	ReleaseMemory();	
	DeleteCriticalSection(&m_csFuncList);
	DeleteCriticalSection(&m_csThreads);
}
//------------------------------------------------------------------------------

void CThreadPool::ReleaseMemory()
{
	FunctionList::iterator funcIter;
	
	for(funcIter = m_functionList.begin(); funcIter != m_functionList.end(); funcIter++) 
	{
		if(funcIter->pData != NULL) 
		{
			_FunctionData functionData = (*funcIter);
			UserPoolData* pUserPoolData = static_cast<UserPoolData*>(functionData.pData);

			if(pUserPoolData != NULL)
			{
				delete pUserPoolData;
				pUserPoolData = NULL;
			}
		}
	}

	// empty all collections
	m_functionList.clear();
	m_threads.clear();
}

//------------------------------------------------------------------------------

/* Description	: Use this method to destory the thread pool. The destructor of
				  this class will destory the pool anyway. Make sure you 
				  this method before calling a Create() when an existing pool is 
				  already existing.
   Returns		: void
  *****************************************************************************/
void CThreadPool::Destroy()
{
	if(m_state == Destroying || m_state == Destroyed)
		return;
	
	// Set the state to 'destroying'. Pooled functions might be checking this 
	// state to see if they need to stop.
	m_state = Destroying;
		
	// tell all threads to shutdown.
	ASSERT(NULL != m_hNotifyShutdown);
	SetEvent(m_hNotifyShutdown);

	Sleep(2000); // give the threads a chance to complete

	if(GetWorkingThreadCount() > 0)
	{
		// There are still threads in processing mode..
		// lets give the thread one last chance to finish based on configured setting
		Sleep(m_nWaitForThreadsToDieMS);
	}
	
	ThreadMap::iterator iter;
	_ThreadData ThreadData;
	
	// walk through the events and threads and close them all
	for(iter = m_threads.begin(); iter != m_threads.end(); iter++)
	{
		CloseHandle(iter->second.WaitHandle);
		CloseHandle(iter->second.hThread);
	}

	// close the shutdown event
	CloseHandle(m_hNotifyShutdown);
	m_hNotifyShutdown = NULL;

	ReleaseMemory(); // free any remaining UserPoolData objects 

	InterlockedDecrement(&CThreadPool::m_lInstance);

	m_state = Destroyed;
}
//------------------------------------------------------------------------------

int CThreadPool::GetPoolSize()
{
	return m_nPoolSize;
}
//------------------------------------------------------------------------------

/* Parameters	: nSize - number of threads in the pool.   
   ****************************************************************************/
void CThreadPool::SetPoolSize(int nSize)
{
	ASSERT(nSize > 0);
	if(nSize <= 0)
	{
		return;
	}

	m_nPoolSize = nSize;
}

//------------------------------------------------------------------------------
HANDLE CThreadPool::GetShutdownHandle()
{
	return m_hNotifyShutdown;
}
//------------------------------------------------------------------------------

/* Parameters	: hThread - Handle of the thread that is invoking this function.
   Return		: A ThreadProc function pointer. This function pointer will be 
			      executed by the actual calling thread.
				  NULL is returned if no functions list is empty.
																			  */
bool CThreadPool::GetThreadProc(DWORD dwThreadId, LPTHREAD_START_ROUTINE& Proc, 
								LPVOID* Data, IRunObject** runObject)
{
	LPTHREAD_START_ROUTINE  lpResult = NULL;
	_FunctionData			FunctionData;
	FunctionList::iterator	iter;

	// get the first function info in the function list	
	EnterCriticalSection(&m_csFuncList);
	
	iter = m_functionList.begin();

	if(iter != m_functionList.end())
	{
		FunctionData = (*iter);

		Proc = FunctionData.lpStartAddress;
		
		if(NULL == Proc) // is NULL for function objects
		{		
			*runObject = static_cast<IRunObject*>(FunctionData.pData);
		}
		else		
		{
			// this is a function pointer
			*Data		= FunctionData.pData;
			runObject	= NULL;
		}		

		m_functionList.pop_front(); // remove the function from the list
	
		LeaveCriticalSection(&m_csFuncList);
		return true;	
	}
	else
	{
		LeaveCriticalSection(&m_csFuncList);
		return false;
	}
}
//------------------------------------------------------------------------------

/* Parameters	: hThread - Handle of the thread that is invoking this function.
   Description	: When ever a thread finishes executing the user function, it 
				  should notify the pool that it has finished executing.      
																			  */
void CThreadPool::FinishNotify(DWORD dwThreadId)
{
	ThreadMap::iterator iter;
	
	EnterCriticalSection(&m_csThreads);

	iter = m_threads.find(dwThreadId);

	if(iter == m_threads.end())	// if search found no elements
	{			
		LeaveCriticalSection(&m_csThreads);
		TRACE(_T("No matching thread found."));
		return;
	}
	else
	{	
		m_threads[dwThreadId].bFree = true;

		#ifdef _DEBUG
		TCHAR szMessage[256];
		_stprintf(szMessage, _T("Thread free, thread id = %d\n"), dwThreadId);
		TRACE(szMessage);
		#endif

		if(!m_functionList.empty())
		{
			// there are some more functions that need servicing, lets do that.
			// By not doing anything here we are letting the thread go back and
			// check the function list and pick up a function and execute it.
			LeaveCriticalSection(&m_csThreads);
			return;
		}		
		else
		{
			// back to sleep, there is nothing that needs servicing.
			LeaveCriticalSection(&m_csThreads);
			ResetEvent(m_threads[dwThreadId].WaitHandle);
		}
	}	
}
//------------------------------------------------------------------------------

void CThreadPool::BusyNotify(DWORD dwThreadId)
{
	ThreadMap::iterator iter;
	
	EnterCriticalSection(&m_csThreads);

	iter = m_threads.find(dwThreadId);

	if(iter == m_threads.end())	// if search found no elements
	{
		LeaveCriticalSection(&m_csThreads);
		ASSERT(!_T("No matching thread found."));
	}
	else
	{		
		m_threads[dwThreadId].bFree = false;		

		#ifdef _DEBUG
		TCHAR szMessage[256];
		_stprintf(szMessage, _T("Thread busy, thread id = %d\n"), dwThreadId);
		TRACE(szMessage);
		#endif

		LeaveCriticalSection(&m_csThreads);
	}	
}
//------------------------------------------------------------------------------

/* Parameters	: pFunc - function pointer of type ThreadProc
				  pData - An LPVOID pointer

   Returns		: true is all goes well.
                  false if the pool is in the process of getting destroyed.
   
   Decription	: This function is to be called by clients which want to make 
				  use of the thread pool.
  *****************************************************************************/
bool CThreadPool::Run(LPTHREAD_START_ROUTINE pFunc, LPVOID pData, 
					  ThreadPriority priority)
{
	if(m_state == Destroying || m_state == Destroyed)
		return false;
	
	_FunctionData funcdata;

	UserPoolData* pPoolData = new UserPoolData();
	
	pPoolData->pData = pData;
	pPoolData->pThreadPool = this;

	funcdata.lpStartAddress = pFunc;
	funcdata.pData			= pPoolData;

	// add it to the list
	EnterCriticalSection(&m_csFuncList);
	if(priority == Low)
	{
		m_functionList.push_back(funcdata);
	}
	else
	{
		m_functionList.push_front(funcdata);
	}
	LeaveCriticalSection(&m_csFuncList);

	// See if any threads are free
	ThreadMap::iterator iter;
	_ThreadData ThreadData;

	EnterCriticalSection(&m_csThreads);
	for(iter = m_threads.begin(); iter != m_threads.end(); iter++)
	{
		ThreadData = (*iter).second;
		
		if(ThreadData.bFree)
		{
			// here is a free thread, put it to work
			m_threads[ThreadData.dwThreadId].bFree = false;			
			SetEvent(ThreadData.WaitHandle); 
			// this thread will now call GetThreadProc() and pick up the next
			// function in the list.
			break;
		}
	}
	LeaveCriticalSection(&m_csThreads);

	return true;
}
//------------------------------------------------------------------------------

/* Parameters	: runObject - Pointer to an instance of class which implements
							  IRunObject interface.
				  priority  - Low or high. Based on this the object will be
							  added to the front or back of the list.
   Returns		: true is all goes well.
                  false if the pool is in the process of getting destroyed.

   Decription	: This function is to be called by clients which want to make 
				  use of the thread pool.
  *****************************************************************************/
bool CThreadPool::Run(IRunObject* runObject, ThreadPriority priority)
{
	if(m_state == Destroying || m_state == Destroyed)
		return false;

	ASSERT(runObject != NULL);
		
	runObject->pThreadPool = this; 

	_FunctionData funcdata;

	funcdata.lpStartAddress = NULL; // NULL indicates a function object is being
									// used instead.
	funcdata.pData			= runObject; // the function object

	// add it to the list
	EnterCriticalSection(&m_csFuncList);
	if(priority == Low)
	{
		m_functionList.push_back(funcdata);
	}
	else
	{
		m_functionList.push_front(funcdata);
	}
	LeaveCriticalSection(&m_csFuncList);

	// See if any threads are free
	ThreadMap::iterator iter;
	_ThreadData ThreadData;

	EnterCriticalSection(&m_csThreads);
	for(iter = m_threads.begin(); iter != m_threads.end(); iter++)
	{
		ThreadData = (*iter).second;
		
		if(ThreadData.bFree)
		{
			// here is a free thread, put it to work
			m_threads[ThreadData.dwThreadId].bFree = false;			
			SetEvent(ThreadData.WaitHandle); 
			// this thread will now call GetThreadProc() and pick up the next
			// function in the list.
			break;
		}
	}

	LeaveCriticalSection(&m_csThreads);

	return true;
}
//------------------------------------------------------------------------------

/* Parameters	: ThreadId - the id of the thread for which the wait handle is 
							 being requested.
   Returns		: NULL if no mathcing thread id is present.
				  The HANDLE which can be used by WaitForXXXObject API.
  *****************************************************************************/
HANDLE CThreadPool::GetWaitHandle(DWORD dwThreadId)
{
	HANDLE hWait;
	ThreadMap::iterator iter;
	
	EnterCriticalSection(&m_csThreads);
	iter = m_threads.find(dwThreadId);
	
	if(iter == m_threads.end())	// if search found no elements
	{
		LeaveCriticalSection(&m_csThreads);
		return NULL;
	}
	else
	{		
		hWait = m_threads[dwThreadId].WaitHandle;
		LeaveCriticalSection(&m_csThreads);
	}	

	return hWait;
}
//------------------------------------------------------------------------------

bool CThreadPool::CheckThreadStop()
{
	// This is function expected to be called by thread functions or IRunObject
	// derived. The expectation is that the code will check this 'property' of
	// the pool and stop it's processing as soon as possible.
	return (m_state == Destroying || m_state == Destroyed);
}

//------------------------------------------------------------------------------
 
int CThreadPool::GetWorkingThreadCount()
{
	// Returns true is 
	ThreadMap::iterator iter;
	_ThreadData ThreadData;
	int nCount = 0;

    for(iter = m_threads.begin(); iter != m_threads.end(); iter++) 
	{
		ThreadData = (*iter).second;

        if(!ThreadData.bFree) 
		{
			nCount++;
		}
	}

    return nCount;
}
//------------------------------------------------------------------------------

PoolState CThreadPool::GetState()
{
	return m_state;
}
//------------------------------------------------------------------------------