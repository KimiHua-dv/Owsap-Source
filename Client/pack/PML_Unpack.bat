
@echo off
setlocal

title PackMakerLite - Unpack

echo =====================================
echo TESTE PACKMAKERLITE
echo =====================================

if "%~1"=="" (
    echo Arraste um arquivo IDX sobre este BAT.
    pause
    exit /b 1
)

if not exist "%~dp0PackMakerLite.exe" (
    echo ERRO: PackMakerLite.exe nao encontrado.
    pause
    exit /b 1
)

if /i not "%~x1"==".idx" (
    echo ERRO: O arquivo precisa ter extensao .idx
    pause
    exit /b 1
)

if not exist "%~dp1%~n1.data" (
    echo ERRO: Arquivo DATA correspondente nao encontrado:
    echo %~dp1%~n1.data
    pause
    exit /b 1
)

echo Executavel: %~dp0PackMakerLite.exe
echo Indice: %~f1
echo Dados: %~dp1%~n1.data
echo Pacote: %~n1

pushd "%~dp1"

"%~dp0PackMakerLite.exe" -u "%~n1"

set "RESULTADO=%ERRORLEVEL%"

popd

echo.
if "%RESULTADO%"=="0" (
    echo Processo finalizado.
) else (
    echo Processo retornou codigo: %RESULTADO%
)

pause
