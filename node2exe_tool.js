var fs = require('fs');
var path = require('path');
var http = require('http');
var zlib = require('zlib');
var admZip = require('adm-zip');
var child_process = require('child_process');
var tar = require('tar-stream');
var zlib = require('zlib');

var upxPath = path.join(path.dirname(process.argv[1]), 'upx.exe');
var upxZipPath = path.join(path.dirname(process.argv[1]), 'upx391w.zip');
var upxUrl = 'http://upx.sourceforge.net/download/upx391w.zip';

var resHackerPath = path.join(path.dirname(process.argv[1]), 'ResourceHacker.exe');
var resHackerZipPath = path.join(path.dirname(process.argv[1]), 'resource_hacker.zip');
var resHackerUrl = 'http://www.angusj.com/resourcehacker/resource_hacker.zip';

var gorcPath = path.join(path.dirname(process.argv[1]), 'gorc.exe');
var gorcZipPath = path.join(path.dirname(process.argv[1]), 'Gorc.zip');
var gorcUrl = 'http://www.godevtool.com/Gorc.zip';

// for debugging
var debug = false;
if (debug) {
	if (process.argv.length === 2) {
		process.argv.push(
			'hello.exe',
			'-icon', 'examples/Example2/foo/Example.ico',
			'-copyright', 'copy but do not steal',
			'-description', 'this is great',
			'-company', 'My company',
			'-product', 'Some prog',
			'-version', '1.3',
			'-fileversion', '1.2',
			'-upx', 'true',
			'examples/Example2/Example.js',
			'examples/Example2/foo');
	}
}
//~ node2exe('hello.exe', 'examples/Example2/Example.js', 'examples/Example2/foo');
//~ node2exe('hello.exe', 'examples/Example/Example.js');

install(function() {
	var args = [];
	args.push.apply(args, process.argv);
	args.shift();
	args.shift();
	if (!args[0]) {
		console.log('usage: node2exe_tool [options] <output> <script> [<other>...]');
		console.log('Options:');
		console.log(' -icon <value> path to icon');
		console.log(' -copyright <value> version info property');
		console.log(' -description <value> version info property');
		console.log(' -company <value> version info property');
		console.log(' -product <value> version info property');
		console.log(' -version <value> version info property');
		console.log(' -fileversion <value> version info property');
		console.log(' -upx (true|false)');
		console.log(' -cleanup (none|*normal*|force) node2exe setting, delete files after execution');
		console.log(' -temp <value> node2exe setting, path to unpack to');
		console.log(' -execute (*true*|false) node2exe setting, execute js after unpack');
		console.log(' -overwrite (true|*false*) node2exe setting, overwrite files that already exist');

		return;
	}
	args.push(function() {
		// some code I used while debugging to launch my example
		if (debug) {
			var hello = child_process.spawn(args[0]);
			hello.stdout.on('data', function(data) {
				console.log('' + data);
			});
			hello.stderr.on('data', function(data) {
				console.log('' + data);
			});
		}
	});
	node2exe.apply(this, args);
});

// I don't distribute other peoples binaries with this script so this fetches them
// if they are not present
function install(cb) {

	var readyCount = 3;
	var ready = function() {
		if (--readyCount) return;
		cb();
	};

	if (!fs.existsSync(upxPath)) {
		var done = function(err) {
			if (err) throw err;
			unzip(upxZipPath, 'upx391w/upx.exe', upxPath, function(err) {
				if (err) throw err;
				ready();
			});
		}
		if (!fs.existsSync(upxZipPath)) {
			download(upxUrl, upxZipPath, done)
		} else {
			done();
		}
	} else {
		ready();
	}

	if (!fs.existsSync(resHackerPath)) {
		var done = function(err) {
			if (err) throw err;
			unzip(resHackerZipPath, 'ResourceHacker.exe', resHackerPath, function(err) {
				if (err) throw err;
				ready();
			});
		}
		if (!fs.existsSync(resHackerZipPath)) {
			download(resHackerUrl, resHackerZipPath, done)
		} else {
			done();
		}
	} else {
		ready();
	}

	if (!fs.existsSync(gorcPath)) {
		var done = function(err) {
			if (err) throw err;
			unzip(gorcZipPath, 'GoRC.exe', gorcPath, function(err) {
				if (err) throw err;
				ready();
			});
		}
		if (!fs.existsSync(gorcZipPath)) {
			download(gorcUrl, gorcZipPath, done)
		} else {
			done();
		}
	} else {
		ready();
	}
}

function unzip(zip, file, dest, cb) {
	console.log('Unzipping ' + dest);
	// reading archives
	var adm = new admZip(zip);
	var zipEntries = adm.getEntries(); // an array of ZipEntry records
	var done = false;
	var list = [];
	zipEntries.forEach(function(zipEntry) {
		list.push(zipEntry.entryName.trim()); // outputs zip entries information
		if (!done && zipEntry.entryName === file) {
			var buffer = zipEntry.getData();
			var fd = fs.openSync(dest, 'w+', 0666);
			fs.writeSync(fd, buffer, 0, buffer.length);
			fs.closeSync(fd);
			done = true;
			cb(null);
		}
	});
	if (!done) cb('"' + file + '" not found in "' + zip + '"\nfiles:\n\t' + list.join('\n\t'));
}

function download(url, dest, cb) {
	console.log('Starting download ' + url);
	var file = fs.createWriteStream(dest);
	var request = http.get(url, function(response) {
		response.pipe(file);
		file.on('finish', function() {
			console.log('Download complete ' + url);
			file.close(cb);  // close() is async, call cb after close completes.
		});
	}).on('error', function(err) { // Handle errors
		fs.unlink(dest); // Delete the file async. (But we don't check the result)
		if (cb) cb(err.message);
	});
};

/**
 * Makes a node2exe program.
 * @param {string} output Path of the file to create
 * @param {object} [opt] Options
 * @param {string} ... Paths of files or directories to add, the first should be the javascript file to execute
 * @param {function} cb Callback when complete
 */
function node2exe(/* output, [opt], paths..., cb */) {
	var paths = [];
	var cb = null;
	var optObject = {};
	var opt = {};
	for (var i = 0; i < arguments.length; i++) {
		if (typeof arguments[i] === 'object' && optObject === null) {
			optObject = arguments[i];
		} else if (typeof arguments[i] === 'function' && cb === null) {
			cb = arguments[i];
		} else if (typeof arguments[i] === 'string') {
			var m;
			if (m = /^[\/\-]+(.*)/.exec(arguments[i])) {
				opt[m[1]] = arguments[i + 1];
				i++;
			} else {
				paths.push(arguments[i]);
			}
		} else {
			throw Error('unexpected argument ' + arguments[i]);
		}
	}

	var output = paths.shift();
	if (path.extname(output).toLowerCase() !== '.exe') {
		throw Error('output path should have .exe extension');
	}

	if (optObject) {
		for(var k in optObject) {
			if(!opt[k]) {
				opt[k] = optObject[k];
			}
		}
	}

	if (!opt.cleanup) opt.cleanup = 'normal';
	if (!opt.temp) opt.temp = '%TEMP%\\node2exe\\%NODE2EXE_ISODATE%\\';
	if (!opt.execute) opt.execute = true;
	if (!opt.overwrite) opt.overwrite = false;
	opt.upx = !/^(?:false|0)$/i.test('' + opt.upx);
	if (!opt.icon) opt.icon = null;
	if (!opt.description) opt.description = '';
	if (!opt.copyright) opt.copyright = '';
	if (!opt.company) opt.company = '';
	if (!opt.product) opt.product = '';
	if (!opt.version) opt.version = '1.0.0.0';
	if (!opt.fileversion) opt.fileversion = opt.version;

	function toVersion(val) {
		val = (val + ',0,0,0,0').replace(/\D+/g, ',').replace(/^,+|,+$/g, '').split(',');
		return val[0] % 65536 + ', ' + val[1] % 65536 + ', ' + val[2] % 65536+ ', ' + val[3] % 65536;
	}

	var node2exePath = path.join(path.dirname(process.argv[1]), 'node2exe.exe');

	preBuild();

	// to set other stuff follow http://answers.bitrock.com/questions/325/how-can-i-modify-the-description-or-version-information-for-the-windows-installers
	// but I need windres first
	function preBuild() {
		addVersionInfo();
		function addVersionInfo() {
			if (true) {

				console.log('Adding version info');
				if (fs.existsSync(resHackerPath) && fs.existsSync(gorcPath)) {

					var resHacker = child_process.exec(
						'"' + resHackerPath + '"' +
						' -extract' +
						' "' + node2exePath.replace(/\\/g, '\\\\') + '",' +
						' "' + path.resolve('temp.rc').replace(/\\/g, '\\\\') + '",versioninfo,,',
						function() {
							var rc = fs.readFileSync(path.resolve('temp.rc'), 'utf-8');
							rc = rc.replace(/"CompanyName".*/, '"CompanyName", "' + opt.company + '"');
							rc = rc.replace(/"ProductName".*/, '"ProductName", "' + opt.product + '"');
							rc = rc.replace(/"FileDescription".*/, '"FileDescription", "' + opt.description + '"');
							rc = rc.replace(/"FileVersion".*/, '"FileVersion", "' + opt.fileversion + '"');
							rc = rc.replace(/^FILEVERSION .*/m, 'FILEVERSION ' + toVersion(opt.fileversion));
							rc = rc.replace(/"ProductVersion".*/, '"ProductVersion", "' + opt.version + '"');
							rc = rc.replace(/^PRODUCTVERSION .*/m, 'PRODUCTVERSION ' + toVersion(opt.version));
							rc = rc.replace(/"OriginalFilename".*/, '"OriginalFilename", "' + path.basename(output) + '"');
							rc = rc.replace(/"InternalName".*/, '"InternalName", "' + path.basename(output, path.extname(output))+ '"');
							if (opt.copyright) rc = rc.replace(/"LegalCopyright".*/, '"LegalCopyright", "' + opt.copyright + '"');
							fs.writeFileSync(path.resolve('temp.rc'), rc, 'utf-8');
							child_process.exec(
								'"' + gorcPath + '"' +
								' "' + path.resolve('temp.rc') + '"',
								function() {
									var resHacker = child_process.exec(
										'"' + resHackerPath + '"' +
										' -addoverwrite' +
										' "' + node2exePath.replace(/\\/g, '\\\\') + '",' +
										' "' + path.resolve('temp.tmp').replace(/\\/g, '\\\\') + '",' +
										' "' + path.resolve('temp.res').replace(/\\/g, '\\\\') + '",' +
										' versioninfo,' +
										' 1,' +
										' ',
										function() {
										//~ console.log('f');
											addIcon(path.resolve('temp.tmp'));
										});
								});
						});
				} else {
					console.log('Skipping because "' + resHackerPath + '" or "' + gorcPath + '" is missing');
					addIcon(node2exePath);
				}
			} else {
				addIcon(node2exePath);
			}
		}

		function addIcon(node2exePath) {
			if (opt.icon) {
				if (fs.existsSync(resHackerPath)) {
					console.log('Adding icon');
					var resHacker = child_process.exec(
						'"' + resHackerPath + '"' +
						' -addoverwrite' +
						' "' + node2exePath.replace(/\\/g, '\\\\') + '",' +
						' "' + path.resolve('temp2.tmp').replace(/\\/g, '\\\\') + '",' +
						' "' + path.resolve(opt.icon).replace(/\\/g, '\\\\') + '",' +
						' ICONGROUP,' +
						' IDR_MAINFRAME,' +
						' 1033',
						function() {
							build(path.resolve('temp2.tmp'), function(err, stdout, stderr) {
								if (err || stdout || stderr ) throw Error(stdout + ' ' + stderr);
								postBuild();
							})
						});
				} else {
					console.log('Skipping adding icon because "' + resHackerPath + '" is missing');
					build(node2exePath, postBuild)
				}
			} else {
				build(node2exePath, postBuild)
			}
		}
	}

	function postBuild() {
		if (opt.upx) {
			if (fs.existsSync(upxPath)) {
				console.log('Compressing with upx');
				child_process.exec(
					'"' + upxPath + '"' +
					' ' +
					' "' + output + '"' +
					'',
					function(err, stdout, stderr) {
						if (err) throw Error(stdout + ' ' + stderr);
						cleanUp();
					});
			} else {
				console.log('Skipping because "' + upxPath + '" is missing');
				cleanUp();
			}
		} else {
			cleanUp();
		}
	}

	function cleanUp() {
		var list = fs.readdirSync('.');
		for (var i in list) {
			var f = list[i];
			if (path.extname(f).toLowerCase() === '.tmp' || f === 'temp.rc' || f === 'temp.res' || f === 'temp.obj') {
				console.log('Delete ' + f);
				fs.unlinkSync(f);
			}
		}
		cb();
	}

	function build(node2exePath, cb) {
		console.log('Building ' + output);

		var delimiter = '*/\r\n*/\'"NODE2EXE\r\n*/\r\n';

		var n = 0;
		var config = [
			'=any line that starts with equals or does not contain equals is ignored\r\n' +
			'=unrecognised keys are loaded but never used e.g. #cleanup\r\n' +
			'temp=' + opt.temp + '\r\n' +
			'execute=' + opt.execute + '\r\n' +
			'cleanup=' + opt.cleanup + '\r\n' +
			'overwrite=' + opt.overwrite
		];

		var files = [];
		for (var i = 0; i < paths.length; i++) {
			if (fs.statSync(paths[i]).isDirectory()) {
				addDirectory(paths[i]);
			} else {
				addFile(paths[i]);
			}
		}

		function addFile(file) {
			config.push('name_' + (n++) + '=' + file);
			files.push(file);
		}

		function addDirectory(dir) {
			var files = fs.readdirSync(dir);
			for (var i in files) {
				var name = dir + '/' + files[i];
				if (fs.statSync(name).isDirectory()){
					addDirectory(name);
				} else {
					addFile(name);
				}
			}
		}

		config = config.join('\r\n') + '\r\n';

		var fd = fs.openSync(output, 'w+', 0666);
		appendFile(fd, node2exePath);
		fs.writeSync(fd, delimiter);
		fs.writeSync(fd, config);
		for (var i = 0; i < files.length; i++) {
			fs.writeSync(fd, delimiter);
			appendFile(fd, files[i]);
		}
		fs.closeSync(fd);

		if (cb) cb();

		function appendFile(fd, file) {
			var buffer = fs.readFileSync(file);
			fs.writeSync(fd, buffer, 0, buffer.length);
		}
	}
}
