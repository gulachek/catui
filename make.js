import { cli, Path } from "iheartmake";
import { platformCompiler, C } from "iheartmake-c";

cli((book) => {
  const c = new C(platformCompiler(), {
    cVersion: "C17",
    book,
  });

  const unixsocket = "unixsocket";
  const msgstream = "msgstream";
  const libcjson = "libcjson";

  book.add("all", []);

  /*
  const catui = linkLibrary(
    book,
    "libcatui.dylib",
    [buffer, catuiObj],
    [unixsocket, msgstream]
  );
	*/
  const catui = c.addLibrary({
    name: "catui",
    version: "0.1.0",
    src: ["src/catui.c"],
    link: [unixsocket, msgstream],
  });

  const catuiServer = c.addLibrary({
    name: "catui-server",
    version: "0.1.0",
    src: ["src/catui_server.c"],
    link: [unixsocket, msgstream],
  });

  // Load balancer implementation
  c.addExecutable({
    name: "catuid",
    version: "0.1.0",
    src: ["src/catuid.c"],
    link: [unixsocket, libcjson, msgstream, catuiServer],
  });

  // Testing
  c.addExecutable({
    name: "echo_app",
    src: ["test/echo_app.c"],
    link: [catui],
  });

  c.addExecutable({
    name: "echo_server",
    src: ["test/echo_server.c"],
    link: [catuiServer],
  });

  const catuid = Path.build("catuid");
  const echoApp = Path.build("echo_app");
  const echoServer = Path.build("echo_server");

  const compileCommands = Path.build("compile_commands.json");
  c.addCompileCommands();

  book.add("all", [compileCommands, echoApp, echoServer, catuid]);
});
