#include "webnetworkfactory.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkCookieJar>
#include <QTimer>
#include <QUrl>

// Allowlisted by the pokerth.net Cloudflare configuration – identical to the
// Qt-Widgets client (src/net/upload-/downloadhelper.cpp).
static const QByteArray POKERTH_USER_AGENT = QByteArrayLiteral("PokerTH/2.0 (Qt Network)");

namespace
{

// Offline build: QML must not reach the public internet. Returning a reply
// that is already failed means no DNS lookup, no TCP connect and no TLS
// handshake happen at all - the request is refused inside the process.
class OfflineReply : public QNetworkReply
{
public:
	OfflineReply(const QNetworkRequest &req, Operation op, QObject *parent)
		: QNetworkReply(parent)
	{
		setOperation(op);
		setRequest(req);
		setUrl(req.url());
		setAttribute(QNetworkRequest::HttpStatusCodeAttribute, QVariant(403));
		setHeader(QNetworkRequest::ContentLengthHeader, QVariant(0));
		open(QIODevice::ReadOnly | QIODevice::Unbuffered);
		// Queued, so the caller has a chance to connect() before finished().
		QTimer::singleShot(0, this, [this]() {
			setError(QNetworkReply::ContentAccessDenied,
					 QStringLiteral("Network access is disabled in this build."));
			emit errorOccurred(QNetworkReply::ContentAccessDenied);
			emit finished();
		});
	}

	void abort() override {}
	qint64 readData(char *, qint64) override { return -1; }
	qint64 bytesAvailable() const override { return 0; }
	bool isSequential() const override { return true; }
};

} // namespace

WebNetworkAccessManager::WebNetworkAccessManager(QObject *parent)
	: QNetworkAccessManager(parent)
{
	// Explicit in-memory cookie jar so the CSRF session cookie set by the GET
	// is sent back on the following POST (BBC/WEC leaderboard & player APIs).
	setCookieJar(new QNetworkCookieJar(this));
}

QNetworkReply *WebNetworkAccessManager::createRequest(Operation op,
		const QNetworkRequest &request, QIODevice *outgoingData)
{
	// Only real network schemes are regulated. qrc:, file: and data: never
	// reach the socket layer, so local resources, deck images and inline
	// images keep working.
	const QString scheme = request.url().scheme().toLower();
	if (scheme == QLatin1String("http") || scheme == QLatin1String("https")
			|| scheme == QLatin1String("ftp")) {
		return new OfflineReply(request, op, this);
	}

	QNetworkRequest req(request);
	req.setRawHeader(QByteArrayLiteral("User-Agent"), POKERTH_USER_AGENT);
	return QNetworkAccessManager::createRequest(op, req, outgoingData);
}

QNetworkAccessManager *WebNetworkAccessManagerFactory::create(QObject *parent)
{
	return new WebNetworkAccessManager(parent);
}
