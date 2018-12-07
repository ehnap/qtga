#ifndef QTGAHANDLER_H
#define QTGAHANDLER_H

#include <QtGui/QImageIOHandler>

QT_BEGIN_NAMESPACE

class QTgaFile;

class QTgaHandler : public QImageIOHandler
{
public:
	QTgaHandler();
	~QTgaHandler();

	bool canRead() const override;
	bool read(QImage *image) override;

	QByteArray name() const override;

	static bool canRead(QIODevice *device);

	QVariant option(ImageOption option) const override;
	void setOption(ImageOption option, const QVariant &value) override;
	bool supportsOption(ImageOption option) const override;

private:
	mutable QTgaFile *tga;
};

QT_END_NAMESPACE

#endif // QTGAHANDLER_H