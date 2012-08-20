/*
 * Copyright (C) by Klaas Freitag <freitag@owncloud.com>
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

#include "mirall/mirallconfigfile.h"
#include "mirall/owncloudtheme.h"
#include "mirall/miralltheme.h"

#include <QtCore>
#include <QtGui>

#define DEFAULT_REMOTE_POLL_INTERVAL 30000 // default remote poll time in milliseconds
#define DEFAULT_LOCAL_POLL_INTERVAL  10000 // default local poll time in milliseconds
#define DEFAULT_POLL_TIMER_EXEED     10

#define OC_ORGANIZATION QLatin1String("ownCloud")
#define OC_APPLICATION  QLatin1String("ownCloudClient")

#define CA_CERTS_KEY QLatin1String("CaCertificates")

namespace Mirall {

QString MirallConfigFile::_passwd;
QString MirallConfigFile::_oCVersion;
bool    MirallConfigFile::_askedUser = false;

MirallConfigFile::MirallConfigFile( const QString& appendix )
    :_customHandle(appendix)
{
}

QString MirallConfigFile::configPath() const
{
    QString dir = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    if( !dir.endsWith(QLatin1Char('/')) ) dir.append(QLatin1Char('/'));
    return dir;
}

QString MirallConfigFile::excludeFile() const
{
    const QString exclFile(QLatin1String("exclude.lst"));
    QString dir = configPath();
    dir += exclFile;

    QFileInfo fi( dir );
    if( fi.isReadable() ) {
        return dir;
    }

    // Check alternative places...
#ifdef Q_OS_WIN32
    fi.setFile( QApplication::applicationDirPath(), exclFile );
#endif
#ifdef Q_OS_LINUX
    fi.setFile( QString("/etc"), exclFile );
#endif
#ifdef Q_OS_MAC
    // exec path is inside the bundle
    fi.setFile( QApplication::applicationDirPath(),
                QLatin1String("../Resources/") + exclFile );
#endif
    if( fi.isReadable() ) {
        qDebug() << "  ==> returning exclude file path: " << fi.absoluteFilePath();
        return fi.absoluteFilePath();
    }
    qDebug() << "EMPTY exclude file path!";
    return QString::null;
}

QString MirallConfigFile::configFile() const
{
#ifdef OWNCLOUD_CLIENT
    ownCloudTheme theme;
#else
    mirallTheme theme;
#endif
    if( qApp->applicationName().isEmpty() ) {
        qApp->setApplicationName( theme.appName() );
    }
    QString dir = configPath() + theme.configFileName();
    if( !_customHandle.isEmpty() ) {
        dir.append( QLatin1Char('_'));
        dir.append( _customHandle );
        qDebug() << "  OO Custom config file in use: " << dir;
    }
    return dir;
}

bool MirallConfigFile::exists()
{
    QFile file( configFile() );
    return file.exists();
}

QString MirallConfigFile::defaultConnection() const
{
    return QLatin1String("ownCloud");
}

bool MirallConfigFile::connectionExists( const QString& conn )
{
    QString con = conn;
    if( conn.isEmpty() ) con = defaultConnection();

    QSettings settings( configFile(), QSettings::IniFormat);
    settings.setIniCodec( "UTF-8" );

    return settings.contains( QString::fromLatin1("%1/url").arg( conn ) );
}


void MirallConfigFile::writeOwncloudConfig( const QString& connection,
                                            const QString& url,
                                            const QString& user,
                                            const QString& passwd,
                                            bool skipPwd )
{
    const QString file = configFile();
    qDebug() << "*** writing mirall config to " << file << " Skippwd: " << skipPwd;
    QString pwd( passwd );

    QSettings settings( file, QSettings::IniFormat);
    settings.setIniCodec( "UTF-8" );
    QString cloudsUrl( url );

    if( !cloudsUrl.startsWith( QLatin1String("http")) )
        cloudsUrl.prepend(QLatin1String("http://"));

    settings.beginGroup( connection );
    settings.setValue( QLatin1String("url"), cloudsUrl );
    settings.setValue( QLatin1String("user"), user );
    if( skipPwd ) {
        pwd.clear();
    }

    QByteArray pwdba = pwd.toUtf8();
    settings.setValue( QLatin1String("passwd"), QVariant(pwdba.toBase64()) );
    settings.setValue( QLatin1String("nostoredpassword"), QVariant(skipPwd) );
    settings.sync();

    // check the perms, only read-write for the owner.
    QFile::setPermissions( file, QFile::ReadOwner|QFile::WriteOwner );

}

// set the url, called from redirect handling.
void MirallConfigFile::setOwnCloudUrl( const QString& connection, const QString & url )
{
    const QString file = configFile();

    QSettings settings( file, QSettings::IniFormat);
    settings.setIniCodec( "UTF-8" );
    settings.beginGroup( connection );
    settings.setValue( QLatin1String("url"), url );

    settings.sync();
}

QByteArray MirallConfigFile::caCerts( )
{
    QSettings settings( configFile(), QSettings::IniFormat );
    settings.setIniCodec( "UTF-8" );

    QByteArray certs = settings.value( CA_CERTS_KEY ).toByteArray();

    return certs;
}

void MirallConfigFile::setCaCerts( const QByteArray & certs )
{
    const QString file = configFile();

    QSettings settings( file, QSettings::IniFormat);
    settings.setIniCodec( "UTF-8" );
    settings.setValue( CA_CERTS_KEY, certs );

    settings.sync();
}


void MirallConfigFile::removeConnection( const QString& connection )
{
    QString con( connection );
    if( connection.isEmpty() ) con = defaultConnection();

    qDebug() << "    removing the config file for connection " << con;

    // Currently its just removing the entire config file
    QSettings settings( configFile(), QSettings::IniFormat);
    settings.setIniCodec( "UTF-8" );
    settings.beginGroup( con );
    settings.remove(QString::null);  // removes all content from the group
    settings.sync();
}

/*
 * returns the configured owncloud url if its already configured, otherwise an empty
 * string.
 * The returned url always has a trailing hash.
 * If webdav is true, the webdav-server url is returned.
 */
QString MirallConfigFile::ownCloudUrl( const QString& connection, bool webdav ) const
{
    QString con( connection );
    if( connection.isEmpty() ) con = defaultConnection();

    QSettings settings( configFile(), QSettings::IniFormat );
    settings.setIniCodec( "UTF-8" );
    settings.beginGroup( con );

    // For the WebDAV connect it is required to know which version the server is running
    // because the url changed :-/
    if( webdav && _oCVersion.isEmpty() ) {
        qDebug() << "######## Config does not yet know the ownCloud server version #########";
        qDebug() << "###################### THIS SHOULD NOT HAPPEN! ########################";
    }

    QString url = settings.value( QLatin1String("url") ).toString();
    if( ! url.isEmpty() ) {
        if( ! url.endsWith(QLatin1Char('/'))) url.append(QLatin1String("/"));
        if( webdav ) url.append( QLatin1String("files/webdav.php/") );
    }

    // qDebug() << "Returning configured owncloud url: " << url;

  return url;
}

QString MirallConfigFile::ownCloudUser( const QString& connection ) const
{
    QString con( connection );
    if( connection.isEmpty() ) con = defaultConnection();

    QSettings settings( configFile(), QSettings::IniFormat );
    settings.setIniCodec( "UTF-8" );
    settings.beginGroup( con );

    QString user = settings.value( QLatin1String("user") ).toString();
    // qDebug() << "Returning configured owncloud user: " << user;

    return user;
}

int MirallConfigFile::remotePollInterval( const QString& connection ) const
{
  QString con( connection );
  if( connection.isEmpty() ) con = defaultConnection();

  QSettings settings( configFile(), QSettings::IniFormat );
  settings.setIniCodec( "UTF-8" );
  settings.beginGroup( con );

  int remoteInterval = settings.value( QLatin1String("remotePollInterval"), DEFAULT_REMOTE_POLL_INTERVAL ).toInt();
  int localInterval  = settings.value(QLatin1String("localPollInterval"), DEFAULT_LOCAL_POLL_INTERVAL ).toInt();
  if( remoteInterval < 2*localInterval ) {
    qDebug() << "WARN: remote poll Interval should at least be twice as local poll interval!";
  }
  if( remoteInterval < 5000 || remoteInterval < localInterval ) {
    qDebug() << "Remote Interval is smaller than local Interval";
    remoteInterval = DEFAULT_REMOTE_POLL_INTERVAL;
  }
  return remoteInterval;
}

int MirallConfigFile::localPollInterval( const QString& connection ) const
{
  QString con( connection );
  if( connection.isEmpty() ) con = defaultConnection();

  QSettings settings( configFile(), QSettings::IniFormat );
  settings.setIniCodec( "UTF-8" );
  settings.beginGroup( con );

  int remoteInterval = settings.value( QLatin1String("remotePollInterval"), DEFAULT_REMOTE_POLL_INTERVAL ).toInt();
  int localInterval  = settings.value(QLatin1String("localPollInterval"), DEFAULT_LOCAL_POLL_INTERVAL ).toInt();
  if( remoteInterval < 2*localInterval ) {
    qDebug() << "WARN: remote poll Interval should at least be twice as local poll interval!";
  }
  if( localInterval < 2500 || remoteInterval < localInterval ) {
    qDebug() << "Remote Interval is smaller than local Interval";
    localInterval = DEFAULT_LOCAL_POLL_INTERVAL;
  }
  return localInterval;
}

int MirallConfigFile::pollTimerExceedFactor( const QString& connection ) const
{
  QString con( connection );
  if( connection.isEmpty() ) con = defaultConnection();

  QSettings settings( configFile(), QSettings::IniFormat );
  settings.setIniCodec( "UTF-8" );
  settings.beginGroup( con );

  int pte = settings.value( QLatin1String("pollTimerExeedFactor"), DEFAULT_POLL_TIMER_EXEED).toInt();

  if( pte < 1 ) pte = DEFAULT_POLL_TIMER_EXEED;

  return pte;
}

QString MirallConfigFile::ownCloudPasswd( const QString& connection ) const
{
    QString con( connection );
    if( connection.isEmpty() ) con = defaultConnection();

    QSettings settings( configFile(), QSettings::IniFormat );
    settings.setIniCodec( "UTF-8" );
    settings.beginGroup( con );

    QString pwd;

    bool skipPwd = settings.value( QLatin1String("nostoredpassword"), false ).toBool();
    if( skipPwd ) {
        if( ! _askedUser ) {
            bool ok;
            QString text = QInputDialog::getText(0, QApplication::translate("MirallConfigFile","ownCloud Password Required"),
                                                 QApplication::translate("MirallConfigFile","Please enter your ownCloud password:"),
                                                 QLineEdit::Password,
                                                 QString::null, &ok);
            if( ok && !text.isEmpty() ) { // empty password is not allowed on ownCloud
                _passwd = text;
                _askedUser = true;
            }
        }
        pwd = _passwd;
    } else {
        QByteArray pwdba = settings.value(QLatin1String("passwd")).toByteArray();
        if( pwdba.isEmpty() ) {
            // check the password entry, cleartext from before
            // read it and convert to base64, delete the cleartext entry.
            QString p = settings.value(QLatin1String("password")).toString();

            if( ! p.isEmpty() ) {
                // its there, save base64-encoded and delete.

                pwdba = p.toUtf8();
                settings.setValue( QLatin1String("passwd"), QVariant(pwdba.toBase64()) );
                settings.remove( QLatin1String("password") );
                settings.sync();
            }
        }
        pwd = QString::fromUtf8( QByteArray::fromBase64(pwdba) );
    }

    return pwd;
}

QString MirallConfigFile::ownCloudVersion() const
{
    return _oCVersion;
}

void MirallConfigFile::setOwnCloudVersion( const QString& ver)
{
    qDebug() << "** Setting ownCloud Server version to " << ver;
    _oCVersion = ver;
}


bool MirallConfigFile::ownCloudSkipUpdateCheck( const QString& connection ) const
{
    QString con( connection );
    if( connection.isEmpty() ) con = defaultConnection();

    QSettings settings( configFile(), QSettings::IniFormat );
    settings.setIniCodec( "UTF-8" );
    settings.beginGroup( con );

    bool skipIt = settings.value( QLatin1String("skipUpdateCheck"), false ).toBool();

    return skipIt;
}

int MirallConfigFile::maxLogLines() const
{
    QSettings settings( configFile(), QSettings::IniFormat );
    settings.setIniCodec( "UTF-8" );
    settings.beginGroup(QLatin1String("Logging"));
    int logLines = settings.value( QLatin1String("maxLogLines"), 20000 ).toInt();
    return logLines;
}

QByteArray MirallConfigFile::basicAuthHeader() const
{
    QString concatenated = ownCloudUser() + QLatin1Char(':') + ownCloudPasswd();
    const QString b(QLatin1String("Basic "));
    QByteArray data = b.toLocal8Bit() + concatenated.toLocal8Bit().toBase64();

    return data;
}

// remove a custom config file.
void MirallConfigFile::cleanupCustomConfig()
{
    if( _customHandle.isEmpty() ) {
        qDebug() << "SKipping to erase the main configuration.";
        return;
    }
    QString file = configFile();
    if( QFile::exists( file ) ) {
        QFile::remove( file );
    }
}

// accept a config identified by the customHandle as general config.
void MirallConfigFile::acceptCustomConfig()
{
    if( _customHandle.isEmpty() ) {
        qDebug() << "WRN: Custom Handle is empty. Can not accept.";
        return;
    }

    QString srcConfig = configFile(); // this considers the custom handle
    _customHandle.clear();
    QString targetConfig = configFile();
    QString targetBak = targetConfig + QLatin1String(".bak");

    bool bakOk = false;
    // remove an evtl existing old config backup.
    if( QFile::exists( targetBak ) ) {
        QFile::remove( targetBak );
    }
    // create a backup of the current config.
    bakOk = QFile::rename( targetConfig, targetBak );

    // move the custom config to the master place.
    if( ! QFile::rename( srcConfig, targetConfig ) ) {
        // if the move from custom to master failed, put old backup back.
        if( bakOk ) {
            QFile::rename( targetBak, targetConfig );
        }
    }
    QFile::remove( targetBak );
}

QVariant MirallConfigFile::customMedia( customMediaType type )
{
    QVariant re;
    QString key;

    if( type == oCSetupTop ) {
        key = QLatin1String("oCSetupTop");
    } else if( type == oCSetupSide ) {
        key = QLatin1String("oCSetupSide");
    } else if( type == oCSetupBottom) {
        key = QLatin1String("oCSetupBottom");
    } else if( type == oCSetupFixUrl ) {
        key = QLatin1String("oCSetupFixUrl");
    } else if( type == oCSetupResultTop ) {
        key = QLatin1String("oCSetupResultTop");
    } else {
        qDebug() << "Wrong media type.";
    }

    if( !key.isEmpty() ) {
        QSettings settings( QSettings::IniFormat, QSettings::SystemScope, OC_ORGANIZATION, OC_APPLICATION );
        QString cfg = settings.fileName();
        qDebug() << "Trying to read config ini file at " << cfg;

        settings.setIniCodec( "UTF-8" );
        settings.beginGroup(QLatin1String("GUICustomize"));
        QString val = settings.value( key ).toString();

        if( !val.isEmpty() ) {
            QPixmap pix( val );
            if( pix.isNull() ) {
                // pixmap loading hasn't succeeded. We take the text instead.
                re.setValue( val );
            } else {
                re.setValue( pix );
            }
        }
    }
    return re;
}

void MirallConfigFile::setProxyType(int proxyType,
                  const QString& host,
                  int port,
                  const QString& user,
                  const QString& pass)
{
    QSettings settings( configFile(), QSettings::IniFormat );
    settings.setIniCodec( "UTF-8" );
    settings.beginGroup(QLatin1String("proxy"));

    settings.setValue(QLatin1String("type"), proxyType);
    settings.setValue(QLatin1String("host"), host);
    settings.setValue(QLatin1String("port"), port);
    settings.setValue(QLatin1String("user"), user);
    settings.setValue(QLatin1String("pass"), pass);

    settings.sync();
}

QVariant MirallConfigFile::getValue(const QString& param, const QString& group) const
{
    QSettings settings( configFile(), QSettings::IniFormat );
    settings.setIniCodec( "UTF-8" );
    settings.beginGroup(group);

    return settings.value(param);
}

int MirallConfigFile::proxyType() const
{
    return getValue(QLatin1String("type"), QLatin1String("proxy")).toInt();
}

QString MirallConfigFile::proxyHostName() const
{
    return getValue(QLatin1String("host"), QLatin1String("proxy")).toString();
}

int MirallConfigFile::proxyPort() const
{
    return getValue(QLatin1String("port"), QLatin1String("proxy")).toInt();
}

QString MirallConfigFile::proxyUser() const
{
    return getValue(QLatin1String("user"), QLatin1String("proxy")).toString();
}

QString MirallConfigFile::proxyPassword() const
{
    QByteArray pass = getValue(QLatin1String("pass"), QLatin1String("proxy")).toByteArray();
    return QString::fromUtf8(QByteArray::fromBase64(pass));
}

}
