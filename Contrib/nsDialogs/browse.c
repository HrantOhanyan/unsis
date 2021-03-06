// Unicode support by Jim Park -- 08/10/2007

#include <windows.h>
#include <shlobj.h>

#include <nsis/pluginapi.h> // nsis plugin

#include "defs.h"

int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData) {
  if (uMsg == BFFM_INITIALIZED)
    SendMessage(hwnd, BFFM_SETSELECTION, TRUE, pData);

  return 0;
}

void __declspec(dllexport) SelectFolderDialog(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
  BROWSEINFO bi;

  TCHAR result[MAX_PATH];
  TCHAR initial[MAX_PATH];
  TCHAR title[1024];
  LPITEMIDLIST resultPIDL;

  EXDLL_INIT();

  if (popstringn(title, sizeof(title)/sizeof(title[0])))
  {
    pushstring(_T("error"));
    return;
  }

  if (popstringn(initial, sizeof(initial)/sizeof(initial[0])))
  {
    pushstring(_T("error"));
    return;
  }

  bi.hwndOwner = hwndParent;
  bi.pidlRoot = NULL;
  bi.pszDisplayName = result;
  bi.lpszTitle = title;
#ifndef BIF_NEWDIALOGSTYLE
#define BIF_NEWDIALOGSTYLE 0x0040
#endif
  bi.ulFlags = BIF_STATUSTEXT | BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
  bi.lpfn = BrowseCallbackProc;
  bi.lParam = (LPARAM) initial;
  bi.iImage = 0;

  /*if (pField->pszRoot) {
    LPSHELLFOLDER sf;
    ULONG eaten;
    LPITEMIDLIST root;
    int ccRoot = (lstrlen(pField->pszRoot) * 2) + 2;
    LPWSTR pwszRoot = (LPWSTR) MALLOC(ccRoot);
    MultiByteToWideChar(CP_ACP, 0, pField->pszRoot, -1, pwszRoot, ccRoot);
    SHGetDesktopFolder(&sf);
    sf->ParseDisplayName(hConfigWindow, NULL, pwszRoot, &eaten, &root, NULL);
    bi.pidlRoot = root;
    sf->Release();
    FREE(pwszRoot);
  }*/

  resultPIDL = SHBrowseForFolder(&bi);
  if (!resultPIDL)
  {
    pushstring(_T("error"));
    return;
  }

  if (SHGetPathFromIDList(resultPIDL, result))
  {
    pushstring(result);
  }
  else
  {
    pushstring(_T("error"));
  }

  CoTaskMemFree(resultPIDL);
}

void __declspec(dllexport) SelectFileDialog(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
  OPENFILENAME ofn={0,}; // XXX WTF
  int save;
  TCHAR type[5];
  const int len = 1024;
  // Avoid _chkstk by using allocated arrays.
  TCHAR* path = (TCHAR*) GlobalAlloc(GPTR, len*sizeof(TCHAR));
  TCHAR* filter =(TCHAR*) GlobalAlloc(GPTR, len*sizeof(TCHAR)); 
  TCHAR* currentDirectory = (TCHAR*) GlobalAlloc(GPTR, len*sizeof(TCHAR));
  TCHAR* initialDir = (TCHAR*) GlobalAlloc(GPTR, len*sizeof(TCHAR));
  DWORD  gfa;

  EXDLL_INIT();

  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hwndParent;
  ofn.lpstrFilter = filter;
  ofn.lpstrFile = path;
  ofn.nMaxFile  = len;
  //ofn.Flags = pField->nFlags & (OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_CREATEPROMPT | OFN_EXPLORER);
  ofn.Flags = OFN_CREATEPROMPT | OFN_EXPLORER;

  popstringn(type, len);
  popstringn(path, len);
  popstringn(filter, len);

  save = !lstrcmpi(type, _T("save"));

  // Check if the path given is a folder. If it is we initialize the 
  // ofn.lpstrInitialDir parameter
  gfa = GetFileAttributes(path);
  if ((gfa != INVALID_FILE_ATTRIBUTES) && (gfa & FILE_ATTRIBUTE_DIRECTORY))
  {
    lstrcpy(initialDir, path);
    ofn.lpstrInitialDir = initialDir;
    path[0] = _T('\0'); // disable initial file selection as path is actually a directory
  }

  if (!filter[0])
  {
    lstrcpy(filter, _T("All Files|*.*"));
  }

  {
    // Convert the filter to the format required by Windows: NULL after each
    // item followed by a terminating NULL
    TCHAR *p = filter;
    while (*p) // XXX take care for 1024
    {
      if (*p == _T('|'))
      {
        *p++ = 0;
      }
      else
      {
        p = CharNext(p);
      }
    }
    p++;
    *p = 0;
  }

  GetCurrentDirectory(sizeof(currentDirectory), currentDirectory); // save working dir

  if ((save ? GetSaveFileName(&ofn) : GetOpenFileName(&ofn)))
  {
    pushstring(path);
  }
  else if (CommDlgExtendedError() == FNERR_INVALIDFILENAME)
  {
    *path = _T('\0');
    if ((save ? GetSaveFileName(&ofn) : GetOpenFileName(&ofn)))
    {
      pushstring(path);
    }
    else
    {
      pushstring(_T(""));
    }
  }
  else
  {
    pushstring(_T(""));
  }

  // restore working dir
  // OFN_NOCHANGEDIR doesn't always work (see MSDN)
  SetCurrentDirectory(currentDirectory);

  GlobalFree(path);
  GlobalFree(filter);
  GlobalFree(currentDirectory);
  GlobalFree(initialDir);
}
