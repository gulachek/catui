import { cli, Path } from "esmakefile";
import { platformCompiler, C } from "esmakefile-c";
import { writeFile, rm } from "node:fs/promises";

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

  book.add("catui", catui);

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
    link: [catui, msgstream],
  });

  c.addExecutable({
    name: "echo_server",
    src: ["test/echo_server.c"],
    link: [catuiServer, msgstream],
  });

  const catuid = Path.build("catuid");
  const echoApp = Path.build("echo_app");
  const echoServer = Path.build("echo_server");

  const compileCommands = Path.build("compile_commands.json");
  c.addCompileCommands();

  const echoVersion = "1.0.0";
  const catuiDir = Path.build("catui");
  const echoConfig = catuiDir.join(
    `com.example.echo/${echoVersion}/config.json`
  );

  book.add(echoConfig, async (args) => {
    const [config, server] = args.absAll(echoConfig, echoServer);
    const json = JSON.stringify({
      exec: [server],
    });

    await writeFile(config, json, "utf8");
  });

  book.add("all", [compileCommands, echoApp, echoServer, catuid, echoConfig]);

  book.add("serve", async (args) => {
    const [server, sock, search] = args.absAll(
      catuid,
      Path.build("test.sock"),
      catuiDir
    );

    try {
      await rm(sock, { force: true });
    } catch (ex) {
      args.logStream.write(ex.message);
    }

    return args.spawn(server, ["-s", sock, "-p", search]);
  });
});
