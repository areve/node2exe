// Copyright Andrew Challen
//
// Permission is hereby granted, free of charge, to any person obtaining a<ve
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef SRC_NODE2EXE_H_
#define SRC_NODE2EXE_H_

#include <iostream>  //cout
#include <string>
#include <vector>
#include <map>
#include <io.h> //fopen
#include <windows.h> // FindFirstFile
#include <sys/stat.h> // _S_IFREG
#include <sstream> //ss
#include <algorithm> // find_if
#include <functional> //not1
#include <cctype> //isspace

#define BUFFER_SIZE ((size_t)8 * 1024 * 1024)

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef THROW_LAST
#define THROW_LAST(msg)\
	int lastError = GetLastError(); \
	std::wprintf(L"%s: %d", msg, lastError); \
	throw(lastError, __FILE__, __LINE__);

#endif

#pragma warning(disable: 4996)

namespace node {
	// renamed Start in node.cc
	int Start_orig(int argc, char *argv[]);
}

namespace node2exe {
	// a list of paths that were created, files and folders
	// in the order they were created
	std::vector<std::wstring> createdPaths;

	// config for the program, keys will be lowercase always
	std::map<std::wstring, std::wstring> config;

	// return lower case version of the string
	std::string toLower(std::string data) {
		std::string out(data);
		std::transform(out.begin(), out.end(), out.begin(), ::tolower);
		return out;
	}

	// return lower case version of the string
	std::wstring toLower(std::wstring data) {
		std::wstring out(data);
		std::transform(out.begin(), out.end(), out.begin(), ::tolower);
		return out;
	}

	// return wstring version of the string
	std::wstring toWstring(std::string data) {
		return std::wstring(data.begin(), data.end());
	}

	// toBool
	bool toBool(std::wstring value) {
		std::wstring v = toLower(value);
		return v == L"1" ||
			v == L"t" ||
			v == L"true" ||
			v == L"y" ||
			v == L"yes";
	}

	// returns true if the config value matches the key
	bool configIs(std::wstring key, std::wstring value) {
		if (1 == config.count(key)) {
			return config[key] == value;
		}
		return false;
	}

	// returns the config value or the default value if not specified
	bool configGetBool(std::wstring key, bool defaultValue) {
		if (1 == config.count(key)) {
			return toBool(config[key]);
		}
		return defaultValue;
	}

	// returns the config value or the default value if not specified
	std::wstring configGet(std::wstring key, std::wstring defaultValue) {
		if (1 == config.count(key)) {
			return config[key];
		}
		return defaultValue;
	}

	// expands environment variables in the input string
	std::wstring expandEnv(const std::wstring& source)
	{
		DWORD len;
		std::wstring result;
		len = ExpandEnvironmentStringsW(source.c_str(), 0, 0);
		if (len == 0) {
			THROW_LAST(L"ExpandEnvironmentStringsW failed");
		}
		result.resize(len);
		len = ExpandEnvironmentStringsW(source.c_str(), &result[0], len);
		if (len == 0) {
			THROW_LAST(L"ExpandEnvironmentStringsW failed");
		}
		result.pop_back(); //Get rid of extra null
		return result;
	}

	// sets environment variable
	void env(const std::wstring& name, const std::wstring& value){
		if (0 == SetEnvironmentVariableW(name.c_str(), value.c_str())) {
			THROW_LAST(L"SetEnvironmentVariableW failed");
		}
	}

	// gets environment variable, throws error if it does not exist
	std::wstring env(const wchar_t* name) {
		const DWORD buffSize = 65535;
		static wchar_t buffer[buffSize];
		if (GetEnvironmentVariableW(name, buffer, buffSize))
		{
			return std::wstring(buffer);
		}
		else
		{
			THROW_LAST(L"GetEnvironmentVariableW failed");
		}
	}

	// get time with millisecond precision as iso format string
	std::string timeIso() {
		SYSTEMTIME stime;
		FILETIME ltime;
		FILETIME ftTimeStamp;
		char buffer[256];
		GetSystemTimeAsFileTime(&ftTimeStamp); //Gets the current system time
		FileTimeToLocalFileTime(&ftTimeStamp, &ltime); //convert in local time and store in ltime
		FileTimeToSystemTime(&ltime, &stime); //convert in system time and store in stime
		sprintf(buffer, "%04d-%02d-%02dT%02d%02d%02d.%03d",
			stime.wYear,
			stime.wMonth,
			stime.wDay,
			stime.wHour,
			stime.wMinute,
			stime.wSecond,
			stime.wMilliseconds);
		return std::string(buffer);
	}

	// split the string on delim, stops after finding max parts, remiander of the string will be put in the last part
	// if max is 0 continue to the end
	std::vector<std::wstring> &split(const std::wstring &s, wchar_t delim, size_t max, std::vector<std::wstring> &elems) {
		std::wstringstream ss(s);
		std::wstring item;
		size_t count = 0;
		while (std::getline(ss, item, delim)) {
			elems.push_back(item);
			if (max != 0 && ++count == max) {
				if (!ss.eof()) {
					std::istreambuf_iterator<wchar_t> eos;
					std::wstring final(std::istreambuf_iterator<wchar_t>(ss), eos);
					elems.push_back(final);
				}
				return elems;
			}
		}

		return elems;
	}

	// split the string on delim
	std::vector<std::wstring> split(const std::wstring &s, wchar_t delim, size_t max) {
		std::vector<std::wstring> elems;
		split(s, delim, max, elems);
		return elems;
	}

	// split the string on delim
	std::vector<std::wstring> split(const std::wstring &s, wchar_t delim) {
		std::vector<std::wstring> elems;
		split(s, delim, 0, elems);
		return elems;
	}

	// trim whitespace from start
	static inline std::wstring &ltrim(std::wstring &s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
		return s;
	}

	// trim whitespace from end
	static inline std::wstring &rtrim(std::wstring &s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
		return s;
	}

	// trim whitespace from both ends
	static inline std::wstring &trim(std::wstring &s) {
		return ltrim(rtrim(s));
	}

	// trim chars in supplied string  from start
	static inline std::wstring &ltrim(std::wstring &s, const wchar_t* chars) {
		size_t startpos = s.find_first_not_of(chars);
		if (std::wstring::npos != startpos)
		{
			s = s.substr(startpos);
		}

		return s;
	}

	// trim chars in supplied string  from end
	static inline std::wstring &rtrim(std::wstring &s, const wchar_t* chars) {
		size_t endpos = s.find_last_not_of(chars);
		if (std::wstring::npos != endpos)
		{
			s = s.substr(0, endpos + 1);
		}

		return s;
	}

	// trim chars in supplied string from both ends
	static inline std::wstring &trim(std::wstring &s, const wchar_t* chars) {
		return ltrim(rtrim(s, chars), chars);
	}

	// get high precision time as long long
	unsigned long long nanoTime() {
		FILETIME ft;
		GetSystemTimeAsFileTime(&ft);
		unsigned long long tt = ft.dwHighDateTime;

		tt <<= 32;
		tt |= ft.dwLowDateTime;
		tt /= 10;
		tt -= 11644473600000000ULL;
		return tt;
	}

	// Returns the path to a the windows temp dir with trailing slash
	std::wstring getTemp() {
		TCHAR temp[MAX_PATH];
		DWORD result = ::GetTempPath(0, temp);
		if(result == 0) {
			throw std::runtime_error("Could not get system temp path");
		}

		// Allocate temporary buffer. The retured length includes the terminating _T('\0').
		// std::vector is guaranteed to be sequential, thus can serve as a buffer that can be written to.
		std::vector<TCHAR> tempv(result);

		// If the buffer is large enough, the return value does _not_ include the terminating _T('\0')
		result = ::GetTempPath(static_cast<DWORD>(tempv.size()), &tempv[0]);
		if((result == 0) || (result > tempv.size())) {
			throw std::runtime_error("Could not get system temp path");
		}

		return std::wstring(tempv.begin(), tempv.begin() + static_cast<std::size_t>(result));
	}

	// gets the current working directory with trailing slash
	std::wstring getCwd() {
		wchar_t* cwd = _wgetcwd(NULL, 0);
		std::wstring out(cwd);
		free(cwd);
		return out;
	}

	// join the string with delim
	std::wstring join(std::vector<std::wstring> elems, std::wstring delim) {
		std::wstringstream ss;
		for(size_t i = 0; i < elems.size(); ++i)
		{
			if(i != 0) {
				ss << delim;
			}

			ss << elems[i];
		}

		return ss.str();
	}


	// extract bytes from file to file at dest
	void extractFile(FILE *dest, FILE *file, const size_t pos, const size_t size) {
		size_t bytes;
		char* buf = new char[BUFFER_SIZE];

		fseek(file, pos, SEEK_SET);

		size_t toWrite = size;
		while (bytes = fread(buf, sizeof(char), min(BUFFER_SIZE, toWrite), file)) {
			size_t x = min(BUFFER_SIZE, toWrite);
			fwrite(buf, sizeof(char), bytes, dest);
			toWrite -= bytes;
		}

		delete[] buf;
	}

	// extract bytes from file to string at dest
	std::wstring extractString(FILE *file, const size_t pos, const size_t size) {
		char* buf = new char[size];
		fseek(file, pos, SEEK_SET);
		fread(buf, sizeof(char), size, file);
		std::wstring value = toWstring(std::string(buf, size));
		delete[] buf;
		return value;
	}

	// read the config data, into config map, keys will be lowercase always
	void readConfig(std::wstring data) {
		std::vector<std::wstring> lines = split(data, '\n');
		for (size_t i = 0; i < lines.size(); i++) {
			std::vector<std::wstring> line = split(lines[i], '=', 1);
			std::wstring key = toLower(trim(line[0]));
			if (line.size() > 1 && key.length() != 0) {
				if (config.count(key) == 0) {
					std::wstring warg = trim(line[1]);
					config[key] = warg;
				}
			}
		}
	}

	// returns a list of the files and directories in the path
	// (not including .. and not . (although i never saw .))
	std::vector<std::wstring> listDir(std::wstring path)
	{
		std::vector<std::wstring> paths;
		WIN32_FIND_DATAW data;
		HANDLE hFile = FindFirstFileW((path + L"\\*").c_str(), &data);

		if (hFile == INVALID_HANDLE_VALUE) {
			throw std::runtime_error("Invalid folder path.");
		}

		while(FindNextFileW(hFile, &data) != 0 || GetLastError() != ERROR_NO_MORE_FILES)
		{
			if (0 != wcscmp(data.cFileName, L"..") && 0 != wcscmp(data.cFileName, L".")) {
				paths.push_back(data.cFileName);
			}
		}

		FindClose(hFile);

		return paths;
	}

	// trim \ from end of a path
	static std::wstring trimSlash(std::wstring s) {
		if (s[s.size() - 1] == L'\\') {
			return std::wstring(s.begin(), s.end() - 1);
		} else {
			return s;
		}
	}

	// deletes the path, whether it is a file or directory
	// please be very careful what you pass this!
	// returns true if the item is no longer present when this has exited
	bool rmpath(std::wstring path, bool recusive) {
		struct _stat buf;
		std::wstring trimmedPath = trimSlash(path);
		_wstat(trimmedPath.c_str(), &buf);

		if ((buf.st_mode & _S_IFREG) != 0) {
			return (0 == _wunlink(trimmedPath.c_str()));
		} else if ((buf.st_mode & _S_IFDIR) != 0) {
			std::vector<std::wstring> list = listDir(trimmedPath);

			if (list.size() == 0) {
				return (0 == _wrmdir(trimmedPath.c_str()));
			} else {
				bool clean = true;
				if (recusive) {
					for (size_t i = 0; i < list.size(); i++) {
						clean = clean & rmpath(trimmedPath + L"\\" + list[i], true);
					}
					if (clean) _wrmdir(trimmedPath.c_str());

					return clean;
				} else {
					return false;
				}
			}
		} else {
			// i presume the path does not exist, in which cast just exit
			return true;
		}
	}

	// ensures all the directories in the specified path exist
	// adds to createdPaths any directories created
	void mkpath(std::wstring path) {
		struct _stat buf;
		int result = _wstat(path.c_str(), &buf);
		if (result == -1 || (buf.st_mode & _S_IFDIR) == 0) {
			std::vector<std::wstring> splitPath = split(path, L'\\');
			splitPath.pop_back();
			mkpath(join(splitPath, L"\\"));
			_wmkdir(path.c_str());
			createdPaths.push_back(path);
		}
	}

	// search for pattern in file, start at position, max positions in elems (if max is 0 all will be returned)
	void findInFile(const std::string &pattern, FILE *file, const size_t start, const size_t max, std::vector<size_t> &elems, const size_t buffer_size) {
		size_t patternLen = pattern.length();
		const char* patternCstr = pattern.c_str();

		// Create a buffer, it must be at least twice the length of the pattern
		if (BUFFER_SIZE < patternLen * 2) {
			throw std::runtime_error("pattern is too long, increase buffer size or reduce pattern size");
		}

		char* buf = new char[BUFFER_SIZE];

		// number of bytes read by fread
		size_t bytes;

		// offset, where to start writing data to inside the buffer
		// normally 0 but if a potential match is found at the end of a buffer it will be
		// copied to start of the buffer and next load will be offset
		size_t offset = 0;

		// the position in the file of the start of the buffer
		size_t pos = start;

		// get to the start spot
		fseek(file, pos, SEEK_SET);

		// copy a chunk to the buffer
		while (bytes = fread(buf + offset, sizeof(char), BUFFER_SIZE - offset, file)){
			if (bytes == 0) {
				break;
			}

			// start searching at the beginning of the buffer
			// cursor is current position in buffer
			char* cursor = buf;

			while (true) {
				// look in the buffer for first char
				cursor = (char*)memchr(cursor, pattern[0], offset + bytes - (cursor - buf));
				if (cursor == NULL) {
					// not found, load next chunk into buffer
					pos += bytes + offset;
					offset = 0;
					break;
				}

				// found first char
				char *end = buf + offset + bytes;

				// if it too near the end to be a match
				if ((size_t)(end - cursor) < patternLen) {
					offset = end - cursor;
					// copy end to start of buffer
					memcpy(buf, cursor, offset);
					pos += cursor - buf;
					break;
				} else {
					// if it a match
					if (0 == memcmp(cursor, patternCstr, patternLen)) {
						// found one, return value
						elems.push_back(pos + cursor - buf);
						if (max != 0 && elems.size() == max) {
							goto exit;
						}
					}

					// look for next first char
					*cursor++;
				}
			}
		}

		exit:
		delete[] buf;
	}

	// overloaded findInFile that returns a vector rather than needing one
	// to be passed by reference
	std::vector<size_t> findInFile(const std::string &pattern, FILE *file, const size_t pos, const size_t max, const size_t buffer_size) {
		std::vector<size_t> elems;
		findInFile(pattern, file, pos, max, elems, buffer_size);
		return elems;
	}

	// overloaded findInFile with default buffer size of 8MB
	// and returns a vector rather than needing one to be passed by reference
	std::vector<size_t> findInFile(const std::string &pattern, FILE *file, const size_t pos, const size_t max) {
		return findInFile(pattern, file, pos, max, BUFFER_SIZE);
	}

	// overloaded findInFile with default buffer size of 8MB
	// and just returns the position of the match or -1 if none found
	size_t findInFile(const std::string &pattern, FILE *file, const size_t pos) {
		std::vector<size_t> elems;
		findInFile(pattern, file, pos, 1, elems, BUFFER_SIZE);
		if (elems.size() == 0) {
			return -1;
		} else {
			return elems[0];
		}
	}

	// takes a relative path and turns it into an absolute path
	std::wstring resolvePath(std::wstring path) {
		wchar_t full[_MAX_PATH];
		if (_wfullpath(full, path.c_str(), _MAX_PATH) != NULL) {
			return full;
		}
		else{
			THROW_LAST("Invalid path");
		}
	}

	// unless the path starts with .\ ./ ..\ ../ \\ or have second char : the will be treated as relative
	// to the NODE2EXE_DEST folder
	std::wstring resolvePathSpecial(std::wstring path) {
		if (path.substr(0, 2) == L".\\" ||
			path.substr(0, 2) == L"./" ||
			path.substr(0, 2) == L"..\\" ||
			path.substr(0, 2) == L"../" ||
			path.substr(0, 2) == L"\\\\" ||
			path.substr(1, 1) == L":") {
			return resolvePath(path);
		} else {
			return resolvePath(expandEnv(L"%NODE2EXE_DEST%\\") + path);
		}
	}

	// returns true if the path exists already
	bool pathExists(std::wstring path) {
		struct _stat fi;
		return 0 == _wstat(path.c_str(), &fi);
	}

	// returns the dirname of a path, no trailing slash
	std::wstring dirname(std::wstring path) {
		wchar_t drive[MAX_PATH];
		wchar_t dir[MAX_PATH];

		_wsplitpath_s(
			path.c_str(),
			drive, MAX_PATH,
			dir, MAX_PATH,
			nullptr, 0,
			nullptr, 0);
		return rtrim(std::wstring(drive) + dir, L"\\");
	}

	// extract part of the exe, deciding on the name and path first
	void extractExePart(const size_t count, FILE *file, const size_t start, const size_t end) {
		env(L"NODE2EXE_DEST", rtrim(expandEnv(configGet(L"temp", L"%TEMP%\\node2exe\\%NODE2EXE_ISODATE%\\")), L"\\"));

		std::wstring dest_path;
		if (count == 0) {
			dest_path = resolvePathSpecial(expandEnv(configGet(L"name_0", L"node2exe.js")));
			env(L"NODE2EXE_SCRIPT", dest_path);
		} else {
			std::wstring key = L"name_" + std::to_wstring((long)count);

			dest_path = resolvePathSpecial(expandEnv(configGet(key, L"" + std::to_wstring((long)count))));
		}

		// debugging code
		//std::wcout << dest_path << L"\n";
		//std::cout << "extract " << start << ' ' << end << '\n';

		// test for file exists
		if (!pathExists(dest_path) || configGetBool(L"overwrite", false)) {
			// ensure the directory to unpack to exits
			try {
				mkpath(dirname(dest_path));
			}
			catch (...) {
				std::wcerr << L"Can't create path: " << env(L"NODE2EXE_DEST") << std::endl;
				exit(9999);
			}

			FILE* dest;
			_wfopen_s(&dest, dest_path.c_str(), L"wb");
			extractFile(dest, file, start, end - start);
			fclose(dest);
			createdPaths.push_back(dest_path);
		} //else {
		//	std::wcout << "skipping, file exists " << dest_path << L"\n";
		//}
	}

	// seek through the bytes of the currently executing file for boundary
	// when found save the parts to files named node2exe.js, 1, 2, 3, 4 ...
	// until no more boundaries are found
	bool unpackExe(FILE *file) {
		bool retVal = false;

		// splitting the pattern into front + const is done here
		// to ensure this executable doesn't
		// get an instances of this pattern after compile
		const std::string front("*/\r\n*/'\"");
		const std::string boundary(front + "NODE2EXE\r\n*/\r\n");

		// current position in the file, i could start searching
		// at about 1800000 because even a upx compressed node2exe
		// doesn't get this small, however this fast enough that I'll
		// start the search at 0 and reduce any risk of node2exe being smaller
		// perhaps because different compile options are used
		size_t cur = 0;
		size_t pos = -1;
		size_t count = -1; // -1 is for future config section, 0 is node2exe.js
		while (true) {
			pos = findInFile(boundary, file, cur);

			// if no match found stop
			if (pos == -1) break;

			// if cur == 0 it's the node2exe part don't export that
			if (cur != 0) {
				// found a boundary immediately after a boundary set cur
				// and stop unless it's a zero byte config section
				if (cur == pos && count != -1) {
					cur = pos + boundary.length();
					break;
				}

				if (count == -1) {
					// code to read the config section to go here
					// don't need to extract to file
					readConfig(extractString(file, cur, pos - cur));
					count++;
				} else {
					extractExePart(count++, file, cur, pos);
				}
			}

			// search for another part
			cur = pos + boundary.length();
		}

		// if cur == 0 no boundaries exist in the file
		// count will stay at 0
		if (cur != 0) {
			if (count == -1) {
				// code to read the config section to go here
				// don't need to extract to file
				readConfig(extractString(file, cur, pos - cur));
				count++;
			} else {
				// there is one last part to get
				fseek(file, 0, SEEK_END);
				size_t file_size = ftell(file);
				extractExePart(count++, file, cur, file_size);
			}
		}

		return count > 0;
	}

	// clean up the temp folder
	void cleanUp() {
		if (!configIs(L"cleanup", L"none")) {
			for (int i = createdPaths.size() - 1; i > -1 ; --i)
			{
				if (!rmpath(createdPaths[i], configIs(L"cleanup", L"force"))) {
					std::wcout << L"Path not removed: " << createdPaths[i] << std::endl;
				}
			}
		}
	}

	// handle CTRL+C, close etc.
	// whatever happened clean up
	BOOL WINAPI consoleHandler(DWORD signal) {
		cleanUp();
		return FALSE;
	}

	// node2exe's start decides on a temp folder,
	// unpacks the file parts, if any then injects node2exe.js as the first argv
	// tries to trap any exit of the program CTRL+C, close etc.
	int start(int argc, char *argv[]) {

		// set some environment variables for use in paths
		node2exe::env(L"NODE2EXE_ISODATE", node2exe::toWstring(node2exe::timeIso()));
		node2exe::env(L"NODE2EXE_NANODATE", std::to_wstring(node2exe::nanoTime()));
		node2exe::env(L"CD", getCwd());

		// get path of this program
		char exePath[MAX_PATH];
		GetModuleFileName(NULL, exePath, MAX_PATH);

		// copy argv into a vector
		std::vector<std::string> argv_temp_all(argv, argv + argc);

		std::vector<std::string> argv_temp;
		for (size_t i = 0; i != argv_temp_all.size(); ++i)
		{
			std::string arg = argv_temp_all[i];

			// if switch starts with --node2exe- it's to override config not to be treated as a config setting
			// e.g. --node2exe-temp=local
			if (toLower(arg.substr(0, 11)) == "--node2exe-") {
				readConfig(toWstring(arg.substr(11)));
			} else {
				argv_temp.push_back(arg);
			}
		}

		// open the current executable as a file
		FILE *file = fopen(exePath, "rb");

		if (!SetConsoleCtrlHandler(consoleHandler, TRUE)) {
			std::cerr << "Error: " << GetLastError() << '\n';
			return -1;
		}

		int ret = -1;
		if (unpackExe(file)) {
			// boundaries were found so inject node2exe.js argument
			fclose(file);

			if (configGetBool(L"execute", true)) {
				// insert node2exe.js path as second argv, there's a problem here
				// if the path has got non-ascii characters
				std::wstring w = env(L"NODE2EXE_SCRIPT");
				argv_temp.insert(argv_temp.begin() + 1, std::string(w.begin(), w.end()));

				// convert the vector back to char*[]
				std::vector<char *> argv2(argv_temp.size() + 1);    // one extra for the null
				for (std::size_t i = 0; i != argv_temp.size(); ++i)
				{
					argv2[i] = &argv_temp[i][0];
				}

				// Now that conversion is done, we can finally start.
				ret = node::Start_orig(argv2.size() - 1, &argv2[0]);
			} else {
				ret = 0;
			}

			cleanUp();
		} else {
			// no boundaries were found so carry on as normal node
			fclose(file);

			// convert the vector back to char*[]
			std::vector<char *> argv2(argv_temp.size() + 1);    // one extra for the null
			for (std::size_t i = 0; i != argv_temp.size(); ++i)
			{
				argv2[i] = &argv_temp[i][0];
			}

			ret = node::Start_orig(argv2.size() - 1, &argv2[0]);
		}

		// code to list the config settings
		////for (std::map<std::string, std::string>::iterator iter = config.begin(); iter != config.end(); ++iter) {
		////	std::cout << iter->first << ":" << iter->second << "\n";
		////}

		return ret;
	}
}

namespace node {
	int Start(int argc, char *argv[]) {
		return node2exe::start(argc, argv);
	}

	// an exit method that does clean up first
	void exit(int exitCode) {
		node2exe::cleanUp();
		std::exit(exitCode);
	}

	// an exit method that does clean up first
	void _exit(int exitCode) {
		node2exe::cleanUp();
		_exit(exitCode);
	}
}

#endif  // SRC_NODE2EXE_H_
