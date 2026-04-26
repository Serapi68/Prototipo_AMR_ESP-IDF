@echo off
REM Configurar ESP-IDF y compilar
cd /d C:\Users\Sergio\OneDrive\Desktop\Proyecto_Final\Proyecto_Final_Software\ESPCAM_VISION_ARTIFICIAL

REM Limpiar build anterior
if exist build rmdir /s /q build

REM Ejecutar idf.py build
call C:\Espressif\idf_cmd_init.bat
python C:/Espressif/frameworks/esp-idf-v5.5.1/tools/idf.py build

pause
