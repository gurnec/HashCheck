/**
* HashVerify unit tests
* Copyright (C) 2016 Christopher Gurnee.  All rights reserved.
*
* Please refer to readme.md for information about this source code.
* Please refer to license.txt for details about distribution and modification.
**/

using Xunit;
using TestStack.White;
using TestStack.White.UIItems;
using TestStack.White.UIItems.Finders;
using System;
using System.IO;
using System.Diagnostics;
using System.Windows.Automation;

namespace UnitTests
{
    public class HashVerifySetup
    {
        // Relative path from the class .dll to the test vectors
        const string RELATIVE_PATH_PREFIX = @"..\..\vectors\";
        internal string PATH_PREFIX;

        internal Application app;

        public HashVerifySetup()
        {
            // Combine the path to the class .dll with the relative path to the test vectors
            PATH_PREFIX = Path.Combine(
                Path.GetDirectoryName(
                    Uri.UnescapeDataString(
                        new Uri(System.Reflection.Assembly.GetExecutingAssembly().CodeBase).AbsolutePath
                    )
                ),
                RELATIVE_PATH_PREFIX
            );

            Process proc = Process.GetCurrentProcess();
            app = Application.Attach(proc);

            // Workaround required when debugging unit tests in the VS GUI -
            // for some reason the first time this is called the DLL loads but HashVerify doesn't start
            if (Debugger.IsAttached && proc.MainModule.ModuleName.StartsWith("vstest.executionengine."))
                Process.Start(Path.Combine(PATH_PREFIX, "SHA3_256ShortMsg.rsp.sha3-256"));
        }
    }

    public class HashVerify : IClassFixture<HashVerifySetup>
    {
        // from HashCheckResources.h
        const string IDC_MATCH_RESULTS   = "404";
        const string IDC_PENDING_RESULTS = "410";

        HashVerifySetup common;

        public HashVerify(HashVerifySetup c)
        {
            common = c;
        }

        [Theory]
        [InlineData("SHA1ShortMsg.rsp.sha1")]
        [InlineData("SHA1LongMsg.rsp.sha1")]
        [InlineData("SHA256ShortMsg.rsp.sha256")]
        [InlineData("SHA256LongMsg.rsp.sha256")]
        [InlineData("SHA512ShortMsg.rsp.sha512")]
        [InlineData("SHA512LongMsg.rsp.sha512")]
        [InlineData("SHA3_256ShortMsg.rsp.sha3-256")]
        [InlineData("SHA3_256LongMsg.rsp.sha3-256")]
        [InlineData("SHA3_512ShortMsg.rsp.sha3-512")]
        [InlineData("SHA3_512LongMsg.rsp.sha3-512")]
        [InlineData("hashcheck.md5")]
        [InlineData("hashcheck.sha1")]
        public void NistTest(string name)
        {
            // This opens a HashCheck Verify window in the current process
            Process.Start(Path.Combine(common.PATH_PREFIX, name));

            var verify_window = common.app.GetWindow(name);
            try
            {
                // Wait for hashing to complete (there are 0 pending results)
                var re0 = new System.Text.RegularExpressions.Regex(@"\b0\b");
                Label pending_results = verify_window.Get<Label>(SearchCriteria.ByNativeProperty(
                    AutomationElement.AutomationIdProperty, IDC_PENDING_RESULTS));
                while (! re0.IsMatch(pending_results.Text))
                    System.Threading.Thread.Sleep(0);

                // On success, the hash matches should be "# of #" where the #'s are the same
                Label match_results = verify_window.Get<Label>(SearchCriteria.ByNativeProperty(
                    AutomationElement.AutomationIdProperty, IDC_MATCH_RESULTS));
                Assert.Matches(@"\b(\d{2,3})\b.*\b\1\b", match_results.Text);
            }
            finally
            {
                verify_window.Close();
            }
        }
    }
}
