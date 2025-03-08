import { cli, Path } from "esmakefile";
import { writeFile, rm, readFile } from "node:fs/promises";
import { Distribution, addCompileCommands } from 'esmakefile-cmake';

const packageContent = await readFile('package.json', 'utf8');
const { name, version } = JSON.parse(packageContent);

cli((make) => {
  make.add("all", []);

	const d = new Distribution(make, {
		name, version,
		cStd: 17,
	});

	const unix = d.findPackage('unixsocket');
	const msgstream = d.findPackage('msgstream');
	const cjson = d.findPackage('libcjson');

	const catui = d.addLibrary({
		name: 'catui',
		src: ['src/catui.c', 'src/catui_server.c'],
		linkTo: [unix, msgstream, cjson]
	});

	// TODO - remove these executables from library distribution

  // Load balancer implementation
  const catuid = d.addExecutable({
    name: "catuid",
    src: ["src/catuid.c"],
    linkTo: [unix, cjson, msgstream, catui],
  });

  // Testing
  const echoApp = d.addExecutable({
    name: "echo_app",
    src: ["test/echo_app.c"],
    linkTo: [catui],
  });

  const echoServer = d.addExecutable({
    name: "echo_server",
    src: ["test/echo_server.c"],
    linkTo: [catui],
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
