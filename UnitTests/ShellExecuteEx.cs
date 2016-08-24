using System;
using System.Runtime.InteropServices;

namespace UnitTests
{
    class ShellExecuteEx
    {
        // struct definition required to call ShellExecuteExW
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        public struct SHELLEXECUTEINFO
        {
            public uint cbSize;
            public uint fMask;
            public IntPtr hwnd;
            [MarshalAs(UnmanagedType.LPTStr)]
            public string lpVerb;
            [MarshalAs(UnmanagedType.LPTStr)]
            public string lpFile;
            [MarshalAs(UnmanagedType.LPTStr)]
            public string lpParameters;
            [MarshalAs(UnmanagedType.LPTStr)]
            public string lpDirectory;
            public int nShow;
            public IntPtr hInstApp;
            public IntPtr lpIDList;
            [MarshalAs(UnmanagedType.LPTStr)]
            public string lpClass;
            public IntPtr hkeyClass;
            public uint dwHotKey;
            public IntPtr hIconOrMonitor;
            public IntPtr hProcess;
        }

        [DllImport("shell32.dll")]
        public static extern bool ShellExecuteExW(ref SHELLEXECUTEINFO lpExecInfo);
    }
}
