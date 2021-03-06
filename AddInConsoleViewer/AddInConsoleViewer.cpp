// AddInConsoleViewer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <strsafe.h>

void ErrorExit(LPTSTR lpszFunction)
{
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(dw);
}

class IComponentBase
{
public:
	virtual ~IComponentBase() {}
};

typedef const wchar_t* (*GetClassNamesPtr)();
typedef long(*GetClassObjectPtr)(const wchar_t* wsName, IComponentBase** pIntf);
typedef long(*DestroyObjectPtr)(IComponentBase** pIntf);


int main(int argc, char* argv[])
{
	if (argc < 2) {
		printf("run as AddInConsoleViewer <PathToDLL>");
		return EXIT_FAILURE;
	}

	int sizePathToDllString = MultiByteToWideChar(CP_ACP, 0, argv[1], -1, NULL, 0);
	LPWSTR pathToDLL = new WCHAR[sizePathToDllString];
	MultiByteToWideChar(CP_ACP, 0, argv[1], -1, pathToDLL, sizePathToDllString);

	HMODULE addInModule = LoadLibrary(pathToDLL);
	if (addInModule == NULL)
		ErrorExit(L"addInModule");

	GetClassNamesPtr addInGetClassNames = (GetClassNamesPtr)GetProcAddress(addInModule, "GetClassNames");
	if (addInGetClassNames == NULL)
	{
		FreeLibrary(addInModule);
		ErrorExit(L"addInGetClassNames");
	}
	const wchar_t* classNames = addInGetClassNames();

	// TODO: classNames может содержать имена нескольких объектов через разделитель "|". Сейчас идет рассчет на то, что объект будет только один.

	IComponentBase** addInObject = new (IComponentBase*);
	*addInObject = NULL;

	GetClassObjectPtr addInGetClassObject = (GetClassObjectPtr)GetProcAddress(addInModule, "GetClassObject");
	if (addInGetClassObject == NULL)
	{
		FreeLibrary(addInModule);
		ErrorExit(L"addInGetClassObject");
	}
	long createSuccess = addInGetClassObject(classNames, addInObject);
	
	if (createSuccess == 0)
	{
		FreeLibrary(addInModule);
		ErrorExit(L"createSuccess = addInGetClassObject");
	}

	DestroyObjectPtr addInDestroyObject = (DestroyObjectPtr)GetProcAddress(addInModule, "DestroyObject");
	if (addInDestroyObject == NULL)
	{
		FreeLibrary(addInModule);
		ErrorExit(L"addInDestroyObject");
	}
	long destroySuccess = addInDestroyObject(addInObject);

	if (destroySuccess != 0)
	{
		FreeLibrary(addInModule);
		ErrorExit(L"destroySuccess = addInDestroyObject");
	}

	FreeLibrary(addInModule);

	return EXIT_SUCCESS;
}