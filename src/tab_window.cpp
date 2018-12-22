#include "tab_window.h"
#include "buffer.h"
#include "display.h"
#include "editor.h"
#include "window.h"

// A 'window' is like a Vim Tab
namespace Zep
{

ZepTabWindow::ZepTabWindow(ZepEditor& editor)
    : ZepComponent(editor)
    , m_editor(editor)
{
}

ZepTabWindow::~ZepTabWindow()
{
    std::for_each(m_windows.begin(), m_windows.end(), [](ZepWindow* pWindow) { delete pWindow; });
}

ZepWindow* ZepTabWindow::AddWindow(ZepBuffer* pBuffer)
{
    auto pWin = new ZepWindow(*this, pBuffer);
    m_windows.push_back(pWin);

    if (m_pActiveWindow == nullptr)
    {
        m_pActiveWindow = pWin;
    }
    return pWin;
}

void ZepTabWindow::CloseActiveWindow()
{
    if (m_pActiveWindow)
    {
        RemoveWindow(m_pActiveWindow);
    }
}

void ZepTabWindow::RemoveWindow(ZepWindow* pWindow)
{
    assert(pWindow);
    if (!pWindow)
        return;

    auto itrFound = std::find(m_windows.begin(), m_windows.end(), pWindow);
    if (itrFound == m_windows.end())
    {
        assert(!"Not found?");
        return;
    }

    delete pWindow;
    m_windows.erase(itrFound);

    if (m_windows.empty())
    {
        m_pActiveWindow = nullptr;
        GetEditor().RemoveTabWindow(this);
    }
    else
    {
        if (m_pActiveWindow == pWindow)
        {
            // TODO: Active window ordering - remember the last active and switch to it when this one is closed
            m_pActiveWindow = m_windows[m_windows.size() - 1];
        }
    }
}

void ZepTabWindow::Notify(std::shared_ptr<ZepMessage> payload)
{
    // Nothing yet.
}

void ZepTabWindow::SetDisplayRegion(const Region& region)
{
    if (m_lastRegionRect != region.rect)
    {
        m_lastRegionRect = region.rect;
        m_windowRegion = region;
        m_buffersRegion = m_windowRegion;

        for (auto& pWin : m_windows)
        {
            pWin->SetDisplayRegion(m_buffersRegion.rect);
        }
    }
}

void ZepTabWindow::Display()
{
    for (auto& pWin : m_windows)
    {
        pWin->Display();
    }
}

} // namespace Zep
