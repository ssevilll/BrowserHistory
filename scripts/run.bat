@echo off
setlocal
set "ROOT=%~dp0.."
cmake -S "%ROOT%" -B "%ROOT%\build" || exit /b 1
cmake --build "%ROOT%\build" --config Release --target browser_history || exit /b 1
"%ROOT%\build\Release\browser_history.exe" %*
