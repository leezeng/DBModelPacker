//////////////////////////////////////////////////////////////////////////////
// CHN 業務名       : 多语言工具                                            //
// CHN 機能名       : 用于读取文本文件                                      //
// CHN ファイル名   : TextReader.cpp                                        //
//                                                                          //
// CHN  機能概要    : 识别BOM并自动转换，提供一致的接口                     //
//                                                                          //
// CHN  改訂履歴    : R1.0    2013.05.06  新規作成  易杨                    //
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TextReader.h"
#include <cassert>

CTextReader::Error::Error(const char *message)
	: runtime_error(message)
{
}

CTextReader::EndOfStream::EndOfStream()
	: Error("EndOfStream")
{
}

CTextReader::UnexpectedEndOfStream::UnexpectedEndOfStream()
	: Error("UnexpectedEndOfStream")
{
}

CTextReader::InvalidUTF16Sequence::InvalidUTF16Sequence()
	: Error("InvalidUTF16Sequence")
{
}

CTextReader::IOError::IOError()
	: Error("IOError")
{
}

CTextReader::CTextReader()
	: m_pInputStream(NULL)
	, m_pByteBufferBegin(NULL)
	, m_pByteBufferRead(NULL)
	, m_pByteBufferWrite(NULL)
	, m_pByteBufferEnd(NULL)
{
}

CTextReader::~CTextReader()
{
	Close();
}

bool CTextReader::Open(std::istream *pInputStream)
{
	assert(pInputStream != NULL);
	assert(m_pInputStream == NULL);

	if (pInputStream == NULL || m_pInputStream != NULL)
	{
		return false;
	}

	m_pInputStream = pInputStream;

	m_byteBuffer.resize(8192);
	m_pByteBufferBegin = m_byteBuffer.data();
	m_pByteBufferRead = m_pByteBufferWrite = m_pByteBufferBegin;
	m_pByteBufferEnd = m_pByteBufferBegin + m_byteBuffer.size();

	try
	{
		DetectBOM();
	}
	catch (const EndOfStream&)
	{
		m_bom = BOM_NONE;
	}
	catch (const Error&)
	{
		return false;
	}

	return true;
}

char CTextReader::GetChar()
{
	char c;

	if (!m_charQueue.empty())
	{
		c = m_charQueue.front();
		m_charQueue.pop_front();
		return c;
	}

	switch (m_bom)
	{
	case BOM_UTF16LE:
	case BOM_UTF16BE:
		c = GetCharUTF16();
		break;

	case BOM_UTF8:
	case BOM_NONE:
	default:
		c = GetByte();
		break;
	}

	return c;
}

char CTextReader::GetCharNE()
{
	try
	{
		return GetChar();
	}
	catch (const EndOfStream&)
	{
		throw UnexpectedEndOfStream();
	}
}

void CTextReader::UngetChar(char c)
{
	m_charQueue.push_front(c);
}

bool CTextReader::HasMore() const
{
	return HasMoreByte();
}

bool CTextReader::IsUTF8() const
{
	return m_bom != BOM_NONE;
}

void CTextReader::Close()
{
	if (m_pInputStream != NULL)
	{
		m_pInputStream = NULL;
	}
}

void CTextReader::SkipWS()
{
	try
	{
		for (;;)
		{
			auto b = GetChar();
			switch (b)
			{
			case ' ':
			case '\t':
			case '\v':
			case '\f':
				break;

			default:
				UngetChar(b);
				return;
			}
		}
	}
	catch (const EndOfStream&)
	{
	}
}

void CTextReader::SkipLine()
{
	try
	{
		for (;;)
		{
			char c = GetChar();
			switch (c)
			{
			case '\r':
				c = GetChar();
				if (c != '\n')
				{
					UngetChar(c);
				}
				return;

			case '\n':
				return;

			default:
				break;
			}
		}
	}
	catch (const CTextReader::EndOfStream&)
	{
		// CHN 最后一行
	}
}

std::string CTextReader::ReadLine()
{
	std::string line;

	try
	{
		for (;;)
		{
			char c = GetChar();
			switch (c)
			{
			case '\r':
				c = GetChar();
				if (c != '\n')
				{
					UngetChar(c);
				}
				return line;

			case '\n':
				return line;

			default:
				line.push_back(c);
				break;
			}
		}
	}
	catch (const CTextReader::EndOfStream&)
	{
		// CHN 最后一行
	}

	return line;
}

void CTextReader::DetectBOM()
{
	Byte b1 = GetByte();
	Byte b2 = GetByte();

	if (b1 == 0xFE && b2 == 0xFF)
	{
		m_bom = BOM_UTF16BE;
		return;
	}

	if (b1 == 0xFF && b2 == 0xFE)
	{
		m_bom = BOM_UTF16LE;
		return;
	}

	if (b1 == 0xEF && b2 == 0xBB)
	{
		Byte b3 = GetByte();
		if (b3 == 0xBF)
		{
			m_bom = BOM_UTF8;
			return;
		}

		UngetByte(b3);
	}

	UngetByte(b2);
	UngetByte(b1);
	m_bom = BOM_NONE;
}

wchar_t CTextReader::GetUTF16()
{
	Byte b1 = GetByte();
	Byte b2 = GetByteNE();

	wchar_t wc;

	if (m_bom == BOM_UTF16LE)
	{
		wc = b2;
		wc <<= 8;
		wc |= b1;
	}
	else
	{
		wc = b1;
		wc <<= 8;
		wc |= b2;
	}

	return wc;
}

char CTextReader::GetCharUTF16()
{
	wchar_t c1 = GetUTF16();
	if (c1 >= 0xD800 && c1 <= 0xDBFF)
	{
		try
		{
			wchar_t c2 = GetUTF16();
			if (c2 < 0xDC00 || c2 > 0xDFFF)
			{
				// Trail Surrogate 不在范围 [0xDC00, 0xDFFF]
				throw InvalidUTF16Sequence();
			}

			UINT32 uc = c1 - 0xD800;
			uc <<= 16;
			uc |= c2 - 0xDC00;
			uc += 0x10000;

			UngetUnicode(uc);
		}
		catch (const EndOfStream&)
		{
			throw UnexpectedEndOfStream();
		}
	}
	else
	{
		UngetUnicode(c1);
	}

	char c = m_charQueue.front();
	m_charQueue.pop_front();
	return c;
}

void CTextReader::UngetUnicode(UINT32 uc)
{
	std::deque<char> q;

	if (uc < 0x80)
	{
		q.push_back(static_cast<Byte>(uc & 0x7F));
	}
	else if (uc < 0x800)
	{
		q.push_back(0xC0 | static_cast<Byte>((uc >> 6) & 0x1F));
		q.push_back(0x80 | static_cast<Byte>(uc & 0x3F));
	}
	else if (uc < 0x10000)
	{
		q.push_back(0xE0 | static_cast<Byte>((uc >> 12) & 0x0F));
		q.push_back(0x80 | static_cast<Byte>((uc >> 6) & 0x3F));
		q.push_back(0x80 | static_cast<Byte>(uc & 0x3F));
	}
	else if (uc < 0x200000)
	{
		q.push_back(0xF0 | static_cast<Byte>((uc >> 18) & 0x07));
		q.push_back(0x80 | static_cast<Byte>((uc >> 12) & 0x3F));
		q.push_back(0x80 | static_cast<Byte>((uc >> 6) & 0x3F));
		q.push_back(0x80 | static_cast<Byte>(uc & 0x3F));
	}
	else if (uc < 0x4000000)
	{
		q.push_back(0xF8 | static_cast<Byte>((uc >> 24) & 0x03));
		q.push_back(0x80 | static_cast<Byte>((uc >> 18) & 0x3F));
		q.push_back(0x80 | static_cast<Byte>((uc >> 12) & 0x3F));
		q.push_back(0x80 | static_cast<Byte>((uc >> 6) & 0x3F));
		q.push_back(0x80 | static_cast<Byte>(uc & 0x3F));
	}
	else if (uc < 0x80000000)
	{
		q.push_back(0xFC | static_cast<Byte>((uc >> 30) & 0x01));
		q.push_back(0x80 | static_cast<Byte>((uc >> 24) & 0x3F));
		q.push_back(0x80 | static_cast<Byte>((uc >> 18) & 0x3F));
		q.push_back(0x80 | static_cast<Byte>((uc >> 12) & 0x3F));
		q.push_back(0x80 | static_cast<Byte>((uc >> 6) & 0x3F));
		q.push_back(0x80 | static_cast<Byte>(uc & 0x3F));
	}
	else
	{
		assert(false);
	}

	if (m_charQueue.empty())
	{
		m_charQueue.swap(q);
	}
	else
	{
		while (!q.empty())
		{
			m_charQueue.push_front(q.back());
			q.pop_back();
		}
	}
}

bool CTextReader::HasMoreByte() const
{
	if (!m_byteQueue.empty())
	{
		return true;
	}

	if (m_pByteBufferRead < m_pByteBufferWrite)
	{
		return true;
	}

	if (m_pInputStream != NULL && !m_pInputStream->eof())
	{
		return true;
	}

	return false;
}

CTextReader::Byte CTextReader::GetByte()
{
	// CHN 断言检查编程错误，而不是运行时错误
	assert(m_pInputStream != NULL);

	if (m_pInputStream == NULL)
	{
		throw std::logic_error("m_pInputStream == NULL");
	}

	Byte b;

	if (!m_byteQueue.empty())
	{
		b = m_byteQueue.front();
		m_byteQueue.pop_front();
		return b;
	}

	if (m_pByteBufferRead < m_pByteBufferWrite)
	{
		b = *m_pByteBufferRead++;
		return b;
	}

	if (m_pInputStream->eof())
	{
		throw EndOfStream();
	}

	if (m_pByteBufferWrite >= m_pByteBufferEnd)
	{
		m_pByteBufferRead = m_pByteBufferWrite = m_pByteBufferBegin;
	}

	m_pInputStream->read(reinterpret_cast<char*>(m_pByteBufferWrite),
		m_pByteBufferEnd - m_pByteBufferWrite);

	auto bytesRead = m_pInputStream->gcount();

	if (bytesRead < 1)
	{
		// CHN 有数据可读，却没有读出
		throw IOError();
	}

	m_pByteBufferWrite += bytesRead;

	b = *m_pByteBufferRead++;
	return b;
}

CTextReader::Byte CTextReader::GetByteNE()
{
	try
	{
		return GetByte();
	}
	catch (const EndOfStream&)
	{
		throw UnexpectedEndOfStream();
	}
}

void CTextReader::UngetByte(Byte b)
{
	m_byteQueue.push_front(b);
}
