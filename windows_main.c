#include <windows.h>
#include <stdio.h>
#include <assert.h>

#define STRPWSLBEGIN "powershell -NoProfile -ExecutionPolicy Bypass -Command \"& {" \
                            "$ErrorActionPreference='Stop';" \
                            "try { & '..\\.venv\\Scripts\\Activate.ps1';"
#define STRPWSLEND "; exit $LASTEXITCODE } catch { Write-Error $_; exit 1 } }\""
#define OPTION_INSTALL "[\033[1mI\033[0mnstall]"
#define OPTION_UPDATE "[\033[1mU\033[0mpdate]"
#define OPTION_QUIT "[\033[1mQ\033[0muit]"
const char *strSuccess = "\nSuccess!\n\n";

inline void print_menu(void)
{
    assert(printf("Choose:\n"
                "1. " OPTION_INSTALL "\n"
                "2. " OPTION_UPDATE "\n"
                "3. " OPTION_QUIT "\n"));
}

DWORD exec_commandline(LPTSTR strCommandLine)
{
    STARTUPINFO si = { .cb = sizeof(si) };
    PROCESS_INFORMATION pi;
    DWORD dwExitCode;
    assert(CreateProcess(NULL, strCommandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi));
    assert(WaitForSingleObject(pi.hProcess, INFINITE) == WAIT_OBJECT_0);
    assert(GetExitCodeProcess(pi.hProcess, &dwExitCode));
    assert(CloseHandle(pi.hProcess));

    return dwExitCode;
}

int main()
{
    const HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    assert(hStdin != 0 && hStdin != INVALID_HANDLE_VALUE);

    INPUT_RECORD irInBuf[256];
    DWORD fdwMode;
    assert(GetConsoleMode(hStdin, &fdwMode));
    assert(SetConsoleMode(hStdin, fdwMode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_MOUSE_INPUT)));

    print_menu();

    while (1)
    {
main_loop_start:
        assert(WaitForSingleObject(hStdin, INFINITE) == WAIT_OBJECT_0);

        DWORD cNumRead;
        assert(ReadConsoleInput(hStdin, irInBuf, sizeof(irInBuf) / sizeof(irInBuf[0]), &cNumRead));

        for (DWORD i = 0; i < cNumRead; i++)
        {
            if (irInBuf[i].EventType == KEY_EVENT && irInBuf[i].Event.KeyEvent.bKeyDown)
            {
                //assert(printf("[DEBUG] Key pressed: %c\n", irInBuf[i].Event.KeyEvent.uChar.AsciiChar));
                switch (irInBuf[i].Event.KeyEvent.uChar.AsciiChar)
                {
                    case 'I':
                    case 'i':
                    case '1':
                    {
                        assert(printf("Selected " OPTION_INSTALL "\n"));

                        TCHAR strCommandLinePrereq[] = TEXT("winget install Kitware.CMake Ninja-build.Ninja oss-winget.gperf python Git.Git oss-winget.dtc wget 7zip.7zip");
                        if (exec_commandline(strCommandLinePrereq))
                            goto main_end;

                        TCHAR strCommandLineEnv[] = TEXT("python -m venv ..\\.venv");
                        if (exec_commandline(strCommandLineEnv))
                            goto main_end;

                        TCHAR strCommandLineInstall[] = STRPWSLBEGIN TEXT("pip install west; west init -l .") STRPWSLEND;
                        if (exec_commandline(strCommandLineInstall))
                            goto main_end;

                        assert(printf(strSuccess));
                        print_menu();
                        goto main_loop_start;
                    }
                    break;
                    case 'U':
                    case 'u':
                    case '2':
                    {
                        assert(printf("Selected " OPTION_UPDATE "\n"));
                        TCHAR strCommandLine[] = STRPWSLBEGIN TEXT("west update") STRPWSLEND;
                        if (exec_commandline(strCommandLine))
                            goto main_end;
                        
                        assert(printf(strSuccess));
                        print_menu();
                        goto main_loop_start;
                    }
                    break;
                    case 'Q':
                    case 'q':
                    case '3':
                    {
                        assert(printf("Selected " OPTION_QUIT "\n"));
                        goto main_end;
                    }
                    break;
                    default:
                    break;
                }

                if (irInBuf[i].Event.KeyEvent.wVirtualKeyCode == VK_CANCEL)
                {
                    goto main_end;
                }
            }
        }
    }

main_end:
    SetConsoleMode(hStdin, fdwMode);
    return 0;
}