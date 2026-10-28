#pragma once
#include "afxdialogex.h"

// CMainDialog 클래스 정의
class CMainDialog : public CDialogEx
{
    DECLARE_DYNAMIC(CMainDialog)

public:
    CMainDialog(CWnd* pParent = nullptr); // 생성자
    virtual ~CMainDialog();              // 소멸자

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_DIALOG1 }; // 다이얼로그 ID
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()

private:
    // UI 컨트롤 변수
    CStatic m_staticStatus;
    CStatic m_staticCountdown;
    CButton m_btnCall119;
    CButton m_btnStopAlarm;
    CButton m_btnEditInfo;
    CButton m_btnNearHospital;

    // 내부 상태 변수
    HANDLE m_hPipe;              // Named pipe 핸들
    CWinThread* m_pPipeThread;   // 백그라운드 스레드 핸들
    BOOL m_isPipeRunning;        // 파이프 실행 상태 플래그

    // 파이프 초기화 및 스레드 함수
    BOOL InitializeNamedPipe();
    BOOL m_isAlarmPlayed;
    static UINT PipeReadThread(LPVOID pParam);
    void ProcessPipeMessage(const CString& message);

public:
    // 버튼 이벤트 핸들러
    afx_msg void OnBnClickedButtonCall119();
    afx_msg void OnBnClickedButtonStopAlarm();
    afx_msg void OnBnClickedButtonEditInfo();
    afx_msg void OnBnClickedButtonNearHospital();

    afx_msg void OnDestroy(); // 다이얼로그 종료 시 호출
    afx_msg LRESULT OnPipeMessage(WPARAM wParam, LPARAM lParam);
};
