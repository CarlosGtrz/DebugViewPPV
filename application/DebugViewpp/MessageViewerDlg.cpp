// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "MessageViewerDlg.h"
#include "LogView.h"

// Static variables to track window positions for horizontal tiling
static int s_currentX = 10;  // Current X position
static int s_currentY = 10;  // Current Y position
static const int WINDOW_WIDTH = 450;
static const int WINDOW_HEIGHT = 200;
static const int MARGIN = 10;  // Margin between windows and screen edges

namespace fusion {
namespace debugviewpp {

CMessageViewerDlg::CMessageViewerDlg(const std::wstring& message,
                                     const std::wstring& line,
                                     const std::wstring& time,
                                     const std::wstring& pid,
                                     const std::wstring& process,
                                     CLogView* pLogView) :
    m_message(message),
    m_line(line),
    m_time(time),
    m_pid(pid),
    m_process(process),
    m_pLogView(pLogView)
{
}

BOOL CMessageViewerDlg::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    // Initialize the dialog resizing
    DlgResize_Init();
    
    // Create background brush with explicit white color to match main window
    m_backgroundBrush.CreateSolidBrush(RGB(255, 255, 255));
    
    // Get the edit control and set it up
    m_editMessage = GetDlgItem(IDC_MESSAGE_TEXT);
    m_editMessage.SetReadOnly(TRUE);
    
    // Set ONLY the message text in the edit control
    m_editMessage.SetWindowText(m_message.c_str());
    
    // Build title with all metadata: Line, Time, PID, Process
    std::wstring title = L"Line " + m_line + L" - " + m_time + L" [" + m_pid + L"] " + m_process;
    SetWindowText(title.c_str());
    
    // Get screen dimensions
    RECT desktopRect;
    ::GetWindowRect(::GetDesktopWindow(), &desktopRect);
    int screenWidth = desktopRect.right - desktopRect.left;
    int screenHeight = desktopRect.bottom - desktopRect.top;

    // Check if window would fit on current row
    if (s_currentX + WINDOW_WIDTH > screenWidth - MARGIN)
    {
        // Move to next row
        s_currentX = MARGIN;
        s_currentY += WINDOW_HEIGHT + MARGIN;
        
        // Check if we've reached bottom of screen
        if (s_currentY + WINDOW_HEIGHT > screenHeight - MARGIN)
        {
            // Reset to top-left corner
            s_currentX = MARGIN;
            s_currentY = MARGIN;
        }
    }

    // Position the window at calculated location
    SetWindowPos(nullptr, s_currentX, s_currentY, WINDOW_WIDTH, WINDOW_HEIGHT, SWP_NOZORDER);

    // Update X position for next window (move right)
    s_currentX += WINDOW_WIDTH + MARGIN;
    
    return TRUE;
}

HBRUSH CMessageViewerDlg::OnCtlColorStatic(CDCHandle dc, CStatic /*wndStatic*/)
{
    // Set explicit colors to match main window appearance
    dc.SetTextColor(RGB(0, 0, 0));        // Pure black text
    dc.SetBkColor(RGB(255, 255, 255));    // Pure white background
    return m_backgroundBrush;
}

void CMessageViewerDlg::OnGetMinMaxInfo(LPMINMAXINFO pMinMaxInfo)
{
    // Set minimum window size to 200x150
    pMinMaxInfo->ptMinTrackSize.x = 200;
    pMinMaxInfo->ptMinTrackSize.y = 150;
}

void CMessageViewerDlg::OnClose()
{
    // Notify the LogView that this dialog is closing
    if (m_pLogView)
    {
        m_pLogView->OnMessageViewerClosing(this);
        m_pLogView = nullptr; // Prevent double notification
    }
    
    // Destroy the window
    DestroyWindow();
}

void CMessageViewerDlg::OnContextMenu(HWND hWnd, CPoint pt)
{
    // Only show context menu for the edit control
    if (hWnd != m_editMessage.m_hWnd)
        return;
        
    // Create a simple context menu for copy
    CMenu menu;
    menu.CreatePopupMenu();
    
    // Check if there's a selection
    int startChar, endChar;
    m_editMessage.GetSel(startChar, endChar);
    bool hasSelection = (startChar != endChar);
    
    menu.AppendMenu(MF_STRING | (hasSelection ? MF_ENABLED : MF_GRAYED), ID_MESSAGE_EDIT_COPY, L"&Copy\tCtrl+C");
    menu.AppendMenu(MF_STRING, ID_MESSAGE_EDIT_SELECTALL, L"Select &All\tCtrl+A");
    
    // If pt is (-1, -1), get cursor position from keyboard
    if (pt.x == -1 && pt.y == -1)
    {
        RECT rc;
        m_editMessage.GetWindowRect(&rc);
        pt.x = rc.left + (rc.right - rc.left) / 2;
        pt.y = rc.top + (rc.bottom - rc.top) / 2;
    }
    
    menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_LEFTALIGN, pt.x, pt.y, m_hWnd);
}

void CMessageViewerDlg::OnEditCopy(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
    if (nID == ID_MESSAGE_EDIT_COPY)
    {
        m_editMessage.Copy();
    }
    else if (nID == ID_MESSAGE_EDIT_SELECTALL)
    {
        m_editMessage.SetSel(0, -1);
    }
}

void CMessageViewerDlg::OnCancel(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    OnClose();
}

BOOL CMessageViewerDlg::PreTranslateMessage(MSG* pMsg)
{
    // Handle keyboard shortcuts
    if (pMsg->message == WM_KEYDOWN)
    {
        if (GetKeyState(VK_CONTROL) & 0x8000)
        {
            switch (pMsg->wParam)
            {
            case 'C':
            case 'c':
                m_editMessage.Copy();
                return TRUE;
                
            case 'A':
            case 'a':
                m_editMessage.SetSel(0, -1);
                return TRUE;
            }
        }
        else if (pMsg->wParam == VK_ESCAPE)
        {
            OnClose();
            return TRUE;
        }
    }
    
    return FALSE;
}

void CMessageViewerDlg::SetMessageFont(HFONT hFont)
{
    if (m_editMessage.m_hWnd && hFont)
        m_editMessage.SetFont(hFont);
}

} // namespace debugviewpp
} // namespace fusion