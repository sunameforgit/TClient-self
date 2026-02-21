# TClient 自定义功能技术文档

本文档记录 TClient 中所有自定义功能的实现细节，包括皮肤窃取、自动表情、快速输入等。

## 目录

1. [皮肤窃取 (Skin Steal)](#皮肤窃取-skin-steal)
2. [自动表情 (Auto Emote)](#自动表情-auto-emote)
3. [快速输入 (Fast Input)](#快速输入-fast-input)

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
    int m_LastFireTick;          // 上次攻击tick（用于锤子检测）
    int m_LastStolenFrom;        // 上次窃取的玩家ID（防止重复）
    
    void StealFromPredictedHammerHit(CCharacter *pLocalChar);
};
```

### 锤子窃取检测

**原理**: 使用预测世界检测锤子攻击

```cpp
// 获取预测角色
CCharacter *pLocalChar = GameClient()->m_PredictedWorld.GetCharacterById(LocalId);
if(pLocalChar && pLocalChar->GetActiveWeapon() == WEAPON_HAMMER)
{
    int CurrentAttackTick = pLocalChar->GetAttackTick();
    if(CurrentAttackTick > m_LastFireTick)
    {
        // 锤子刚攻击，查找命中目标
        StealFromPredictedHammerHit(pLocalChar);
        m_LastFireTick = CurrentAttackTick;
    }
}
```

**目标查找**: 使用游戏原始逻辑查找命中目标

```cpp
float ProximityRadius = pLocalChar->GetProximityRadius();
float Angle = pLocalChar->Core()->m_Angle * (pi / 180.0f) / 256.0f;
vec2 Direction = vec2(cosf(Angle), sinf(Angle));
vec2 ProjStartPos = pLocalChar->Core()->m_Pos + Direction * (ProximityRadius * 0.75f);

// 使用 FindEntities 查找范围内的玩家（与游戏逻辑一致）
CEntity *apEnts[MAX_CLIENTS];
int Num = GameClient()->m_PredictedWorld.FindEntities(ProjStartPos, ProximityRadius * 0.5f, apEnts,
    MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
```

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

- **冷却时间**: 50ms
- **实现**: 使用 `time()` 比较上次窃取时间
- **防重复**: 使用 `m_LastStolenFrom` 防止钩子和锤子同时触发时重复窃取

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
| 配置变量 | - | `src/engine/shared/config_variables_tclient.h` |
| UI设置 | - | `src/game/client/components/tclient/menus_tclient.cpp` |
| 快速输入 | - | `src/game/client/gameclient.cpp` |
| 聊天命令 | `src/game/client/gameclient.h` | `src/game/client/components/chat.cpp` |

---

## 最后更新

- **日期**: 2025-01-21
- **版本**: TClient Custom Features v1.1
- **更新内容**:
  - 添加 `/stealskin` 聊天命令
  - 优化原皮窃取逻辑（先重置颜色）
  - 表情持续时间改为间隔的一半
  - 减少皮肤窃取冷却时间至 50ms
  - 添加防重复窃取机制
