#include "qtgafile.h"

#include <QtCore/QIODevice>
#include <QtCore/QDebug>
#include <QtCore/QDateTime>

struct RleInfo
{
	int repeat;
	int number;
};

struct TgaRleReader
{
	Q_DISABLE_COPY(TgaRleReader)
		TgaRleReader() = default;
	RleInfo operator()(QIODevice *s) const
	{
		char ch1;
		if (s->getChar(&ch1)) {
			RleInfo rleInfo;
			rleInfo.repeat = uchar(ch1) >> 7;
			rleInfo.number = (uchar(ch1) & 0x7F) + 1;
			return rleInfo;
		}
		else {
			return RleInfo{};
		}
	}
};

struct TgaReader
{
	Q_DISABLE_COPY(TgaReader)
		TgaReader() = default;

	virtual ~TgaReader() {}
	virtual QRgb operator()(QIODevice *s) const = 0;
	virtual int getIndex(QIODevice* s) const = 0;
};

struct Tga8Reader : public TgaReader
{
	~Tga8Reader() {}
	QRgb operator()(QIODevice *s) const override
	{
		char ch1;
		if (s->getChar(&ch1)) {
			return qRgb(quint8(ch1), quint8(ch1), quint8(ch1));
		}
		else
		{
			return 0;
		}
	}

	int getIndex(QIODevice* s) const override
	{
		char ch1;
		if (s->getChar(&ch1)) {
			return quint8(ch1);
		}
		else
		{
			return 0;
		}
	}
};

struct Tga16Reader : public TgaReader
{
	~Tga16Reader() {}
	QRgb operator()(QIODevice *s) const override
	{
		char ch1, ch2;
		if (s->getChar(&ch1) && s->getChar(&ch2)) {
			quint16 d = uchar(ch1) | (uchar(ch2) << 8);
			int alpha = (d & 0x8000) ? 0xff000000 : 0x00000000;
			QRgb result = alpha | ((d & 0x7C00) << 9) | ((d & 0x03E0) << 6) | ((d & 0x001F) << 3);
			return result;
		}

		return 0;
	}

	int getIndex(QIODevice* s) const override
	{
		char ch1, ch2;
		if (s->getChar(&ch1) && s->getChar(&ch2))
		{
			quint16 d = (uchar(ch1) & 0xFF) | ((uchar(ch2) & 0xFF) << 8);
			return d;
		}

		return 0;
	}
};

struct Tga24Reader : public TgaReader
{
	QRgb operator()(QIODevice *s) const override
	{
		char r, g, b;
		if (s->getChar(&b) && s->getChar(&g) && s->getChar(&r))
			return qRgb(uchar(r), uchar(g), uchar(b));

		return 0;
	}

	int getIndex(QIODevice* s) const override
	{
		char ch1, ch2, ch3;
		if (s->getChar(&ch1) && s->getChar(&ch2) && s->getChar(&ch3))
		{
			quint32 d = (uchar(ch3) << 16) | (uchar(ch2) << 8) | uchar(ch1);
			return d;
		}
		return 0;
	}
};

struct Tga32Reader : public TgaReader
{
	QRgb operator()(QIODevice *s) const override
	{
		char r, g, b, a;
		if (s->getChar(&b) && s->getChar(&g) && s->getChar(&r) && s->getChar(&a))
			return qRgba(uchar(r), uchar(g), uchar(b), uchar(a));

		return 0;
	}

	int getIndex(QIODevice* s) const override
	{
		char ch1, ch2, ch3, ch4;
		if (s->getChar(&ch1) && s->getChar(&ch2) && s->getChar(&ch3) && s->getChar(&ch4))
		{
			quint32 d = (uchar(ch4) << 24) | (uchar(ch3) << 16) | (uchar(ch2) << 8) | uchar(ch1);
			return d;
		}

		return 0;
	}
};

static TgaReader* newReader(int depth)
{
	TgaReader* reader = Q_NULLPTR;
	if (depth == 8)
		reader = new Tga8Reader();
	else if (depth == 16)
		reader = new Tga16Reader();
	else if (depth == 24)
		reader = new Tga24Reader();
	else if (depth == 32)
		reader = new Tga32Reader();

	return reader;
}


QTgaFile::QTgaFile(QIODevice *device)
	: mDevice(device)
	, m_bHighVersion(false)
{
	::memset(mHeader, 0, HeaderSize);
	if (!mDevice->isReadable())
	{
		mErrorMessage = tr("Could not read image data");
		return;
	}
	if (mDevice->isSequential())
	{
		mErrorMessage = tr("Sequential device (eg socket) for image read not supported");
		return;
	}
	if (!mDevice->seek(0))
	{
		mErrorMessage = tr("Seek file/device for image read failed");
		return;
	}
	if (device->read(reinterpret_cast<char*>(mHeader), HeaderSize) != HeaderSize) {
		mErrorMessage = tr("Image header read failed");
		return;
	}
	bool bSupport = (mHeader[ImageType] >= 0 && mHeader[ImageType] <= 3)
		|| (mHeader[ImageType] >= 9 && mHeader[ImageType] <= 11);
	if (!bSupport)
	{
		// TODO: should support other image types
		mErrorMessage = tr("Image type not supported");
		return;
	}
	int bitsPerPixel = mHeader[PixelDepth];
	bool validDepth = (bitsPerPixel == 8 || bitsPerPixel == 16 || bitsPerPixel == 24 || bitsPerPixel == 32);
	if (!validDepth)
	{
		mErrorMessage = tr("Image depth not valid");
	}
	int curPos = mDevice->pos();
	int fileBytes = mDevice->size();
	if (!mDevice->seek(fileBytes - FooterSize))
	{
		mErrorMessage = tr("Could not seek to image read footer");
		return;
	}
	char footer[FooterSize];
	if (mDevice->read(reinterpret_cast<char*>(footer), FooterSize) != FooterSize) {
		mErrorMessage = tr("Could not read footer");
	}
	m_bHighVersion = (qstrncmp(&footer[SignatureOffset], "TRUEVISION-XFILE", 16) == 0);
	if (!mDevice->seek(curPos))
	{
		mErrorMessage = tr("Could not reset to read data");
	}
}

QTgaFile::~QTgaFile()
{
}

QImage QTgaFile::readImage()
{
	if (!isValid())
		return QImage();

	if (mHeader[ImageType] == 0) //The type is No Image Data
		return QImage();

	int offset = mHeader[IdLength];  // Mostly always zero
	mDevice->seek(HeaderSize + offset);

	// color map
	int cmapLength = littleEndianInt(&mHeader[CMapLength]);
	int cmapDepth = littleEndianInt(&mHeader[CMapDepth]);
	
	QVector<QRgb> colorMap;
	TgaReader* cmapReader = newReader(cmapDepth);
	TgaReader& cmapRead = *cmapReader;
	while (cmapLength--)
		colorMap.push_back(cmapRead(mDevice));
	delete cmapReader;

	int bitsPerPixel = mHeader[PixelDepth];
	int imageWidth = width();
	int imageHeight = height();

	QImage im(imageWidth, imageHeight, QImage::Format_ARGB32);
	TgaReader* reader = newReader(bitsPerPixel);
	if (compression() == QTgaFile::NoCompression)
	{
		if (mHeader[ImageType] == 2 || mHeader[ImageType] == 3)
			noCompressProcess(reader, im);
		if (mHeader[ImageType] == 1)
			noCompressCMapProcess(colorMap, reader, im);
	}
	else
	{
		if (mHeader[ImageType] == 10 || mHeader[ImageType] == 11)
			rleProcess(reader, im);
		if (mHeader[ImageType] == 9)
			rleCMapProcess(colorMap, reader, im);
	}

	delete reader;

	// TODO: add processing of TGA extension information - ie TGA 2.0 files
	return im;
}

void QTgaFile::rleCMapProcess(QVector<QRgb>& cMap, TgaReader* pReader, QImage& img)
{
	int y = 0, x = 0;

	TgaRleReader rleReader;
	TgaReader& read = *pReader;
	while (y < height())
	{
		RleInfo rleInfo = rleReader(mDevice);
		if (rleInfo.number == 0) break;
		int index;
		if (rleInfo.repeat == 1)
			index = read.getIndex(mDevice);
		while (rleInfo.number--)
		{
			if (rleInfo.repeat == 0)
				index = read.getIndex(mDevice);
			int curX = xCorner() ? width() - x - 1 : x;
			int curY = !yCorner() ? height() - y - 1 : y;
			img.setPixel(curX, curY, cMap[index]);
			x++;
			if (x == width())
			{
				x = 0;
				y++;
			}
		}
	}
}

void QTgaFile::rleProcess(TgaReader* pReader, QImage& img)
{
	int y = 0, x = 0;
	TgaRleReader rleReader;
	TgaReader& read = *pReader;
	while (y < height())
	{
		RleInfo rleInfo = rleReader(mDevice);
		if (rleInfo.number == 0)
			break;
		QRgb curColor;
		if (rleInfo.repeat == 1)
			curColor = read(mDevice);
		while (rleInfo.number--)
		{
			if (rleInfo.repeat == 0)
				curColor = read(mDevice);

			int curX = xCorner() ? width() - x - 1 : x;
			int curY = !yCorner() ? height() - y - 1 : y;
			img.setPixel(curX, curY, curColor);
			x++;
			if (x == width())
			{
				x = 0;
				y++;
			}
		}
	}
}

void QTgaFile::noCompressProcess(TgaReader* pReader, QImage& img)
{
	TgaReader& read = *pReader;
	for (int y = 0; y < height(); ++y)
		for (int x = 0; x < width(); ++x)
		{
			int curX = xCorner() ? width() - x - 1 : x;
			int curY = !yCorner() ? height() - y - 1 : y;
			img.setPixel(curX, curY, read(mDevice));
		}
}

void QTgaFile::noCompressCMapProcess(QVector<QRgb>& cMap, TgaReader* pReader, QImage& img)
{
	TgaReader& read = *pReader;
	for (int y = 0; y < height(); ++y)
		for (int x = 0; x < width(); ++x)
		{
			int curX = xCorner() ? width() - x - 1 : x;
			int curY = !yCorner() ? height() - y - 1 : y;
			img.setPixel(curX, curY, cMap[read.getIndex(mDevice)]);
		}
}
