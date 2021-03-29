#pragma once

#include <utility>
#include "platform.h"
#include "FileAccess.h"
#include "BIGFile.h"


struct SFileDescription
{
	SFileDescription()
		: path()
		, name()
		, simplifiedName()
		, hierarchy(0)
		, isBigFile(false)
	{}

	void Swap(SFileDescription& other)
	{
		std::swap(path, other.path);
		std::swap(name, other.name);
		std::swap(simplifiedName, other.simplifiedName);
		std::swap(hierarchy, other.hierarchy);
		std::swap(isBigFile, other.isBigFile);
	}

	std::wstring path;
	std::string name;
	std::string simplifiedName;
	uint32 hierarchy;
	bool isBigFile;
};

class CFileFinder
{
private:
	enum : uint32
	{
		InvalidFileId = ~0u,
	};

	typedef std::vector<SFileDescription> TFiles;

public:
	typedef std::vector<char> TData;
	typedef CBIGFile::TFlags TFlags;

public:
	CFileFinder();
	~CFileFinder();

	bool Initialize(const wchar_t* wcsRootdir, TFlags flags);
	void Clean();

	uint32 GetFileCount() const { return m_files.size(); }

	const SFileDescription* GetCurrentFileDescription() const;
	
	const char* GetFileNameById(uint32 fileId, uint32 subFileId = InvalidFileId);
	const char* GetCurrentFileName();
	const char* GetCurrentTopFileName();
	const char* GetFirstFileName();
	const char* GetNextFileName();

	fileaccess::EError ReadDataFromCurrentFile(TData& data);
	fileaccess::EError WriteDataToCurrentFile(const TData& data, bool immediateWriteOut = false);

	bool HasPendingFileChanges() const;
	bool WriteOutPendingFileChanges();
	void ClearPendingFileChanges();

private:
	void InitializeInternal(const wchar_t* wcsRootdir);

	static void PopulateFilesFromRoot(TFiles& loseFiles, TFiles& bigFiles, const wchar_t* wcsRootdir, const wchar_t* wcsSubdir, CBIGFile::TFlags bigFlags, const uint32 maxHierarchy, uint32 hierarchy = 0);
	static bool BuildFileDescription(SFileDescription& fileDesc, const wchar_t* fileName, const wchar_t* wcsRootdir, const wchar_t* wcsSubdir, CBIGFile::TFlags bigFlags, uint32 hierarchy);
	
	static bool SortFilepathAlphabetical(SFileDescription& left, SFileDescription& right);
	static void AddTrailingPathSeparator(std::wstring& str);
	static bool AppendWideString(std::string& str, const wchar_t* wideStr);

	CBIGFile m_bigFile;
	TFlags m_bigFlags;
	uint32 m_bigFileId;

	TFiles m_files;
	uint32 m_fileId;
	uint32 m_subFileId;
};
