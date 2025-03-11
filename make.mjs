import { cli } from "esmakefile";
import { readFile } from "node:fs/promises";
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

  make.add("all", [cmds, catui.binary]);
  make.add("test", [test.run], () => {});
});
