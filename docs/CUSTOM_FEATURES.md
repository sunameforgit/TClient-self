# TClient 自定义功能技术文档

本文档记录 TClient 中所有自定义功能的实现细节，包括皮肤窃取、自动表情、快速输入等。

## 目录

1. [皮肤窃取 (Skin Steal)](#皮肤窃取-skin-steal)
2. [自动表情 (Auto Emote)](#自动表情-auto-emote)
3. [好友上线通知 (Friend Online Notification)](#好友上线通知-friend-online-notification)
4. [快速输入 (Fast Input)](#快速输入-fast-input)

---

## 皮肤窃取 (Skin Steal)

### 功能描述

允许玩家通过锤子击打或钩子钩中其他玩家时，自动窃取对方的皮肤配置。

### 配置文件

- **头文件**: `src/game/client/components/tclient/skin_steal.h`
- **实现文件**: `src/game/client/components/tclient/skin_steal.cpp`

### 配置变量

```cpp
MACRO_CONFIG_INT(TcHammerStealSkin, tc_hammer_steal_skin, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Steal skin when hammer hits other players")
MACRO_CONFIG_INT(TcHookStealSkin, tc_hook_steal_skin, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Steal skin when hook attaches to other players")
```

### 核心类

```cpp
class CSkinSteal : public CComponent
{
public:
    virtual int Sizeof() const override { return sizeof(*this); }
    virtual void OnInit() override;
    virtual void OnRender() override;
    
    // Called from chat command
    void StealSkin(int TargetId);

private:
    int64_t m_LastStealTime;    // 上次窃取时间（用于冷却）
    int m_LastHookedPlayer;      // 上次钩中的玩家ID
    bool m_WasHooked;            // 是否正在钩中状态
    bool m_WasFiringHammer;      // 是否正在锤击（用于检测锤击开始）
    int m_LastStolenFrom;        // 上次窃取的玩家ID（防止重复）
    
    // For predicted hammer hit detection
    int m_LastProcessedEventTick;   // 上次处理的事件tick
    int m_LastHammerHitTick;        // 上次锤击tick
    bool m_HammerStealTriggeredThisTick;  // 当前tick是否已触发
    int m_LastStealTargetId;        // 上次窃取的目标ID（防止重复）
    
    void StealFromHammerHit();
    void StealFromHammerHitPredicted();
    void CheckPredictedHammerEvents();
};
```

### 锤子窃取检测

采用**双重检测机制**，结合事件驱动和预测数据，提高检测可靠性：

#### 方法1: 预测事件驱动（主要）

监听预测世界中的锤子命中事件，这是最准确的检测方式：

```cpp
void CheckPredictedHammerEvents()
{
    CGameWorld *pPredictedWorld = &GameClient()->m_PredictedWorld;
    
    // 遍历预测事件
    for(const auto &Event : pPredictedWorld->m_PredictedEvents)
    {
        // 只处理锤子命中事件
        if(Event.m_EventId != NETEVENTTYPE_HAMMERHIT)
            continue;
        
        if(Event.m_Id != LocalId)
            continue;
        
        // 跳过已处理的事件
        if(Event.m_Tick <= m_LastProcessedEventTick)
            continue;
        
        // 在命中位置附近查找目标
        vec2 HammerHitPos = Event.m_Pos;
        // ... 查找并窃取皮肤
    }
}
```

#### 方法2: 预测世界检测（备用）

当事件检测失效时，使用预测世界的角色位置进行几何检测：

```cpp
void StealFromHammerHitPredicted()
{
    CGameWorld *pPredictedWorld = &GameClient()->m_PredictedWorld;
    CCharacter *pLocalChar = pPredictedWorld->GetCharacterById(LocalId);
    
    // 从预测世界获取位置和角度
    vec2 LocalPos = pLocalChar->Core()->m_Pos;
    float Angle = pLocalChar->Core()->m_Angle / 256.0f * (pi / 180.0f);
    vec2 Direction = vec2(cosf(Angle), sinf(Angle));
    vec2 HammerPos = LocalPos + Direction * 28.0f;
    
    // 在预测世界中查找目标（范围稍大：50.0f）
    // ... 查找并窃取皮肤
}
```

#### 方法3: Snap 数据检测（后备）

当预测数据不可用时，使用服务器同步的 snap 数据：

```cpp
// 获取当前和上一帧的角色数据
const CNetObj_Character &Char = GameClient()->m_Snap.m_aCharacters[LocalId].m_Cur;
const CNetObj_Character &PrevChar = GameClient()->m_Snap.m_aCharacters[LocalId].m_Prev;

// 检测条件：武器是锤子 且 攻击tick变化
bool IsFiringHammer = (Char.m_Weapon == WEAPON_HAMMER) && 
                      (Char.m_AttackTick != PrevChar.m_AttackTick);

if(IsFiringHammer && !m_WasFiringHammer)
{
    StealFromHammerHitPredicted();  // 优先使用预测世界
}
```

**三种检测方式的关系**:
1. 每帧首先检查预测事件（最准确）
2. 当检测到 `m_AttackTick` 变化时，使用预测世界查找目标
3. 如果预测世界不可用，回退到 snap 数据

**为什么这样更稳定**:
- **事件驱动**: 直接监听游戏内建的锤子命中事件，不依赖 tick 比较
- **预测数据**: 使用客户端预测的角色位置，比服务器 snap 数据更实时
- **多重保障**: 三种方法互为备份，提高检测成功率

### 钩子窃取检测

**原理**: 通过 `m_HookState` 和钩子位置检测

```cpp
if(Char.m_HookState == HOOK_GRABBED)
{
    // 查找钩子位置附近的玩家
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(length(vec2(Char.m_HookX, Char.m_HookY) - vec2(Target.m_X, Target.m_Y)) < 28.0f)
        {
            // 找到钩中的玩家
        }
    }
}
```

### 窃取内容

1. **皮肤名称**: `player_skin <name>`
2. **身体颜色**: `player_color_body <color>`（仅当目标使用自定义颜色时）
3. **脚部颜色**: `player_color_feet <color>`（仅当目标使用自定义颜色时）
4. **启用自定义颜色**: `player_use_custom_color <0/1>`（根据目标设置）

### 原皮处理

当窃取原皮（无自定义颜色）时，先重置颜色再设置皮肤，避免看到之前的颜色：

```cpp
if(!UseCustomColor)
{
    // 先重置颜色
    Console()->ExecuteLine("player_use_custom_color 0", -1);
    Console()->ExecuteLine("player_color_body 0", -1);
    Console()->ExecuteLine("player_color_feet 0", -1);
    
    // 再设置皮肤
    Console()->ExecuteLine("player_skin xxx", -1);
}
```

### 防Spam机制

- **冷却时间**: 50ms（防止重复触发）
- **实现**: 使用 `time()` 比较上次窃取时间
- **防重复**: 
  - 使用 `m_LastStolenFrom` 防止钩子和锤子同时触发时重复窃取
  - 使用 `m_LastStealTargetId` 防止短时间内重复窃取同一目标（500ms内）

### 聊天命令

**手动窃取皮肤**:
```
/stealskin <player_id>
```

**实现位置**: `src/game/client/components/chat.cpp`

```cpp
// 在 SendChat 函数中拦截命令
if(str_startswith(pLine, "/stealskin "))
{
    const char *pIdStr = pLine + 11;
    int TargetId = str_toint(pIdStr);
    GameClient()->StealSkinFromChat(TargetId);
    return; // 不发送到服务器
}
```

**GameClient 实现**: `src/game/client/gameclient.cpp`

```cpp
void CGameClient::StealSkinFromChat(int TargetId)
{
    // 直接调用皮肤窃取组件
    m_SkinSteal.StealSkin(TargetId);
}
```

---

## 自动表情 (Auto Emote)

### 功能描述

自动在 Normal 表情和选定表情之间切换，支持随机表情模式。

### 配置文件

- **头文件**: `src/game/client/components/tclient/auto_emote.h`
- **实现文件**: `src/game/client/components/tclient/auto_emote.cpp`

### 配置变量

```cpp
MACRO_CONFIG_INT(TcAutoEmoteToggle, tc_auto_emote_toggle, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Auto emote toggle")
MACRO_CONFIG_INT(TcAutoEmoteType, tc_auto_emote_type, 0, 0, 5, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Auto emote type (0=happy, 1=pain, 2=surprise, 3=angry, 4=blink, 5=random)")
MACRO_CONFIG_INT(TcAutoEmoteInterval, tc_auto_emote_interval, 1000, 100, 5000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Auto emote interval in ms")
```

### 核心类

```cpp
class CAutoEmote : public CComponent
{
private:
    int64_t m_LastEmoteTime;     // 上次发送表情时间
    bool m_WasChatOpen;          // 聊天是否打开（避免冲突）
    bool m_IsNormalState;        // 当前是否为Normal状态
    
    void SendEmote(int EmoteType);
    int GetCurrentEmoteType();
    const char* GetEmoteCommand(int EmoteType);
};
```

### 表情类型映射

| 配置值 | 表情类型 | 命令 |
|--------|----------|------|
| 0 | Happy | happy |
| 1 | Pain | pain |
| 2 | Surprise | surprise |
| 3 | Angry | angry |
| 4 | Blink | blink |
| 5 | Random | random |

### 切换逻辑

```cpp
// 在 Normal 和选定表情之间切换
m_IsNormalState = !m_IsNormalState;

if(m_IsNormalState)
    return EMOTE_NORMAL;
else
    return SelectedEmote;  // 根据配置返回对应表情
```

### 随机模式

```cpp
if(SelectedType == 5)  // Random mode
{
    if(m_IsNormalState)
        return EMOTE_NORMAL;
    else
    {
        static const int s_aRandomEmotes[] = {EMOTE_HAPPY, EMOTE_PAIN, EMOTE_SURPRISE, EMOTE_ANGRY, EMOTE_BLINK};
        return s_aRandomEmotes[(time_get() / time_freq()) % 5];
    }
}
```

### 发送方式

```cpp
// 使用聊天命令发送表情
// 持续时间 = 间隔 / 2，实现无缝切换
int Duration = g_Config.m_TcAutoEmoteInterval / 2;
if(Duration < 100) Duration = 100;      // 最小 100ms
if(Duration > 3000) Duration = 3000;    // 最大 3s

char aBuf[64];
str_format(aBuf, sizeof(aBuf), "/emote %s %d", pCmd, Duration);
GameClient()->m_Chat.SendChatQueued(aBuf);
```

### 注意事项

- **持续时间**: 间隔的一半（100ms - 3000ms），实现无缝切换
- **聊天冲突**: 聊天打开时不发送表情
- **间隔设置**: 通过 `TcAutoEmoteInterval` 控制（100-5000ms）

---

## 好友上线通知 (Friend Online Notification)

### 功能描述

检测好友上线并在聊天框显示通知消息。

### 配置文件

- **头文件**: `src/game/client/components/tclient/friend_notify.h`
- **实现文件**: `src/game/client/components/tclient/friend_notify.cpp`

### 配置变量

```cpp
MACRO_CONFIG_INT(TcFriendOnlineNotify, tc_friend_online_notify, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Notify when friend comes online")
```

### 核心类

```cpp
class CFriendNotify : public CComponent
{
private:
    int64_t m_LastCheckTime;
    std::unordered_set<std::string> m_LastOnlineFriends;
    
    void CheckFriendsOnline();
    void NotifyFriendOnline(const char *pName);
};
```

### 检测原理

遍历服务器浏览器中的所有服务器，检查每个服务器上的玩家是否为好友：

```cpp
// 检查所有服务器
for(int i = 0; i < ServerBrowser()->NumServers(); i++)
{
    const CServerInfo *pInfo = ServerBrowser()->Get(i);
    if(!pInfo || pInfo->m_FriendState == IFriends::FRIEND_NO)
        continue;

    // 检查服务器上的每个玩家
    for(int j = 0; j < pInfo->m_NumClients; j++)
    {
        const CServerInfo::CClient &Client = pInfo->m_aClients[j];
        if(Client.m_FriendState != IFriends::FRIEND_NO)
        {
            // 这是好友，加入在线列表
            CurrentOnlineFriends.insert(std::string(Client.m_aName));
        }
    }
}
```

### 通知方式

在聊天框显示绿色系统消息：

```cpp
void CFriendNotify::NotifyFriendOnline(const char *pName)
{
    char aBuf[256];
    str_format(aBuf, sizeof(aBuf), "[TClient] Friend '%s' is now online!", pName);
    GameClient()->m_Chat.AddLine(-2, 0, aBuf); // -2 = CLIENT_MSG (绿色)
}
```

### 检测范围

- **所有服务器**: 包括服务器浏览器中显示的所有服务器
- **在线好友面板**: 右边好友列表中的好友
- **其他服务器上的好友**: 即使不在当前查看的服务器上

---

## 快速输入 (Fast Input)

### 功能描述

减少输入延迟，让玩家操作更快反映在游戏中。

### 配置变量

```cpp
MACRO_CONFIG_INT(TcFastInput, tc_fast_input, 0, 0, 50, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Fast input amount (x0.1 ticks)")
MACRO_CONFIG_INT(TcFastInputOthers, tc_fast_input_others, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Extra tick for other tees")
```

### 核心实现

**位置**: `src/game/client/gameclient.cpp`

```cpp
// 计算快速输入tick数
float FastInputTicks = g_Config.m_TcFastInput / 10.0f;
int FinalTickSelf = FinalTickRegular + (int)std::ceil(FastInputTicks);
```

**引擎层处理**: `src/engine/client/client.cpp`

```cpp
if(g_Config.m_TcFastInput > 0 && GameClient()->CheckNewInput())
{
    // 立即发送输入，不等待下一帧
}
```

### UI设置

**位置**: `src/game/client/components/tclient/menus_tclient.cpp`

```cpp
// Fast Input Amount 滑动条 (0-50, x0.1)
DoSliderWithScaledValue(&g_Config.m_TcFastInput, &g_Config.m_TcFastInput, 
    &Button, TCLocalize("Fast Input Amount"), 0, 50, 1, 
    &CUi::ms_LinearScrollbarScale, CUi::SCROLLBAR_OPTION_NOCLAMPVALUE, " (x0.1)");
```

### 数值说明

- **范围**: 0-50（对应 0.0-5.0 ticks）
- **单位**: 1 tick = 20ms
- **显示**: `12 (x0.1)` 表示 1.2 ticks = 24ms

---

## 通用技术要点

### 时间处理

```cpp
// 获取当前时间（ticks）
int64_t CurrentTime = time();

// 转换为毫秒
int64_t Ms = (CurrentTime - m_LastTime) * 1000 / time_freq();
```

### Snap数据访问

```cpp
// 获取当前角色数据
const CNetObj_Character &Char = GameClient()->m_Snap.m_aCharacters[LocalId].m_Cur;
const CNetObj_Character &PrevChar = GameClient()->m_Snap.m_aCharacters[LocalId].m_Prev;

// 检查是否有效
if(!GameClient()->m_Snap.m_aCharacters[LocalId].m_Active)
    return;
```

### 玩家数据访问

```cpp
// 获取玩家信息
const CGameClient::CClientData &Target = GameClient()->m_aClients[TargetId];

// 检查是否有效
if(!Target.m_Active)
    return;

// 访问皮肤信息
Target.m_aSkinName        // 皮肤名称
Target.m_ColorBody        // 身体颜色
Target.m_ColorFeet        // 脚部颜色
Target.m_UseCustomColor   // 是否使用自定义颜色
```

### 控制台命令执行

```cpp
// 执行控制台命令
Console()->ExecuteLine("player_skin default", -1);

// 带参数的命令
char aBuf[256];
str_format(aBuf, sizeof(aBuf), "player_skin %s", SkinName);
Console()->ExecuteLine(aBuf, -1);
```

---

## 文件位置汇总

| 功能 | 头文件 | 实现文件 |
|------|--------|----------|
| 皮肤窃取 | `src/game/client/components/tclient/skin_steal.h` | `src/game/client/components/tclient/skin_steal.cpp` |
| 自动表情 | `src/game/client/components/tclient/auto_emote.h` | `src/game/client/components/tclient/auto_emote.cpp` |
| 好友上线通知 | `src/game/client/components/tclient/friend_notify.h` | `src/game/client/components/tclient/friend_notify.cpp` |
| 配置变量 | - | `src/engine/shared/config_variables_tclient.h` |
| UI设置 | - | `src/game/client/components/tclient/menus_tclient.cpp` |
| 快速输入 | - | `src/game/client/gameclient.cpp` |
| 聊天命令 | `src/game/client/gameclient.h` | `src/game/client/components/chat.cpp` |

---

## 最后更新

- **日期**: 2025-01-21
- **版本**: TClient Custom Features v1.4
- **更新内容**:
  - 修复 `/stealskin` 聊天命令（使用 `m_SkinSteal` 直接调用）
  - **重构锤子检测系统**（三重检测机制）：
    - 新增预测事件驱动检测（`NETEVENTTYPE_HAMMERHIT`）
    - 新增预测世界角色位置检测
    - 保留 snap 数据作为后备检测
  - 优化皮肤窃取冷却机制：
    - 冷却时间从 10ms 调整为 50ms
    - 添加 `m_LastStealTargetId` 防止短时间内重复窃取同一目标
  - 优化原皮窃取逻辑（先重置颜色）
  - 表情持续时间改为间隔的一半
  - 添加防重复窃取机制
  - 添加好友上线通知功能文档
  - UI 文字汉化（自定义功能区块）
  - 调整自定义功能区块位置到右边列
