/**
* HashProp unit tests
* Copyright (C) 2016 Christopher Gurnee.  All rights reserved.
*
* Please refer to readme.md for information about this source code.
* Please refer to license.txt for details about distribution and modification.
**/

using Xunit;
using TestStack.White;
using TestStack.White.Configuration;
using TestStack.White.UIItems;
using TestStack.White.UIItems.WindowItems;
using TestStack.White.UIItems.Finders;
using System;
using System.IO;
using System.Diagnostics;
using System.Windows.Automation;
using Microsoft.Win32;
using static UnitTests.ShellExecuteEx;

namespace UnitTests
{
    public class HashProp
    {
        // from HashCheckResources.h
        const string IDC_RESULTS   = "301";
        const string IDC_STATUSBOX = "302";

        // Relative path from the class .dll to the test vectors
        const string RELATIVE_PATH_PREFIX = @"..\..\vectors\";


        static string PATH_PREFIX;
        static bool is_com_surrogate;
        //
        Window OpenHashPropWindow(string filename)
        {
            // Combine the path to the class .dll with the relative path to the test vectors
            if (PATH_PREFIX == null)
                PATH_PREFIX = Path.Combine(
                    Path.GetDirectoryName(
                        Uri.UnescapeDataString(
                            new Uri(System.Reflection.Assembly.GetExecutingAssembly().CodeBase).AbsolutePath
                        )
                    ),
                    RELATIVE_PATH_PREFIX
                );

            // Open the Checksums tab in the shell file properties sheet
            SHELLEXECUTEINFO sei = new SHELLEXECUTEINFO();
            sei.cbSize = (uint)System.Runtime.InteropServices.Marshal.SizeOf(sei);
            sei.fMask = 0x0000010C;  // SEE_MASK_INVOKEIDLIST | SEE_MASK_NOASYNC
            sei.lpVerb = "Properties";
            sei.lpFile = Path.Combine(PATH_PREFIX, filename);
            sei.lpParameters = "Checksums";
            sei.nShow = 1;           // SW_SHOWNORMAL
            ShellExecuteExW(ref sei);

            // Find the file properties sheet window
            Window prop_window = null;

            // Prior to Windows 10, the file properties sheet opens in the curent process
            int orig_timeout = CoreAppXmlConfiguration.Instance.FindWindowTimeout;
            if ((int)Registry.GetValue(@"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion", "CurrentMajorVersionNumber", 0) >= 10)
                CoreAppXmlConfiguration.Instance.FindWindowTimeout = 1500;  // likely to fail on Windows 10+, so use a short timeout (1.5s) in that case
            if (! is_com_surrogate)
            {
                Application app = Application.Attach(Process.GetCurrentProcess());
                try
                {
                    prop_window = app.GetWindow(filename + " Properties");
                    is_com_surrogate = false;
                }
                catch (AutomationException) { }
            }

            // Starting with Windows 10, the file properties sheet is hosted in a dllhost COM surrogate
            if (prop_window == null)
            {
                foreach (Process proc in Process.GetProcessesByName("dllhost"))
                {
                    Application app = Application.Attach(proc);
                    try
                    {
                        prop_window = app.GetWindow(filename + " Properties");
                        break;
                    }
                    catch (AutomationException) { }
                }
                Assert.NotNull(prop_window);
                is_com_surrogate = true;
            }
            CoreAppXmlConfiguration.Instance.FindWindowTimeout = orig_timeout;

            return prop_window;
        }


        void VerifyResults(Window prop_window, string expected_results)
        {
            // Wait for hashing to complete (status contains a summary instead of "Progress")
            GroupBox status = prop_window.Get<GroupBox>(SearchCriteria.ByNativeProperty(
                AutomationElement.AutomationIdProperty, IDC_STATUSBOX));
            while (status.Name == "Progress")
                System.Threading.Thread.Sleep(100);

            // Verify the results
            TextBox results = prop_window.Get<TextBox>(SearchCriteria.ByNativeProperty(
                AutomationElement.AutomationIdProperty, IDC_RESULTS));
            Assert.Equal(expected_results, results.BulkText, true, true, true);  // ignores case, line endings and whitespace changes
        }


        [Theory]
        [InlineData(
         "SHA256LongMsg.rsp-0000.dat", false,
@"  File: SHA256LongMsg.rsp-0000.dat
  CRC-32: 29154b1f
   SHA-1: a0f8b5c82a0ee5a9119251cd75c55c0ae350923d
 SHA-256: 3c593aa539fdcdae516cdf2f15000f6634185c88f505b39775fb9ab137a10aa2
 SHA-512: ae4b5efe9d7e301699dc03ec1bc1f60f83565be9a40d8e38c660a7f9463b22b1ed907f9f4024c9d0ba30c9c38ac12d8cc354d8c4539e63dd6f26a0ce9ce7b71a
" + "\n",
@"  File: SHA256LongMsg.rsp-0000.dat
  CRC-32: 29154b1f
     MD5: 632250fa8b3f9d248e0d37044460a4b7
   SHA-1: a0f8b5c82a0ee5a9119251cd75c55c0ae350923d
 SHA-256: 3c593aa539fdcdae516cdf2f15000f6634185c88f505b39775fb9ab137a10aa2
 SHA-512: ae4b5efe9d7e301699dc03ec1bc1f60f83565be9a40d8e38c660a7f9463b22b1ed907f9f4024c9d0ba30c9c38ac12d8cc354d8c4539e63dd6f26a0ce9ce7b71a
SHA3-256: aae3e14fd718aed758574f170c23a2f172a3690587340aa3c5c8d538ebd66bd4
SHA3-512: 2620b58e343e8a80e091405b65ced686c58c3996854e092d01202dc91598d596aea148131c77513a4ae7f509800abcdb363c45864ef2e291206cd56775ff3e46
" + "\n")]
        [InlineData(
         "SHA3_VeryLongMsg.dat", true,
@"  File: SHA3_VeryLongMsg.dat
  CRC-32: 4ed80f64
   SHA-1: 6ca7cca8ac206ec5a82a612ee5d4ba6ac766d239
 SHA-256: 8716fbf9a5f8c4562b48528e2d3085b64c56b5d1169ccf3295ad03e805580676
 SHA-512: 421b072b4fda96eb569ae55b8a9a5b4b5073a623649bd409dbb999e527372994b3a1a91f53c719837868c7fe11bba67640143255a3fbc5c895d2119274b0caff
" + "\n",
@"  File: SHA3_VeryLongMsg.dat
  CRC-32: 4ed80f64
     MD5: 1553faf40e3618f9b902c1e559552828
   SHA-1: 6ca7cca8ac206ec5a82a612ee5d4ba6ac766d239
 SHA-256: 8716fbf9a5f8c4562b48528e2d3085b64c56b5d1169ccf3295ad03e805580676
 SHA-512: 421b072b4fda96eb569ae55b8a9a5b4b5073a623649bd409dbb999e527372994b3a1a91f53c719837868c7fe11bba67640143255a3fbc5c895d2119274b0caff
SHA3-256: 6a934f386ff779e33a1068c5f3e4c5c0a117968be4264b8f80ec511a1c0b6eed
SHA3-512: d11fbca35e2383481bd99253a289c035e7a98a36507e4feabf8151fc51e6d77c2f737c4bd747362896ab61df2c066e6a27e7fa2f5bf645b54d98e07e135c4870
" + "\n")]
        // BUG: only works for the English translation
        public void HashesTest(string name, bool interrupt, string def_expected_results, string full_expected_results)
        {
            // Reset the selected checksums back to the default
            RegistryKey reg_key = Registry.CurrentUser.OpenSubKey("SOFTWARE");
            reg_key = reg_key.OpenSubKey("HashCheck", true);  // (true == with-write-access)
            if (reg_key != null)
                reg_key.DeleteValue("Checksums", false);  // (false == no-exceptions-if-not-found)

            Window prop_window = OpenHashPropWindow(name);
            try
            {
                VerifyResults(prop_window, def_expected_results);

                // If we should try to interrupt hash calculation in-progress, restart it
                // (VerifyResults() above waits for completion, the following does not)
                if (interrupt)
                {
                    prop_window.Close();
                    prop_window = null;
                    prop_window = OpenHashPropWindow(name);
                }

                // Open the options dialog
                prop_window.Get<Button>("Options").Click();
                Window options_window = null;
                while (true)
                {
                    foreach (Window mw in prop_window.ModalWindows())
                    {
                        if (mw.Name.StartsWith("HashCheck Options"))
                        {
                            options_window = mw;
                            goto options_window_found;
                        }
                    }
                    System.Threading.Thread.Sleep(0);
                }
                options_window_found:

                // Enable all the hash option checkboxes and click OK
                foreach (CheckBox cb in options_window.GetMultiple(SearchCriteria.ByControlType(ControlType.CheckBox)))
                    cb.State = ToggleState.On;
                options_window.Get<Button>("OK").Click();

                VerifyResults(prop_window, full_expected_results);
            }
            finally
            {
                if (prop_window != null)
                   prop_window.Close();
            }
        }
    }
}
