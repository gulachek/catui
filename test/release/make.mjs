import { cli } from "esmakefile";
import { Distribution } from "esmakefile-cmake";

cli((make) => {
  const d = new Distribution(make, {
    name: "test",
    version: "1.2.3",
  });

  const catui = d.findPackage("catui");

  const test = d.addTest({
    name: "test",
    src: ["test.c"],
    linkTo: [catui],
  });

  make.add("test", [test.run], () => {});
});
