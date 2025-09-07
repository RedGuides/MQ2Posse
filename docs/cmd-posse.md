---
tags:
  - command
---

# /posse

## Syntax

<!--cmd-syntax-start-->
```eqcommand
/posse [option] [setting]
```
<!--cmd-syntax-end-->

## Description

<!--cmd-desc-start-->
Set alerts, radius, and commands around player detection.
<!--cmd-desc-end-->

## Options

| Option | Description |
|--------|-------------|
| `(no option)` | Lists command syntax |
| `on|off` | Toggles checking of players. default is off |
| `status` | Shows current status and settings |
| `load|save` | Load the ini file, and saves the ini file. <span class="accent">Changes do not auto-save, you must manually issue the save command.</span> |
| `{add|del} <name>` | Adds or deletes a name from friendly list |
| `clear` | Clears all character names |
| `{radius|zradius} <#>` | Sets radius to check for players. e.g. `/posse radius 300` |
| `list` | Lists current names and commands set. |
| `notify [on|off]` | Toggles notification alerts. Default is on |
| `friendnotify [on|off]` | Toggles notification alerts for friends. Default is ON. |
| `strangernotify [on|off]` | Toggles notification alerts for strangers. Default is ON. |
| `guild [on|off]` | Toggles ignoring guild members, meaning guild members are invisible to all checks. Default is OFF. |
| `fellowship [on|off]` | Toggles ignoring fellowship members. Default is OFF |
| `audio [on|off]` | Toggles audio alerts. Default is OFF. |
| `testaudio` | Plays the audio alert |
| `cmdadd <command>` | Adds a command that is executed when a stranger is near |
| `cmddel #` | Deletes a command that is executed when a stranger is near |
| `debug [on|off]` | Turn debugging on or off |
