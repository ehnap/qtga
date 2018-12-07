#ifndef QTGAFILE_H
#define QTGAFILE_H

#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtCore/QCoreApplication>

QT_BEGIN_NAMESPACE

class QIODevice;
class TgaReader;

class QTgaFile
{
	Q_DECLARE_TR_FUNCTIONS(QTgaFile)

public:
	enum Compression {
		NoCompression = 0,
		RleCompression = 1
	};

	enum HeaderOffset {
		IdLength = 0,          /* 00h  Size of Image ID field */
		ColorMapType = 1,      /* 01h  Color map type */
		ImageType = 2,         /* 02h  Image type code */
		CMapStart = 3,         /* 03h  Color map origin */
		CMapLength = 5,        /* 05h  Color map length */
		CMapDepth = 7,         /* 07h  Depth of color map entries */
		XOffset = 8,           /* 08h  X origin of image */
		YOffset = 10,          /* 0Ah  Y origin of image */
		Width = 12,            /* 0Ch  Width of image */
		Height = 14,           /* 0Eh  Height of image */
		PixelDepth = 16,       /* 10h  Image pixel size */
		ImageDescriptor = 17,  /* 11h  Image descriptor byte */
		HeaderSize = 18
	};

	enum FooterOffset {
		ExtensionOffset = 0,
		DeveloperOffset = 4,
		SignatureOffset = 8,
		FooterSize = 26
	};

	QTgaFile(QIODevice *);
	~QTgaFile();

	inline bool isValid() const;
	inline QString errorMessage() const;
	QImage readImage();
	inline int xOffset() const;
	inline int yOffset() const;
	inline int width() const;
	inline int height() const;
	inline bool yCorner() const;
	inline bool xCorner() const;
	inline QSize size() const;
	inline Compression compression() const;

private:
	static inline quint16 littleEndianInt(const unsigned char *d);
	void rleCMapProcess(QVector<QRgb>& cMap, TgaReader* pReader, QImage& img);
	void rleProcess(TgaReader* pReader, QImage& img);
	void noCompressProcess(TgaReader* pReader, QImage& img);
	void noCompressCMapProcess(QVector<QRgb>& cMap, TgaReader* pReader, QImage& img);

	QString mErrorMessage;
	unsigned char mHeader[HeaderSize];
	QIODevice *mDevice;
	bool m_bHighVersion;
};

inline bool QTgaFile::isValid() const
{
	return mErrorMessage.isEmpty();
}

inline QString QTgaFile::errorMessage() const
{
	return mErrorMessage;
}

/*!
\internal
Returns the integer encoded in the two little endian bytes at \a d.
*/
inline quint16 QTgaFile::littleEndianInt(const unsigned char *d)
{
	return d[0] + d[1] * 256;
}

inline int QTgaFile::xOffset() const
{
	return littleEndianInt(&mHeader[XOffset]);
}

inline int QTgaFile::yOffset() const
{
	return littleEndianInt(&mHeader[YOffset]);
}

inline int QTgaFile::width() const
{
	return littleEndianInt(&mHeader[Width]);
}

inline int QTgaFile::height() const
{
	return littleEndianInt(&mHeader[Height]);
}

inline QSize QTgaFile::size() const
{
	return QSize(width(), height());
}

inline QTgaFile::Compression QTgaFile::compression() const
{
	if (mHeader[ImageType] > 8 && mHeader[ImageType] < 12)
		return RleCompression;

	return NoCompression;
}

inline bool QTgaFile::yCorner() const
{
	return mHeader[ImageDescriptor] & 0x20;;
}

inline bool QTgaFile::xCorner() const
{
	return mHeader[ImageDescriptor] & 0x10;;
}

QT_END_NAMESPACE

#endif // QTGAFILE_H
