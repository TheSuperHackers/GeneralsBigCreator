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
	std::cout << "Generals Big Creator 1.1 by xezon" << std::endl;
}

void ExtractBigFile(const SOptions& options)
{
	CBIGFile::TFlags bigFlags = CBIGFile::eFlags_Read;
	bigFlags |= options.simplifyNames ? CBIGFile::eFlags_UseSimplifiedName : 0;
	bigFlags |= options.ignoreDuplicates ? CBIGFile::eFlags_IgnoreDuplicates : 0;

	// TODO: Implement
}

void CreateBigFile(const SOptions& options)
{
	CBIGFile::TFlags bigFlags = CBIGFile::eFlags_Write;
	bigFlags |= options.append ? CBIGFile::eFlags_Read : 0;
	bigFlags |= options.simplifyNames ? CBIGFile::eFlags_UseSimplifiedName : 0;
	bigFlags |= options.ignoreDuplicates ? CBIGFile::eFlags_IgnoreDuplicates : 0;

	CBIGFile bigFile;
	if (bigFile.OpenFile(options.wcsDst, bigFlags))
	{
		bigFile.SetCurrentFileId(~0u);
		CFileFinder fileFinder;
		fileFinder.Initialize(options.wcsSrc, options.wcsWildcard, options.maxDepth, CBIGFile::eFlags_None);

		const char* szFileName = fileFinder.GetFirstFileName();
		
		do
		{
			if (!(szFileName && *szFileName))
				break;

			CBIGFile::TData data;
			if (fileFinder.ReadDataFromCurrentFile(data) == fileaccess::eError_Success)
			{
				std::string fullFileName;
				fullFileName.append(options.szPrefix).append(szFileName);
				bigFile.AddNewFile(fullFileName.c_str(), data);
			}
		}
		while (szFileName = fileFinder.GetNextFileName());
	}
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
		return 1;
	}

	if (!options.wcsDst)
	{
		std::cout << "Error: missing argument for " COMMANDLINE_ARG_DEST << std::endl;
		return 1;
	}

	const bool srcBigFile = CBIGFile::HasBigFileExtension(options.wcsSrc);
	const bool dstBigFile = CBIGFile::HasBigFileExtension(options.wcsDst);
	const bool createBigFile = !srcBigFile && dstBigFile;
	const bool extractBigFile = srcBigFile && !dstBigFile;

	if (!(createBigFile || extractBigFile))
	{
		std::cout << "Error: no creation or extraction job" << std::endl;
		return 0;
	}

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

	// TODO: Add error codes and messages.

	if (extractBigFile)
	{
		ExtractBigFile(options);
	}

	if (createBigFile)
	{
		CreateBigFile(options);
	}

	return 0;
}
