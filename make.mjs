import { cli, Path } from "esmakefile";
import { writeFile, rm, readFile } from "node:fs/promises";
import { Distribution, addCompileCommands } from "esmakefile-cmake";

const packageContent = await readFile("package.json", "utf8");
const { name, version } = JSON.parse(packageContent);

cli((make) => {
  make.add("all", []);

  const d = new Distribution(make, {
    name,
    version,
    cStd: 17,
    cxxStd: 20,
  });

  const unix = d.findPackage("unixsocket");
  const msgstream = d.findPackage("msgstream");
  const cjson = d.findPackage("libcjson");
  const gtest = d.findPackage("gtest_main");

  const catui = d.addLibrary({
    name: "catui",
    src: ["src/catui.c", "src/catui_server.c"],
    linkTo: [unix, msgstream, cjson],
  });

  const test = d.addTest({
    name: "catui_test",
    src: ["test/catui_test.cpp"],
    linkTo: [catui, gtest],
  });

  const cmds = addCompileCommands(make, d);

  const echoVersion = "1.0.0";
  const catuiDir = Path.build("catui");
  const echoConfig = catuiDir.join(
    `com.example.echo/${echoVersion}/config.json`
  );

  make.add(echoConfig, async (args) => {
    const [config, server] = args.absAll(echoConfig, echoServer);
    const json = JSON.stringify({
      exec: [server],
    });

    await writeFile(config, json, "utf8");
  });

  make.add("all", [cmds, catui.binary, echoApp.binary, echoServer.binary]);
  make.add("test", [test.run], () => {});

  make.add("serve", async (args) => {
    const [server, sock, search] = args.absAll(
      catuid.binary,
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
