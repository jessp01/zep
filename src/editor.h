#pragma once

#include "common_namespace.h"
#include <deque>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <threadpool/ThreadPool.hpp>
#include <vector>
#include "mcommon/math/math.h"
#include "mcommon/animation/timer.h"
#include "splits.h"

// Basic Architecture

// Editor
//      Buffers
//      Modes -> (Active BufferRegion)
// Display
//      BufferRegions (->Buffers)
//
// A buffer is just an array of chars in a gap buffer, with simple operations to insert, delete and search
// A display is something that can display a collection of regions and the editor controls in a window
// A buffer region is a single view onto a buffer inside the main display
//
// The editor has a list of ZepBuffers.
// The editor has different editor modes (vim/standard)
// ZepDisplay can render the editor (with imgui or something else)
// The display has multiple BufferRegions, each a window onto a buffer.
// Multiple regions can refer to the same buffer (N Regions : N Buffers)
// The Modes receive key presses and act on a buffer region
namespace Zep
{

using namespace COMMON_NAMESPACE;

class ZepBuffer;
class ZepMode;
class ZepMode_Vim;
class ZepMode_Standard;
class ZepEditor;
class ZepSyntax;
class ZepTabWindow;
class ZepWindow;
class ZepTheme;

class IZepDisplay;

struct Region;

using utf8 = uint8_t;

extern const char* Msg_HandleCommand;
extern const char* Msg_Quit;
extern const char* Msg_GetClipBoard;
extern const char* Msg_SetClipBoard;

class ZepMessage
{
public:
    ZepMessage(const char* id, const std::string& strIn = std::string())
        : messageId(id)
        , str(strIn)
    {
    }

    const char* messageId; // Message ID
    std::string str; // Generic string for simple messages
    bool handled = false; // If the message was handled
};

struct IZepComponent
{
    virtual void Notify(std::shared_ptr<ZepMessage> message) = 0;
    virtual ZepEditor& GetEditor() const = 0;
};

class ZepComponent : public IZepComponent
{
public:
    ZepComponent(ZepEditor& editor);
    virtual ~ZepComponent();
    ZepEditor& GetEditor() const override
    {
        return m_editor;
    }

private:
    ZepEditor& m_editor;
};

// Registers are used by the editor to store/retrieve text fragments
struct Register
{
    Register()
        : text("")
        , lineWise(false)
    {
    }
    Register(const char* ch, bool lw = false)
        : text(ch)
        , lineWise(lw)
    {
    }
    Register(utf8* ch, bool lw = false)
        : text((const char*)ch)
        , lineWise(lw)
    {
    }
    Register(const std::string& str, bool lw = false)
        : text(str)
        , lineWise(lw)
    {
    }

    std::string text;
    bool lineWise = false;
};

using tRegisters = std::map<std::string, Register>;
using tBuffers = std::deque<std::shared_ptr<ZepBuffer>>;
using tSyntaxFactory = std::function<std::shared_ptr<ZepSyntax>(ZepBuffer*)>;

namespace ZepEditorFlags
{
enum
{
    None = (0),
    DisableThreads = (1 << 0)
};
};

const float bottomBorder = 4.0f;
const float textBorder = 4.0f;
const float leftBorder = 60.0f;


class ZepEditor
{
public:
    ZepEditor(IZepDisplay* pDisplay, uint32_t flags = 0);
    ~ZepEditor();

    void Quit();

    void InitWithFileOrDir(const std::string& str);

    ZepMode* GetCurrentMode() const;
    void Display();

    void RegisterMode(std::shared_ptr<ZepMode> spMode);
    void SetMode(const std::string& mode);
    ZepMode* GetCurrentMode();

    void RegisterSyntaxFactory(const std::string& extension, tSyntaxFactory factory);
    bool Broadcast(std::shared_ptr<ZepMessage> payload);
    void RegisterCallback(IZepComponent* pClient)
    {
        m_notifyClients.insert(pClient);
    }
    void UnRegisterCallback(IZepComponent* pClient)
    {
        m_notifyClients.erase(pClient);
    }

    const tBuffers& GetBuffers() const;
    ZepBuffer* AddBuffer(const std::string& str);
    ZepBuffer* GetMRUBuffer() const;
    void SaveBuffer(ZepBuffer& buffer);

    void SetRegister(const std::string& reg, const Register& val);
    void SetRegister(const char reg, const Register& val);
    void SetRegister(const std::string& reg, const char* pszText);
    void SetRegister(const char reg, const char* pszText);
    Register& GetRegister(const std::string& reg);
    Register& GetRegister(const char reg);
    const tRegisters& GetRegisters() const;

    void Notify(std::shared_ptr<ZepMessage> message);
    uint32_t GetFlags() const
    {
        return m_flags;
    }

    // Tab windows
    using tTabWindows = std::vector<ZepTabWindow*>;
    void SetCurrentTabWindow(ZepTabWindow* pTabWindow);
    ZepTabWindow* GetActiveTabWindow() const;
    ZepTabWindow* AddTabWindow();
    void RemoveTabWindow(ZepTabWindow* pTabWindow);
    const tTabWindows& GetTabWindows() const;

    void ResetCursorTimer();
    bool GetCursorBlinkState() const;

    void RequestRefresh();
    bool RefreshRequired() const;

    void SetCommandText(const std::string& strCommand);

    const std::vector<std::string>& GetCommandLines()
    {
        return m_commandLines;
    }

    void UpdateWindowState();

    // Setup the display size for the editor
    void SetDisplayRegion(const NVec2f& topLeft, const NVec2f& bottomRight);
    void UpdateSize();

    IZepDisplay& GetDisplay() const
    {
        return *m_pDisplay;
    }

    ZepTheme& GetTheme() const;

private:
    IZepDisplay* m_pDisplay;
    std::set<IZepComponent*> m_notifyClients;
    mutable tRegisters m_registers;

    std::shared_ptr<ZepTheme> m_spTheme;
    std::shared_ptr<ZepMode_Vim> m_spVimMode;
    std::shared_ptr<ZepMode_Standard> m_spStandardMode;
    std::map<std::string, tSyntaxFactory> m_mapSyntax;
    std::map<std::string, std::shared_ptr<ZepMode>> m_mapModes;

    // Blinking cursor
    timer m_cursorTimer;

    // Active mode
    ZepMode* m_pCurrentMode = nullptr;

    // Tab windows
    tTabWindows m_tabWindows;
    ZepTabWindow* m_pActiveTabWindow = nullptr;

    // List of buffers that the editor is managing
    // May or may not be visible
    tBuffers m_buffers;
    uint32_t m_flags = 0;

    mutable bool m_bPendingRefresh = true;
    mutable bool m_lastCursorBlink = false;

    std::vector<std::string> m_commandLines; // Command information, shown under the buffer

    Region m_tabContentRegion;
    Region m_commandRegion;
    Region m_tabRegion;

    NVec2f m_topLeftPx;
    NVec2f m_bottomRightPx;

    bool m_bRegionsChanged = false;
};

} // namespace Zep
