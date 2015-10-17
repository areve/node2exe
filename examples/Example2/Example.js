// an example script showning some things about itself that will be useful
// for when running inside node2exe
console.log('Example.js');
console.log('----------');
console.log();
console.log('Executable:');
console.log(process.execPath);
console.log();
console.log('Current working directory:');
console.log(process.cwd());
console.log();
console.log('Script name:');
console.log(__filename);
console.log();
console.log('Script directory:');
console.log(__dirname);
console.log();
console.log('Arguments:');
console.log(process.argv);
