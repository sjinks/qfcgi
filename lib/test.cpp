#include <QtCore/QCoreApplication>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include "connection.h"
#include "request.h"
#include "outputstream.h"

static int requests_in_progress = 0;

class MyApplication : public QCoreApplication {
	Q_OBJECT
public:
	MyApplication(int argc, char** argv)
		: QCoreApplication(argc, argv), srv(new QTcpServer(this)), tid(this->startTimer(30000))
	{
	}

	int exec()
	{
		this->srv->listen(QHostAddress::LocalHost, 9999);
		QObject::connect(this->srv, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
		return QCoreApplication::exec();
	}

public Q_SLOTS:
	void onNewConnection()
	{
		QTcpSocket* sock = this->srv->nextPendingConnection();
		auto conn = new FastCGI::LowLevel::Connection(sock);
		++requests_in_progress;
		QObject::connect(conn, SIGNAL(newRequest(FastCGI::LowLevel::Request*)), this, SLOT(onNewRequest(FastCGI::LowLevel::Request*)));
		QObject::connect(conn, &FastCGI::LowLevel::Connection::destroyed, this, &MyApplication::onConnectionDestroyed);
	}

	void onNewRequest(FastCGI::LowLevel::Request* req)
	{
		QObject::connect(req, SIGNAL(parametersParsed(QList<QPair<QByteArray,QByteArray> >)), this, SLOT(onParametersParsed(QList<QPair<QByteArray,QByteArray> >)));
	}

	void onParametersParsed(const QList<QPair<QByteArray, QByteArray> >& params)
	{
		Q_UNUSED(params)
		FastCGI::LowLevel::Request* req = qobject_cast<FastCGI::LowLevel::Request*>(this->sender());
		Q_ASSERT(req != Q_NULLPTR);

//		for (int i=0; i<params.size(); ++i) {
//			qDebug() << params.at(i).first << params.at(i).second;
//		}

		FastCGI::LowLevel::OutputStream* stdout = req->stdOut();
		stdout->write(QByteArray("Status: 200 OK\r\n\r\nHi!\n"));
		req->finish();
	}

	void onConnectionDestroyed()
	{
		--requests_in_progress;
	}

protected:
	void timerEvent(QTimerEvent* e)
	{
		if (e->timerId() == this->tid) {
			qDebug("Requests in progress: %d", requests_in_progress);
		}
	}

private:
	QTcpServer* srv;
	int tid;
};

int main(int argc, char** argv)
{
	MyApplication app(argc, argv);
	return app.exec();
}

#include "test.moc"
