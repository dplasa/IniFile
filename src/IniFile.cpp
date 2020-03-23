#include "IniFile.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <FS.h>

IniFile::IniFile(File& iniFileObject, char* buffer, size_t len, bool caseSensitiv) : 
	IniFileSection(iniFileObject, buffer, len, caseSensitiv)
{
}

IniFileState::error_t IniFile::validate() const
{
	uint32_t pos = 0;
	error_t err;
	while ((err = readLine(_file, _buffer, _len, pos)) == errorNoError)
		;
	if (err == errorEndOfFile) {
		return _error = errorNoError;
	}
	else {
		return _error = err;
	}
}

const IniFileSection& IniFile::findSection(const char* section)
{
	comparefunc* f = (_caseSensitive ? &strncmp : &strncasecmp);
	__findSection(section, f);
	return (const IniFileSection&)(*this);
}

const IniFileSection& IniFile::findSection(const __FlashStringHelper* section)
{
	comparefunc* f = (comparefunc*)(_caseSensitive ? &strncmp_P : &strncasecmp_P);
	__findSection((PGM_P)section, f);
	return (const IniFileSection&)(*this);
}

void IniFile::__findSection(const char* section, comparefunc* _cmpfunc)
{
	// start at position 0
	_readLinePosition = 0;

	// if no section is provided we're all good,
	// sectionless keys must be at the beginning of file
	if (section == NULL || 0==_cmpfunc("", section, SIZE_IRRELEVANT))
	{
		_error = errorNoError;
		return;		
	}

	// go line by line through the file
	// find if line contains [section]
	// if so, _readLinePosition is the file's line position 
	// where the keys of the section start
	char *cp = nullptr;
	for (;;)
	{
		_error = IniFile::readLine(_file, _buffer, _len, _readLinePosition);

		if (_error != errorNoError) 
		{
			// something happened, most likely EOF
			break;
		}

		// skip whitespaces
		cp = skipWhiteSpace(_buffer);

		// skip comment
		if (isCommentChar(*cp)) 
			continue;

		if (*cp == '[') 
		{
			// Start of section
			++cp;
			cp = skipWhiteSpace(cp);
			char *ep = strchr(cp, ']');
			if (ep != NULL) 
			{
				*ep = '\0'; // make ] be end of string
				removeTrailingWhiteSpace(cp);
				if (_cmpfunc(cp, section, SIZE_IRRELEVANT) == 0) 
				{
					_error = errorNoError;
					break;
				}
			}
		}
	}
}

// From the current file location look for the matching key. If
IniFileSectionKey IniFileSection::findKey(const char* key, bool withinSection)
{
	comparefunc* f = (_caseSensitive ? &strncmp : &strncasecmp);
	char* c= __findKey(key, f,  withinSection);
	return IniFileSectionKey(c);
}

IniFileSectionKey IniFileSection::findKey(const __FlashStringHelper* key, bool withinSection)
{
	comparefunc* f = (comparefunc*)(_caseSensitive ? &strncmp_P : &strncasecmp_P);
	char* c= __findKey((PGM_P)key, f,  withinSection);
	return IniFileSectionKey(c);
}

char* IniFileSection::__findKey(const char* key, comparefunc* _cmpfunc, bool withinSection)
{
	// can only search key, if we have found the section
	if (_error != errorNoError && _error != errorKeyNotFound)
		return NULL;

	if (key == NULL || 0==_cmpfunc("", key, SIZE_IRRELEVANT)) 
	{
		_error = errorKeyNotFound;
		return NULL;
	}

	// go line by line through the file
	// find if line contains key=...
	char *cp = nullptr;
	// read next line from current readLinePosition
	uint32_t pos = _readLinePosition;

	for (;;)
	{
		error_t err = IniFile::readLine(_file, _buffer, _len, pos);

		if (err != errorNoError) 
		{
			// something happened, most likely EOF
			break;
		}

		// skip whitespaces
		cp = IniFile::skipWhiteSpace(_buffer);

		// skip comment
		if (IniFile::isCommentChar(*cp)) 
			continue;

		if (*cp == '[')   
		{
			// Start of a new section
			if (withinSection)
			{
				_error = errorKeyNotFound;
				return NULL;
			}

			// else ignore and continue next line
			continue;
		}

		// Find '='
		char *ep = strchr(cp, '=');
		if (ep != NULL) 
		{
			*ep = '\0'; // make '=' be the end of string
			// cut off any white spaces
			IniFile::removeTrailingWhiteSpace(cp);
			if (_cmpfunc(cp, key, SIZE_IRRELEVANT) == 0) 
			{
				// key found, trim value
				cp = IniFile::skipWhiteSpace(ep+1);

				// cut comments in value, cp=(ep+1) beginning of value after '='
				ep = cp;
				while (*ep)
				{
					if (IniFile::isCommentChar(*ep))
					{
						*ep = '\0';
						break; 
					}
					++ep;
				}
				// cut off trailing white spaces
				IniFile::removeTrailingWhiteSpace(cp);
				_error = errorNoError;
				return cp;
			}
			continue;
		}
	}
	return NULL;
}

template<>
IniFileState::error_t IniFileSectionKey::getValue(String& val) const
{
	val = _buffer;
	return IniFileState::errorNoError;
}

template<>
IniFileState::error_t IniFileSectionKey::getValue(int8_t& val) const
{
	int64_t tmp;
	IniFileState::error_t err = __str_to_numeric(tmp);
	if (err == IniFileState::errorNoError)
	{
		val = tmp;
		if (val != tmp)
			err = IniFileState::errorValueTruncated;
	}
	return err;
}

template<>
IniFileState::error_t IniFileSectionKey::getValue(int16_t& val) const
{
	int64_t tmp;
	IniFileState::error_t err = __str_to_numeric(tmp);
	if (err == IniFileState::errorNoError)
	{
		val = tmp;
		if (val != tmp)
			err = IniFileState::errorValueTruncated;
	}
	return err;
}

template<>
IniFileState::error_t IniFileSectionKey::getValue(int32_t& val) const
{
	int64_t tmp;
	IniFileState::error_t err = __str_to_numeric(tmp);
	if (err == IniFileState::errorNoError)
	{
		val = tmp;
		if (val != tmp)
			err = IniFileState::errorValueTruncated;
	}
	return err;
}

template<>
IniFileState::error_t IniFileSectionKey::getValue(int64_t& val) const
{
	return __str_to_numeric(val);
}

template<>
IniFileState::error_t IniFileSectionKey::getValue(uint8_t& val) const
{
	uint64_t tmp;
	IniFileState::error_t err = __str_to_numeric(tmp);
	if (err == IniFileState::errorNoError)
	{
		val = tmp;
		if (val != tmp)
			err = IniFileState::errorValueTruncated;
	}
	return err;
}

template<>
IniFileState::error_t IniFileSectionKey::getValue(uint16_t& val) const
{
	uint64_t tmp;
	IniFileState::error_t err = __str_to_numeric(tmp);
	if (err == IniFileState::errorNoError)
	{
		val = tmp;
		if (val != tmp)
			err = IniFileState::errorValueTruncated;
	}
	return err;
}

template<>
IniFileState::error_t IniFileSectionKey::getValue(uint32_t& val) const
{
	uint64_t tmp;
	IniFileState::error_t err = __str_to_numeric(tmp);
	if (err == IniFileState::errorNoError)
	{
		val = tmp;
		if (val != tmp)
			err = IniFileState::errorValueTruncated;
	}
	return err;
}

template<>
IniFileState::error_t IniFileSectionKey::getValue(uint64_t& val) const
{
	return __str_to_numeric(val);
}

template<>
IniFileState::error_t IniFileSectionKey::getValue(bool& val) const
{
	if (_buffer == NULL)
		return IniFileState::errorKeyNotFound;

	// For true accept: true, yes, 1
	// For false accept: false, no, 0
	if (strcasecmp_P(_buffer, PSTR("true")) == 0 ||
		strcasecmp_P(_buffer, PSTR("yes")) == 0 ||
		strcasecmp_P(_buffer, PSTR("1")) == 0) {
		val = true;
		return IniFileState::errorNoError; // valid conversion
	}
	if (strcasecmp_P(_buffer, PSTR("false")) == 0 ||
		strcasecmp_P(_buffer, PSTR("no")) == 0 ||
		strcasecmp_P(_buffer, PSTR("0")) == 0) {
		val = false;
		return IniFileState::errorNoError; // valid conversion
	}
	return IniFileState::errorFormatError;
}

IniFileState::error_t IniFileSectionKey::__str_to_numeric(int64_t &value) const
{
	if (_buffer == NULL)
		return IniFileState::errorKeyNotFound;
	char *endptr;
	value = strtoll(_buffer, &endptr, 0 /* convert to any base */);
	if (*endptr == '\0') 
	{
		return IniFileState::errorNoError; // valid conversion
	}
	// either  (endptr == buffer) --> no conversion 
	// or
	// buffer has trailing non-numeric characters, and since the buffer
	// already had whitespace removed discard the entire results
	return IniFileState::errorFormatError;
}

IniFileState::error_t IniFileSectionKey::__str_to_numeric(uint64_t &value) const
{
	if (_buffer == NULL)
		return IniFileState::errorKeyNotFound;
	char *endptr;
	value = strtoull(_buffer, &endptr, 0 /* convert to any base */);
	if (*endptr == '\0') 
	{
		return IniFileState::errorNoError; // valid conversion
	}
	// either  (endptr == buffer) --> no conversion 
	// or
	// buffer has trailing non-numeric characters, and since the buffer
	// already had whitespace removed discard the entire results
	return IniFileState::errorFormatError;
}

template<>
IniFileState::error_t IniFileSectionKey::getValue(float& value) const
{
	if (_buffer == NULL)
		return IniFileState::errorKeyNotFound;

	char *endptr;
	value = strtof(_buffer, &endptr);
	if (*endptr == '\0') {
		return IniFileState::errorNoError; // valid conversion
	}
	// either  (endptr == buffer) --> no conversion 
	// or
	// buffer has trailing non-numeric characters, and since the buffer
	// already had whitespace removed discard the entire results
	return IniFileState::errorFormatError;
}

template<>
IniFileState::error_t IniFileSectionKey::getValue(double& value) const
{
	if (_buffer == NULL)
		return IniFileState::errorKeyNotFound;

	char *endptr;
	value = strtod(_buffer, &endptr);
	if (*endptr == '\0') {
		return IniFileState::errorNoError; // valid conversion
	}
	// either  (endptr == buffer) --> no conversion 
	// or
	// buffer has trailing non-numeric characters, and since the buffer
	// already had whitespace removed discard the entire results
	return IniFileState::errorFormatError;
}

IniFileState::error_t IniFileSectionKey::getNumericByteArray(uint8_t* array, size_t& alen, const char* seperators) const
{
	return __str_to_numeric(array, alen, seperators, 0);
}

IniFileState::error_t IniFileSectionKey::getHexByteArray(uint8_t* array, size_t& alen, const char* seperators) const
{
	return __str_to_numeric(array, alen, seperators, 16);
}

#if defined(ARDUINO) && ARDUINO >= 100
IniFileState::error_t IniFileSectionKey::getIPAddress(IPAddress& ip) const
{
	size_t alen = 4;
	IniFileState::error_t err = __str_to_numeric(&(ip[0]), alen, ".", 0);
	if (err == IniFileState::errorNoError)
	{
		if (4 != alen)
			err = IniFileState::errorFormatError;
	}
	return err;
}
#endif

IniFileState::error_t IniFileSectionKey::__str_to_numeric(uint8_t* array, 
	size_t& alen, const char* seperators, uint8_t base) const
{
	if (_buffer == NULL)
		return IniFileState::errorKeyNotFound;

	// tokenize the string
	char* token = strtok(_buffer, seperators);

	size_t i = 0;
	for (; (token!=NULL) && (i < alen); ++i)
	{
		char* endptr;
		uint64_t tmp = strtoull(token, &endptr, base);
		if ((endptr == token) || (*endptr != '\0')) 
		{
			alen = i;
			return IniFileState::errorFormatError; // invalid conversion
		}
		array[i] = tmp;
		if (!(array[i] == tmp))
		{
			alen = i;
			return IniFileState::errorValueTruncated;
		}
		token = strtok(NULL, seperators);
	}
	alen = i;
	return IniFileState::errorNoError;
}

IniFile::error_t IniFile::readLine(File &file, char *buffer, size_t len, uint32_t &pos)
{
	//printf("Call readline(..., pos=%u)\n", pos);
	if (!file)
		return errorFileNotOpen;

	if (len < 3)
		return errorBufferTooSmall;

	if (!file.seek(pos))
		return errorSeekError;

#if defined(ARDUINO_ARCH_ESP32) && !defined(PREFER_SDFAT_LIBRARY)
	size_t bytesRead = file.readBytes(buffer, len);
#else
	size_t bytesRead = file.read((uint8_t*)buffer, len);
#endif
	if (!bytesRead) {
		buffer[0] = '\0';
		return errorEndOfFile;
	}

	for (size_t i = 0; i < bytesRead && i < len-1; ++i) {
		// Test for '\n' with optional '\r' too

		if (buffer[i] == '\n' || buffer[i] == '\r') {
			char match = buffer[i];
			char otherNewline = (match == '\n' ? '\r' : '\n');
			// end of line, discard any trailing character of the other sort
			// of newline
			buffer[i] = '\0';

			if (buffer[i+1] == otherNewline)
				++i;
			pos += (i + 1); // skip past newline(s)
			//return (i+1 == bytesRead && !file.available());
			return errorNoError;
		}
	}

	if (!file.available()) {
		// end of file without a newline
		buffer[bytesRead] = '\0';
		pos += bytesRead;
		return errorNoError;
	}

	buffer[len-1] = '\0'; // terminate the string
	return errorBufferTooSmall;
}

bool IniFile::isCommentChar(char c)
{
	return (c == ';' || c == '#');
}

char* IniFile::skipWhiteSpace(char* str)
{
	char *cp = str;
	if (cp)
		while (isspace(*cp))
			++cp;
	return cp;
}

void IniFile::removeTrailingWhiteSpace(char* str)
{
	if (str == nullptr)
		return;
	char *cp = str + strlen(str) - 1;
	while (cp >= str && isspace(*cp))
		*cp-- = '\0';
}
