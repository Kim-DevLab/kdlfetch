# Design notes

`kdlfetch` targets Linux and uses operating-system interfaces rather than
external libraries:

| Information | Source |
| --- | --- |
| Distribution | `/etc/os-release` (`PRETTY_NAME`) |
| Kernel | `uname(2)` |
| Hostname | `gethostname(2)` |
| Uptime | `/proc/uptime` |
| CPU model | `/proc/cpuinfo` |
| Memory | `/proc/meminfo` (`MemTotal`, `MemAvailable`) |
| Root filesystem | `statvfs(2)` on `/` |

Each field is collected independently. If a source cannot be read or parsed, the
program writes a warning to standard error, prints `Unavailable` for that
field, and exits unsuccessfully after displaying the other values.
