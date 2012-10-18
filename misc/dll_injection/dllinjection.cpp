#include <Windows.h>
#include <stdio.h>
#include <tchar.h>

BOOL dllInjection(HANDLE hProcess,	//�v���Z�X�̃n���h��
								   const wchar_t *szDllName)//Dll�t�@�C����
{
//���g�̐�΃p�X�̎擾�����āAdll�܂ł̐�΃p�X�����
	wchar_t szLibFile[MAX_PATH];
	GetModuleFileNameW(NULL, szLibFile, sizeof(szLibFile));
	lstrcpyW(wcschr(szLibFile, L'\\') + 1, szDllName);
	//����(�g���������̈�)�̌v�Z
	int szLibFileLen;
	szLibFileLen = lstrlenW(szLibFile) + 1;
	szLibFileLen = szLibFileLen * sizeof(TCHAR);

	//�v���Z�X���Ƀ������̈�̊m��
	LPSTR RemoteProcessMemory;
	RemoteProcessMemory = (LPSTR)VirtualAllocEx( hProcess, NULL, szLibFileLen, MEM_COMMIT, PAGE_READWRITE);
	if(RemoteProcessMemory == NULL){
		//setErrorString("�v���Z�X���Ƀ��������m�ۂł��܂���ł����B");
		return FALSE;  
	}
	//��������
	if(WriteProcessMemory(hProcess, RemoteProcessMemory, (PVOID)szLibFile, szLibFileLen, NULL) == 0){
		//setErrorString("�v���Z�X�ɏ������݂��ł��܂���ł����B");
		return FALSE;
	} 
	//LoadLibraryW�֐����n�܂�|�C���^�̎擾
	PTHREAD_START_ROUTINE pfnThreadRtn; 
	pfnThreadRtn = (PTHREAD_START_ROUTINE)GetProcAddress( GetModuleHandle(_TEXT("Kernel32")), "LoadLibraryW");
	if (pfnThreadRtn == NULL){
		//setErrorString("LoadLibrary��������܂���ł����B(���́H)");
		return FALSE;  
	} 
	//�X���b�h�쐬
	HANDLE hThread;
	hThread = CreateRemoteThread(hProcess, NULL, 0, pfnThreadRtn, RemoteProcessMemory,CREATE_SUSPENDED, NULL);
	if (hThread == NULL){
		//setErrorString("�X���b�h�����܂���ł����B");
		return FALSE;
	}

	//�X���b�h���s
	ResumeThread(hThread);
	BYTE *modulePointer = (BYTE *)WaitForSingleObject(hThread,INFINITE);
	DWORD dwExitCode;	//�I���R�[�h
	GetExitCodeThread(hThread,&dwExitCode);
	CloseHandle(hThread);	//�X���b�h�����
	
	//�v���Z�X���Ɋm�ۂ����������̊J��
	VirtualFreeEx(hProcess,RemoteProcessMemory,260,MEM_DECOMMIT);

	return TRUE;
}
