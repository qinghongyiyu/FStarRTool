@echo off
echo ========================================
echo 检测可用的音频设备
echo ========================================
echo.

set FFMPEG_PATH=%~dp0ffmpeg-8.0.1-full_build-shared\bin\ffmpeg.exe

if not exist "%FFMPEG_PATH%" (
    echo 错误: 找不到 FFmpeg
    echo 路径: %FFMPEG_PATH%
    pause
    exit /b 1
)

echo 正在列出所有 DirectShow 音频设备...
echo.

"%FFMPEG_PATH%" -list_devices true -f dshow -i dummy 2>&1 | findstr /C:"DirectShow" /C:"audio" /C:"Alternative"

echo.
echo ========================================
echo 检测完成
echo ========================================
echo.
echo 请查看上面列出的音频设备名称
echo 如果要录制系统声音，需要启用"立体声混音"
echo.
pause
