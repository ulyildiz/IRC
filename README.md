# IRC

A lightweight IRC (Internet Relay Chat) server written from scratch in C++98, using a single `poll()` loop for fully non-blocking, event-driven I/O. It implements the core of the IRC protocol — user registration, channels, private messaging, operator privileges and channel modes — and can be used with any standard IRC client.

## Features

- **Non-blocking, single-threaded I/O** built on `poll()`, handling all client sockets from one event loop (up to 1024 simultaneous clients).
- **Standard user registration** flow via `PASS` → `NICK` → `USER`, with a numeric-reply protocol compatible with real IRC clients.
- **Channels** with join/part, `PRIVMSG` (to users and channels), `TOPIC`, and `KICK`/`INVITE`.
- **Channel modes**: `+i` (invite-only), `+t` (topic settable by operators only), `+k` (channel key/password), `+l` (user limit), and operator status (`+o`).
- **Operator system**: the first user to create a channel becomes its operator; operator status is automatically transferred if the last operator leaves.
- **Built-in channel bot**: operators can spawn a bot in a channel with `BOT <nick> <channel>`; it greets new joiners and responds to `!joke` with a random joke.
- **Custom TCP server library** (`SimpleTcpServerListener`), built as a small static library and linked into the server.

## Project structure

```
.
├── Makefile
├── include/
│   └── IRC_Protocol.hpp        # Core class, message/user/channel structs, numeric reply codes
├── src/
│   ├── main.cpp                 # Entry point, argument validation
│   ├── IRC_Protocol.cpp         # Server bootstrap, connection lifecycle, command dispatch
│   ├── Parser.cpp               # Raw IRC line -> IRCMessage (prefix/command/params)
│   ├── Handler.cpp               # Implementation of every supported IRC command
│   └── Checker.cpp               # Registration-state helpers
└── lib/
    └── SimpleTCPServerListener/  # Standalone poll()-based TCP server (built as a static lib)
```

## Requirements

- A C++ compiler supporting C++98 (the Makefile uses `c++` with `-std=c++98 -Wall -Wextra -Werror`)
- `make`
- A POSIX/Linux-like environment (uses `sys/socket.h`, `poll.h`, etc.)

## Build

```bash
make
```

This first builds the static `libSimpleTcpServerListener.a` library from `lib/SimpleTCPServerListener/`, then compiles and links the server into an `ircserv` binary in the project root.

Other useful targets:

```bash
make clean   # remove object files
make fclean  # remove object files, the binary, and the static library
make re      # fclean + all
```

## Usage

```bash
./ircserv <port> <password>
```

- `<port>` must be numeric.
- `<password>` must consist of printable ASCII characters; it is the connection password clients must supply via `PASS` before registering.

The server binds to the machine's local IP and listens on the given port.

## Connecting

Any standard IRC client works (irssi, HexChat, WeeChat, LimeChat, etc.), or you can test manually with `nc`:

```bash
nc <server-ip> <port>
PASS <password>
NICK <your-nick>
USER <username> 0 * :<Real Name>
```

Once registered you'll receive a welcome reply (`001`), after which you can use standard commands.

## Supported commands

| Command   | Description |
|-----------|-------------|
| `PASS`    | Supply the connection password (required before `NICK`/`USER`) |
| `NICK`    | Set/change nickname |
| `USER`    | Set username and real name, completing registration |
| `PING`    | Replied to with `PONG` |
| `JOIN`    | Join one or more channels (supports keys for `+k` channels) |
| `PART`    | Leave one or more channels |
| `PRIVMSG` | Send a message to a user or channel |
| `TOPIC`   | View or set a channel's topic |
| `MODE`    | View or change channel modes (`+i`, `+t`, `+k`, `+l`, `+o`) |
| `KICK`    | Remove a user from a channel (operator only) |
| `INVITE`  | Invite a user to a channel (operator only, required for `+i` channels) |
| `WHO`     | List the users currently in a channel |
| `QUIT`    | Disconnect from the server |
| `BOT`     | Add a simple bot to a channel (operator only) |

## Contributors

- [Hatice Koçan](https://github.com/hkocan)
- [Ulaş Berke Yıldız](https://github.com/ulyildiz)
- [Yunus Emre Saraç](https://github.com/TroubledKezoo1)

## Notes

- The project uses C++98 exclusively (no C++11-and-later features), a common constraint for this kind of exercise.
- The server currently handles only direct client connections — there is no server-to-server linking.
