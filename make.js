const { CppBuildCommand } = require('gulpachek/cpp');
const fs = require('fs');
const { version } = require('./package.json');
const { Command } = require('commander');
const { series, parallel } = require('bach');
const { spawn } = require('child_process');

if (!version) {
	console.error(new Error('gtree package.json version not specified'));
	process.exit(1);
}

const program = new Command();
const cppBuild = new CppBuildCommand({
	program,
	cppVersion: 20
});

function makeLib(args) {
	const { cpp } = args;

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

	lib.include('include');

	lib.link(cpp.require('com.gulachek.error', '0.1.0'));
	lib.link(cpp.require('com.gulachek.gtree', '0.2.0'));

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

	const lib = makeLib(args);

	const test = cpp.compile({
		name: 'handshake_test',
		src: ['test/handshake_test.cpp']
	});

	test.define({
		TEST_SOCK_ADDR: sys.dest('test_socket').abs()
	});

	const ut = cpp.require('org.boost.unit-test-framework', '1.78.0', 'dynamic');

	test.link(lib);
	test.link(ut);

	const semverTest = cpp.compile({
		name: 'semver_test',
		src: ['test/semver_test.cpp']
	});

	semverTest.link(lib);
	semverTest.link(ut);

	const tests = [
		test,
		semverTest
	];

	const exes = tests.map(t => t.executable());
	const buildRules = exes.map(e => sys.rule(e));
	const runTests = exes.map(e => () => spawn(e.abs(), [], { stdio: 'inherit' }));

	return series(parallel(...buildRules), series(...runTests));
});

cppBuild.pack(makeLib);

program.parse();
