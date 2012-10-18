#include <Windows.h>
#include <stdio.h>
#include <tchar.h>

BOOL dllInjection(HANDLE hProcess,	//プロセスのハンドル
								   const wchar_t *szDllName)//Dllファイル名
{
//自身の絶対パスの取得をして、dllまでの絶対パスを作る
	wchar_t szLibFile[MAX_PATH];
	GetModuleFileNameW(NULL, szLibFile, sizeof(szLibFile));
	lstrcpyW(wcschr(szLibFile, L'\\') + 1, szDllName);
	//長さ(使うメモリ領域)の計算
	int szLibFileLen;
	szLibFileLen = lstrlenW(szLibFile) + 1;
	szLibFileLen = szLibFileLen * sizeof(TCHAR);

	//プロセス内にメモリ領域の確保
	LPSTR RemoteProcessMemory;
	RemoteProcessMemory = (LPSTR)VirtualAllocEx( hProcess, NULL, szLibFileLen, MEM_COMMIT, PAGE_READWRITE);
	if(RemoteProcessMemory == NULL){
		//setErrorString("プロセス内にメモリが確保できませんでした。");
		return FALSE;  
	}
	//書き込み
	if(WriteProcessMemory(hProcess, RemoteProcessMemory, (PVOID)szLibFile, szLibFileLen, NULL) == 0){
		//setErrorString("プロセスに書き込みができませんでした。");
		return FALSE;
	} 
	//LoadLibraryW関数が始まるポインタの取得
	PTHREAD_START_ROUTINE pfnThreadRtn; 
	pfnThreadRtn = (PTHREAD_START_ROUTINE)GetProcAddress( GetModuleHandle(_TEXT("Kernel32")), "LoadLibraryW");
	if (pfnThreadRtn == NULL){
		//setErrorString("LoadLibraryが見つかりませんでした。(何故？)");
		return FALSE;  
	} 
	//スレッド作成
	HANDLE hThread;
	hThread = CreateRemoteThread(hProcess, NULL, 0, pfnThreadRtn, RemoteProcessMemory,CREATE_SUSPENDED, NULL);
	if (hThread == NULL){
		//setErrorString("スレッドが作れませんでした。");
		return FALSE;
	}

	//スレッド実行
	ResumeThread(hThread);
	BYTE *modulePointer = (BYTE *)WaitForSingleObject(hThread,INFINITE);
	DWORD dwExitCode;	//終了コード
	GetExitCodeThread(hThread,&dwExitCode);
	CloseHandle(hThread);	//スレッドを閉じる
	
	//プロセス内に確保したメモリの開放
	VirtualFreeEx(hProcess,RemoteProcessMemory,260,MEM_DECOMMIT);

	return TRUE;
}
