/*
 * Copyright (C) by Klaas Freitag <freitag@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#ifndef OWNCLOUDINFO_H
#define OWNCLOUDINFO_H

#include <QObject>
#include <QtNetwork>

#if QT_VERSION >= 0x040700
#define QT46_IMPL 0
#else
#define QT46_IMPL 1
#endif

namespace Mirall
{

class SslErrorDialog;

class ownCloudInfo : public QObject
{
    Q_OBJECT
public:

  static ownCloudInfo *instance();

    bool isConfigured();

    /**
      * call status.php
      */
    void checkInstallation();

    /**
      * a general GET request to the ownCloud. If the second bool parameter is
      * true, the WebDAV server is queried.
      */
    void getRequest( const QString&, bool );

    /**
      * convenience: GET request to the WebDAV server.
      */
    void getWebDAVPath( const QString& );

    /**
      * There is a global flag here if the user once decided against trusting the
      * SSL connection. This method resets it so that the ssl dialog is shown again.
      */
    void resetSSLUntrust();

    /**
      * Create a collection via owncloud. Provide a relative path.
      */
    void mkdirRequest( const QString& );

    /**
     * Use a custom ownCloud configuration file identified by handle
     */
    void setCustomConfigHandle( const QString& );

signals:
    // result signal with url- and version string.
    void ownCloudInfoFound( const QString&, const QString&, const QString&, const QString& );
    void noOwncloudFound( QNetworkReply* );
    void ownCloudDirExists( const QString&, QNetworkReply* );

    void webdavColCreated( QNetworkReply::NetworkError );

public slots:

protected slots:
    void slotReplyFinished( );
    void slotError( QNetworkReply::NetworkError );
    void slotAuthentication( QNetworkReply*, QAuthenticator *);
    void slotSSLFailed( QNetworkReply *reply, QList<QSslError> errors );

#if QT46_IMPL
    void qhttpRequestFinished(int id, bool success );
    void qhttpRequestStarted(int id);
    void qhttpResponseHeaderReceived(const QHttpResponseHeader& header);
//    void qhttpAuthenticationRequired(const QString& hostname, quint16 port ,QAuthenticator* authenticator);
#else
    void slotMkdirFinished();
#endif

private:
    explicit ownCloudInfo( const QString& = QString(), QObject *parent = 0);

    QUrl redirectUrl(const QUrl&, const QUrl& ) const;

    ~ownCloudInfo();

    void setupHeaders(QNetworkRequest &req, quint64 size );
#if QT46_IMPL
#else
    QNetworkReply* davRequest(const QString&, QNetworkRequest&, QByteArray* );
#endif

    static ownCloudInfo           *_instance;

    QNetworkAccessManager         *_manager;
    QString                        _connection;
    QString                        _configHandle;
    QUrl                           _urlRedirectedTo;
    QHash<QNetworkReply*, QString> _directories;
    QHash<QNetworkReply*, QString> _configHandleMap;
    SslErrorDialog                *_sslErrorDialog;
    bool                           _certsUntrusted;
    int                            _authAttempts;
};

};

#endif // OWNCLOUDINFO_H
