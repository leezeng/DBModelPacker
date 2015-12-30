//////////////////////////////////////////////////////////////////////////////
// CHN 業務名       : 多语言工具                                            //
// CHN 機能名       : 用于读取文本文件                                      //
// CHN ファイル名   : TextReader.h                                          //
//                                                                          //
// CHN  機能概要    : 识别BOM并自动转换，提供一致的接口                     //
//                                                                          //
// CHN  改訂履歴    : R1.0    2013.05.06  新規作成  易杨                    //
//////////////////////////////////////////////////////////////////////////////

#ifndef __TextReader_h__
#define __TextReader_h__

#include <deque>
#include <istream>
#include <stdexcept>
#include <vector>

class CTextReader
{
public:
	CTextReader();
	~CTextReader();

	class Error
		: public std::runtime_error
	{
	public:
		Error(const char *message);
	};

	class EndOfStream
		: public Error
	{
	public:
		EndOfStream();
	};

	class UnexpectedEndOfStream
		: public Error
	{
	public:
		UnexpectedEndOfStream();
	};

	class InvalidUTF16Sequence
		: public Error
	{
	public:
		InvalidUTF16Sequence();
	};

	class IOError
		: public Error
	{
	public:
		IOError();
	};

	bool Open(std::istream *pInputStream);
	char GetChar();
	char GetCharNE();
	void UngetChar(char c);
	bool HasMore() const;
	bool IsUTF8() const;
	void Close();

	void SkipLine();
	std::string ReadLine();

	void SkipWS();

private:
	typedef unsigned char Byte;

	enum BOM
	{
		BOM_NONE,
		BOM_UTF8,
		BOM_UTF16LE,
		BOM_UTF16BE,
	};

	void DetectBOM();
	wchar_t GetUTF16();
	char GetCharUTF16();
	void UngetUnicode(UINT32 uc);

	bool HasMoreByte() const;
	Byte GetByte();
	Byte GetByteNE();
	void UngetByte(Byte b);

	BOM m_bom;
	std::deque<char> m_charQueue;

	std::deque<Byte> m_byteQueue;

	std::vector<Byte> m_byteBuffer;
	Byte *m_pByteBufferBegin;
	Byte *m_pByteBufferRead;
	Byte *m_pByteBufferWrite;
	Byte *m_pByteBufferEnd;

	std::istream *m_pInputStream;
};

#endif // __TextReader_h__
