#ifndef YOUTUBE_H
#define YOUTUBE_H
#include <QString>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMetaObject>
#include <QRegularExpression>
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <QtConcurrent/QtConcurrent>
#include <QtXml>
#include <QMediaPlayer>
#include <QSound>

class VideoPlayer;

class Youtube : public QObject
{
	Q_OBJECT
public:
	explicit Youtube(QObject* parent = nullptr);
	void searchVideos(QString keyword);
	void getPage(QString url);
public slots:

	void processPage(QNetworkReply * pReply);
	void processVideo(QNetworkReply * pReply);
	void processStaticVideo(QNetworkReply * pReply);
	void processStaticAudio(QNetworkReply * pReply);
	void processAudio(QNetworkReply * pReply);
	void getURLs(QDomElement docElem, QStringList &urls);
	void setPlayer(VideoPlayer* pl);
	void changeBuffer(QMediaPlayer::MediaStatus status);


signals:
	void tryDownload(QString url);
private:
	void requestVideo(QString videoId);

	QString title;
	QNetworkAccessManager* _netManager = nullptr;
	QNetworkAccessManager* _netManagerAudio = nullptr;
	QNetworkReply* _reply;
	QFuture<void> req;
	QStringList urls;
	QStringList urlsAudio;
	VideoPlayer* pl;
	QBuffer buf1;
	QBuffer buf2;
	QBuffer bufAudio1;
	QBuffer bufAudio2;
	QString baseURL;
	QString baseURLAudio;
	QStringList decoded;
	QStringList decodedAudio;
	bool firstBuff = true;
	bool firstAudioBuff = true;
};

#endif // YOUTUBE_H
