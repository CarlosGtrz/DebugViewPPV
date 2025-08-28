// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "CobaltFusion/AtlWinExt.h"
#include "resource.h"
#include "atleverything.h"
#include "DebugViewppLib/Colors.h"
#include <string>

namespace fusion {
namespace debugviewpp {

class CLogView;

class CMessageViewerDlg : public CDialogImpl<CMessageViewerDlg>,
                          public CDialogResize<CMessageViewerDlg>
{
public:
    explicit CMessageViewerDlg(const std::wstring& message,
                              const std::wstring& line,
                              const std::wstring& time,
                              const std::wstring& pid, 
                              const std::wstring& process,
                              CLogView* pLogView);

    enum
    {
        IDD = IDD_MESSAGE_VIEWER
    };

    BEGIN_MSG_MAP(CMessageViewerDlg)
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_CLOSE(OnClose)
        MSG_WM_CONTEXTMENU(OnContextMenu)
        MSG_WM_CTLCOLORSTATIC(OnCtlColorStatic)
        MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo)
        COMMAND_ID_HANDLER_EX(ID_MESSAGE_EDIT_COPY, OnEditCopy)
        COMMAND_ID_HANDLER_EX(ID_MESSAGE_EDIT_SELECTALL, OnEditCopy)
        COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
        CHAIN_MSG_MAP(CDialogResize<CMessageViewerDlg>)
    END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(CMessageViewerDlg)
        DLGRESIZE_CONTROL(IDC_MESSAGE_TEXT, DLSZ_SIZE_X | DLSZ_SIZE_Y)
    END_DLGRESIZE_MAP()

    BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
    void OnClose();
    void OnContextMenu(HWND hWnd, CPoint pt);
    HBRUSH OnCtlColorStatic(CDCHandle dc, CStatic wndStatic);
    void OnGetMinMaxInfo(LPMINMAXINFO pMinMaxInfo);
    void OnEditCopy(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);

    BOOL PreTranslateMessage(MSG* pMsg);
    
    // Public method to reset LogView pointer (used during cleanup)
    void SetLogView(CLogView* pLogView) { m_pLogView = pLogView; }
    
    // Public method to set font to match main window
    void SetMessageFont(HFONT hFont);

private:
    std::wstring m_message;
    std::wstring m_line;
    std::wstring m_time;
    std::wstring m_pid;
    std::wstring m_process;
    CLogView* m_pLogView;
    CEdit m_editMessage;
    CBrush m_backgroundBrush;
};

} // namespace debugviewpp
} // namespace fusion