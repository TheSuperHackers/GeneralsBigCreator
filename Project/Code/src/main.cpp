#include "BIGFile.h"
#include "FileFinder.h"
#include "commandline.h"
#include <ctime>
#include <iostream>

#pragma comment(lib, "Shlwapi.lib")

#define COMMANDLINE_ARG_HELP             "-help"
#define COMMANDLINE_ARG_SIMPLIFYNAME     "-simplifyname"
#define COMMANDLINE_ARG_IGNOREDUPLICATES "-ignoreduplicates"
#define COMMANDLINE_ARG_SOURCE           "-source"
#define COMMANDLINE_ARG_DESTINATION      "-dest"


void InitRandom()
{
	::srand((unsigned int)::time(0));
}

void Welcome()
{
	std::cout << "Generals Big Creator 1.0 by xezon" << std::endl;
	std::cout << "---------------------------------" << std::endl;
}

void ExtractBigFile(const wchar_t* wcsSrc, const wchar_t* wcsDst, bool simplifyNames, bool ignoreDuplicates)
{
	CBIGFile::TFlags bigFlags = CBIGFile::eFlags_Read;
	bigFlags |= simplifyNames ? CBIGFile::eFlags_UseSimplifiedName : 0;
	bigFlags |= ignoreDuplicates ? CBIGFile::eFlags_IgnoreDuplicates : 0;

	// TODO: Implement
}

void CreateBigFile(const wchar_t* wcsSrc, const wchar_t* wcsDst, bool simplifyNames, bool ignoreDuplicates)
{
	CBIGFile::TFlags bigFlags = CBIGFile::eFlags_Write;
	bigFlags |= simplifyNames ? CBIGFile::eFlags_UseSimplifiedName : 0;
	bigFlags |= ignoreDuplicates ? CBIGFile::eFlags_IgnoreDuplicates : 0;

	CBIGFile bigFile;
	if (bigFile.OpenFile(wcsDst, bigFlags))
	{
		CFileFinder fileFinder;
		fileFinder.Initialize(wcsSrc, 0);

		const char* szFileName = fileFinder.GetFirstFileName();
		
		do
		{
			if (!(szFileName && *szFileName))
				break;

			CBIGFile::TData data;
			if (fileFinder.ReadDataFromCurrentFile(data) == fileaccess::eError_Success)
			{
				bigFile.AddNewFile(szFileName, data);
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
	const bool simplifyNames = commandline.HasArg(W(COMMANDLINE_ARG_SIMPLIFYNAME));
	const bool ignoreDuplicates = commandline.HasArg(W(COMMANDLINE_ARG_IGNOREDUPLICATES));
	const wchar_t* wcsSrc = commandline.FindArgAssignment(W(COMMANDLINE_ARG_SOURCE));
	const wchar_t* wcsDst = commandline.FindArgAssignment(W(COMMANDLINE_ARG_DESTINATION));

	if (!wcsSrc || !wcsDst)
	{
		help = true;
	}

	if (help)
	{
		std::cout
			<< "Help:" << std::endl
			<< COMMANDLINE_ARG_SOURCE           " [PATH|FILE.BIG] > Specifies path to create .big from or big file to extract from" << std::endl
			<< COMMANDLINE_ARG_DESTINATION      " [PATH|FILE.BIG] > Specifies path to extract to or big file to create .big to" << std::endl
			<< COMMANDLINE_ARG_SIMPLIFYNAME     " > Simplify working character set in big file" << std::endl
			<< COMMANDLINE_ARG_IGNOREDUPLICATES " > Ignore duplicates in big file" << std::endl;
	}

	if (!wcsSrc)
	{
		std::cout << "Error: missing argument for " COMMANDLINE_ARG_SOURCE << std::endl;
		return 1;
	}

	if (!wcsDst)
	{
		std::cout << "Error: missing argument for " COMMANDLINE_ARG_DESTINATION << std::endl;
		return 1;
	}

	const bool srcBigFile = CBIGFile::HasBigFileExtension(wcsSrc);
	const bool dstBigFile = CBIGFile::HasBigFileExtension(wcsDst);
	const bool createBigFile = !srcBigFile && dstBigFile;
	const bool extractBigFile = srcBigFile && !dstBigFile;

	if (!(createBigFile || extractBigFile))
	{
		std::cout << "Error: no creation or extraction job" << std::endl;
		return 0;
	}

	// TODO: Add error codes and messages.

	if (extractBigFile)
	{
		ExtractBigFile(wcsSrc, wcsDst, simplifyNames, ignoreDuplicates);
	}

	if (createBigFile)
	{
		CreateBigFile(wcsSrc, wcsDst, simplifyNames, ignoreDuplicates);
	}

	return 0;
}
