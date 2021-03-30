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


void InitRandom()
{
	::srand((unsigned int)::time(0));
}

void Welcome()
{
	std::cout << "Generals Big Creator 1.1 by xezon" << std::endl;
	std::cout << "---------------------------------" << std::endl;
}

void ExtractBigFile(const wchar_t* wcsSrc,
					const wchar_t* wcsDst,
					uint32 maxDepth,
					bool simplifyNames,
					bool ignoreDuplicates)
{
	CBIGFile::TFlags bigFlags = CBIGFile::eFlags_Read;
	bigFlags |= simplifyNames ? CBIGFile::eFlags_UseSimplifiedName : 0;
	bigFlags |= ignoreDuplicates ? CBIGFile::eFlags_IgnoreDuplicates : 0;

	// TODO: Implement
}

void CreateBigFile(const wchar_t* wcsSrc,
				   const wchar_t* wcsWildcard,
				   const wchar_t* wcsDst,
				   uint32 maxDepth,
				   const char* szPrefix,
				   bool simplifyNames,
				   bool ignoreDuplicates)
{
	CBIGFile::TFlags bigFlags = CBIGFile::eFlags_Write;
	bigFlags |= simplifyNames ? CBIGFile::eFlags_UseSimplifiedName : 0;
	bigFlags |= ignoreDuplicates ? CBIGFile::eFlags_IgnoreDuplicates : 0;

	CBIGFile bigFile;
	if (bigFile.OpenFile(wcsDst, bigFlags))
	{
		CFileFinder fileFinder;
		fileFinder.Initialize(wcsSrc, wcsWildcard, maxDepth, CBIGFile::eFlags_None);

		const char* szFileName = fileFinder.GetFirstFileName();
		
		do
		{
			if (!(szFileName && *szFileName))
				break;

			CBIGFile::TData data;
			if (fileFinder.ReadDataFromCurrentFile(data) == fileaccess::eError_Success)
			{
				std::string fullFileName;
				fullFileName.append(szPrefix).append(szFileName);
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
	const bool simplifyNames = commandline.HasArg(W(COMMANDLINE_ARG_SIMPLIFYNAMES));
	const bool ignoreDuplicates = commandline.HasArg(W(COMMANDLINE_ARG_IGNOREDUPLICATES));
	const wchar_t* wcsSrc = commandline.FindArgAssignment(W(COMMANDLINE_ARG_SOURCE));
	const wchar_t* wcsDst = commandline.FindArgAssignment(W(COMMANDLINE_ARG_DEST));
	const wchar_t* wcsPrefixNames = commandline.FindArgAssignment(W(COMMANDLINE_ARG_PREFIXNAMES));
	const wchar_t* wcsMaxDepth = commandline.FindArgAssignment(W(COMMANDLINE_ARG_SOURCEMAXDEPTH));
	const wchar_t* wcsWildcard = commandline.FindArgAssignment(W(COMMANDLINE_ARG_SOURCEWILDCARD));

	if (!wcsSrc || !wcsDst)
	{
		help = true;
	}

	if (help)
	{
		std::cout
		<< "h: " << "ARGUMENT"              "          [TYPE1|TYPE2 {default}]"                                                              << std::endl
		<< "   " << COMMANDLINE_ARG_SOURCE "           [PATH|FILE.BIG {}] -> Specifies path to create .big from or big file to extract from" << std::endl
		<< "   " << COMMANDLINE_ARG_DEST "             [PATH|FILE.BIG {}] -> Specifies path to extract to or big file to create .big to"     << std::endl
		<< "   " << COMMANDLINE_ARG_SIMPLIFYNAMES "    [{}]               -> Simplify file names in big file"                                << std::endl
		<< "   " << COMMANDLINE_ARG_IGNOREDUPLICATES " [{}]               -> Ignore file duplicates in big file"                             << std::endl
		<< "   " << COMMANDLINE_ARG_PREFIXNAMES "      [STRING {}]        -> Prefix file names in created big file"                          << std::endl
		<< "   " << COMMANDLINE_ARG_SOURCEMAXDEPTH "   [NUMBER {999}]     -> Max folder depth to use files from"                             << std::endl
		<< "   " << COMMANDLINE_ARG_SOURCEWILDCARD "   [STRING {*.*}]     -> Filter for file name or extension"                              << std::endl;
	}

	if (!wcsSrc)
	{
		std::cout << "Error: missing argument for " COMMANDLINE_ARG_SOURCE << std::endl;
		return 1;
	}

	if (!wcsDst)
	{
		std::cout << "Error: missing argument for " COMMANDLINE_ARG_DEST << std::endl;
		return 1;
	}

	if (!wcsWildcard)
	{
		wcsWildcard = L"*.*";
	}

	const bool srcBigFile = CBIGFile::HasBigFileExtension(wcsSrc);
	const bool dstBigFile = CBIGFile::HasBigFileExtension(wcsDst);
	const bool createBigFile = !srcBigFile && dstBigFile;
	const bool extractBigFile = srcBigFile && !dstBigFile;
	const uint32 maxDepth = wcsMaxDepth ? static_cast<uint32>(::_wtoi(wcsMaxDepth)) : 999;
	std::string prefixNames;
	
	if (wcsPrefixNames)
	{
		utils::AppendWideString(prefixNames, wcsPrefixNames);
	}

	if (!(createBigFile || extractBigFile))
	{
		std::cout << "Error: no creation or extraction job" << std::endl;
		return 0;
	}

	// TODO: Add error codes and messages.

	if (extractBigFile)
	{
		ExtractBigFile(wcsSrc, wcsDst, maxDepth, simplifyNames, ignoreDuplicates);
	}

	if (createBigFile)
	{
		CreateBigFile(wcsSrc, wcsWildcard, wcsDst, maxDepth, prefixNames.c_str(), simplifyNames, ignoreDuplicates);
	}

	return 0;
}
