# 1.6 C2

<p align="center">
<img src="./img/c2.jpg" alt="C2" width="500" />
</p>

Using Counter Strike 1.6 as a C2 for fun.

See https://x.com/eversinc33/status/1750144319934046566

The Server is a CS 1.6 Server, the command to execute is passed as the server hostname and the agent parses commands and sends command output back via RCON.

Execute commands on the agent via the in-game console, e.g. `hostname "cmd.exe /c dir C:\"`

### TODO

Investigate the RCON `upload` command for data exfiltration (https://cs1-6cfg.blogspot.com/p/cs-16-client-and-console-commands.html)
