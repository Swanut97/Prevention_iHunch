#include "alphapose.h"

#ifdef UNICODE
#define GetCurrentDirectory  GetCurrentDirectoryW
#else
#define GetCurrentDirectory  GetCurrentDirectoryA
#endif // !UNICODE
#define PIPE_NAME   "\\\\.\\pipe\\test_pipe"

using namespace std;

bool endSignal; //���� sign
PROCESS_INFORMATION ProcessInfo; //���μ��� ����
std::mutex mtx; //���ؽ� ����

void editChildProccessPath(char* path);
int ConnectClient(HANDLE hNamePipe);
void checkEndSignal(bool sign);

int startFix(void)
{
    //���μ��� ����
    STARTUPINFO StartupInfo = { 0 };
    StartupInfo.cb = sizeof(STARTUPINFO);

    //������ ����
    HANDLE hNamePipe;
    TCHAR pipe_name[] = _T(PIPE_NAME);

    //������ ����
    hNamePipe = CreateNamedPipe(
        pipe_name,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        0,
        0,
        20000,
        NULL
    );

    if (hNamePipe == INVALID_HANDLE_VALUE)
    {
        printf("CreateNamePipe error! \n");
        return -1;
    }

    //alphapose ������ ���μ��� ��� ��������
    TCHAR path[128]; //�����ڵ� ���ڿ�
    char cpath[128];
    GetCurrentDirectory(128, path); //cwd���
    WideCharToMultiByte(CP_ACP, 0, path, 128, cpath, 128, NULL, NULL); //TCHAR to char
    editChildProccessPath(cpath); //�������� ��η� ����
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, cpath, 128, path, 128); //char to TCHAR

    //�ڽ� ���μ��� ����
    CreateProcess(
        path, //���
        NULL,
        NULL, NULL,
        FALSE, //�θ����μ��� �� ��� ������ �ڵ� ���
        0,
        NULL, NULL,
        &StartupInfo,
        &ProcessInfo //���μ��� ����
    );

    //������ ����
    while (1)
    {
        //������ Named Pipe�� �ڵ��� ������ �� ������ ���  
        if (!(ConnectNamedPipe(hNamePipe, NULL))) {
            CloseHandle(hNamePipe);
            return -1;
        }
        else {
            if (ConnectClient(hNamePipe) == -1)
                break;
        }
    }

    DisconnectNamedPipe(hNamePipe);
    CloseHandle(hNamePipe);
    return 0;
}

//path�� alphapose ������ ���μ����� �������� ��η� ����
void editChildProccessPath(char* path)
{
    int len = strlen(path) - 1;
    char* pp = path + len;
    while (*pp-- != '\\');
    strcpy(pp + 1, "\\x64\\Debug\\caller.exe");
}

int ConnectClient(HANDLE hNamePipe)
{
    TCHAR Message[100];
    DWORD recvSize;

    while (1) {
        checkEndSignal(false);
        if (endSignal) break;
        int n, x, y;
        //recvSize -> NULL ������ ����Ʈ ��
        ReadFile(
            hNamePipe,
            Message,
            sizeof(Message) - sizeof(TCHAR) * 1,
            &recvSize,
            NULL
        );
        _stscanf(Message, _T("%d %d %d"), &n, &x, &y);

        //TODO: 8���� �а� 
        if (n == 0)
            printf("��(%d, %d)\n", x, y);
        if (n == 1)
            printf("�޴�(%d, %d)\n", x, y);
        if (n == 2)
            printf("����(%d, %d)\n", x, y);
        if (n == 3)
            printf("�ޱ�(%d, %d)\n", x, y);
        if (n == 4)
            printf("����(%d, %d)\n", x, y);
        if (n == 5)
            printf("�޾�(%d, %d)\n", x, y);
        if (n == 6)
            printf("����(%d, %d)\n", x, y);
        if (n == 17)
            printf("��(%d, %d)\n", x, y);
    }
    return 1;
}

void checkEndSignal(bool sign)
{
    mtx.lock();
    if (sign)
        endSignal = true;
    mtx.unlock();
}