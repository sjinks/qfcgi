#include <QtCore/QtEndian>
#include <QtCore/QObject>
#include <QtCore/QIODevice>
#include <cstring>
#include <new>
#include "protocol/beginrequest_p.h"
#include "protocol/unknowntype_p.h"
#include "connection.h"
#include "connection_p.h"
#include "request.h"
#include "request_p.h"
#include "outputstream.h"
#include "utils.h"

static bool reg = false;

Q_DECLARE_METATYPE(QLocalSocket::LocalSocketError)

FastCGI::LowLevel::ConnectionPrivate::ConnectionPrivate(QIODevice* sock, Connection* q)
	: q_ptr(q), m_sock(sock), m_info()
{
	if (!reg) {
		qRegisterMetaType<QAbstractSocket::SocketError>();
		qRegisterMetaType<QLocalSocket::LocalSocketError>();
		qRegisterMetaType<FastCGI::LowLevel::Request*>();
		qRegisterMetaType<QList<QPair<QByteArray, QByteArray> > >();
		reg = true;
	}

	QObject::connect(sock, SIGNAL(readyRead()), q, SLOT(_q_readyRead()));
	QAbstractSocket* x;
	QLocalSocket* y;
	if ((x = qobject_cast<QAbstractSocket*>(sock)) != Q_NULLPTR) {
		QObject::connect(x, SIGNAL(error(QAbstractSocket::SocketError)), q, SLOT(_q_error()), Qt::QueuedConnection);
		QObject::connect(x, SIGNAL(disconnected()), q, SLOT(_q_disconnected()), Qt::QueuedConnection);
	}
	else if ((y = qobject_cast<QLocalSocket*>(sock)) != Q_NULLPTR) {
		QObject::connect(y, SIGNAL(error(QLocalSocket::LocalSocketError)), q, SLOT(_q_error()), Qt::QueuedConnection);
		QObject::connect(y, SIGNAL(disconnected()), q, SLOT(_q_disconnected()), Qt::QueuedConnection);
	}
}

FastCGI::LowLevel::ConnectionPrivate::~ConnectionPrivate()
{
}

void FastCGI::LowLevel::ConnectionPrivate::_q_readyRead()
{
	SocketInfo& info = this->m_info;
	while (this->m_sock && this->m_sock->bytesAvailable() > 0) {
		if (SocketInfo::BeforeHeader == info.state) {
			info.read  = 0;
			info.state = SocketInfo::ReadingHeader;
		}

		if (SocketInfo::ReadingHeader == info.state) {
			qint64 n = this->m_sock->read(reinterpret_cast<char*>(info.hdr.b) + info.read, static_cast<qint64>(sizeof(info.hdr) - static_cast<std::size_t>(info.read)));
			if (n < 0) {
				// TODO: report error
				this->killSocket();
				return;
			}

			info.read += n;
			info.state = (static_cast<quint64>(info.read) == sizeof(info.hdr))
				? SocketInfo::BeforeData
				: SocketInfo::ReadingHeader
			;
		}

		if (SocketInfo::BeforeData == info.state) {
			if (info.hdr.h.version != FASTCGI_VERSION || info.hdr.h.getFullLength() > FCGI_MAX_LENGTH) {
				this->killSocket();
				return;
			}

			int len = static_cast<int>(info.hdr.h.getFullLength());
			info.buf.clear();
			info.buf.reserve(len);
			info.state = SocketInfo::ReadingData;
		}

		if (SocketInfo::ReadingData == info.state) {
			int len = static_cast<int>(info.hdr.h.getFullLength());
			if (len > 0) {
				qint64 avail   = this->m_sock->bytesAvailable();
				QByteArray buf = this->m_sock->read(len);

				if (!buf.isEmpty()) {
					info.buf.append(buf);
				}
				else if (avail) {
					// error
					this->killSocket();
					return;
				}
			}

			info.state = (info.buf.size() == len)
				? SocketInfo::BeforeHeader
				: SocketInfo::ReadingData
			;
		}

		if (SocketInfo::BeforeHeader == info.state) {
			info.buf.truncate(info.hdr.h.getContentLength());
			if (info.hdr.h.isManagementRecord()) {
				this->processManagementRecord();
			}
			else if (FCGI_BEGIN_REQUEST == info.hdr.h.getType()) {
				this->processBeginRequestRecord();
			}
			else {
				this->processOtherRecord();
			}
		}
	}
}

void FastCGI::LowLevel::ConnectionPrivate::_q_disconnected()
{
	if (this->m_sock) {
		Q_Q(FastCGI::LowLevel::Connection);
		this->disconnectSocketSignals();
		this->m_sock.reset();
		Q_EMIT q->disconnected();
	}
}

void FastCGI::LowLevel::ConnectionPrivate::_q_error()
{
	Q_Q(FastCGI::LowLevel::Connection);
//	QIODevice* dev = qobject_cast<QIODevice*>(q->sender());
//	qWarning("%s", qPrintable(dev->errorString()));

	if (this->m_sock) {
		this->disconnectSocketSignals();
		this->m_sock.reset();
		Q_EMIT q->disconnected();
	}
}

void FastCGI::LowLevel::ConnectionPrivate::_q_requestFinished(quint16 id)
{
	this->m_info.reqs.remove(id);

	QAbstractSocket* x;
	QLocalSocket* y;

	if ((x = qobject_cast<QAbstractSocket*>(this->m_sock.data())) != Q_NULLPTR) {
		if (x->state() != QAbstractSocket::ConnectedState) {
			this->m_sock.reset();
		}
	}
	else if ((y = qobject_cast<QLocalSocket*>(this->m_sock.data())) != Q_NULLPTR) {
		if (y->state() != QLocalSocket::ConnectedState) {
			this->m_sock.reset();
		}
	}
}

void FastCGI::LowLevel::ConnectionPrivate::processManagementRecord()
{
	quint8 type = this->m_info.hdr.h.getType();
	switch (type) {
		case FCGI_GET_VALUES: {
			QList<QPair<QByteArray, QByteArray> > pairs;
			if (FastCGI::LowLevel::parseNV(this->m_info.buf, pairs)) {
				FastCGI::Protocol::Header h(FCGI_GET_VALUES_RESULT);
				QList<QPair<QByteArray, QByteArray> > resp;
				h.setRequestId(0);

				for (int i=0; i<pairs.size(); ++i) {
					const QPair<QByteArray, QByteArray>& pair = pairs.at(i);
					if (pair.first == QByteArrayLiteral("FCGI_MAX_CONNS")) {
					}
					else if (pair.first == QByteArrayLiteral("FCGI_MAX_REQS")) {
					}
					else if (pair.first == QByteArrayLiteral("FCGI_MPXS_CONNS")) {
						resp.append(qMakePair(pair.first, QByteArrayLiteral("1")));
					}
				}

				QByteArray payload = FastCGI::LowLevel::flattenNV(resp);
				quint16 len        = static_cast<quint16>(payload.size());
				h.setContentLength(len);

				this->m_sock->write(static_cast<const char*>(h.raw()), sizeof(h));
				this->m_sock->write(payload);
				return;
			}
		}
	}

	// (4.2) When an application receives a management record whose type T it does not understand, the application responds with {FCGI_UNKNOWN_TYPE, 0, {T}}.
	FastCGI::Protocol::UnknownTypeRecord r(type);
	this->m_sock->write(static_cast<const char*>(r.raw()), sizeof(r));
}

void FastCGI::LowLevel::ConnectionPrivate::processBeginRequestRecord()
{
	Q_Q(FastCGI::LowLevel::Connection);

	FastCGI::Protocol::BeginRequest& b = *new(this->m_info.buf.data()) FastCGI::Protocol::BeginRequest(nullptr);
	auto reqid = this->m_info.hdr.h.getRequestId();
	auto role  = b.getRole();

	Request* req = new Request(reqid, b, this->m_sock);
	req->setParent(q);
	QObject::connect(req, SIGNAL(requestFinished(quint16)), q, SLOT(_q_requestFinished(quint16)), Qt::QueuedConnection);

	if (role < FCGI_ROLE_MIN || role > FCGI_ROLE_MAX) {
		Q_EMIT req->protocolError();
		req->finish(FastCGI::LowLevel::UnknownRole);
		return;
	}

	this->m_info.reqs.insert(reqid, req);
	Q_EMIT q->newRequest(req);
}

void FastCGI::LowLevel::ConnectionPrivate::processOtherRecord()
{
	// (3.3) While a request ID R is inactive, the application ignores records with requestId == R, except for FCGI_BEGIN_REQUEST records
	auto it = this->m_info.reqs.find(this->m_info.hdr.h.getRequestId());
	if (it != this->m_info.reqs.end()) {
		Request* req = it.value();
		QMetaObject::invokeMethod(req, "_q_processRecord", Qt::QueuedConnection, Q_ARG(quint8, this->m_info.hdr.h.getType()), Q_ARG(QByteArray, this->m_info.buf));
	}
}

void FastCGI::LowLevel::ConnectionPrivate::disconnectSocketSignals()
{
	Q_Q(FastCGI::LowLevel::Connection);
	this->m_sock->disconnect(q);
}

void FastCGI::LowLevel::ConnectionPrivate::killSocket()
{
	this->disconnectSocketSignals();
	if (this->m_sock) {
		QAbstractSocket* x;
		QLocalSocket* y;
		if ((x = qobject_cast<QAbstractSocket*>(this->m_sock.data())) != Q_NULLPTR) {
			if (x->state() != QAbstractSocket::UnconnectedState) {
				x->close();
			}
		}
		else if ((y = qobject_cast<QLocalSocket*>(this->m_sock.data())) != Q_NULLPTR) {
			if (y->state() != QLocalSocket::UnconnectedState) {
				y->close();
			}
		}

		this->m_sock.reset();
	}
}

#include "moc_connection.cpp"
