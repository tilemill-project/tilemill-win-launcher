#include <windows.h>
#include <winbase.h>
#include <tchar.h>
#include <strsafe.h>
#include <stdio.h>
#include <Shlobj.h> // for SHGetSpecialFolderPath
#include <string>

#define BUFSIZE 4096

HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;
HANDLE g_hInputFile = NULL;

void ErrorExit(LPTSTR lpszFunction, DWORD dw)
{
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
                                      (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 100) * sizeof(TCHAR));
    StringCchPrintf((LPTSTR)lpDisplayBuf,
                    LocalSize(lpDisplayBuf) / sizeof(TCHAR),
                    TEXT("%s\n\n Failed with error %d: %s\nPlease report this problem to https://github.com/mapbox/tilemill/issues"),
                    lpszFunction, dw, lpMsgBuf);
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("TileMill Error"), MB_OK|MB_SYSTEMMODAL);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(dw);
}

void ErrorExit(LPTSTR lpszFunction)
{
    ErrorExit(lpszFunction,GetLastError());
}

bool writeToLog(const char* chBuf)
{
    DWORD dwRead = strlen(chBuf);
    DWORD dwWritten(0);
    BOOL bSuccess = FALSE;

    return WriteFile(g_hInputFile, chBuf,
                     dwRead, &dwWritten, NULL);
}

void ReadFromPipe(void)
{
    DWORD dwRead, dwWritten;
    CHAR chBuf[BUFSIZE];
    BOOL bSuccess = FALSE;
    HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    // Close the write end of the pipe before reading from the
    // read end of the pipe, to control child process execution.
    // The pipe is assumed to have enough buffer space to hold the
    // data the child process has already written to it.

    if (!CloseHandle(g_hChildStd_OUT_Wr))
        ErrorExit(TEXT("StdOutWr CloseHandle"));

    bool fatal = false;
    for (;;)
    {
        bSuccess = ReadFile( g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
        if(!bSuccess )
        {
            break;
        }
        std::string debug_line(chBuf);
        std::string substring = debug_line.substr(0,static_cast<size_t>(dwRead));
        substring += "\nPlease report this to https://github.com/mapbox/tilemill/issues\n";
        if (!fatal &&
            (substring.find("Client Error:") == std::string::npos) &&
            ((substring.find("throw e; // process") != std::string::npos) || (substring.find("EADDRINUSE") !=std::string::npos))
           )
        {
            if (substring.find("EADDRINUSE") !=std::string::npos)
            {
                MessageBox(NULL, static_cast<LPCSTR>("TileMill appears to already be running. If you have another TileMill instance open please close it. If not then you may have 'runaway' processes (see http://tilemill.com/docs/troubleshooting/ for help)"), TEXT("TileMill Error"), MB_OK|MB_SYSTEMMODAL);
            }
            else
            {
                MessageBox(NULL, static_cast<LPCSTR>(substring.c_str()), TEXT("TileMill Error"), MB_OK|MB_SYSTEMMODAL);
            }
            fatal = true;
        }
        bSuccess = WriteFile(g_hInputFile, chBuf,
                             dwRead, &dwWritten, NULL);
        if (!bSuccess)
        {
            break;
        }
    }
    if (fatal)
    {
        writeToLog("Exiting TileMill due to fatal error...\n");
        ExitProcess(1);
    }
}

void msgExit(LPTSTR message)
{
    MessageBox(NULL, message, TEXT("TileMill Error"), MB_OK|MB_SYSTEMMODAL);
    ExitProcess(1);
}

void CreateChildProcess(TCHAR * szCmdline)
{
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;
    ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
    ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );

    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = g_hChildStd_OUT_Wr;
    siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
    siStartInfo.hStdInput = g_hChildStd_IN_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    BOOL bSuccess = FALSE;
    bSuccess = CreateProcess(NULL,
                             szCmdline,     // command line
                             NULL,          // process security attributes
                             NULL,          // primary thread security attributes
                             TRUE,          // handles are inherited
                             CREATE_NO_WINDOW,             // creation flags
                             NULL,          // use parent's environment
                             NULL,          // use parent's current directory
                             &siStartInfo,  // STARTUPINFO pointer
                             &piProcInfo);  // receives PROCESS_INFORMATION

    if (!bSuccess)
    {
        ErrorExit(TEXT("Could not launch TileMill's node process"));
    }
    else
    {
        // Close handles to the child process and its primary thread.
        // Some applications might keep these handles to monitor the status
        // of the child process, for example.
        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);
    }
}

bool FileExists(const TCHAR *fileName)
{
    DWORD       fileAttr;
    fileAttr = GetFileAttributes(fileName);
    if (0xFFFFFFFF == fileAttr)
    {
        return false;
    }
    return true;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR    lpCmdLine,
                     int       nCmdShow) {

    if (!FileExists("tilemill\\node.exe") && !FileExists("tilemill\\node_modules"))
    {
        msgExit(TEXT("Could not start: TileMill.exe could not find supporting files"));
    }
    
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    if (! CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0) )
    {
        ErrorExit(TEXT("could not create pipe to stdout"));
    }

    if (! SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0) )
    {
        ErrorExit(TEXT("could not get stdout handle info"));
    }

    if (! CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
    {
        ErrorExit(TEXT("could not create pipe to stdin"));
    }

    if (! SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) )
    {
        ErrorExit(TEXT("could not get stdin handle info"));
    }

    /*
     * Clear out the PATH environment so avoid potential dll problems if system has duplicates
	 * The PATH should be set by the node ./index.js file as it starts up
     */
    if (!SetEnvironmentVariableA("PATH",NULL))
    {
        ErrorExit("TileMill.exe clearing env: ",GetLastError());
    }
	char* env = ::GetEnvironmentStrings();
	if (0 != env) {
	    std::string senv = env;
		std::string msg = std::string("TileMill PATH env: ") + senv;
		writeToLog(msg.c_str());
	}

    // Create the child process.
    TCHAR cmd[]=TEXT(".\\tilemill\\node.exe .\\tilemill\\index.js");
    CreateChildProcess(cmd);

    TCHAR strPath[ MAX_PATH ];

    // Get the special folder path.
    SHGetSpecialFolderPath(
        0,       // Hwnd
        strPath, // String buffer.
        CSIDL_PROFILE, // CSLID of folder
        FALSE ); // Create if doesn't exists?

    std::string logpath(strPath);
    logpath += "\\tilemill.log";
    g_hInputFile = CreateFile(
        logpath.c_str(),
        FILE_APPEND_DATA,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    // if it already existed then the error code will be ERROR_FILE_NOT_FOUND
    if (GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        g_hInputFile = CreateFile(
            logpath.c_str(),
            FILE_APPEND_DATA,
            FILE_SHARE_READ,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    }

    if ( g_hInputFile == INVALID_HANDLE_VALUE )
    {
        std::string err_msg("Could not create the TileMill log file at: '");
        err_msg += logpath;
        err_msg += "' (is another instance of TileMill already running?)";
        LPTSTR l_msg = (LPTSTR)(err_msg.c_str());
        ErrorExit(l_msg);
    }

    writeToLog("Starting TileMill...\n");
    ReadFromPipe();
    return 0;
}
