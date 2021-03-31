#pragma once

#include <string>
#include <vector>
#include <fstream>
#include "types.h"
#include "utildef.h"
#include "smartptr.h"

// --- BIG HEADER
// .BIG signature (4 bytes) - it must be 0x46474942 - 'BIGF'
// .BIG file size (4 bytes)
// FILE HEADER count (4 bytes)
// BIG HEADER + FILE HEADER (count) + LAST HEADER size in bytes (4 bytes)
//
// --- FILE HEADER
// File data offset (4 bytes) - position in the .BIG file where the content of this specific file starts
// File data size (4 bytes)
// File name - null terminated string
//
// --- LAST HEADER
// Unknown value (4 bytes)
// Unknown value (4 bytes)

class CBIGFile
{
public:
	typedef std::vector<char> TData;

	struct SDataRef : public _reference_target_t
	{
		explicit SDataRef(const TData& data) : data(data) {}

		TData data;
	};
	
	typedef _smart_ptr<SDataRef> TDataPtr;
	typedef std::vector<TDataPtr> TDataPtrVector;
	typedef uint32 TFlags;

	enum EFlags : uint32
	{
		eFlags_None               = 0,
		eFlags_Read               = BIT(1),
		eFlags_Write              = BIT(2),
		eFlags_UseSimplifiedName  = BIT(3),
		eFlags_IgnoreDuplicates   = BIT(4),
		eFlags_WriteOutOnDestruct = BIT(5),
	};

private:
	struct SBigHeader
	{
		inline SBigHeader()
			: bigf(0)
			, bigFileSize(0)
			, fileCount(0)
			, headerSize(0)
		{}

		static uint32 GetSignature()
		{
			return 'BIGF';
		}

		inline bool IsGood() const
		{
			return bigf == GetSignature();
		}

		inline bool HasExpectedFileSize(uint64 size) const
		{
			return bigFileSize == size;
		}

		inline uint32 FileHeaderSize() const
		{
			return headerSize - this->SizeOnDisk() - SBigLastHeader::SizeOnDisk();
		}

		static uint32 SizeOnDisk()
		{
			return sizeof(SBigHeader);
		}

		uint32 bigf;        // File identifier 'BIGF'
		uint32 bigFileSize; // Entire file size in bytes
		uint32 fileCount;   // count of SBigFileHeader
		uint32 headerSize;  // SBigHeader + SBigFileHeader * fileCount + SBigLastHeader
	};

	struct SBigFileHeader
	{
		inline SBigFileHeader()
			: offset(0)
			, size(0)
			, name()
		{}

		inline bool IsGood() const
		{
			return size != 0;
		}

		inline size_t SizeOnDisk() const
		{
			return sizeof(offset) + sizeof(size) + name.size() + 1;
		}

		uint32 offset;              // Offset in bytes where file content starts in .big file
		uint32 size;                // Size in bytes of the file this header refers to
		std::string name;           // File name that this header refers to and is reflected in the actual .big file
	};

	struct SBigFileHeaderEx : public SBigFileHeader
	{
		inline SBigFileHeaderEx()
			: SBigFileHeader()
			, simplifiedName()
			, ignore(false)
			, physical(false)
		{}

		std::string simplifiedName; // File name that this header refers to and is reflected to the user of this class
		bool ignore;                // Whether or not this header is ignored for access
		bool physical;              // Whether ot not this file already exists on disk
	};

	struct SBigLastHeader
	{
		inline SBigLastHeader()
			: unknown1(0)
			, unknown2(0)
		{}

		static uint32 SizeOnDisk()
		{
			return sizeof(SBigLastHeader);
		}

		uint32 unknown1;
		uint32 unknown2;
	};

	typedef std::vector<SBigFileHeader> TBigFileHeaders;
	typedef std::vector<SBigFileHeaderEx> TBigFileHeadersEx;
	typedef std::vector<uint32> TIntegers;

	struct SBigFullHeader
	{
		template <typename TBigFullHeader>
		inline void Copy(const TBigFullHeader& other)
		{
			const size_t count = other.fileHeaders.size();
			bigHeader = other.bigHeader;
			lastHeader = other.lastHeader;
			fileHeaders.resize(count);
			for (size_t i = 0; i < count; ++i)
				fileHeaders[i] = other.fileHeaders[i];
		}
		void Clear();

		SBigHeader bigHeader;
		TBigFileHeaders fileHeaders;
		SBigLastHeader lastHeader;
	};

	struct SBigFullHeaderEx
	{
		void Clear();

		SBigHeader bigHeader;
		TBigFileHeadersEx fileHeaders;
		SBigLastHeader lastHeader;
	};

public:
	CBIGFile();
	~CBIGFile();

	bool OpenFile(const wchar_t* wcsBigFileName, TFlags flags);
	void CloseFile();

	bool IsOpen() const;
	uint32 GetFileCount() const;

	bool        SetFileNameById(uint32 id, const char* szName);
	const char* GetFileNameById(uint32 id) const;
	void        SetCurrentFileId(uint32 id);
	uint32      GetCurrentFileId() const;

	bool        SetCurrentFileName(const char* szName);
	const char* GetCurrentFileName() const;
	const char* GetFirstFileName();
	const char* GetNextFileName();

	bool AddNewFile(const char* szName, const TData& data, bool immediateWriteOut = false);
	bool AddNewFile(uint32 id, const char* szName, const TData& data, bool immediateWriteOut = false);

	bool ReadFileDataById(uint32 id, TData& data);
	bool WriteFileDataById(uint32 id, const TData& data, bool immediateWriteOut = false);

	bool ReadDataFromCurrentFile(TData& data);
	bool WriteDataToCurrentFile(const TData& data, bool immediateWriteOut = false);

	bool HasPendingFileChanges() const;
	bool WriteOutPendingFileChanges();
	void ClearPendingFileChanges();

	static bool HasBigFileExtension(const wchar_t* wcsBigFileName);

	static const char* GetSimplifiedCharset();
	static void ApplySimplifiedCharset(std::string& str);

private:
	void OpenFileStream();
	void CloseFileStream();

	bool BuildFromFileStream();
	bool BuildDefault();

	SBigFileHeaderEx* GetFileHeader(uint32 id);
	const SBigFileHeaderEx* GetFileHeader(uint32 id) const;

	static bool ReadDataFromStream(TData& data, std::istream& istream, uint32 offset = 0u);
	static bool WriteDataToStream(const TData& data, std::ostream& ostream, uint32 offset = 0xFFFFFFFFu);

	static bool ReadBigHeaderFromData(SBigHeader& bigHeader, const TData& data);
	static bool WriteBigHeaderToData(TData& data, const SBigHeader& bigHeader);

	static bool ReadFileHeadersFromData(TBigFileHeadersEx& fileHeaders, const SBigHeader& bigHeader, const TData& data, uint32 offset = 0u, TFlags flags = eFlags_None);
	static bool WriteFileHeadersToData(TData& data, const TBigFileHeadersEx& fileHeaders, uint32 offset = 0u);

	static bool ReadLastHeaderFromData(SBigLastHeader& lastHeader, const TData& data, uint32 offset = 0u);
	static bool WriteLastHeaderToData(TData& data, const SBigLastHeader& lastHeader, uint32 offset = 0u);

	static void BuildFileHeaderIndices(TIntegers& fileHeaderIndices, const TBigFileHeadersEx& fileHeaders);
	static void BuildBigHeaderAndFileHeaders(SBigHeader& bigHeader, TBigFileHeadersEx& fileHeaders, const TDataPtrVector& fileDataVector);

	static uint32 GetSizeOnDisk(const TBigFileHeadersEx& fileHeaders);
	static uint32 GetMaxFileSize(const TBigFileHeadersEx& fileHeaders);

private:
	static const char s_simplified_charset[256];

	SBigFullHeaderEx m_workingHeader;
	SBigFullHeader m_physicalHeader;

	// File data that exists in memory only and can be written to .big file
	TDataPtrVector m_fileDataVector;

	// Contains indexes to all usable files inside .big file
	TIntegers m_fileHeaderIndices;

	std::wstring m_bigFileName;
	std::fstream m_fstream;

	uint32 m_fileId;
	TFlags m_flags;
	bool m_hasPendingFileChanges;
};
