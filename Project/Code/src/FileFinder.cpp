#include "FileFinder.h"
#include "utils.h"
#include <Shlwapi.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <algorithm>


CFileFinder::CFileFinder()
: m_bigFlags(CBIGFile::eFlags_None)
, m_bigFileId(InvalidFileId)
, m_fileId(0)
, m_subFileId(InvalidFileId)
{
}

CFileFinder::~CFileFinder()
{
	WriteOutPendingFileChanges();
}

bool CFileFinder::Initialize(const wchar_t* wcsRootdir, TFlags flags)
{
	if (wcsRootdir != NULL)
	{
		m_bigFlags = flags;
		InitializeInternal(wcsRootdir);
		return true;
	}
	return false;
}

void CFileFinder::InitializeInternal(const wchar_t* wcsRootdir)
{
	std::wstring rootdir;
	rootdir.assign(wcsRootdir);
	AddTrailingPathSeparator(rootdir);

	const wchar_t* wcsSubdir = L"";
	
	TFiles loseFiles;
	TFiles bigFiles;

	PopulateFilesFromRoot(loseFiles, bigFiles, rootdir.c_str(), wcsSubdir, m_bigFlags, 1000);

	std::sort(bigFiles.begin(), bigFiles.end(), SortFilepathAlphabetical);

	m_files.swap(loseFiles);
	m_files.insert(m_files.end(), bigFiles.begin(), bigFiles.end());
	m_bigFileId = InvalidFileId;
	m_fileId = 0;
	m_subFileId = InvalidFileId;
}

bool CFileFinder::SortFilepathAlphabetical(SFileDescription& left, SFileDescription& right)
{
	return ::StrCmpW(left.path.c_str(), right.path.c_str()) < 0;
}

void CFileFinder::Clean()
{
	WriteOutPendingFileChanges();

	m_bigFile.CloseFile();
	m_bigFlags = CBIGFile::eFlags_None;
	m_bigFileId = InvalidFileId;
	utils::ClearMemory(m_files);
	m_fileId = 0;
	m_subFileId = InvalidFileId;
}

void CFileFinder::PopulateFilesFromRoot(TFiles& loseFiles, TFiles& bigFiles, const wchar_t* wcsRootdir, const wchar_t* wcsSubdir, CBIGFile::TFlags bigFlags, const uint32 maxHierarchy, uint32 hierarchy)
{
	std::wstring wildcard;
	wildcard.reserve(MAX_PATH);
	wildcard.assign(wcsRootdir).append(wcsSubdir).append(L"*.*");

	WIN32_FIND_DATAW win32fd;
	HANDLE hFile = ::FindFirstFileW(wildcard.c_str(), &win32fd);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		std::wstring newSubdir;
		newSubdir.reserve(MAX_PATH);

		if (hierarchy == 0)
		{
			// When in root directory, look here into files first before going into sub folders

			do
			{
				if (!(win32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					SFileDescription fileDesc;
					if (BuildFileDescription(fileDesc, win32fd.cFileName, wcsRootdir, wcsSubdir, bigFlags, hierarchy))
					{
						TFiles& files = fileDesc.isBigFile ? bigFiles : loseFiles;
						files.push_back(SFileDescription());
						files.back().Swap(fileDesc);
					}
				}
			}
			while (::FindNextFile(hFile, &win32fd));

			::FindClose(hFile);
			hFile = ::FindFirstFileW(wildcard.c_str(), &win32fd);

			do
			{
				if (win32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if (hierarchy < maxHierarchy)
					{
						if (::wcscmp(win32fd.cFileName, L".") == 0)
							continue;
						if (::wcscmp(win32fd.cFileName, L"..") == 0)
							continue;

						newSubdir.assign(wcsSubdir).append(win32fd.cFileName);
						AddTrailingPathSeparator(newSubdir);
						PopulateFilesFromRoot(loseFiles, bigFiles, wcsRootdir, newSubdir.c_str(), bigFlags, maxHierarchy, hierarchy + 1);
					}
				}
			}
			while (::FindNextFile(hFile, &win32fd));

			::FindClose(hFile);
		}
		else
		{
			// When in sub directory, look into other sub folders first

			do
			{
				if (win32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if (hierarchy < maxHierarchy)
					{
						if (::wcscmp(win32fd.cFileName, L".") == 0)
							continue;
						if (::wcscmp(win32fd.cFileName, L"..") == 0)
							continue;

						newSubdir.assign(wcsSubdir).append(win32fd.cFileName);
						AddTrailingPathSeparator(newSubdir);
						PopulateFilesFromRoot(loseFiles, bigFiles, wcsRootdir, newSubdir.c_str(), bigFlags, maxHierarchy, hierarchy + 1);
					}
				}
				else
				{
					SFileDescription fileDesc;
					if (BuildFileDescription(fileDesc, win32fd.cFileName, wcsRootdir, wcsSubdir, bigFlags, hierarchy))
					{
						TFiles& files = fileDesc.isBigFile ? bigFiles : loseFiles;
						files.push_back(SFileDescription());
						files.back().Swap(fileDesc);
					}
				}
			}
			while (::FindNextFile(hFile, &win32fd));

			::FindClose(hFile);
		}
	}
}

bool CFileFinder::BuildFileDescription(SFileDescription& fileDesc, const wchar_t* fileName, const wchar_t* wcsRootdir, const wchar_t* wcsSubdir, CBIGFile::TFlags bigFlags, uint32 hierarchy)
{
	// File names can be converted to ANSI, because the game does not use Unicode file names
	// For simplicity convert name to lower case by using simplified char set
	std::string name;
	name.reserve(120);
	bool ok = true;
	ok = ok && AppendWideString(name, wcsSubdir);
	ok = ok && AppendWideString(name, fileName);
	
	if (ok)
	{
		std::string simplifiedName = name;
		assert(simplifiedName.size() == name.size());

		if (bigFlags & CBIGFile::eFlags_UseSimplifiedName)
		{
			CBIGFile::ApplySimplifiedCharset(simplifiedName);
		}

		std::wstring path;
		path.reserve(MAX_PATH);
		path.assign(wcsRootdir).append(wcsSubdir).append(fileName);

		fileDesc.path.swap(path);
		fileDesc.name.swap(name);
		fileDesc.simplifiedName.swap(simplifiedName);
		fileDesc.hierarchy = hierarchy;

		if (CBIGFile::HasBigFileExtension(fileName))
		{
			fileDesc.isBigFile = true;
		}
		return true;
	}
	return false;
}

const SFileDescription* CFileFinder::GetCurrentFileDescription() const
{
	if (m_fileId < GetFileCount())
	{
		return &m_files[m_fileId];
	}
	return NULL;
}

const char* CFileFinder::GetFileNameById(uint32 fileId, uint32 subFileId)
{
	if (fileId < GetFileCount())
	{
		const SFileDescription& fileDesc = m_files[fileId];

		if (subFileId == InvalidFileId)
		{
			return fileDesc.simplifiedName.c_str();
		}
		else if (fileDesc.isBigFile)
		{
			if (m_bigFile.IsOpen() && fileId != m_bigFileId)
			{
				m_bigFile.CloseFile();
				m_bigFileId = InvalidFileId;
			}

			if (!m_bigFile.IsOpen())
			{
				if (m_bigFile.OpenFile(fileDesc.path.c_str(), m_bigFlags))
				{
					m_bigFileId = fileId;
				}
			}

			if (m_bigFile.IsOpen())
			{
				if (const char* fileName = m_bigFile.GetFileNameById(subFileId))
				{
					return fileName;
				}
			}
		}
	}
	return NULL;
}

const char* CFileFinder::GetCurrentFileName()
{
	return GetFileNameById(m_fileId, m_subFileId);
}

const char* CFileFinder::GetCurrentTopFileName()
{
	return GetFileNameById(m_fileId, InvalidFileId);
}

const char* CFileFinder::GetFirstFileName()
{
	m_fileId = 0;
	m_subFileId = InvalidFileId;
	return GetFileNameById(m_fileId, m_subFileId);
}

const char* CFileFinder::GetNextFileName()
{
	if (m_fileId < GetFileCount())
	{
		if (const char* fileName = GetFileNameById(m_fileId, ++m_subFileId))
		{
			return fileName;
		}

		m_subFileId = InvalidFileId;
		return GetFileNameById(++m_fileId, m_subFileId);
	}
	return NULL;
}

fileaccess::EError CFileFinder::ReadDataFromCurrentFile(TData& data)
{
	if (m_fileId < GetFileCount())
	{
		const SFileDescription& fileDesc = m_files[m_fileId];

		if (m_subFileId == InvalidFileId)
		{
			return fileaccess::ReadDataFromFile(fileDesc.path.c_str(), data);
		}
		else if (fileDesc.isBigFile)
		{
			return m_bigFile.ReadFileDataById(m_subFileId, data)
				? fileaccess::eError_Success
				: fileaccess::eError_ReadError;
		}
	}
	return fileaccess::eError_FileAccessError;
}

fileaccess::EError CFileFinder::WriteDataToCurrentFile(const TData& data, bool immediateWriteOut)
{
	if (m_fileId < GetFileCount())
	{
		const SFileDescription& fileDesc = m_files[m_fileId];

		if (m_subFileId == InvalidFileId)
		{
			return fileaccess::WriteDataToFile(fileDesc.path.c_str(), data);
		}
		else if (fileDesc.isBigFile)
		{
			return m_bigFile.WriteFileDataById(m_subFileId, data, immediateWriteOut)
				? fileaccess::eError_Success
				: fileaccess::eError_WriteError;
		}
	}
	return fileaccess::eError_FileAccessError;
}

bool CFileFinder::HasPendingFileChanges() const
{
	return m_bigFile.HasPendingFileChanges();
}

bool CFileFinder::WriteOutPendingFileChanges()
{
	return m_bigFile.WriteOutPendingFileChanges();
}

void CFileFinder::ClearPendingFileChanges()
{
	m_bigFile.ClearPendingFileChanges();
}

void CFileFinder::AddTrailingPathSeparator(std::wstring& str)
{
	if (const size_t i = str.size())
	{
		if (str[i-1] != L'\\' && str[i-1] != L'/')
		{
			str.push_back(L'\\');
		}
	}
}

bool CFileFinder::AppendWideString(std::string& str, const wchar_t* wideStr)
{
	bool success = true;

	if (wideStr)
	{
		const std::locale locale = std::locale();

		while (*wideStr)
		{
			char c = std::use_facet<std::ctype<wchar_t>>(locale).narrow(*wideStr, '\0');
			if (c == '\0')
			{
				success = false;
				break;
			}
			str.push_back(c);
			wideStr++;
		}
	}

	return success;
}
