# Fall Detector

**2024년 12월 · 오픈소스개발프로젝트**

웹캠 영상의 자세 정보를 이용해 낙상 의심 상태를 판단하고, Windows MFC 화면으로 상태를 전달하는 학업 프로젝트입니다. 수업에서 사용한 소스를 정리해 보관했습니다.

## 코드에서 확인한 기능

- Python/OpenCV로 기본 웹캠 영상을 읽고 MediaPipe Pose로 신체 랜드마크를 추출합니다.
- 어깨·엉덩이 위치, 신체 각도와 깊이 차이를 조합해 낙상 의심 상태를 판단합니다.
- 기본 설정은 조건을 5프레임 연속 만족하면 `FALL_DETECTED` 상태로 전환하고, 상태를 60초 동안 유지합니다.
- Windows Named Pipe(`\\.\pipe\fall_detector_pipe`)로 `FALL_DETECTED` 또는 `NO_FALL`을 MFC 프로그램에 전달합니다.
- MFC 화면에서 상태를 표시하고 Windows 알람 소리를 재생·중지합니다.

`119 호출`, `근처 병원`, `정보 수정` 버튼은 현재 코드에서 메시지 상자를 표시합니다. 실제 긴급전화 연결, 병원 조회, 정보 저장은 구현되어 있지 않습니다.

## 구성

```text
MFC_fall_detector/       C++/MFC 화면, Named Pipe 서버, Visual Studio 솔루션
python_fall_detector/    자세 추정, 상태 판정, Named Pipe 클라이언트
requirements.txt        Python import 기준 의존성 목록
```

## 준비 환경

- Windows, 웹캠
- Visual Studio의 C++ 데스크톱 개발 도구와 MFC 구성 요소
- MSVC v143 및 Windows SDK 10.0 계열: 원본 `.vcxproj` 기준
- Python, OpenCV, MediaPipe, pywin32

원래 사용한 Python 및 라이브러리의 정확한 버전 기록은 포함되어 있지 않습니다. 이 코드는 `mediapipe.solutions.pose` API를 사용하므로 해당 API를 제공하는 환경이 필요합니다. `requirements.txt`는 버전을 고정한 재현 환경 파일이 아닙니다.

## 빌드와 실행

저장소 루트의 PowerShell에서 Python 환경을 준비합니다.

```powershell
py -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
```

Visual Studio에서 `MFC_fall_detector/MFC_fall_detector.sln`을 열고 `Debug / x64` 구성으로 빌드합니다. MSBuild가 준비된 Developer PowerShell에서는 다음 명령을 사용할 수 있습니다.

```powershell
msbuild .\MFC_fall_detector\MFC_fall_detector.sln /m /p:Configuration=Debug /p:Platform=x64
```

MFC 프로그램을 먼저 실행한 뒤 Python 클라이언트를 실행합니다.

```powershell
Start-Process .\MFC_fall_detector\x64\Debug\MFC_fall_detector.exe
.\.venv\Scripts\python.exe .\python_fall_detector\python_fall_detector.py
```

Python 프로그램은 기본 카메라 `0`을 사용하고 Named Pipe 서버가 준비될 때까지 연결을 재시도합니다. 영상 창에서 `Esc`를 누르면 종료합니다. 알람 재생에는 코드에 지정된 `C:\Windows\Media\Alarm01.wav` 파일이 필요합니다.

## 공개용 정리와 확인 범위

Visual Studio 캐시, 중간 빌드 파일, 실행 파일, 개인별 `.user` 설정은 제외했습니다. 원본 Python 파일의 CP949 주석은 UTF-8로 변환했고, C++ 리소스 빌드 설정의 개발자 PC 절대 경로는 `$(IntDir)` 기준으로 변경했습니다.

소스 구성과 Python 문법을 확인했습니다. MFC 빌드, 웹캠 입력, Named Pipe 연동 및 낙상 판정 성능은 이번 정리 과정에서 실행 검증하지 않았습니다.

## 출처와 라이선스

제공된 프로젝트에는 별도 라이선스 파일이 없습니다. 원래 소스의 표기는 보존했으며, 외부 라이브러리의 이용 조건은 각각의 라이선스를 따릅니다.
