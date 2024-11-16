import { readFile, writeFile } from "node:fs/promises";
import { Path } from "esmakefile";
import { basename, resolve } from "node:path";

export class Clang {
  constructor(make, config) {
    this.make = make;

    this.cc = config.cc;
    this.cxx = config.cxx;
    this.cppflags = config.cppflags;
    this.cflags = config.cflags;
    this.cxxflags = config.cxxflags;
    this.ldflags = config.ldflags;

    this.commands = [];
  }

  compile(cSrc, opts) {
    const src = Path.src(cSrc);

    let compiler = this.cxx;
    let langflags = this.cxxflags;
    if (src.extname === ".c") {
      compiler = this.cc;
      langflags = this.cflags;
    }

    opts = opts || {};
    const obj = opts.out || Path.gen(src, { ext: ".o" });
    const deps = Path.gen(obj, { ext: ".d" });

    const extra = opts.extraFlags || [];

    const allFlags = [
      "-c",
      ...this.cppflags,
      ...langflags,
      ...extra,
      "-o",
      this.make.abs(obj),
      "-MMD",
      "-MF",
      this.make.abs(deps),
      this.make.abs(src),
    ];

    this.commands.push({
      directory: this.make.srcRoot,
      file: this.make.abs(src),
      arguments: [compiler, ...allFlags],
    });

    this.make.add(obj, src, async (args) => {
      const result = await args.spawn(compiler, allFlags);

      if (!result) return false;
      await addClangDeps(args, args.abs(deps));

      return true;
    });

    return obj;
  }

  link(opts) {
    if (["c", "c++"].indexOf(opts.runtime) === -1) {
      throw new Error(
        `Invalid runtime '${opts.runtime}'. Must be 'c' or 'c++'.`
      );
    }

    const binaries = opts.binaries;
    if (!(binaries && binaries.length > 0)) {
      throw new Error("link(): opts.binaries must be nonempty array");
    }

    const name = opts.name || basename(binaries[0].rel, binaries[0].extname);
    const linkType = opts.linkType;
    if (["static", "shared", "executable"].indexOf(linkType) === -1) {
      throw new Error(
        `link(): opts.linkType '${linkType}' is invalid. Must be 'static', 'shared', or 'executable'`
      );
    }

    if (linkType === "static") {
      const lib = Path.build(`lib${name}.a`);
      this.make.add(lib, binaries, (args) => {
        return args.spawn("ar", [
          "rcs",
          args.abs(lib),
          ...args.absAll(...binaries),
        ]);
      });
      return lib;
    }

    let compiler = this.cxx;
    let langflags = this.cxxflags;
    if (opts.runtime === "c") {
      compiler = this.cc;
      langflags = this.cflags;
    }

    const extra = opts.extraFlags || [];

    if (linkType === "shared") {
      const lib = Path.build(`lib${name}.dylib`);
      this.make.add(lib, binaries, (args) => {
        return args.spawn(compiler, [
          ...langflags,
          "-dynamiclib",
          "-o",
          args.abs(lib),
          ...args.absAll(...binaries),
          ...extra,
          ...this.ldflags,
        ]);
      });
      return lib;
    }

    const exe = Path.build(name);
    this.make.add(exe, binaries, (args) => {
      return args.spawn(compiler, [
        ...langflags,
        "-o",
        args.abs(exe),
        ...args.absAll(...binaries),
        ...extra,
        ...this.ldflags,
      ]);
    });
    return exe;
  }

  compileCommands() {
    const p = Path.build("compile_commands.json");

    this.make.add(p, async (args) => {
      const contents = JSON.stringify(this.commands);
      await writeFile(args.abs(p), contents, "utf8");
    });

    return p;
  }
}

async function addClangDeps(args, depsAbs) {
  const depContents = await readFile(depsAbs, "utf8");
  const escapedLines = depContents.replaceAll("\\\n", " ");

  const lines = escapedLines.split("\n");
  if (lines.length < 1) return;

  const colonIndex = lines[0].indexOf(":");
  if (colonIndex < 0) return;

  for (const postreq of lines[0].slice(colonIndex + 1).split(/ +/)) {
    if (!postreq) continue;

    args.addPostreq(resolve(postreq));
  }
}
