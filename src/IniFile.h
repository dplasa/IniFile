#ifndef _INIFILE_H
#define _INIFILE_H

#define INIFILE_VERSION "2.0.2"

#include <stdint.h>
#include <IPAddress.h>
#include <FS.h>

class IniFileState {
public:
	IniFileState(const IniFileState& _other) = default;
	IniFileState& operator= (const IniFileState& _other)
	{
		_file = _other._file;
		_readLinePosition = _other._readLinePosition;
		_buffer = _other._buffer;
		_len = _other._len;

		_error = _other._error;
		_caseSensitive = _other._caseSensitive;
		return *this;
	}

	IniFileState(File& iniFileObject, char* buffer, size_t len, bool caseSensitiv) :
		 _file(iniFileObject),
		 _buffer(buffer),
		 _len(len),
		 _caseSensitive(caseSensitiv)
	{
		_readLinePosition = 0;
		_error = errorNoError; 
	}

	enum error_t {
		errorNoError = 0,
		errorFileNotOpen,
		errorBufferTooSmall,
		errorSeekError,
		errorSectionNotFound,
		errorKeyNotFound,
		errorEndOfFile,
		errorValueTruncated,
		errorFormatError,
		errorUnknownError,
	};

	inline error_t getError() const
	{
		return _error;
	}

	inline void clearError(void) const
	{
		_error = errorNoError;
	}

	inline bool getCaseSensitive(void) const
	{
		return _caseSensitive;
	}

	inline void setCaseSensitive(bool cs)
	{
		_caseSensitive = cs;		
	}

protected:
	File& _file;
	uint32_t _readLinePosition;
	char* _buffer;
	size_t _len;

	mutable error_t _error;
	bool _caseSensitive;

	friend class IniFile;
};

class IniFileSectionKey;
class IniFileSection : public IniFileState {
public:
	IniFileSection(File& iniFileObject, char* buffer, size_t len, bool caseSensitiv) :
		IniFileState(iniFileObject, buffer, len, caseSensitiv) {}

	IniFileSection(const IniFileState& _iniFileState) : IniFileState(_iniFileState) {}

	IniFileSection& operator=(const IniFileSection& rhs)
	{
		((IniFileState&) (*this))  = (const IniFileState&) rhs;
		return *this;
	}

	// get a key from a section
	IniFileSectionKey findKey(const String& key, bool withinSection=true);
	IniFileSectionKey findKey(const __FlashStringHelper *key, bool withinSection=true);

private:
	typedef int comparefunc(const char*, const char*, size_t);
	char*  __findKey(const char* key, comparefunc* _cmpfunc, bool withinSection=true);

	friend class IniFile;
};

class IniFileSectionKey {
public:
	IniFileSectionKey(char *thebuffer) : _buffer(thebuffer) {}
	IniFileSectionKey(const IniFileSectionKey& other) : _buffer(other._buffer) {}

	// check is key was found
	inline operator bool() const
	{
		return _buffer != NULL;
	}

	inline operator const char*() const
	{
		return _buffer;
	}

	// get it as numeric value
	template <typename T>
	IniFileState::error_t getValue(T&) const;

	// Get an array of numeric bytes with a given seperator,
	// e.g. an IP address 192.168.0.1 with separator "."
	IniFileState::error_t getNumericByteArray(uint8_t* array, size_t& alen, const char* seperators) const;
	// same as above but treat numerbers as hex without prefix
	// e.g. a MAC adress 12:34:56:78:9a:bc
	IniFileState::error_t getHexByteArray(uint8_t* array, size_t& alen, const char* seperators) const;

#if defined(ARDUINO) && ARDUINO >= 100
	IniFileState::error_t getIPAddress(IPAddress& ip) const;
#endif

private:
	char *_buffer;
	friend class IniFile;
	friend class IniFileSection;
	IniFileState::error_t __str_to_numeric(int64_t &value) const;
	IniFileState::error_t __str_to_numeric(uint64_t &value) const;
	IniFileState::error_t __str_to_numeric(uint8_t* array, size_t& alen, const char* seperators, uint8_t base) const;
};

class IniFile : protected IniFileSection {
public:

	// Create an IniFile object. iniFileObject& is an open File object to read from
	IniFile(File& iniFileObject, char* buffer, size_t len, bool caseSensitive = false);
	~IniFile() = default;

	// check if file is valid
	IniFileState::error_t validate() const;

	// find a section from the file
	const IniFileSection& findSection(const String& section);
	const IniFileSection& findSection(const __FlashStringHelper* section);

	// Utility function to read a line from a file, make available to all
	static error_t readLine(File &file, char *buffer, size_t len, uint32_t &pos);
	static bool isCommentChar(char c);
	static char* skipWhiteSpace(char* str);
	static void removeTrailingWhiteSpace(char* str);

private:
	typedef int comparefunc(const char*, const char*, size_t);
	void __findSection(const char* section, comparefunc* _cmpfunc);
};

#endif
