#include "FileAccess.h"
#include "strlcpy.h"
#include "utils.h"
#include "platform.h"
#include <Shlwapi.h>
#include <io.h>
#include <errno.h>
#include <fstream>


namespace fileaccess {
namespace {

template <typename TData>
EError ReadDataFromFileInternal(const wchar_t* fileName, TData& data)
{
	try
	{
		std::ifstream file(fileName, std::ios::in | std::ios::ate | std::ios::binary);

		if (file.is_open() && file.good())
		{
			const size_t fileSize = file.tellg();
			if (fileSize != 0)
			{
				data.resize(fileSize);
				file.seekg(0, std::ios::beg);
				file.read(&data[0], data.size());
			}
			return file.good() ? eError_Success : eError_ReadError;
		}
		return eError_FileAccessError;
	}
	catch (std::exception&)
	{
		assert(0);
		return eError_FileException;
	}
}

template <typename TData>
EError WriteDataToFileInternal(const wchar_t* fileName, const TData& data)
{
	try
	{
		std::ofstream file(fileName, std::ios::out | std::ios::ate | std::ios::binary);

		if (file.is_open() && file.good())
		{
			file.write(&data[0], data.size());
			return file.good() ? eError_Success : eError_WriteError;
		}
		return eError_FileAccessError;
	}
	catch (std::exception&)
	{
		assert(0);
		return eError_FileException;
	}
}

template <typename TData>
EError ReadDataFromStreamInternal(std::istream& stream, TData& data, size_t offset)
{
	try
	{
		if (stream.good())
		{
			stream.seekg(0, std::ios::end);
			const size_t size = stream.tellg();
			offset = std::min(offset, size);

			if (size != 0)
			{
				data.resize(size - offset);
				stream.seekg(offset, std::ios::beg);
				stream.read(&data[0], data.size());

				return stream.good() ? eError_Success : eError_ReadError;
			}
		}
		return eError_ReadError;
	}
	catch (std::exception&)
	{
		assert(0);
		return eError_ReadError;
	}
}

template <typename TData>
EError WriteDataToStreamInternal(std::ostream& stream, const TData& data, size_t offset)
{
	try
	{
		if (stream.good())
		{
			stream.seekp(0, std::ios::end);
			const size_t size = stream.tellp();
			offset = std::min(offset, size);

			stream.seekp(offset, std::ios::beg);
			stream.write(&data[0], data.size());

			return stream.good() ? eError_Success : eError_WriteError;
		}
		return eError_WriteError;
	}
	catch (std::exception&)
	{
		assert(0);
		return eError_WriteError;
	}
}

} // namespace


EError ReadDataFromFile(const wchar_t* fileName, TStringData& data)
{
	return ReadDataFromFileInternal(fileName, data);
}

EError ReadDataFromFile(const wchar_t* fileName, TVectorData& data)
{
	return ReadDataFromFileInternal(fileName, data);
}

EError WriteDataToFile(const wchar_t* fileName, const TStringData& data)
{
	return WriteDataToFileInternal(fileName, data);
}

EError WriteDataToFile(const wchar_t* fileName, const TVectorData& data)
{
	return WriteDataToFileInternal(fileName, data);
}

EError ReadDataFromStream(std::istream& stream, TStringData& data, size_t offset)
{
	return ReadDataFromStreamInternal(stream, data, offset);
}

EError ReadDataFromStream(std::istream& stream, TVectorData& data, size_t offset)
{
	return ReadDataFromStreamInternal(stream, data, offset);
}

EError WriteDataToStream(std::ostream& stream, const TStringData& data, size_t offset)
{
	return WriteDataToStreamInternal(stream, data, offset);
}

EError WriteDataToStream(std::ostream& stream, const TVectorData& data, size_t offset)
{
	return WriteDataToStreamInternal(stream, data, offset);
}

void GetHexString(const char* input, size_t len, std::string& output)
{
	static const char* hex = "0123456789abcdef";
	output.clear();
	output.reserve(len*2);

	for (size_t i=0; i<len; ++i)
	{
		unsigned char ch = static_cast<unsigned char>(input[i]);
		output.push_back(hex[ch >> 4]);
		output.push_back(hex[ch & 0xF]);
	}
}

void ConvertToWinPath(wchar_t* filePath, const size_t len)
{
	for (size_t i=0; i<len; ++i)
	{
		if (filePath[i]=='/')
		{
			filePath[i]='\\';
		}
	}
}

void ConvertToWinPath(std::wstring& filePath)
{
	for (size_t i=0, len=filePath.size(); i<len; ++i)
	{
		if (filePath[i]=='/')
		{
			filePath[i]='\\';
		}
	}
}

bool GetModuleDir(std::wstring& path)
{
	wchar_t buffer[MAX_PATH] = {0};

	if (0 != ::GetModuleFileNameW(NULL, buffer, ARRAY_SIZE(buffer)))
	{
		if (::PathRemoveFileSpecW(buffer))
		{
			path.assign(buffer).append(L"\\");
			return true;
		}
	}
	return false;
}

bool GetSystemDir(std::wstring& path)
{
	wchar_t buffer[MAX_PATH] = {0};

	if (0 != ::GetSystemDirectoryW(buffer, ARRAY_SIZE(buffer)))
	{
		path.assign(buffer).append(L"\\");
		return true;
	}
	return false;
}

bool GetWorkingDir(std::wstring& path)
{
	wchar_t buffer[MAX_PATH] = {0};

	if (0 != ::GetCurrentDirectoryW(ARRAY_SIZE(buffer), buffer))
	{
		path.assign(buffer).append(L"\\");
		return true;
	}
	return false;
}

bool GetFileInModuleDir(const wchar_t* fileName, std::wstring& path)
{
	if (GetModuleDir(path))
	{
		path.append(fileName);
		return true;
	}
	return false;
}

bool GetFileInSystemDir(const wchar_t* fileName, std::wstring& path)
{
	if (GetSystemDir(path))
	{
		path.append(fileName);
		return true;
	}
	return false;
}

bool GetFileInWorkingDir(const wchar_t* fileName, std::wstring& path)
{
	if (GetWorkingDir(path))
	{
		path.append(fileName);
		return true;
	}
	return false;
}

bool FileExists(const wchar_t* fileName)
{
	return GetFileAccess(fileName, eAccessMode_Existence) == eAccess_Success;
}

bool FileWritable(const wchar_t* fileName)
{
	return GetFileAccess(fileName, eAccessMode_Write) == eAccess_Success;
}

bool FileReadable(const wchar_t* fileName)
{
	return GetFileAccess(fileName, eAccessMode_Read) == eAccess_Success;
}

EAccess GetFileAccess(const wchar_t* fileName, int accessMode)
{
	const errno_t err = ::_waccess_s(fileName, accessMode);
	switch (err)
	{
	case 0:      return eAccess_Success;
	case EACCES: return eAccess_Denied;
	case ENOENT: return eAccess_NotFound;
	case EINVAL: return eAccess_InvalidParameter;
	default:     return eAccess_UnknownError;
	}
}

int64 GetFileSize(const wchar_t* path)
{
	std::fstream file;
	file.open(path, std::ios::in | std::ios::binary);
	if (!file.is_open())
		return 0;

	file.seekg(0, std::ios::end);
	int64 size = file.tellg();
	file.seekg(0, std::ios::beg);
	file.close();
	return size;
}

int64 GetFileSize(std::fstream& file)
{
	if (!file.is_open())
		return 0;

	file.seekg(0, std::ios::end);
	int64 size = file.tellg();
	file.seekg(0, std::ios::beg);
	return size;
}

bool CreateRecursiveDirectory(const wchar_t* filePath, const int maxLevel)
{
	wchar_t pathCopy[MAX_PATH] = {0};
	::wcslcpy(pathCopy, filePath, ARRAY_SIZE(pathCopy));
	std::vector<std::wstring> pathCollection;
	
	for (int level = 0; ::PathRemoveFileSpecW(pathCopy) && level < maxLevel; ++level)
	{
		pathCollection.push_back(pathCopy);
	}
	for (int i = pathCollection.size()-1; i >= 0; --i)
	{
		if (::PathIsDirectoryW(pathCollection[i].c_str()) == FALSE)
			if (::CreateDirectoryW(pathCollection[i].c_str(), NULL) == FALSE)
				return false;
	}
	return true;
}

} // namespace fileaccess
