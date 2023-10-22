# Catui protocol

There are 3 parties in a catui session.

1. load balancer
1. server
1. application

At a high level, the application initiates a connection with the
load balancer according to a platform-specific mechanism. The
application tells the load balancer which UI protocol it
conforms to. Assuming the load balancer can find an
implementation of this protocol, the load balancer will forward
the connection to a conforming server that implements the
protocol. The server tells the application that it's ready to
communicate, and then the rest of the session is governed by the
protocol that the application and server implement.

# Definitions

`CATUI_RUNTIME_DIR`:

- linux: `$XDG_RUNTIME_DIR/catui` (`/run/user/{uid}/catui`)
- macOS: `$XDG_RUNTIME_DIR/catui` (`~/Library/Caches/TemporaryItems/catui`)
- windows: `$XDG_RUNTIME_DIR/catui` (`%HOME%\\AppData\\Local\\Temp\\gulachek\\catui`)

`CATUI_DATA_HOME`:

- linux: `$XDG_DATA_HOME/catui` (`$HOME/.local/share/catui`)
- macOS: `$XDG_DATA_HOME/catui` (`$HOME/Application Support/catui`)
- windows: `$XDG_DATA_HOME/catui` (`%HOME%\\AppData\\Local\\gulachek\\catui`)

`CATUI_DATA_DIRS`:

- linux/macOS: `$XDG_DATA_DIRS/catui` (`/usr/local/share/catui:/usr/share/catui`)
- windows: TODO (maybe program files?)

# Load Balancer (user machine)

## Unix like systems

On a unix like system (macOS, linux, etc), the load balancer
should have a unix domain socket listening at `$CATUI_RUNTIME_DIR/load_balancer.sock`.
After accepting a connection and reading the requested protocol,
it should look for a conforming server first at
`CATUI_DATA_HOME` and then at `CATUI_DATA_DIRS`. For each
directory (`$DIR`), if a directory exists at
`$DIR/<protocol>/<major-version>` (`$SERVER`), then the load
balancer should read a json file named `$SERVER/server.json`.
The load balancer should confirm that the `catui-semver` key is
compatible with it's implementation. Then it should confirm
that the `semver` key is compatible with the requested
protocol/version. It should then fork a process and exec the
command specified by the `command` string array.

If a subsequent request comes in and the protocol/version is
compatible with a running server, then the load balancer should
reuse that server.

Before forking the server, the load balancer should create a
`unixsocketpair`. The parent process and child process should
keep one and close the other, such that one of each file
descriptor is owned exclusively by the parent or the child. The
forked child process should set an environment variable
`CATUI_LOAD_BALANCER_FD` to a decimal representation of the file
descriptor. The exec'ed process should look at this enviroment
variable to know how to communicate with the load balancer.

The load balancer should forward the file descriptor associated
with the connecting application to the server using an ancillary
message over the unix domain socket on `CATUI_LOAD_BALANCER_FD`.
It should then close the file descriptor in its own process so
that the server is the sole owner of the connection. The server
should handle both incoming connections on
`CATUI_LOAD_BALANCER_FD` via reading ancillary messages and
incoming communication from the existing application connections
via an implementation defined mechanism, such as using a select
loop to multiplex.

## Windows

TODO - design named pipe server

# Application (any connected machine)

An application can connect to a catui server by first connecting
to the load balancer. It does so by reading an environment
variable `$CATUI_ADDRESS`. If this does not exist, it defaults
to `$CATUI_RUNTIME_DIR/load_balancer.sock` on unix like systems
and (TODO) on windows. According to the platform, the
application connects to the specified address. Upon connecting
successfully, the application sends a JSON message to the load
balancer detailing the protocol that the application seeks to
communicate, as described in the Load Balancer section. It then
waits for a JSON message of acknowledgement from the server. If
the payload has zero length, this is considered successful.
Otherwise, the application can read an error message from the
load balancer or server via the `error` key in the received JSON
object. The rest of the session is defined by the protocol.
