# PunchDataExchangeHost

Make DataExchangeHost working when UAC disabled!

Currently fixes WindowsTerminal's Drag&Drop

## Usage

1. Download or Compile to get PunchDataExchangeHost.dll

2. Download `IFEODllInjector` tool (download x64 one): https://github.com/NyaMisty/IFEODllInjector

3. Go to `HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\`, Create a new subkey called `DataExchangeHost.exe`

4. Under `HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\DataExchangeHost.exe`, Create a reg string value (REG_SZ) called `Debugger`

5. Fill the `Debugger` key with something like this:
    ```
    C:\path\to\injector\exe\IFEODllInjector-x64.exe C:\path\to\dll\PunchDataExchangeHost.dll
    ```

6. Profit :)


## What Happened?

TL;DR: `DataExchangeHost.exe` forgets to consider the UAC-disabled case.

When UAC was disabled by `EnableLUA=0`, UIPI will be totally turned of in win32k driver (`fEnableUIPI` will become `false`), which will allow everyone drags anything to everywhere.

However `DataExchangeHost.exe` hardcoded a check in `CDragDropBroker::GetOLETarget`

```
__int64 __fastcall CDragDropBroker::GetOLETarget(
        CDragDropBroker *this,
        HWND a2,
        void **a3,
        struct IDragUIContentProviderPriv *a4,
        const struct _GUID *a5,
        void **a6)
{
  // [COLLAPSED LOCAL DECLARATIONS. PRESS KEYPAD CTRL-"+" TO EXPAND]

  v19[0] = 0i64;
  CallingProcessHandle = CallerIdentity::GetCallingProcessHandle(this, v19, a3);
  OLETarget = CallingProcessHandle;
  if ( CallingProcessHandle >= 0 )
  {
    ProcessIL = GetProcessIL(v19[0], (DWORD *)&ilLevel);
    OLETarget = ProcessIL;
    if ( ProcessIL >= 0 )
    {
      if ( (unsigned int)(ilLevel - 0x2000) <= 0xFFF )
      {
        v15 = v19[0];
        v19[0] = 0i64;
        OLETarget = OLEBroker::GetOLETarget(
                      (CDragDropBroker *)((char *)this + 16),
                      v15,
                      a2,
                      (const unsigned __int16 *)a3,
                      a4,
                      a5,
                      a6);
        goto LABEL_10;
      }
      OLETarget = E_ACCESSDENIED;
    }
    ...
```

As shown in the code, if `ilLevel` was over 0x3000 (i.e. SECURITY_MANDATORY_HIGH_RID, corresponding to High IL), `CDragDropBroker::GetOLETarget` will directly fails with no other checks.

When UAC disabled, all normal process are running on High IL, so this code will always fail, causing all WinUI3 app fails to handle drag request, unless they manually add a overlay and handle WM_DROPFILES (like NewNotepad does).
