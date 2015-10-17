node2exe
========

node2exe is a modified version of node.js's node.exe for windows. The purpose
is to allow node and a javascript (or many) to be distributed as one exe file.
It's basically a self extractor that starts running a node script as soon as
it's extracted.

For simpler creation of an exe run use `node2exe_tool.js` e.g.

    npm install
    node node2exe_tool.js Example.exe Example.js node_modules otherfile.txt -filedescription "Some description" -icon favicon.ico -copyright "Copyright 2015"

For more details run `node node2exe_tool.js`


The I slightly modified node.cc [node.cc.patch](node.cc.patch) and added
[node2exe.h](node2exe.h) to the same src folder. Then I built
with `vcbuild.bat nosign` (I'm using VS2010 Express although other versions
should work). That produces node.exe which I then rename to
[node2exe.exe](node2exe.exe)

Once built node2exe.exe behaves almost identically to node.exe. The difference
is that when it runs it looks through itself for a 22 character delimiter, if
it is not found it continues as normal. If delimiter's are found:
  * The bytes between each delimiter will be treated as separate files. The first
	file is a config section, others will be saved to a temp folder file as
	node2exe.js, 1, 2, 3, ... *
  * Then node2exe.js will be executed. Nothing is done with 1, 2, 3, ... they
	can be used by node2exe.js, execute, copied, renamed or anything.
  * Command line arguments you pass to the exe will be passed on. For example if you execute
    `Example.exe hello world` it will have the same effect as running
    `node node2exe.js hello world`.
  * If two delimiters are found together, after the config section, then searching for delimiters will
    stop. The remaining part of the file will be extracted as the last file part. *

* (configuration settings may alter this default behaviour)

To build an Example.exe I run:

	copy /b node2exe.exe + node2exe.delimiter + node2exe.delimiter + example.js + node2exe.delimiter + other.data Example.exe

When it executes example.js will be node2exe.js and other.data will be 1, it's
possible for node2exe.js to rename 1 or copy it elsewhere or whatever.
Default temp folders will be in **%TEMP%\node2exe\nnnnnnnnn** which is usually
**C:\Users\%username%\AppData\Local\Temp\node2exe\nnnnnnnnn**. the temp folder will be
deleted if the program finishes or if the console is closed,
CTRL+C is pressed etc.

The delimiter file contains
`"*/\r\n*/'"NODE2EXE\r\n*/\r\n"` a string delimiter of 22 bytes that can
never occur in a valid javascript file. The delimiter
can be created with a batch file as follows:

	> node2exe.delimiter echo */
	>> node2exe.delimiter echo */'"NODE2EXE
	>> node2exe.delimiter echo */

Once built you can change the file icon or version info if you want to with
[Resource Hacker](http://www.angusj.com/resourcehacker/).
[UPX](http://upx.sourceforge.net/) Can be used on the node2exe.exe before or
after data is appended to the binary, in my experiments. The icon can only be
changed before upx is done though. Or you could just recompile everything.

There are other ways I could have hooked into node.exe, this method
was fairly simple to implement and required few edits to node source. To build
an executable with node2exe no compliers are required. It is Windows only at
present, I'm sure it could be made to work on other platforms too but that's
beyond my current requirements.

Configuration
-------------
Config keys are understood by the node2exe program in two ways. They can be
found in the config section or passed as command line arguments.
Each line in the config section with an equals sign (=) in it will be interpreted
as a key and value for config. e.g. `temp = local` will set config key 'temp' to 'local'
Command line arguments that start with `--node2exe-` will not be passed on to
the javascript they will be treated as config settings. The --node2exe- part
will be removed and the rest of the argument treated the same as a line in the
config section. e.g. `--node2exe-temp=local` will be treated the same as `temp=local`
in the config section. Command line arguments are read first before the config
section. If a config key is present more than once the first entry will be used.
Config keys are case insensitve. Little validation on config keys is done, if
you use invalid keys they will be ignored.

Config keys
-----------
Defaults are indicated in bold.

  * cleanup - (none|**normal**|force) delete files and directories created on complete.
    * none - nothing will be deleted, after the node script executes
    * normal - all create directories will be deleted, directories will only be deleted if they are empty
	* force - all create directories will be deleted, directories will be deleted even if they are not empty
  * temp - path to unpack to default is **%TEMP%\node2exe\%NODE2EXE_ISODATE%\**
    * e.g. `%CD%\`
  * name_(n) - alters the name of the files that are written out from the defaults
    name_0 defaults to **node2exe.js** name_1 to **1**, name_2 to **2**, ...
	Unless the path after variable expansion starts with .\ ./ ..\ ../ \\ or have second char : the path will be treated as relative to the NODE2EXE_DEST folder
	* e.g. `name_0=example.js`
	* e.g. `name_1=7za.exe`
  * execute - (**true**|false) if false then no execution of a node script will be done after unpacking
  * overwrite = (true|**false**) if false then the program will not overwrite in the way files

Environment Variables
---------------------
Environment variables can be used in some of the config settings only, node2exe
sets the following during execution time for you to make use of.
  * CD - the current working directory
  * NODE2EXE_NANODATE - start up time in nanoseconds
  * NODE2EXE_ISODATE - date in iso string format
  * NODE2EXE_DEST - the temp path once it's been resolved, trailing backslashes will have been removed
  * NODE2EXE_SCRIPT - full path to the script that will be executed
