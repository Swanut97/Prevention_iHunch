#include "alphapose.h"
#include "iHunch.h"

#ifdef UNICODE
#define GetCurrentDirectory  GetCurrentDirectoryW
#else
#define GetCurrentDirectory  GetCurrentDirectoryA
#endif // !UNICODE
#define PIPE_NAME   "\\\\.\\pipe\\test_pipe"

using namespace std;

extern iHunch* w;

PROCESS_INFORMATION ProcessInfo; //���μ��� ����
std::mutex mtx1, mtx2; //���ؽ� ����
bool endSignal; //���� sign

queue<Points> poseData; //��ǥ�� ������
double stdPoseRate = 5;    //������ �Ǵ� �ʱ��ڼ� ����

RecordTime recordedTime;    //�ڼ��� �ð����
double healthySec, unhealthySec; //����, �����ڼ� �� �ð�
double alarmInterval = 3; //�˶� �︱ �ð� ����

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
    char tmp[100];
    DWORD recvSize;

    Points cur;
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
     
        if (n == -1 && x == -1 && y == -1) {
            operatorQueue(&cur, 0);
            cur = Points();
        }
        else {
            switch (n) {
            case 0:
                cur.Nose.x = x;
                cur.Nose.y = y;
                break;
            case 1:
                cur.lEye.x = x;
                cur.lEye.y = y;
                break;
            case 2:
                cur.rEye.x = x;
                cur.rEye.y = y;
                break;
            case 3:
                cur.lEar.x = x;
                cur.lEar.y = y;
                break;
            case 4:
                cur.rEar.x = x;
                cur.rEar.y = y;
                break;
            case 5:
                cur.lShoulder.x = x;
                cur.lShoulder.y = y;
                break;
            case 6:
                cur.rShoulder.x = x;
                cur.rShoulder.y = y;
                break;
            case 17:
                cur.Neck.x = x;
                cur.Neck.y = y;
                break;
            }
        }
    }
    return 1;
}

void checkEndSignal(bool sign)
{
    mtx1.lock();
    if (sign)
        endSignal = true;
    mtx1.unlock();
}

void judgePose() {
    bool curStatus;
    clock_t curTime;

    while (1) {
        checkEndSignal(false);
        if (endSignal) break;

        Points cur;
        operatorQueue(&cur, 1); //���� �ڼ� ��ǥ�� ��������
        if (cur.length(cur.lEye, cur.rEye) == 0) continue;
        
        curStatus = judge(cur); //�ڼ� �Ǵ�
        curTime = clock(); //���� �ð�
        if (recordedTime.status == -1) {
            //���� ����� ��
            recordedTime.status = curStatus;
            recordedTime.prev = curTime;
            recordedTime.alarmed = curTime;
        }
        else if (recordedTime.status != curStatus) {
            //�ڼ��� �޶����� ��
            
            if (recordedTime.status == GOOD)
                healthySec += (curTime - recordedTime.prev) / CLOCKS_PER_SEC;
            else
                unhealthySec += (curTime - recordedTime.prev) / CLOCKS_PER_SEC;
            
            recordedTime.status = curStatus;
            recordedTime.prev = curTime;
        }
        else { //�ڼ��� ��� ���� ���
            if (recordedTime.status == BAD) {
                double continuedSec = (curTime - recordedTime.prev) / CLOCKS_PER_SEC;
                double lastAlarmed = (curTime - recordedTime.alarmed) / CLOCKS_PER_SEC;

                //�˶� ȣ������ 5�� �̳��̰�, �˶� �︱ ���� �ƴ϶�� ��ŵ
                if (lastAlarmed < 5 && lastAlarmed < alarmInterval) continue; 

                if (continuedSec > alarmInterval) {
                    (*w).alramMessage();
                    recordedTime.alarmed = curTime;
                }
            }
        }
    }
}

void operatorQueue(Points *ret, bool how)
{
    mtx2.lock();
    if (how == false) {//enqueue
        poseData.push(*ret);
    }
    else { //dequeue
        if (!poseData.empty()) {
            *ret = poseData.front();
            poseData.pop();
        }
    }
    mtx2.unlock();
}

bool judge(Points cur) {
    double curRate = cur.length(cur.lShoulder, cur.rShoulder) / cur.length(cur.lEye, cur.rEye);
    if (stdPoseRate * 1.1 > curRate && curRate > stdPoseRate * 0.9)
        return GOOD;
    return BAD;
}