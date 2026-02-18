[![DDraceNetwork](docs/assets/TClient_Logo_Horizontal.svg)](https://tclient.app)

[![Build status](https://github.com/TaterClient/TClient/workflows/Build/badge.svg)](https://github.com/TaterClient/TClient/actions/workflows/build.yaml)

## ⚠️ Disclaimer

This is a **customized version** of TClient (based on DDNet) with additional features for personal use. 

- **Use at your own risk** - Not guaranteed to be bug-free
- **Not affiliated** with the official DDNet or TClient developers
- **May violate server rules** - Some features could be considered unfair advantages on certain servers
- **No warranty** - The authors are not responsible for any bans, data loss, or other issues

## 🙏 Credits & Acknowledgments

This project is built upon the amazing work of:

- **[DDNet](https://github.com/ddnet/ddnet)** - The original DDraceNetwork game and client
- **[TClient](https://github.com/TaterClient/TClient)** - The base client this fork is built on
- **Tela** - For the TClient logo design
- **Solly** - For the SVG work
- All contributors to DDNet and TClient

Original projects:
- DDNet: https://github.com/ddnet/ddnet
- TClient: https://github.com/TaterClient/TClient

---

## ✨ Custom Features (Added by suname)

### 🎭 Auto Emote Toggle
Automatically switch between eye emotes with customizable interval and type.
- `tc_auto_emote_toggle` - Enable/disable auto emote
- `tc_auto_emote_interval` - Switch interval in milliseconds (100-5000ms)
- `tc_auto_emote_type` - Emote type: 0=Happy, 1=Pain, 2=Surprise, 3=Angry, 4=Blink, 5=Random
- Automatically pauses when chat is open

### 🔨 Hammer Skin Steal
Automatically steal other players' skins when hitting them with hammer.
- `tc_hammer_steal_skin` - Enable/disable skin steal on hammer hit

### 🪝 Hook Skin Steal
Automatically steal other players' skins when hooking them.
- `tc_hook_steal_skin` - Enable/disable skin steal on hook

### 👥 Friend Online Notification
Get notified when your friends join the server.
- `tc_friend_online_notify` - Show notification when friend comes online (green message in chat)
- 5-second cooldown to prevent spam
- Only visible to you

### 💬 Improved Chat Experience
- Regular chat messages are sent immediately without delay
- Emote commands have rate limiting (0.5s) to prevent spam
- Fixed message duplication issues
- Chat history no longer polluted with /emote commands

---

## 📥 Installation

* Download the latest [release](https://github.com/sunameforgit/TClient-self/releases)
* Download a [nightly (dev/unstable) build](https://github.com/sunameforgit/TClient-self/actions/workflows/fast-build.yml?query=branch%3Amaster)
* [Clone](https://docs.github.com/en/repositories/creating-and-managing-repositories/cloning-a-repository) this repo and build using the [guide from DDNet](https://github.com/ddnet/ddnet?tab=readme-ov-file#cloning)

### Links

[Discord](https://discord.gg/BgPSapKRkZ)
[Website](https://tclient.app)

---

## 🌍 Translation

FTAPI (a simple wrapper for Google translate) will work out of the box, however it will quickly become overloaded

This is a guide for setting up [libretranslate](https://docs.libretranslate.com/guides/installation/)

First you need an old version of python (3.8, 3.9 or 3.10), along with `pip`

If you do not have this you can use [conda](https://www.anaconda.com/docs/getting-started/miniconda/install#quickstart-install-instructions) to install it

```sh
conda create -n libretranslate python=3.9
conda activate libretranslate
```

Then you can install and run libretranslate, do note that this requires large libraries like `torch` so it's a couple of gigs

```sh
pip install libretranslate
libretranslate
```

You can then set `tc_translate_backend libretranslate`, the port is automatically 5000

---

## 📜 Scripting

TClient supports the [ChaiScript](https://chaiscript.com/) language for simple tasks

Add scripts to your config dir then run them with `chai [scriptname] [args]`

> [!CAUTION]
> There are no runtime restrictions, you can easily `while (true) {}` yourself or run out of memory, be careful!

```js
var a // Declare a variable
a = 1 // Set it
var b = 2 // Do both at once
var c = "strings"
var d = ["lists", 2] // not strongly typed
// var e, f = d // no list deconstruction
print(d[0] + to_string(d[1])) // explicit to_string required for string concat
var bass = "ba" + "s" + "s"
var ass = bass.substr(1, -1) // both indices required, use -1 for end
if (a == b) { // brackets required
	print("this will never happen") // output
} else if (c == "strings") { // string comparison
	exec("echo hello world") // run console stuff
}
var current_game_mode = state("game_mode") // Get the current game mode, all states you can get are listed below
def myfunc(a, b, c) { // yeah it uses def for function definition idk
	print(a, b, c)
	if (a == b) { return "early" }
	c // last statement returns like in rust
}
print(myfunc(1, 2, 3)) // prints "early"
for (var i = 0; i < 10; i += 1) { // for loops (c style)
	print(i) // auto converts to string, will throw if it cant
}
return "top level return"
```

Here is a list of states which are available:

| Return type | Call | Description |
| --- | -- | --- |
| `string` | `state("game_mode")` | Returns the current game mode name (e.g., "DM", "TDM", "CTF"). |
| `bool` | `state("game_mode_pvp")` | Whether the current mode is PvP. |
| `bool` | `state("game_mode_race")` | Whether the current mode is a race mode. |
| `bool` | `state("eye_wheel_allowed")` | Whether the "eye wheel" feature is allowed on this server. |
| `bool` | `state("zoom_allowed")` | Whether camera zoom is allowed. |
| `bool` | `state("dummy_allowed")` | Whether using a dummy client is allowed. |
| `bool` | `state("dummy_connected")` | Whether the dummy client is currently connected. |
| `bool` | `state("rcon_authed")` | Whether the client is authenticated with RCON (admin access). |
| `int` | `state("team")` | The player's current team number. |
| `int` | `state("ddnet_team")` | The player's DDNet team number. |

---

## 📸 Screenshots

<details>
<summary>Click to expand</summary>

![image](https://user-images.githubusercontent.com/22122579/182528700-4c8238c3-836e-49c3-9996-68025e7f5d58.png)

</details>

---

## 🔧 Original TClient Features

> [!NOTE]
> This section documents the original TClient features. Some may be modified in this custom version.

```
tc_run_on_join_console
tc_run_on_join_chat
```

Commands to run when joining a server, one per line

```
tc_chat_client_prefix
```

Prefix for client messages (default: ★)

```
tc_auto_reply
```

Auto reply message when AFK

```
tc_translations
```

Enable translations

```
tc_translate_backend
```

Translation backend: `ftapi` or `libretranslate`

```
tc_translate_outgoing
```

Translate outgoing messages

```
tc_translate_incoming
```

Translate incoming messages

```
tc_target_language
```

Target language for translations (e.g., `en`, `zh`, `ja`)

---

## 🤝 Contributing

If DDNet devs are reading this and want to steal my changes please feel free.

---

## 📄 License

This project follows the same license as DDNet and TClient. See the original repositories for license details.

- DDNet: https://github.com/ddnet/ddnet
- TClient: https://github.com/TaterClient/TClient
