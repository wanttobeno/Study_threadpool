

##### 优先级实现
使用stl::list的可以前后插入

```cpp
if (priority == Low)
	m_jobList.push_back(jobData);
else
	m_jobList.push_front(jobData);
```

##### 支持全局函数和类对象
```cpp	
// Get the job and remove it from the queue
LPTHREAD_START_ROUTINE proc = iter->lpStartAddress;
LPVOID data = iter->pData;
IRunObject* runObject = iter->runObject;
pool->m_jobList.pop_front();
LeaveCriticalSection(&pool->m_cs);
// Run the job
if (proc)	
	proc(data);
else if (runObject)
{
	bool autoDelete = runObject->AutoDelete();
	runObject->Run(); // may execute "delete this"
	if (autoDelete)
		delete runObject;
}
```

##### 任务自动删除
```cpp
// free any jobs not released
for (JobList::iterator funcIter = m_jobList.begin(); 
	funcIter != m_jobList.end(); funcIter++) 
if (funcIter->runObject && funcIter->runObject->AutoDelete())	delete funcIter->runObject;
```

##### bug修复
写了个demo测试，发现启动的时候有一定几率奔溃。
提示“vector iterators incompatible”。
定位到

```cpp
HANDLE ThreadPool::GetWaitHandle(DWORD dwThreadId)
{
	HANDLE hWait = NULL;
	EnterCriticalSection(&m_cs);
	for (ThreadList::iterator iter = m_threads.begin(); !hWait && iter != m_threads.end(); ++iter)
		if (iter->dwThreadId == dwThreadId)
			hWait = iter->hWait;
	LeaveCriticalSection(&m_cs);
	return hWait;
}
```
这个函数又有UINT __stdcall ThreadPool::ThreadProc(LPVOID pParam)调用的。
搜索ThreadProc的所有调用，最终定位到bool ThreadPool::Create(int poolSize)，
添加一个关键锁

```cpp
EnterCriticalSection(&m_cs);
m_threads.push_back(td);
LeaveCriticalSection(&m_cs);
```








