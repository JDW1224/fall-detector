import os
import time
import cv2
import mediapipe as mp
import win32file
import win32pipe
from datetime import datetime, timedelta
import math

# Named Pipe 설정
PIPE_NAME = r'\\.\pipe\fall_detector_pipe'

### NamedPipeManager 클래스
class NamedPipeManager:
    def __init__(self, pipe_name):
        self.pipe_name = pipe_name
        self.pipe = None

    def connect_pipe(self):
        #"""Named Pipe 연결"""
        while True:
            try:
                print(f"[Python] Trying to connect to pipe: {self.pipe_name}")
                self.pipe = win32file.CreateFile(
                    self.pipe_name,
                    win32file.GENERIC_WRITE,
                    0,
                    None,
                    win32file.OPEN_EXISTING,
                    0,
                    None
                )
                print("[Python] Pipe connection established.")
                break
            except Exception as e:
                print(f"[Python] Pipe not ready, retrying... ({e})")
                time.sleep(1)

    def send_data(self, data):
        #"""Named Pipe를 통해 데이터 전송"""
        try:
            message = data + "\n"
            win32file.WriteFile(self.pipe, message.encode())
            print(f"[Python] Sent: {message.strip()}")
        except Exception as e:
            print(f"[Python] Named pipe communication error: {e}")
            self.connect_pipe()

    def close_pipe(self):
        #"""파이프 닫기"""
        if self.pipe:
            win32file.CloseHandle(self.pipe)
            self.pipe = None
            print("[Python] Pipe closed.")


### PoseData 클래스
class PoseData:
    def __init__(self, landmarks, prev_landmarks=None):
        self.landmarks = landmarks
        self.poses = self.extract_landmarks()
        self.prev_landmarks = prev_landmarks

    def extract_landmarks(self):
        #"""랜드마크를 딕셔너리 형태로 변환"""
        return {
            idx: (lm.x, lm.y, lm.z) for idx, lm in enumerate(self.landmarks)
        }

    def get_average_y(self, point1, point2):
        #"""두 랜드마크의 평균 Y 좌표 반환"""
        return (self.poses[point1][1] + self.poses[point2][1]) / 2

    def calculate_angle(self, a, b, c):
        #"""세 점의 각도를 계산"""
        ax, ay = self.poses[a][:2]
        bx, by = self.poses[b][:2]
        cx, cy = self.poses[c][:2]

        ab = math.sqrt((ax - bx)**2 + (ay - by)**2)
        bc = math.sqrt((bx - cx)**2 + (by - cy)**2)
        ac = math.sqrt((ax - cx)**2 + (ay - cy)**2)

        if ab * bc == 0:
            return 180.0
        return math.degrees(math.acos((ab**2 + bc**2 - ac**2) / (2 * ab * bc)))

    def is_fall_detected(self):
    #"""넘어짐 감지 조건"""
        hips_y = self.get_average_y(mp.solutions.pose.PoseLandmark.LEFT_HIP.value,
                                mp.solutions.pose.PoseLandmark.RIGHT_HIP.value)
        shoulders_y = self.get_average_y(mp.solutions.pose.PoseLandmark.LEFT_SHOULDER.value,
                                     mp.solutions.pose.PoseLandmark.RIGHT_SHOULDER.value)

    # 어깨-엉덩이 각도
        angle = self.calculate_angle(mp.solutions.pose.PoseLandmark.LEFT_SHOULDER.value,
                                 mp.solutions.pose.PoseLandmark.LEFT_HIP.value,
                                 mp.solutions.pose.PoseLandmark.LEFT_KNEE.value)

    # Z축 깊이 차이로 상체 기울기 확인
        shoulder_z = (self.landmarks[mp.solutions.pose.PoseLandmark.LEFT_SHOULDER.value].z +
                  self.landmarks[mp.solutions.pose.PoseLandmark.RIGHT_SHOULDER.value].z) / 2
        hip_z = (self.landmarks[mp.solutions.pose.PoseLandmark.LEFT_HIP.value].z +
             self.landmarks[mp.solutions.pose.PoseLandmark.RIGHT_HIP.value].z) / 2

        z_diff = abs(shoulder_z - hip_z)

    # 좌우(X축) 회전 필터링
        shoulder_x_diff = abs(self.landmarks[mp.solutions.pose.PoseLandmark.LEFT_SHOULDER.value].x -
                          self.landmarks[mp.solutions.pose.PoseLandmark.RIGHT_SHOULDER.value].x)

    # 조건 수정: Z축 깊이 차이와 X축 대칭성 고려
        is_shoulders_low = shoulders_y > hips_y + 0.1  # 어깨가 엉덩이보다 충분히 낮은 경우
        is_body_tilted = angle < 45.0  # 상체 각도가 45도 이하로 기울어진 경우
        is_z_tilted = z_diff > 0.1     # Z축 깊이 차이가 클 경우
        is_no_x_rotation = shoulder_x_diff < 0.2  # X축 비대칭 회전이 없을 경우

        return is_shoulders_low and is_body_tilted and is_z_tilted and is_no_x_rotation



### PoseStateManager 클래스
class PoseStateManager:
    def __init__(self, threshold_frames=5, sustain_time=60):
        self.previous_state = "NO_FALL"
        self.fall_detected_count = 0
        self.threshold_frames = threshold_frames
        self.sustain_time = timedelta(seconds=sustain_time)
        self.fall_detected_time = None

    def update_state(self, pose_data):
        #"""포즈 데이터를 바탕으로 상태 업데이트"""
        if pose_data and pose_data.is_fall_detected():
            self.fall_detected_count += 1
            if self.fall_detected_count >= self.threshold_frames:
                self.previous_state = "FALL_DETECTED"
                self.fall_detected_time = datetime.now() + self.sustain_time
        else:
            if self.fall_detected_time and datetime.now() < self.fall_detected_time:
                self.previous_state = "FALL_DETECTED"
            else:
                self.previous_state = "NO_FALL"
                self.fall_detected_count = 0
                self.fall_detected_time = None

        return self.previous_state


### 메인 함수
def main():
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("Failed to open webcam.")
        return

    detector = mp.solutions.pose.Pose()
    pipe_manager = NamedPipeManager(PIPE_NAME)
    pipe_manager.connect_pipe()
    state_manager = PoseStateManager()
    prev_landmarks = None

    while cap.isOpened():
        ret, frame = cap.read()
        if not ret:
            print("Failed to read frame.")
            break

        image = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        results = detector.process(image)

        pose_data = None
        if results.pose_landmarks:
            pose_data = PoseData(results.pose_landmarks.landmark, prev_landmarks)
            prev_landmarks = pose_data.landmarks

        current_state = state_manager.update_state(pose_data)
        pipe_manager.send_data(current_state)

        # 비디오 화면 표시
        cv2.putText(frame, f"State: {current_state}", (10, 30), cv2.FONT_HERSHEY_SIMPLEX,
                    1, (0, 255, 0), 2, cv2.LINE_AA)
        cv2.imshow('Fall Detector', frame)

        if cv2.waitKey(10) & 0xFF == 27:  # ESC 키로 종료
            break

    cap.release()
    pipe_manager.close_pipe()
    cv2.destroyAllWindows()
    print("[Python] Program terminated.")


if __name__ == "__main__":
    main()
