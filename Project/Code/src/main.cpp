#include "BIGFile.h"
#include "FileFinder.h"
#include "commandline.h"
#include "utils.h"
#include <ctime>
#include <iostream>

#pragma comment(lib, "Shlwapi.lib")

#define COMMANDLINE_ARG_HELP             "-help"
#define COMMANDLINE_ARG_SOURCE           "-source"
#define COMMANDLINE_ARG_SOURCEMAXDEPTH   "-sourcemaxdepth"
#define COMMANDLINE_ARG_SOURCEWILDCARD   "-sourcewildcard"
#define COMMANDLINE_ARG_DEST             "-dest"
#define COMMANDLINE_ARG_PREFIXNAMES      "-prefixnames"
#define COMMANDLINE_ARG_SIMPLIFYNAMES    "-simplifynames"
#define COMMANDLINE_ARG_IGNOREDUPLICATES "-ignoreduplicates"
#define COMMANDLINE_ARG_APPEND           "-append"


namespace
{
	int Error = 1;
	int NoError = 0;
}

struct SOptions
{
	SOptions()
		: wcsSrc(0)
		, wcsWildcard(L"*.*")
		, wcsDst(0)
		, maxDepth(999)
		, szPrefix("")
		, simplifyNames(false)
		, ignoreDuplicates(false)
		, append(false)
	{}

	const wchar_t* wcsSrc;
	const wchar_t* wcsWildcard;
	const wchar_t* wcsDst;
	uint32 maxDepth;
	const char* szPrefix;
	bool simplifyNames;
	bool ignoreDuplicates;
	bool append;
};


void InitRandom()
{
	::srand((unsigned int)::time(0));
}

void Welcome()
{
	std::cout << "Generals Big Creator 1.2 by xezon" << std::endl;
}

bool ExtractBigFile(const SOptions& options)
{
	CBIGFile::TFlags bigFlags = CBIGFile::eFlags_Read;
	bigFlags |= options.simplifyNames ? CBIGFile::eFlags_UseSimplifiedName : 0;
	bigFlags |= options.ignoreDuplicates ? CBIGFile::eFlags_IgnoreDuplicates : 0;

	// TODO: Implement.

	std::cout << "Error: not implemented" << std::endl;

	return false;
}

bool CreateBigFile(const SOptions& options)
{
	CBIGFile::TFlags bigFlags = CBIGFile::eFlags_Write;
	bigFlags |= options.append ? CBIGFile::eFlags_Read : 0;
	bigFlags |= options.simplifyNames ? CBIGFile::eFlags_UseSimplifiedName : 0;
	bigFlags |= options.ignoreDuplicates ? CBIGFile::eFlags_IgnoreDuplicates : 0;

	CBIGFile bigFile;
	if (!bigFile.OpenFile(options.wcsDst, bigFlags))
	{
		std::cout << "Error: '" << options.wcsDst << "' cannot be opened" << std::endl;
		return false;
	}

	bigFile.SetCurrentFileId(~0u);
	CFileFinder fileFinder;
	if (!fileFinder.Initialize(options.wcsSrc, options.wcsWildcard, options.maxDepth, CBIGFile::eFlags_None))
	{
		std::cout << "Error: '" << options.wcsSrc << "' '" << options.wcsWildcard << "' cannot be used" << std::endl;
		return false;
	}

	const char* szFileName = fileFinder.GetFirstFileName();
	
	do
	{
		if (!(szFileName && *szFileName))
			break;

		CBIGFile::TData data;
		std::string fullFileName;
		fullFileName.append(options.szPrefix).append(szFileName);

		if (fileFinder.ReadDataFromCurrentFile(data) != fileaccess::eError_Success)
		{
			std::cout << "Error: '" << fullFileName << "' cannot be read" << std::endl;
			return false;
		}

		if (!bigFile.AddNewFile(fullFileName.c_str(), data))
		{
			std::cout << "Error: '" << fullFileName << "' cannot be added to BIG file" << std::endl;
			return false;
		}

		std::cout << "OK '" << fullFileName << "'" << std::endl;

		// TODO: Check pending file size and write out if necessary otherwise process memory will grow large.
	}
	while (szFileName = fileFinder.GetNextFileName());

	if (!bigFile.WriteOutPendingFileChanges())
	{
		std::cout << "Error: '" << options.wcsDst << "' write out failed" << std::endl;
		return false;
	}

	return true;
}

int main(int argc, wchar_t* argv[])
{
	Welcome();

	InitRandom();

	CommandLineRAII commandline;
	bool help = commandline.HasArg(W(COMMANDLINE_ARG_HELP));

	SOptions options;

	options.simplifyNames = commandline.HasArg(W(COMMANDLINE_ARG_SIMPLIFYNAMES));
	options.ignoreDuplicates = commandline.HasArg(W(COMMANDLINE_ARG_IGNOREDUPLICATES));
	options.append = commandline.HasArg(W(COMMANDLINE_ARG_APPEND));
	options.wcsSrc = commandline.FindArgAssignment(W(COMMANDLINE_ARG_SOURCE));
	options.wcsDst = commandline.FindArgAssignment(W(COMMANDLINE_ARG_DEST));
	const wchar_t* wcsPrefixNames = commandline.FindArgAssignment(W(COMMANDLINE_ARG_PREFIXNAMES));
	const wchar_t* wcsMaxDepth = commandline.FindArgAssignment(W(COMMANDLINE_ARG_SOURCEMAXDEPTH));
	const wchar_t* wcsWildcard = commandline.FindArgAssignment(W(COMMANDLINE_ARG_SOURCEWILDCARD));

	if (!options.wcsSrc || !options.wcsDst)
	{
		help = true;
	}

	if (help)
	{
		std::cout
		<< "h: " << "ARGUMENT"              "          [TYPE1|TYPE2 {default}]"                                                             << std::endl
		<< "   " << COMMANDLINE_ARG_SOURCE "           [PATH|FILE.big {}] -> Specifies path to create BIG from or BIG file to extract from" << std::endl
		<< "   " << COMMANDLINE_ARG_SOURCEMAXDEPTH "   [NUMBER {999}]     -> Max folder depth to use files from"                            << std::endl
		<< "   " << COMMANDLINE_ARG_SOURCEWILDCARD "   [STRING {*.*}]     -> Filter for file name or extension"                             << std::endl
		<< "   " << COMMANDLINE_ARG_DEST "             [PATH|FILE.big {}] -> Specifies path to extract to or BIG file to create to"         << std::endl
		<< "   " << COMMANDLINE_ARG_SIMPLIFYNAMES "    [{}]               -> Simplify file names in BIG file"                               << std::endl
		<< "   " << COMMANDLINE_ARG_IGNOREDUPLICATES " [{}]               -> Ignore file duplicates in BIG file"                            << std::endl
		<< "   " << COMMANDLINE_ARG_PREFIXNAMES "      [STRING {}]        -> Prefix file names in created BIG file"                         << std::endl
		<< "   " << COMMANDLINE_ARG_APPEND "           [{}]               -> Append to existing BIG file instead of creating new one"       << std::endl;
	}

	if (!options.wcsSrc)
	{
		std::cout << "Error: missing argument for " COMMANDLINE_ARG_SOURCE << std::endl;
		return Error;
	}

	if (!options.wcsDst)
	{
		std::cout << "Error: missing argument for " COMMANDLINE_ARG_DEST << std::endl;
		return Error;
	}

	const bool srcBigFile = CBIGFile::HasBigFileExtension(options.wcsSrc);
	const bool dstBigFile = CBIGFile::HasBigFileExtension(options.wcsDst);
	const bool createBigFile = !srcBigFile && dstBigFile;
	const bool extractBigFile = srcBigFile && !dstBigFile;

	if (!(createBigFile || extractBigFile))
	{
		std::cout << "Error: no creation or extraction job" << std::endl;
		return Error;
	}

	if (createBigFile)
	{
		if (!fileaccess::FileExists(options.wcsSrc))
		{
			std::wcout << "Error: '" << options.wcsSrc << "' is no valid path" << std::endl;
			return Error;
		}

		if (options.append && !fileaccess::FileExists(options.wcsDst))
		{
			std::wcout << "Error: '" << options.wcsDst << "' is no valid file" << std::endl;
			return Error;
		}
	}

	// TODO: Add error codes and messages.

	std::string prefixNames;

	if (wcsPrefixNames)
	{
		utils::AppendWideString(prefixNames, wcsPrefixNames);
		options.szPrefix = prefixNames.c_str();
	}

	if (wcsMaxDepth)
	{
		options.maxDepth = static_cast<uint32>(::_wtoi(wcsMaxDepth));
	}

	if (wcsWildcard)
	{
		options.wcsWildcard = wcsWildcard;
	}

	bool success = false;

	if (extractBigFile)
	{
		success = ExtractBigFile(options);
	}

	if (createBigFile)
	{
		success = CreateBigFile(options);
	}

	if (success)
	{
		std::cout << "Operation completed" << std::endl;
		return NoError;
	}
	else
	{
		std::cout << "Error: operation failed" << std::endl;
		return Error;
	}
}
