#include "youtube.h"
#include "videoplayer.h"

Youtube::Youtube(QObject *parent)
{
	connect(this, &Youtube::tryDownload,[&](QString url){this->getPage(url);});
	_netManager = new QNetworkAccessManager(this);
	_netManagerAudio = new QNetworkAccessManager(this);
}

void Youtube::getPage(QString url){

	qDebug()<<url;
	QNetworkRequest request(url);

	_netManager->disconnect();
	connect(_netManager, SIGNAL(finished(QNetworkReply *)), this, SLOT(processPage(QNetworkReply *)));
	_reply = _netManager->get(request);
}

void Youtube::getURLs(QDomElement docElem, QStringList &urls){

	if(docElem.hasChildNodes()){
		auto child = docElem.firstChild();
		while(!child.isNull()){
			getURLs(child.toElement(), urls);
			child = child.nextSibling();
		}
	}

	if(!docElem.localName().compare("S"))
		return;

	auto attrs = docElem.attributes();
	for(int i=0; i<attrs.length();i++)
		urls.append(attrs.item(i).nodeValue());

}

void Youtube::processPage(QNetworkReply * pReply)
{
	QVariant statusCodeV = pReply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
	QVariant redirectionTargetUrl = pReply->attribute(QNetworkRequest::RedirectionTargetAttribute);

	pl->errorLabel->setText("Parsing youtube page..");

	QJsonObject results;

	QRegularExpression regExp("ytplayer\\.config\\s*=\\s*(\\{.+?\\});");

	if (pReply->error() == QNetworkReply::NoError){
		QByteArray message = pReply->readAll();
		if(message.startsWith(QString("<?xml").toUtf8())){
			QFile xmlFile("xml.txt");
			xmlFile.open(QIODevice::WriteOnly);
			xmlFile.write(message);
			xmlFile.close();

			QString errMsg;
			QDomDocument doc;
			doc.setContent(message,true, &errMsg);
			QDomElement docElem = doc.documentElement();
			//docElem.elementsByTagNameNS()
			qDebug()<<errMsg;

			QDomNodeList domList = docElem.elementsByTagName("Representation");

			auto correctNode = domList.item(0);
			auto correctNodeAudio = domList.item(0);

			for(int i = 0;i<domList.size();i++){
				qDebug()<<domList.item(i).toElement().attribute("frameRate");
				if(domList.item(i).toElement().attribute("frameRate").length()){
					correctNode = domList.item(i);
					break;
				}
			}
			baseURL = correctNode.toElement().text();

			for(int i = 0;i<domList.size();i++){
				qDebug()<<domList.item(i).toElement().attribute("audioSamplingRate");
				if(domList.item(i).toElement().attribute("audioSamplingRate").length()){
					correctNodeAudio = domList.item(i);
					break;
				}
			}
			baseURLAudio = correctNodeAudio.toElement().text();

			domList = correctNode.toElement().elementsByTagName("SegmentList");
			qDebug()<<domList.size();
			for(int i=0;i<domList.size();i++){
				getURLs(domList.item(i).toElement(), urls);
				qDebug()<<"URLS: "<<urls.size();
				qDebug()<<urls.first();
				if(urls.size())
					break;
			}

			domList = correctNodeAudio.toElement().elementsByTagName("SegmentList");
			qDebug()<<domList.size();
			for(int i=0;i<domList.size();i++){
				getURLs(domList.item(i).toElement(), urlsAudio);
				qDebug()<<"URLS: "<<urlsAudio.size();
				qDebug()<<urlsAudio.first();
				if(urlsAudio.size())
					break;
			}

			_netManager->disconnect();
			connect(_netManager, SIGNAL(finished(QNetworkReply *)), this, SLOT(processVideo(QNetworkReply *)));

			connect(_netManagerAudio, SIGNAL(finished(QNetworkReply *)), this, SLOT(processAudio(QNetworkReply *)));
			pl->errorLabel->setText("Requesting Streaming data");
			//while(urls.size()){
				QNetworkRequest request(baseURL+urls.first());
				_reply = _netManager->get(request);
				//this->pl->addMedia(QMediaResource(request));
				urls.pop_front();


				QNetworkRequest request1(baseURL+urls.first());
				_reply = _netManager->get(request1);
				//this->pl->addMedia(QMediaResource(request));
				urls.pop_front();

				QNetworkRequest request2(baseURLAudio+urlsAudio.first());
				_reply = _netManagerAudio->get(request2);
				//this->pl->addMedia(QMediaResource(request));
				urlsAudio.pop_front();


				QNetworkRequest request3(baseURLAudio+urlsAudio.first());
				_reply = _netManagerAudio->get(request3);
				//this->pl->addMedia(QMediaResource(request));
				urlsAudio.pop_front();


			//}

			//

			//

			return;
		}

		QString str = QString::fromUtf8(message.data(), message.size());
		int statusCode = pReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

		qDebug() << QVariant(statusCode).toString();


		auto regMatch = regExp.match(str);
		QString foundUrl = regMatch.capturedTexts().first().mid(18);
		foundUrl = foundUrl.mid(0,foundUrl.length()-1);
		qDebug()<<foundUrl;



		QJsonParseError err;
		QJsonDocument jsonResponse =
				QJsonDocument::fromJson(foundUrl.toUtf8(),&err);
		auto hz = jsonResponse.object();
		results = jsonResponse.object()["args"].toObject();

		title = results["title"].toString();
		qDebug() << "request works.";
		qDebug() << err.errorString();

		if(results["dashmpd"].toString().length()){
			emit tryDownload(QUrl::fromPercentEncoding(results["dashmpd"].toString().toUtf8()));
			return;
		}

		QString key = "adaptive_fmts";
		key = "url_encoded_fmt_stream_map";
		QStringList urls = results[key].toString().split(",",QString::SkipEmptyParts);
		key = "url_encoded_fmt_stream_map";
		//urls.append(results[key].toString().split(",",QString::SkipEmptyParts));
		foreach (auto url, urls){
			//qDebug()<<url.mid(0, url.indexOf("url="));
			QUrl hz(url.mid(url.indexOf("url=")+4));
			decoded.append(QUrl::fromPercentEncoding(hz.toString().toUtf8()));
			//qDebug()<<hz.isValid();
		}

		QFile urlFile("urls.txt");
		urlFile.open(QIODevice::WriteOnly);
		foreach (auto url, decoded){
			urlFile.write(QUrl::fromPercentEncoding(url.toUtf8()).toUtf8());
			urlFile.write("\n");
		}
		urlFile.close();

		qDebug()<<decoded.first();
		QNetworkRequest request(decoded.first());
		_netManager->disconnect();
		pl->errorLabel->setText("Downloading full video data");
		//connect(_netManager, SIGNAL(finished(QNetworkReply *)), this, SLOT(processVideo(QNetworkReply *)));
		connect(_netManager, SIGNAL(finished(QNetworkReply *)), this, SLOT(processStaticVideo(QNetworkReply *)));

		//connect(_netManager, SIGNAL(finished(QNetworkReply *)), this, SLOT(processStaticVideo(QNetworkReply *)));
		//connect(this, &Youtube::tryDownload,[&](QString url){this->getPage(url);});
		_reply = _netManager->get(request);



	}else{
		qDebug() << "Error requesting video on Youtube : " << QString(pReply->error()).toUtf8();
		qDebug()<<pReply->errorString();
		qDebug()<<"Asd";
	}
}


void Youtube::processStaticVideo(QNetworkReply * pReply){
	QVariant statusCodeV = pReply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
	QVariant redirectionTargetUrl = pReply->attribute(QNetworkRequest::RedirectionTargetAttribute);

	if (pReply->error() == QNetworkReply::NoError)
	{

		qDebug()<<"Got response!";
		pl->errorLabel->setText("Video download complete...");

		auto data = pReply->readAll();
		if(!data.size())
			return;

		QString titlePref = "downloaded";
		QString title;
		for(uint i=0;;i++){
			QFile file(titlePref+QString::number(i)+".mp4");

			if(!file.exists()){
				title = titlePref+QString::number(i)+".mp4";
				break;
			}

		}

		pl->errorLabel->setText("Video saved as: "+title);

		QFile file(title);

		file.open(QIODevice::WriteOnly);
		file.write(data);
		file.close();



		auto& pl = this->pl->mediaPlayer;
		pl.setPlaylist(nullptr);
		QBuffer *buf = new QBuffer(this);
		buf->setData(data);
		buf->open(QIODevice::ReadOnly);
		pl.setMedia(QMediaContent(), buf);
		pl.play();

	}else{
		qDebug() << "Error requesting video on Youtube : " << QString(pReply->error()).toUtf8();
		qDebug()<<pReply->errorString();
		qDebug()<<"Retrying:";
		decoded.pop_front();
		if(!decoded.size()){
			getPage(this->pl->edit->text());
			return;
		}
		qDebug()<<QUrl::fromPercentEncoding(decoded.first().toUtf8());
		QNetworkRequest request(decoded.first());
		pl->errorLabel->setText("Error downloading video. Retrying...");
		_reply = _netManager->get(request);

	}
}



void Youtube::processStaticAudio(QNetworkReply * pReply){
	QVariant statusCodeV = pReply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
	QVariant redirectionTargetUrl = pReply->attribute(QNetworkRequest::RedirectionTargetAttribute);

	if (pReply->error() == QNetworkReply::NoError)
	{

		auto data = pReply->readAll();
		QFile file("1.mp3");

		file.open(QIODevice::WriteOnly);
		file.write(data);
		file.close();



		auto& pl = this->pl->audioPlayer;
		QBuffer *buf = new QBuffer(this);
		buf->setData(data);
		buf->open(QIODevice::ReadOnly);
		pl.setMedia(QMediaContent(), buf);

	}else{
		qDebug() << "Error requesting video on Youtube : " << QString(pReply->error()).toUtf8();
		qDebug()<<pReply->errorString();
		qDebug()<<"Retrying:";
		decodedAudio.pop_front();
		qDebug()<<QUrl::fromPercentEncoding(decodedAudio.first().toUtf8());
		QNetworkRequest request(decodedAudio.first());
		_reply = _netManagerAudio->get(request);
	}
}

void Youtube::processVideo(QNetworkReply * pReply){
	QVariant statusCodeV = pReply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
	QVariant redirectionTargetUrl = pReply->attribute(QNetworkRequest::RedirectionTargetAttribute);

	if (pReply->error() == QNetworkReply::NoError)
	{

		auto data = pReply->readAll();
		QFile* file;
		if(firstBuff)
			file = new QFile("1.mp4");
		else
			file = new QFile("2.mp4");

		file->open(QIODevice::WriteOnly);
		file->write(data);
		file->close();

		auto pl = this->pl->mediaPlayer.playlist();
		if(!pl){
			pl = new QMediaPlaylist();
			this->pl->mediaPlayer.setPlaylist(pl);
		}

		if(pl->mediaCount()<2)
			pl->addMedia(QUrl::fromLocalFile(file->fileName()));
		firstBuff = !firstBuff;

		if(this->pl->mediaPlayer.state() == QMediaPlayer::StoppedState)
			this->pl->mediaPlayer.play();

		delete file;

	}else{
		qDebug() << "Error requesting video on Youtube : " << QString(pReply->error()).toUtf8();
		qDebug()<<pReply->errorString();
		qDebug()<<"Asd";


		emit tryDownload(pl->edit->text());
	}
}


void Youtube::processAudio(QNetworkReply * pReply){
	QVariant statusCodeV = pReply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
	QVariant redirectionTargetUrl = pReply->attribute(QNetworkRequest::RedirectionTargetAttribute);

	if (pReply->error() == QNetworkReply::NoError)
	{

		auto data = pReply->readAll();
		QFile* file;
		if(firstAudioBuff)
			file = new QFile("1.mp3");
		else
			file = new QFile("2.mp3");

		file->open(QIODevice::WriteOnly);
		file->write(data);

		//qDebug()<<file->errorString();
		file->close();



		auto pl = this->pl->audioPlayer.playlist();
		if(pl->mediaCount()<2)
			pl->addMedia(QUrl::fromLocalFile(file->fileName()));
		if(this->pl->audioPlayer.state() == QMediaPlayer::StoppedState)
			this->pl->audioPlayer.play();
		firstAudioBuff = !firstAudioBuff;
		delete file;

	}
}


void Youtube::setPlayer(VideoPlayer* pl){
	this->pl = pl;
	connect(&pl->mediaPlayer, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), this, SLOT(changeBuffer(QMediaPlayer::MediaStatus)));
}

void Youtube::changeBuffer(QMediaPlayer::MediaStatus status){
	qDebug()<<status;
	if(status != QMediaPlayer::EndOfMedia)
		return;

	if(!pl->mediaPlayer.playlist()){
		pl->mediaPlayer.stop();
		auto hz = (QBuffer*)pl->mediaPlayer.mediaStream();
		hz->close();
		return;
	}

	QNetworkRequest request(baseURLAudio+urlsAudio.first());
	_reply = _netManagerAudio->get(request);
	//this->pl->addMedia(QMediaResource(request));
	urlsAudio.pop_front();

	pl->audioPlayer.playlist()->next();
	QNetworkRequest request1(baseURL+urls.first());
	_reply = _netManager->get(request1);
	//this->pl->addMedia(QMediaResource(request));
	urls.pop_front();
}
