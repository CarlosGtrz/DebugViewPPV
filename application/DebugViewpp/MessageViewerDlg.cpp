// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "MessageViewerDlg.h"
#include "LogView.h"

// Static variables to track window positions for horizontal tiling
static int s_currentX = 10;  // Current X position
static int s_currentY = 10;  // Current Y position
static HMONITOR s_currentMonitor = NULL;  // Track current monitor
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
                                     CLogView* pLogView,
                                     HWND hParentWnd) :
    m_message(message),
    m_originalMessage(message),  // Store original for re-formatting
    m_line(line),
    m_time(time),
    m_pid(pid),
    m_process(process),
    m_pLogView(pLogView),
    m_hParentWnd(hParentWnd)
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
    
    // Get the button controls
    m_btnSplitComma = GetDlgItem(IDC_SPLIT_COMMA);
    m_btnSplitPipe = GetDlgItem(IDC_SPLIT_PIPE);
    m_btnFormatJSON = GetDlgItem(IDC_FORMAT_JSON);
    m_btnFormatXML = GetDlgItem(IDC_FORMAT_XML);
    
    // Set ONLY the message text in the edit control
    m_editMessage.SetWindowText(m_message.c_str());
    
    // Build title with all metadata: Line, Time, PID, Process
    std::wstring title = L"Line " + m_line + L" - " + m_time + L" [" + m_pid + L"] " + m_process;
    SetWindowText(title.c_str());
    
    // Get the monitor that contains the parent window
    HMONITOR hMonitor = MonitorFromWindow(m_hParentWnd, MONITOR_DEFAULTTONEAREST);
    
    // If we switched to a different monitor, reset positioning
    if (s_currentMonitor != hMonitor)
    {
        s_currentMonitor = hMonitor;
        s_currentX = MARGIN;
        s_currentY = MARGIN;
    }
    
    // Get monitor information
    MONITORINFO monitorInfo = {};
    monitorInfo.cbSize = sizeof(MONITORINFO);
    if (!GetMonitorInfo(hMonitor, &monitorInfo))
    {
        // Fallback to desktop if GetMonitorInfo fails
        ::GetWindowRect(::GetDesktopWindow(), &monitorInfo.rcWork);
    }
    
    // Use work area (excludes taskbar) for positioning
    int monitorLeft = monitorInfo.rcWork.left;
    int monitorTop = monitorInfo.rcWork.top;
    int monitorWidth = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
    int monitorHeight = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;
    
    // Adjust static positions to be relative to this monitor
    int absoluteX = monitorLeft + s_currentX;
    int absoluteY = monitorTop + s_currentY;

    // Check if window would fit on current row within this monitor
    if (s_currentX + WINDOW_WIDTH > monitorWidth - MARGIN)
    {
        // Move to next row
        s_currentX = MARGIN;
        s_currentY += WINDOW_HEIGHT + MARGIN;
        
        // Check if we've reached bottom of monitor
        if (s_currentY + WINDOW_HEIGHT > monitorHeight - MARGIN)
        {
            // Reset to top-left corner of monitor
            s_currentX = MARGIN;
            s_currentY = MARGIN;
        }
        
        // Recalculate absolute position
        absoluteX = monitorLeft + s_currentX;
        absoluteY = monitorTop + s_currentY;
    }

    // Position the window at calculated location
    SetWindowPos(nullptr, absoluteX, absoluteY, WINDOW_WIDTH, WINDOW_HEIGHT, SWP_NOZORDER);

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
    // Set minimum window size to 200x150 (with space for buttons)
    pMinMaxInfo->ptMinTrackSize.x = 200;
    pMinMaxInfo->ptMinTrackSize.y = 150;
}

void CMessageViewerDlg::OnSize(UINT nType, CSize size)
{
    // Let the base class handle basic resizing first
    SetMsgHandled(FALSE);
    
    // Ensure text control doesn't overlap buttons (leave 18 pixels for buttons at bottom)
    if (m_editMessage.m_hWnd)
    {
        RECT clientRect;
        GetClientRect(&clientRect);
        
        // Resize text control to leave space for buttons (12 pixels height + 3 pixel margins)
        int textHeight = clientRect.bottom - 18;
        if (textHeight > 0)
        {
            m_editMessage.SetWindowPos(nullptr, 0, 0, clientRect.right, textHeight, 
                SWP_NOZORDER | SWP_NOMOVE);
        }
    }
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

void CMessageViewerDlg::OnSplitComma(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    std::wstring formatted = FormatWithSplitChars(m_originalMessage, L',');
    m_editMessage.SetWindowText(formatted.c_str());
}

void CMessageViewerDlg::OnSplitPipe(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    std::wstring formatted = FormatWithSplitChars(m_originalMessage, L'|');
    m_editMessage.SetWindowText(formatted.c_str());
}

void CMessageViewerDlg::OnFormatJSON(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    std::wstring formatted = FormatJSON(m_originalMessage);
    m_editMessage.SetWindowText(formatted.c_str());
}

void CMessageViewerDlg::OnFormatXML(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    std::wstring formatted = FormatXML(m_originalMessage);
    m_editMessage.SetWindowText(formatted.c_str());
}

std::wstring CMessageViewerDlg::FormatWithSplitChars(const std::wstring& text, wchar_t splitChar)
{
    std::wstring result = text;
    std::wstring search(1, splitChar);
    std::wstring replace = search + L"\r\n";
    
    size_t pos = 0;
    while ((pos = result.find(search, pos)) != std::wstring::npos)
    {
        result.replace(pos, search.length(), replace);
        pos += replace.length();
    }
    
    return result;
}

std::wstring CMessageViewerDlg::FormatJSON(const std::wstring& text)
{
    std::wstring result;
    size_t jsonStart = text.find_first_of(L"{[");
    
    if (jsonStart == std::wstring::npos)
    {
        return text; // No JSON found
    }
    
    // Add text before JSON with line separator
    if (jsonStart > 0)
    {
        result = text.substr(0, jsonStart);
        if (!result.empty() && result.back() != L'\n')
            result += L"\r\n";
    }
    
    // Find matching closing bracket
    size_t jsonEnd = jsonStart;
    int braceCount = 0;
    int bracketCount = 0;
    bool inString = false;
    bool escaped = false;
    
    for (size_t i = jsonStart; i < text.length(); ++i)
    {
        wchar_t c = text[i];
        
        if (!inString)
        {
            if (c == L'{') braceCount++;
            else if (c == L'}') braceCount--;
            else if (c == L'[') bracketCount++;
            else if (c == L']') bracketCount--;
            else if (c == L'"') inString = true;
        }
        else
        {
            if (escaped)
                escaped = false;
            else if (c == L'\\')
                escaped = true;
            else if (c == L'"')
                inString = false;
        }
        
        // Stop at syntax error or when all brackets are closed
        if ((braceCount < 0 || bracketCount < 0) || (braceCount == 0 && bracketCount == 0 && i > jsonStart))
        {
            jsonEnd = i;
            break;
        }
    }
    
    // Extract and format JSON portion
    if (jsonEnd > jsonStart)
    {
        std::wstring jsonText = text.substr(jsonStart, jsonEnd - jsonStart + 1);
        std::wstring formattedJson;
        int indent = 0;
        inString = false;
        escaped = false;
        
        for (size_t i = 0; i < jsonText.length(); ++i)
        {
            wchar_t c = jsonText[i];
            
            if (!inString)
            {
                if (c == L'{' || c == L'[')
                {
                    formattedJson += c;
                    formattedJson += L"\r\n";
                    indent++;
                    for (int j = 0; j < indent; ++j)
                        formattedJson += L"  ";
                }
                else if (c == L'}' || c == L']')
                {
                    if (!formattedJson.empty() && formattedJson.back() == L' ')
                    {
                        // Remove trailing spaces
                        while (!formattedJson.empty() && (formattedJson.back() == L' ' || formattedJson.back() == L'\t'))
                            formattedJson.pop_back();
                    }
                    formattedJson += L"\r\n";
                    indent--;
                    for (int j = 0; j < indent; ++j)
                        formattedJson += L"  ";
                    formattedJson += c;
                }
                else if (c == L',')
                {
                    formattedJson += c;
                    formattedJson += L"\r\n";
                    for (int j = 0; j < indent; ++j)
                        formattedJson += L"  ";
                }
                else if (c == L':')
                {
                    formattedJson += c;
                    formattedJson += L" ";
                }
                else if (c == L'"')
                {
                    formattedJson += c;
                    inString = true;
                }
                else if (c != L' ' && c != L'\t' && c != L'\r' && c != L'\n')
                {
                    formattedJson += c;
                }
            }
            else
            {
                formattedJson += c;
                if (escaped)
                    escaped = false;
                else if (c == L'\\')
                    escaped = true;
                else if (c == L'"')
                    inString = false;
            }
        }
        
        result += formattedJson;
        
        // Add text after JSON with line separator
        if (jsonEnd + 1 < text.length())
        {
            result += L"\r\n";
            result += text.substr(jsonEnd + 1);
        }
    }
    else
    {
        result += text.substr(jsonStart); // Include rest as-is if formatting failed
    }
    
    return result;
}

std::wstring CMessageViewerDlg::FormatXML(const std::wstring& text)
{
    std::wstring result;
    size_t xmlStart = text.find(L'<');
    
    if (xmlStart == std::wstring::npos)
    {
        return text; // No XML found
    }
    
    // Add text before XML with line separator
    if (xmlStart > 0)
    {
        result = text.substr(0, xmlStart);
        if (!result.empty() && result.back() != L'\n')
            result += L"\r\n";
    }
    
    // Find end of XML content
    size_t xmlEnd = text.find_last_of(L'>');
    if (xmlEnd == std::wstring::npos || xmlEnd <= xmlStart)
    {
        return text; // Invalid XML structure
    }
    
    std::wstring xmlText = text.substr(xmlStart, xmlEnd - xmlStart + 1);
    std::wstring formattedXml;
    int indent = 0;
    
    size_t pos = 0;
    while (pos < xmlText.length())
    {
        if (xmlText[pos] == L'<')
        {
            // Find end of tag
            size_t tagEnd = xmlText.find(L'>', pos);
            if (tagEnd == std::wstring::npos)
                break;
                
            std::wstring tag = xmlText.substr(pos, tagEnd - pos + 1);
            
            // Check tag type
            bool isClosingTag = (tag.length() > 1 && tag[1] == L'/');
            bool isProcessingInstruction = (tag.length() > 1 && tag[1] == L'?');
            bool isSelfClosing = (tag.length() > 2 && tag[tag.length() - 2] == L'/');
            bool isComment = (tag.length() > 3 && tag.substr(1, 3) == L"!--");
            
            // Add indentation for non-processing instructions
            if (!isProcessingInstruction)
            {
                if (isClosingTag)
                    indent--;
                    
                if (!formattedXml.empty())
                    formattedXml += L"\r\n";
                    
                for (int i = 0; i < indent; ++i)
                    formattedXml += L"  ";
            }
            else if (!formattedXml.empty())
            {
                formattedXml += L"\r\n";
            }
            
            // Format the tag with attributes
            if (!isProcessingInstruction && !isComment && !isClosingTag)
            {
                // Parse tag for attributes
                size_t spacePos = tag.find(L' ');
                if (spacePos != std::wstring::npos && spacePos < tag.length() - 1)
                {
                    // Has attributes - format them
                    std::wstring tagName = tag.substr(0, spacePos);
                    std::wstring attributes = tag.substr(spacePos + 1, tag.length() - spacePos - 2); // Remove > at end
                    if (isSelfClosing && attributes.length() > 1)
                        attributes = attributes.substr(0, attributes.length() - 1); // Remove / before >
                    
                    formattedXml += tagName;
                    
                    // Calculate alignment for attributes (tag name + 1 space)
                    std::wstring alignmentSpaces;
                    for (int i = 0; i < indent; ++i)
                        alignmentSpaces += L"  ";
                    for (size_t i = 1; i < tagName.length(); ++i) // Skip '<'
                        alignmentSpaces += L" ";
                    alignmentSpaces += L"  ";
                    
                    // Parse and format attributes
                    size_t attrPos = 0;
                    bool firstAttr = true;
                    bool inQuote = false;
                    wchar_t quoteChar = L'\0';
                    
                    while (attrPos < attributes.length())
                    {
                        if (!inQuote && (attributes[attrPos] == L'"' || attributes[attrPos] == L'\''))
                        {
                            inQuote = true;
                            quoteChar = attributes[attrPos];
                        }
                        else if (inQuote && attributes[attrPos] == quoteChar)
                        {
                            inQuote = false;
                        }
                        else if (!inQuote && attributes[attrPos] == L' ')
                        {
                            // Look for next non-space character
                            size_t nextAttr = attrPos + 1;
                            while (nextAttr < attributes.length() && attributes[nextAttr] == L' ')
                                nextAttr++;
                                
                            if (nextAttr < attributes.length())
                            {
                                // Found next attribute
                                if (firstAttr)
                                {
                                    formattedXml += L" ";
                                    firstAttr = false;
                                }
                                else
                                {
                                    formattedXml += L"\r\n" + alignmentSpaces;
                                }
                                attrPos = nextAttr - 1; // -1 because we'll increment at end of loop
                            }
                        }
                        else
                        {
                            if (firstAttr && attributes[attrPos] != L' ')
                            {
                                formattedXml += L" ";
                                firstAttr = false;
                            }
                            formattedXml += attributes[attrPos];
                        }
                        attrPos++;
                    }
                    
                    if (isSelfClosing)
                        formattedXml += L" />";
                    else
                        formattedXml += L">";
                }
                else
                {
                    // No attributes
                    formattedXml += tag;
                }
            }
            else
            {
                // Processing instruction, comment, or closing tag
                formattedXml += tag;
            }
            
            // Update indent for opening tags
            if (!isProcessingInstruction && !isComment && !isClosingTag && !isSelfClosing)
                indent++;
                
            pos = tagEnd + 1;
            
            // Add content between tags
            size_t nextTag = xmlText.find(L'<', pos);
            if (nextTag != std::wstring::npos && nextTag > pos)
            {
                std::wstring content = xmlText.substr(pos, nextTag - pos);
                // Trim whitespace
                size_t start = content.find_first_not_of(L" \t\r\n");
                if (start != std::wstring::npos)
                {
                    size_t end = content.find_last_not_of(L" \t\r\n");
                    content = content.substr(start, end - start + 1);
                    formattedXml += content;
                }
                pos = nextTag;
            }
        }
        else
        {
            pos++;
        }
    }
    
    result += formattedXml;
    
    // Add text after XML with line separator
    if (xmlEnd + 1 < text.length())
    {
        result += L"\r\n";
        result += text.substr(xmlEnd + 1);
    }
    
    return result;
}

void CMessageViewerDlg::ResetWindowPositioning()
{
    s_currentX = MARGIN;
    s_currentY = MARGIN;
    s_currentMonitor = NULL;
}

} // namespace debugviewpp
} // namespace fusion