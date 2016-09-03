#define IDC_STATIC                      -1

#define IDR_RT_MANIFEST                 ISOLATIONAWARE_MANIFEST_RESOURCE_ID

#define IDI_FILETYPE                    100
#define IDI_MENUBITMAP                  101

// Controls common to multiple dialogs
#define IDC_PROG_TOTAL                  102
#define IDC_PROG_FILE                   103
#define IDC_PAUSE                       104
#define IDC_STOP                        105
#define IDC_OK                          IDOK
#define IDC_CANCEL                      IDCANCEL

// Hash creation/save (context menu) dialog
#define IDD_HASHSAVE                    200

// Save separate checksum files dialog
#define IDD_HASHSAVE_SEP                600
#define IDC_SEP_CHK                     601
#define IDC_SEP_CHK_CRC32               602
#define IDC_SEP_CHK_MD5                 603
#define IDC_SEP_CHK_SHA1                604
#define IDC_SEP_CHK_SHA256              605
#define IDC_SEP_CHK_SHA512              606
#define IDC_SEP_CHK_SHA3_256            607
#define IDC_SEP_CHK_SHA3_512            608
#define IDC_SEP_CHK_FIRSTID             IDC_SEP_CHK_CRC32

// Hash calculation property sheet and controls
// (some of these must be copied to HashProp.cs)
#define IDD_HASHPROP                    300
#define IDC_RESULTS                     301
#define IDC_STATUSBOX                   302
#define IDC_SEARCHBOX                   303
#define IDC_FIND_NEXT                   304
#define IDC_SAVE                        305
#define IDC_OPTIONS                     306

// Hash verification dialog and controls
// (some of these must be copied to HashVerify.cs)
#define IDD_HASHVERF                    400
#define IDC_LIST                        401
#define IDC_SUMMARY                     402
#define IDC_MATCH_LABEL                 403
#define IDC_MATCH_RESULTS               404
#define IDC_MISMATCH_LABEL              405
#define IDC_MISMATCH_RESULTS            406
#define IDC_UNREADABLE_LABEL            407
#define IDC_UNREADABLE_RESULTS          408
#define IDC_PENDING_LABEL               409
#define IDC_PENDING_RESULTS             410
#define IDC_EXIT                        IDCANCEL

// Options dialog
#define IDD_OPTIONS                     500
#define IDC_OPT_CM                      501
#define IDC_OPT_CM_ALWAYS               502
#define IDC_OPT_CM_EXTENDED             503
#define IDC_OPT_CM_NEVER                504
#define IDC_OPT_CM_FIRSTID              IDC_OPT_CM_ALWAYS
#define IDC_OPT_ENCODING                505
#define IDC_OPT_ENCODING_UTF8           506
#define IDC_OPT_ENCODING_UTF16          507
#define IDC_OPT_ENCODING_ANSI           508
#define IDC_OPT_ENCODING_FIRSTID        IDC_OPT_ENCODING_UTF8
#define IDC_OPT_CHK                     509
#define IDC_OPT_CHK_CRC32               510
#define IDC_OPT_CHK_MD5                 511
#define IDC_OPT_CHK_SHA1                512
#define IDC_OPT_CHK_SHA256              513
#define IDC_OPT_CHK_SHA512              514
#define IDC_OPT_CHK_SHA3_256            515
#define IDC_OPT_CHK_SHA3_512            516
#define IDC_OPT_FONT                    517
#define IDC_OPT_FONT_CHANGE             518
#define IDC_OPT_FONT_PREVIEW            519
#define IDC_OPT_LINK                    520
