#include "Process.hpp"

#ifdef WIN32
#include <windows.h>
#include "Common/StringUtil.hpp"

// taken from https://learn.microsoft.com/en-us/archive/blogs/twistylittlepassagesallalike/everyone-quotes-command-line-arguments-the-wrong-way
void ArgvQuote(const std::wstring& Argument, std::wstring& CommandLine, bool Force)
/*++

Routine Description:

    This routine appends the given argument to a command line such
    that CommandLineToArgvW will return the argument string unchanged.
    Arguments in a command line should be separated by spaces; this
    function does not add these spaces.

Arguments:

    Argument - Supplies the argument to encode.

    CommandLine - Supplies the command line to which we append the encoded argument string.

    Force - Supplies an indication of whether we should quote
            the argument even if it does not contain any characters that would
            ordinarily require quoting.

Return Value:

    None.

Environment:

    Arbitrary.

--*/
{
    //
    // Unless we're told otherwise, don't quote unless we actually
    // need to do so --- hopefully avoid problems if programs won't
    // parse quotes properly
    //

    if (Force == false &&
        Argument.empty () == false &&
        Argument.find_first_of (L" \t\n\v\"") == Argument.npos)
    {
        CommandLine.append (Argument);
    }
    else {
        CommandLine.push_back (L'"');

        for (auto It = Argument.begin () ; ; ++It) {
            unsigned NumberBackslashes = 0;

            while (It != Argument.end () && *It == L'\\') {
                ++It;
                ++NumberBackslashes;
            }

            if (It == Argument.end ()) {

                //
                // Escape all backslashes, but let the terminating
                // double quotation mark we add below be interpreted
                // as a metacharacter.
                //

                CommandLine.append (NumberBackslashes * 2, L'\\');
                break;
            }
            else if (*It == L'"') {

                //
                // Escape all backslashes and the following
                // double quotation mark.
                //

                CommandLine.append (NumberBackslashes * 2 + 1, L'\\');
                CommandLine.push_back (*It);
            }
            else {

                //
                // Backslashes aren't special here.
                //

                CommandLine.append (NumberBackslashes, L'\\');
                CommandLine.push_back (*It);
            }
        }

        CommandLine.push_back (L'"');
    }
}

bool runProcess(const std::vector<std::string>& args, std::string& output, int32_t& exitCode)
{
  std::wstring commandLine;
  for (size_t i = 0; i < args.size(); i++)
  {
    ArgvQuote(Str::utf8ToWstring(args[i]), commandLine, false);
    if (i != args.size() - 1)
      commandLine += L' ';
  }

  HANDLE readPipe = nullptr;
  HANDLE writePipe = nullptr;
  PROCESS_INFORMATION pi = {};
  STARTUPINFOW si = {};
  SECURITY_ATTRIBUTES saAttr = {};
  bool retval = false;

  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle = TRUE;
  saAttr.lpSecurityDescriptor = nullptr;

  if (!CreatePipe(&readPipe, &writePipe, &saAttr, 0))
  {
    readPipe = nullptr;
    writePipe = nullptr;
    goto done;
  }

  if (!SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0))
    goto done;

  si.cb = sizeof(si);
  si.hStdError = writePipe;
  si.hStdOutput = writePipe;
  si.dwFlags |= STARTF_USESTDHANDLES;

  if (!CreateProcessW(nullptr, // No module name (use command line)
                      commandLine.data(), // Command line
                      nullptr, // Process handle not inheritable
                      nullptr, // Thread handle not inheritable
                      TRUE, // Set handle inheritance to TRUE
                      CREATE_NO_WINDOW,
                      nullptr, // Use parent's environment block
                      nullptr, // Use parent's starting directory
                      &si, // Pointer to STARTUPINFO structure
                      &pi) // Pointer to PROCESS_INFORMATION structure
      )
  {
    pi.hProcess = nullptr;
    goto done;
  }

  CloseHandle(pi.hThread);
  pi.hThread = nullptr;

  CloseHandle(writePipe);
  writePipe = nullptr;

  while (true)
  {
    char buffer[1024];
    DWORD bytesRead = 0;
    BOOL success = ReadFile(readPipe, buffer, 1024, &bytesRead, nullptr);

    if (!success || bytesRead == 0)
      break;

    output.append(buffer, bytesRead);
  }

  if (!GetExitCodeProcess(pi.hProcess, (LPDWORD)&exitCode))
    goto done;

  retval = true;

done:
  if (pi.hProcess)
    CloseHandle(pi.hProcess);
  if (readPipe)
    CloseHandle(readPipe);
  if (writePipe)
    CloseHandle(writePipe);

  return retval;
}
#else
#error "implement me!"
#endif
