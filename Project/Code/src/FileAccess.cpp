#include "FileAccess.h"
#include "utils.h"
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

} // namespace fileaccess
