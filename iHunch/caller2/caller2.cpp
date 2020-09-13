#ifdef _DEBUG
#define _DEBUG_WAS_DEFINED 1
#undef _DEBUG
#endif
#define PY_SSIZE_T_CLEAN
#include <Python.h>

#ifdef _DEBUG_WAS_DEFINED
#define _DEBUG 1
#endif

#include <iostream>
#include <cstdio>
#include <windows.h>
#include <tchar.h>
#include <vector>

#define PIPE_NAME   "\\\\.\\pipe\\test_pipe"

using namespace std;

TCHAR pipe_name[] = _T(PIPE_NAME);
HANDLE hNamePipe;

//�������� �޽��� ����
void sendMessage(int n, int x, int y);

int main(void)
{
    TCHAR Message[100];
    DWORD sendSize;
    DWORD recvSize;
    //HWND hWndConsole = GetConsoleWindow();
    //ShowWindow(hWndConsole, SW_HIDE);
    //������ ������Ʈ�� ����
    hNamePipe = CreateFile(pipe_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hNamePipe == INVALID_HANDLE_VALUE) {
        _tprintf(_T("CreateFile error! \n"));
        return -1;
    }
    DWORD pipeMode = PIPE_READMODE_MESSAGE | PIPE_WAIT;
    if (!(SetNamedPipeHandleState(hNamePipe, &pipeMode, NULL, NULL))) {
        _tprintf(_T("SetNamedPipeHandleState error! \n"));
        CloseHandle(hNamePipe);
        return -1;
    }

    //���̽� ����
    PyObject* pName, * pModule, * pFunc, * pValue, * PyArgs, * PyValue;

    Py_Initialize();
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path");

    /*******************************************************
    ************** webcam���� ������ ��� ���� ***************
    ********************************************************/
    PyRun_SimpleString("sys.path.append('C:/Users/golde/source/repos/Prevention_iHunch/iHunch/alphaPose/alphapose-pytorch')");

    pName = PyUnicode_FromString("webcam_measure");

    // fhmm.py ��� ������ �̸��� PyObject�� ����
    pModule = PyImport_Import(pName);
    sendMessage(-2, -1, -1); //import �Ϸ� �޽��� ����
    
    //���� ��ư �Է� ��ȣ ����
    ReadFile(
        hNamePipe,
        Message,
        sizeof(Message) - sizeof(TCHAR) * 1,
        &recvSize,
        NULL
    );

    // fhmm.py�� import �Ѵ�
    if (pModule != NULL) {
        pFunc = PyObject_GetAttrString(pModule, "run");

        while (1) {
            if (pFunc && PyCallable_Check(pFunc))
                // callable���� Ȯ��
            {
                PyObject* tmp, * tmp2;
                pValue = PyObject_CallObject(pFunc, NULL);
                // pFunc�� �Ű����� �����ؼ� �����Ѵ�

                if (PySequence_Check(pValue)) {
                    int cnt = PySequence_Size(pValue);
                    for (int i = 0; i < cnt; i++) {
                        tmp = PySequence_GetItem(pValue, i);
                        
                        if (PySequence_Check(tmp)) {
                            tmp2 = PySequence_GetItem(tmp, 0);
                            int n = 0, x, y;
                            if (PyNumber_Check(tmp2)) {
                                n = PyLong_AsLong(tmp2);
                                tmp2 = PySequence_GetItem(tmp, 1);
                                x = PyLong_AsLong(tmp2);
                                tmp2 = PySequence_GetItem(tmp, 2);
                                y = PyLong_AsLong(tmp2);
                                sendMessage(n, x, y);   //�������� �޽��� ����
                            }
                        }
                    }
                    sendMessage(-1, -1, -1);   //�������� ������ �޽��� ����
                }
            }
        }
    }
    else {
        //pModule ����Ʈ �ȵ����� �Ф�
        printf("pModule is null T.T\n");
    }
    Py_Finalize();

    CloseHandle(hNamePipe);
    return 0;
}

//�������� �޽��� ����
void sendMessage(int n, int x, int y) {
    TCHAR Message[100];
    DWORD sendSize;

    _stprintf(Message, _T("%d %d %d"), n, x, y);

    WriteFile(
        hNamePipe,
        Message,
        (_tcslen(Message) + 1) * sizeof(TCHAR),
        &sendSize,
        NULL
    );
    FlushFileBuffers(hNamePipe);
}