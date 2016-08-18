@echo off
whoami /groups /fo csv | find """S-1-16-12288""" > NUL || echo Administrative privileges required. && exit /b 2
copy Bin\x64\Release\HashCheck.dll   C:\Windows\System32\ShellExt\HashCheck.dll || exit /b
copy Bin\Win32\Release\HashCheck.dll C:\Windows\SysWOW64\ShellExt\HashCheck.dll
