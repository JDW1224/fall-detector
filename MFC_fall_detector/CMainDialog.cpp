#include "pch.h"
#include "CMainDialog.h"
#include "afxdialogex.h"
#include "resource.h"
#include <mmsystem.h>  // PlaySound 함수 사용
#pragma comment(lib, "winmm.lib")  // PlaySound 라이브러리 링크

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainDialog 대화 상자

IMPLEMENT_DYNAMIC(CMainDialog, CDialogEx)

CMainDialog::CMainDialog(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_DIALOG1, pParent)
    , m_hPipe(INVALID_HANDLE_VALUE)
    , m_pPipeThread(nullptr)
    , m_isPipeRunning(FALSE)
{
}

CMainDialog::~CMainDialog()
{
}

void CMainDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_STATIC_STATUS, m_staticStatus);
    DDX_Control(pDX, IDC_STATIC_COUNTDOWN, m_staticCountdown);
    DDX_Control(pDX, IDC_BUTTON_CALL_119, m_btnCall119);
    DDX_Control(pDX, IDC_BUTTON_STOP_ALARM, m_btnStopAlarm);
    DDX_Control(pDX, IDC_BUTTON_EDIT_INFO, m_btnEditInfo);
    DDX_Control(pDX, IDC_BUTTON_NEAR_HOSPTIAL, m_btnNearHospital);
}

BEGIN_MESSAGE_MAP(CMainDialog, CDialogEx)
    ON_MESSAGE(WM_USER + 1, &CMainDialog::OnPipeMessage)
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_BUTTON_CALL_119, &CMainDialog::OnBnClickedButtonCall119)
    ON_BN_CLICKED(IDC_BUTTON_STOP_ALARM, &CMainDialog::OnBnClickedButtonStopAlarm)
    ON_BN_CLICKED(IDC_BUTTON_EDIT_INFO, &CMainDialog::OnBnClickedButtonEditInfo)
    ON_BN_CLICKED(IDC_BUTTON_NEAR_HOSPTIAL, &CMainDialog::OnBnClickedButtonNearHospital)
END_MESSAGE_MAP()

// Named Pipe 초기화
BOOL CMainDialog::InitializeNamedPipe()
{
    OutputDebugString(_T("[MFC] Initializing Named Pipe...\n"));

    m_hPipe = CreateNamedPipe(
        L"\\\\.\\pipe\\fall_detector_pipe",
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        5,  // 최대 5개의 인스턴스 허용
        512, 512, 0, NULL);


    if (m_hPipe == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        CString errorMsg;
        errorMsg.Format(_T("[MFC] Failed to create named pipe. Error: %d\n"), error);
        OutputDebugString(errorMsg);
        return FALSE;
    }

    OutputDebugString(_T("[MFC] Named pipe created successfully.\n"));
    return TRUE;
}

// 파이프 읽기 스레드
UINT CMainDialog::PipeReadThread(LPVOID pParam)
{
    CMainDialog* pDlg = (CMainDialog*)pParam;
    DWORD bytesRead;
    char buffer[512] = { 0 };

    while (pDlg->m_isPipeRunning)
    {
        if (ReadFile(pDlg->m_hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL))
        {
            buffer[bytesRead] = '\0';
            CString message(buffer);
            message.Trim();

            if (!message.IsEmpty()) {
                pDlg->PostMessage(WM_USER + 1, 0, (LPARAM)new CString(message));
            }
        }
        else
        {
            DWORD error = GetLastError();
            if (error == ERROR_BROKEN_PIPE) {
                OutputDebugString(_T("[MFC] Pipe broken. Stopping thread...\n"));
                break;
            }
            Sleep(100);
        }
    }
    return 0;
}

BOOL CMainDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    m_staticStatus.SetWindowText(_T("Status: Ready."));
    m_isAlarmPlayed = FALSE;

    // Named Pipe 초기화 및 스레드 시작
    if (InitializeNamedPipe()) {
        m_isPipeRunning = TRUE;
        m_pPipeThread = AfxBeginThread(PipeReadThread, this);
    }
    else {
        AfxMessageBox(_T("Failed to initialize Named Pipe."));
    }

    return TRUE;
}

// 파이프 메시지 처리
LRESULT CMainDialog::OnPipeMessage(WPARAM wParam, LPARAM lParam)
{
    CString* pMessage = (CString*)lParam;

    // 상태 메시지 표시
    m_staticStatus.SetWindowText(*pMessage);
    OutputDebugString(*pMessage + _T("\n"));

    // "FALL_DETECTED" 상태일 때 알람 재생
    if (*pMessage == _T("FALL_DETECTED")) {
        PlaySound(_T("C:\\Windows\\Media\\Alarm01.wav"), NULL, SND_FILENAME | SND_ASYNC);
        m_isAlarmPlayed = TRUE;
        OutputDebugString(_T("[MFC] FALL_DETECTED: Alarm sound played.\n"));
    }

    delete pMessage;  // 메모리 해제
    return 0;
}

// 버튼 이벤트 핸들러
void CMainDialog::OnBnClickedButtonCall119()
{
    AfxMessageBox(_T("119 Call initiated!"));
}

void CMainDialog::OnBnClickedButtonStopAlarm()
{
    PlaySound(NULL, NULL, SND_ASYNC);  // 알람 소리 중지
    m_isAlarmPlayed = FALSE;  // 알람 재생 플래그 리셋
    AfxMessageBox(_T("Alarm stopped."));
}

void CMainDialog::OnBnClickedButtonEditInfo()
{
    AfxMessageBox(_T("Edit Information selected."));
}

void CMainDialog::OnBnClickedButtonNearHospital()
{
    AfxMessageBox(_T("Showing Nearby Hospitals."));
}

void CMainDialog::OnDestroy()
{
    m_isPipeRunning = FALSE;

    if (m_pPipeThread) {
        WaitForSingleObject(m_pPipeThread->m_hThread, INFINITE);
        m_pPipeThread = nullptr;
    }

    if (m_hPipe != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hPipe);
        m_hPipe = INVALID_HANDLE_VALUE;
    }

    CDialogEx::OnDestroy();
}
