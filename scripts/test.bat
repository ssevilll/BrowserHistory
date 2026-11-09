@echo off
setlocal
set "ROOT=%~dp0.."
cmake -S "%ROOT%" -B "%ROOT%\build" || exit /b 1
cmake --build "%ROOT%\build" --config Release || exit /b 1
"%ROOT%\build\Release\browser_history_native_tests.exe" || exit /b 1
if /I "%~1"=="--xunit" (
  set "BROWSER_HISTORY_EXE=%ROOT%\build\Release\browser_history.exe"
  dotnet test "%ROOT%\tests\xunit\BrowserHistory.XunitTests.csproj" --nologo || exit /b 1
)
