const { CppBuildCommand } = require('gulpachek-cpp');
const { Path, Target, copyFile } = require('gulpachek');
const fs = require('fs');
const { version } = require('./package.json');
const { Command } = require('commander');
const { spawn } = require('child_process');
const {
	writeTree,
	KeyValue,
	Vector,
	Utf8,
	Unsigned,
	Tuple
} = require('@gulachek/gtree');

if (!version) {
	console.error(new Error('gtree package.json version not specified'));
	process.exit(1);
}

class WriteTree extends Target
{
	#tree;

	constructor(sys, dest, tree)
	{
		super(sys, Path.dest(dest));
		this.#tree = tree;
	}

	mtime()
	{
		return null;
	}

	recipe()
	{
		console.log(`writing tree ${this.path}`);
		return writeTree(this.abs, this.#tree);
	}
}

function writeConfig(sys, dest, config)
{
	const semverEnc = new Tuple([
		['major', Unsigned],
		['minor', Unsigned],
		['patch', Unsigned]
	]);

	const configEnc = new KeyValue({
		catui_version: semverEnc,
		version: semverEnc,
		exec: new Vector(Utf8)
	});

	return new WriteTree(sys, dest, configEnc.gtreeEncode(config));
}

function makeSemver(major, minor, patch)
{
	return {major,minor,patch};
}

class Execute extends Target
{
	exe;
	#args;

	constructor(exe, args)
	{
		super(exe.sys);
		this.exe = exe;
		this.#args = args || [];
	}

	deps()
	{
		return this.exe;
	}

	toString()
	{
		return `Execute{${this.exe.path}}`;
	}

	recipe()
	{
		console.log('executing', this.exe.path.toString(), ...this.#args);
		return spawn(this.exe.abs, this.#args, { stdio: 'inherit' }); 
	}
}

function execSeries(...exes)
{
	if (!exes.length)
		return null;

	const lastIndex = exes.length - 1;
	const tail = new Execute(exes[lastIndex]);

	const head = execSeries(...exes.slice(0, lastIndex));
	head && tail.dependsOn(head);
	return tail;
}


const program = new Command();
const cppBuild = new CppBuildCommand({
	program,
	cppVersion: 20
});

cppBuild.on('configure', (cmd) => {
	cmd.option('--catui-install-root <abs>', 'which directory to install catui devices');
});

function makeLib(args) {
	const { cpp, sys } = args;

	const lib = cpp.compile({
		name: 'com.gulachek.catui',
		version,
		apiDef: 'CATUI_API',
		src: [
			'src/handshake.cpp',
			'src/semver.cpp',
			'src/connection.cpp'
		]
	});

	lib.define({
		CATUI_INSTALL_ROOT: { implementation: args.opts.catuiInstallRoot }
	});

	lib.include('include');

	lib.link(cpp.require('com.gulachek.error', '0.1.0'));
	lib.link(cpp.require('com.gulachek.gtree', '0.2.0'));
	lib.link(cpp.require('com.gulachek.dictionary', '0.2.0'));

	return lib;
}

cppBuild.build((args) => {
	const { cpp } = args;
	return cpp.toLibrary(makeLib(args)).binary();
});

const test = program.command('test')
.description('Build and run unit tests');

cppBuild.configure(test, (args) => {
	const { cpp, sys } = args;

	const installRoot = sys.abs(Path.dest('catui_root'));
	args.opts.catuiInstallRoot = installRoot;

	const lib = makeLib(args);

	const test = cpp.compile({
		name: 'handshake_test',
		src: ['test/handshake_test.cpp']
	});

	const overrideRoot = Path.dest('override_root');

	test.define({
		TEST_SOCK_ADDR: sys.abs(Path.dest('test_socket')),
		TEST_OVERRIDE_ROOT: sys.abs(overrideRoot)
	});

	const ut = cpp.require('org.boost.unit-test-framework', '1.78.0', 'dynamic');

	test.link(lib);
	test.link(ut);

	const handshakeTest = test.executable();

	const semverTest = cpp.compile({
		name: 'semver_test',
		src: ['test/semver_test.cpp']
	});

	semverTest.link(lib);
	semverTest.link(ut);

	const exampleCatui = cpp.compile({
		name: 'example_catui',
		src: ['test/example_catui.cpp']
	});

	exampleCatui.link(lib);

	const exampleExe = exampleCatui.executable();

	const config = writeConfig(
		sys, 'catui_root/com.example.catui/0/config.gt',
		{
			exec: [exampleExe.abs],
			catui_version: makeSemver(0,1,0),
			version: makeSemver(0,1,0)
		}
	);

	const override =
		copyFile(config, overrideRoot.join('com.example.override/0'));

	const series = execSeries(semverTest.executable(), handshakeTest);
	series.dependsOn(exampleExe, config, override);
	return series;
});

cppBuild.pack(makeLib);

program.parse();
