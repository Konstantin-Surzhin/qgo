#include "quickconnection.h"
#include "networkconnection.h"

QuickConnection::QuickConnection(char * host, unsigned short port, void * m, NetworkConnection * c, QuickConnectionType t) :
msginfo(m), connection(c), type(t)
{
	qsocket = new QTcpSocket();	//try with no parent passed for now
	if(!qsocket)
	{
		success = 0;
		return;
	}
	
	connect(qsocket, SIGNAL(connected()), SLOT(OnConnected()));
	connect(qsocket, SIGNAL(readyRead()), SLOT(OnReadyRead()));
	connect(qsocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(OnError(QAbstractSocket::SocketError)));
	
	Q_ASSERT(host != 0);
	Q_ASSERT(port != 0);
	qDebug("Connecting to %s %d...\n", host, port);
	
	qsocket->connectToHost(QString(host), (quint16) port);
	
	success = !(qsocket->state() != QTcpSocket::UnconnectedState);
}

QuickConnection::QuickConnection(QTcpSocket * q, void * m, NetworkConnection * c, QuickConnectionType t) :
qsocket(q), msginfo(m), connection(c), type(t)
{
	/* FIXME Can we even do this?  Switch signals while
	 * they're running? */
	disconnect(qsocket, SIGNAL(hostFound()), connection, 0);
	disconnect(qsocket, SIGNAL(connected()), connection, 0);
	disconnect(qsocket, SIGNAL(readyRead()), connection, 0);
	disconnect(qsocket, SIGNAL(disconnected ()), connection, 0);
	disconnect(qsocket, SIGNAL(error(QAbstractSocket::SocketError)), connection, 0);

	connect(qsocket, SIGNAL(connected()), SLOT(OnConnected()));
	connect(qsocket, SIGNAL(readyRead()), SLOT(OnReadyRead()));
	connect(qsocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(OnError(QAbstractSocket::SocketError)));
	
	success = !(qsocket->state() != QTcpSocket::UnconnectedState);
	if(success != 0)
	{
		qDebug("QC passed bad connection");
		return;
	}
	OnConnected();
}

int QuickConnection::checkSuccess(void)
{
	return success;
}

void QuickConnection::OnConnected(void)
{
	int size;
	char * packet;
	//qDebug("QC: OnConnected()");
	switch(type)
	{
		case sendRequestAccountInfo:
			packet = connection->sendRequestAccountInfo(&size, msginfo);
			break;
		case sendAddFriend:
			packet = connection->sendAddFriend(&size, msginfo);
			break;
		case sendRemoveFriend:
			packet = connection->sendRemoveFriend(&size, msginfo);
			break;
	}
	if (qsocket->write(packet, size) < 0)
		qDebug("QuickConnection write failed");
	delete[] packet;
}

void QuickConnection::OnReadyRead(void)
{
	int bytes = qsocket->bytesAvailable();
	if(bytes > 1)
	{
		char * packet = new char[bytes];
		qsocket->read(packet, bytes);
		switch(type)
		{
			case sendRequestAccountInfo:
				connection->handleAccountInfoMsg(bytes, packet);
				break;
			case sendAddFriend:
			case sendRemoveFriend:
				connection->recvFriendResponse(bytes, packet);
				break;
		}
		delete[] packet;
	}
}

void QuickConnection::OnError(QAbstractSocket::SocketError err)
{
	switch(err)
	{
		case QTcpSocket::ConnectionRefusedError: qDebug("ERROR: connection refused...");
			break;
		case QTcpSocket::HostNotFoundError: qDebug("ERROR: host not found...");
			break;
		case QTcpSocket::SocketTimeoutError: qDebug("ERROR: socket time out ...");
			break;
		case QTcpSocket::RemoteHostClosedError: qDebug("ERROR: connection closed by host ...");
			break;
		default: qDebug("ERROR: unknown Error...");
			break;
	}
}

QuickConnection::~QuickConnection()
{
	if(qsocket)
	{
		qsocket->close();
		qsocket->deleteLater();
	}
}