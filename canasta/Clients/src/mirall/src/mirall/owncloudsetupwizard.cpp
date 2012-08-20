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

#include "mirall/owncloudsetupwizard.h"
#include "mirall/mirallconfigfile.h"
#include "mirall/owncloudinfo.h"
#include "mirall/folderman.h"

#include <QtCore>
#include <QProcess>
#include <QMessageBox>
#include <QDesktopServices>

namespace Mirall {

class Theme;

OwncloudSetupWizard::OwncloudSetupWizard( FolderMan *folderMan, Theme *theme, QObject *parent ) :
    QObject( parent ),
    _folderMan(folderMan)
{
    _process = new QProcess( this );

    QObject::connect(_process, SIGNAL(readyReadStandardOutput()),
                     SLOT(slotReadyReadStandardOutput()));

    QObject::connect(_process, SIGNAL(readyReadStandardError()),
                     SLOT(slotReadyReadStandardError()));

    QObject::connect(_process, SIGNAL(stateChanged(QProcess::ProcessState)),
                     SLOT(slotStateChanged(QProcess::ProcessState)));

    QObject::connect(_process, SIGNAL(error(QProcess::ProcessError)),
                     SLOT(slotError(QProcess::ProcessError)));

    QObject::connect(_process, SIGNAL(started()),
                     SLOT(slotStarted()));

    QObject::connect(_process, SIGNAL(finished(int, QProcess::ExitStatus)),
                     SLOT(slotProcessFinished(int, QProcess::ExitStatus)));


    _ocWizard = new OwncloudWizard();

    connect( _ocWizard, SIGNAL(connectToOCUrl( const QString& ) ),
             this, SLOT(slotConnectToOCUrl( const QString& )));

    connect( _ocWizard, SIGNAL(installOCServer()),
             this, SLOT(slotInstallOCServer()));

    connect( _ocWizard, SIGNAL(installOCLocalhost()),
             this, SLOT(slotCreateOCLocalhost()));

    connect( _ocWizard, SIGNAL(finished(int)),this,SLOT(slotAssistantFinished(int)));

    // in case of cancel, terminate the owncloud-admin script.
    connect( _ocWizard, SIGNAL(rejected()), _process, SLOT(terminate()));

    _ocWizard->setWindowTitle( tr("%1 Connection Wizard").arg( theme ? theme->appName() : QLatin1String("Mirall") ) );

}

OwncloudSetupWizard::~OwncloudSetupWizard()
{

}

// Method executed when the user ends the wizard, either with 'accept' or 'reject'.
// accept the custom config to be the main one if Accepted.
void OwncloudSetupWizard::slotAssistantFinished( int result )
{
    MirallConfigFile cfg( _configHandle );

    if( result == QDialog::Rejected ) {
        // the old config remains valid. Remove the temporary one.
        cfg.cleanupCustomConfig();
        qDebug() << "Rejected the new config, use the old!";
    } else if( result == QDialog::Accepted ) {
        qDebug() << "Config Changes were accepted!";
        cfg.acceptCustomConfig();

        // wipe all folder definitions so far.
        if( _folderMan ) _folderMan->removeAllFolderDefinitions();

        // Now write the resulting folder definition if folder names are set.
        if( !( _localFolder.isEmpty() || _remoteFolder.isEmpty() ) ) { // both variables are set.
            if( _folderMan ) {
                _folderMan->addFolderDefinition( QLatin1String("owncloud"), QLatin1String("ownCloud"), _localFolder, _remoteFolder, false );
                _ocWizard->appendToResultWidget(tr("<font color=\"green\"><b>Local sync folder %1 successfully created!</b></font>").arg(_localFolder));
            } else {
                qDebug() << "WRN: Folderman is zero in Setup Wizzard.";
            }
        }
    } else {
        qDebug() << "WRN: Got unknown dialog result code " << result;
    }

    // clear the custom config handle
    _configHandle.clear();
    ownCloudInfo::instance()->setCustomConfigHandle( QString::null );

    // disconnect the ocInfo object
    disconnect(ownCloudInfo::instance(), SIGNAL(ownCloudInfoFound(QString,QString,QString,QString)),
               this, SLOT(slotOwnCloudFound(QString,QString,QString,QString)));
    disconnect(ownCloudInfo::instance(), SIGNAL(noOwncloudFound(QNetworkReply*)),
               this, SLOT(slotNoOwnCloudFound(QNetworkReply*)));
    disconnect(ownCloudInfo::instance(), SIGNAL(webdavColCreated(QNetworkReply::NetworkError)),
               this, SLOT(slotCreateRemoteFolderFinished(QNetworkReply::NetworkError)));

    // notify others.
    emit ownCloudWizardDone( result );
}

void OwncloudSetupWizard::slotConnectToOCUrl( const QString& url )
{
  qDebug() << "Connect to url: " << url;
  _ocWizard->setField(QLatin1String("OCUrl"), url );
  _ocWizard->appendToResultWidget(tr("Trying to connect to ownCloud at %1...").arg(url ));
  testOwnCloudConnect();
}

void OwncloudSetupWizard::testOwnCloudConnect()
{
    // write a temporary config.
    QDateTime now = QDateTime::currentDateTime();
    _configHandle = now.toString(QLatin1String("MMddyyhhmmss"));

    MirallConfigFile cfgFile( _configHandle );

    cfgFile.writeOwncloudConfig( QLatin1String("ownCloud"),
                                 _ocWizard->field(QLatin1String("OCUrl")).toString(),
                                 _ocWizard->field(QLatin1String("OCUser")).toString(),
                                 _ocWizard->field(QLatin1String("OCPasswd")).toString(),
                                 _ocWizard->field(QLatin1String("PwdNoLocalStore")).toBool() );

    // now start ownCloudInfo to check the connection.
    ownCloudInfo::instance()->setCustomConfigHandle( _configHandle );
    if( ownCloudInfo::instance()->isConfigured() ) {
        // reset the SSL Untrust flag to let the SSL dialog appear again.
        ownCloudInfo::instance()->resetSSLUntrust();
        ownCloudInfo::instance()->checkInstallation();
    } else {
        qDebug() << "   ownCloud seems not to be configured, can not start test connect.";
    }
}

void OwncloudSetupWizard::slotOwnCloudFound( const QString& url, const QString& infoString, const QString& version, const QString& )
{
    _ocWizard->appendToResultWidget(tr("<font color=\"green\">Successfully connected to %1: ownCloud version %2 (%3)</font><br/><br/>").arg( url ).arg(infoString).arg(version));

    // enable the finish button.
    _ocWizard->button( QWizard::FinishButton )->setEnabled( true );

    // start the local folder creation
    setupLocalSyncFolder();
}

void OwncloudSetupWizard::slotNoOwnCloudFound( QNetworkReply *err )
{
    _ocWizard->appendToResultWidget(tr("<font color=\"red\">Failed to connect to ownCloud!</font>") );
    _ocWizard->appendToResultWidget(tr("Error: <tt>%1</tt>").arg(err->errorString()) );

    // remove the config file again
    MirallConfigFile cfgFile( _configHandle );
    cfgFile.cleanupCustomConfig();
    finalizeSetup( false );
}

bool OwncloudSetupWizard::isBusy()
{
  return _process->state() > 0;
}

 OwncloudWizard *OwncloudSetupWizard::wizard()
 {
   return _ocWizard;
 }

void OwncloudSetupWizard::slotCreateOCLocalhost()
{
  if( isBusy() ) {
    qDebug() << "Can not install now, busy. Come back later.";
    return;
  }

  qDebug() << "Install OC on localhost";

  QStringList args;

  args << QLatin1String("install");
  args << QLatin1String("--server-type") << QLatin1String("local");
  args << QLatin1String("--root_helper") << QLatin1String("kdesu -c");

  const QString adminUser = _ocWizard->field(QLatin1String("OCUser")).toString();
  const QString adminPwd  = _ocWizard->field(QLatin1String("OCPasswd")).toString();

  args << QLatin1String("--admin-user") << adminUser;
  args << QLatin1String("--admin-password") << adminPwd;

  runOwncloudAdmin( args );

  // define
  _ocWizard->setField( QLatin1String("OCUrl"), QLatin1String( "http://localhost/owncloud/") );
}

void OwncloudSetupWizard::slotInstallOCServer()
{
  if( isBusy() ) {
    qDebug() << "Can not install now, busy. Come back later.";
    return;
  }

  const QString server = _ocWizard->field(QLatin1String("ftpUrl")).toString();
  const QString user   = _ocWizard->field(QLatin1String("ftpUser")).toString();
  const QString passwd = _ocWizard->field(QLatin1String("ftpPasswd")).toString();
  const QString adminUser = _ocWizard->field(QLatin1String("OCUser")).toString();
  const QString adminPwd  = _ocWizard->field(QLatin1String("OCPasswd")).toString();

  qDebug() << "Install OC on " << server << " as user " << user;

  QStringList args;
  args << QLatin1String("install");
  args << QLatin1String("--server-type") << QLatin1String("ftp");
  args << QLatin1String("--server")   << server;
  args << QLatin1String("--ftp-user")     << user;
  if( ! passwd.isEmpty() ) {
    args << QLatin1String("--ftp-password") << passwd;
  }
  args << QLatin1String("--admin-user") << adminUser;
  args << QLatin1String("--admin-password") << adminPwd;

  runOwncloudAdmin( args );
  _ocWizard->setField( QLatin1String("OCUrl"), QString::fromLatin1( "%1/owncloud/")
                       .arg(_ocWizard->field(QLatin1String("myOCDomain")).toString() ));
}

void OwncloudSetupWizard::runOwncloudAdmin( const QStringList& args )
{
  const QString bin(QLatin1String("/usr/bin/owncloud-admin"));
  qDebug() << "starting " << bin << " with args. " << args;
  if( _process->state() != QProcess::NotRunning	) {
    qDebug() << "Owncloud admin is still running, skip!";
    return;
  }
  if( checkOwncloudAdmin( bin )) {
    _ocWizard->appendToResultWidget( tr("Starting script owncloud-admin...") );
    _process->start( bin, args );
  } else {
    slotProcessFinished( 1, QProcess::NormalExit );
  }
}


void OwncloudSetupWizard::slotReadyReadStandardOutput()
{
  QByteArray arr = _process->readAllStandardOutput();
  QTextCodec *codec = QTextCodec::codecForName("UTF-8");
  // render the output to status line
  QString string = codec->toUnicode( arr );
  _ocWizard->appendToResultWidget( string, OwncloudWizard::LogPlain );

}

void OwncloudSetupWizard::slotReadyReadStandardError()
{
  qDebug() << "!! " <<_process->readAllStandardError();
}

void OwncloudSetupWizard::slotStateChanged( QProcess::ProcessState )
{

}

void OwncloudSetupWizard::slotError( QProcess::ProcessError err )
{
  qDebug() << "An Error happend with owncloud-admin: " << err << ", exit-Code: " << _process->exitCode();
}

void OwncloudSetupWizard::slotStarted()
{
  _ocWizard->button( QWizard::FinishButton )->setEnabled( false );
  _ocWizard->button( QWizard::BackButton )->setEnabled( false );
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
}

/*
 *
 */
void OwncloudSetupWizard::slotProcessFinished( int res, QProcess::ExitStatus )
{
  _ocWizard->button( QWizard::FinishButton )->setEnabled( true );
  _ocWizard->button( QWizard::BackButton)->setEnabled( true );
  QApplication::restoreOverrideCursor();

  qDebug() << "exit code: " << res;
  if( res ) {
    _ocWizard->appendToResultWidget( tr("<font color=\"red\">Installation of ownCloud failed!</font>") );
    _ocWizard->showOCUrlLabel( false );
    emit ownCloudSetupFinished( false );
  } else {
    // Successful installation. Write the config.
    _ocWizard->appendToResultWidget( tr("<font color=\"green\">Installation of ownCloud succeeded!</font>") );
    _ocWizard->showOCUrlLabel( true );

    testOwnCloudConnect();
  }
}

void OwncloudSetupWizard::startWizard()
{
    // create the ocInfo object
    connect(ownCloudInfo::instance(),SIGNAL(ownCloudInfoFound(QString,QString,QString,QString)),SLOT(slotOwnCloudFound(QString,QString,QString,QString)));
    connect(ownCloudInfo::instance(),SIGNAL(noOwncloudFound(QNetworkReply*)),SLOT(slotNoOwnCloudFound(QNetworkReply*)));
    connect(ownCloudInfo::instance(),SIGNAL(webdavColCreated(QNetworkReply::NetworkError)),SLOT(slotCreateRemoteFolderFinished(QNetworkReply::NetworkError)));

    MirallConfigFile cfgFile;

    QString url = cfgFile.ownCloudUrl();
    if( !url.isEmpty() ) {
        _ocWizard->setOCUrl( url );
    }
    _ocWizard->restart();
    _ocWizard->show();
}


/*
 *  method to check the if the owncloud admin script is existing
 */
bool OwncloudSetupWizard::checkOwncloudAdmin( const QString& bin )
{
  QFileInfo fi( bin );
  qDebug() << "checking owncloud-admin " << bin;
  if( ! (fi.exists() && fi.isExecutable() ) ) {
    _ocWizard->appendToResultWidget( tr("The owncloud admin script can not be found.\n"
      "Setup can not be done.") );
      return false;
  }
  return true;
}

void OwncloudSetupWizard::setupLocalSyncFolder()
{
    _localFolder = QDir::homePath() + QLatin1String("/ownCloud");

    if( ! _folderMan ) return;

    qDebug() << "Setup local sync folder for new oC connection " << _localFolder;
    QDir fi( _localFolder );

    bool localFolderOk = true;
    if( fi.exists() ) {
        // there is an existing local folder. If its non empty, it can only be synced if the
        // ownCloud is newly created.
        _ocWizard->appendToResultWidget( tr("Local sync folder %1 already exists, setting it up for sync.<br/><br/>").arg(_localFolder));
    } else {
        QString res = tr("Creating local sync folder %1... ").arg(_localFolder);
        if( fi.mkpath( _localFolder ) ) {
            // FIXME: Create a local sync folder.
            res += tr("ok");
        } else {
            res += tr("failed.");
            qDebug() << "Failed to create " << fi.path();
            localFolderOk = false;
        }
        _ocWizard->appendToResultWidget( res );
    }

    if( localFolderOk ) {
        _remoteFolder = QLatin1String("clientsync"); // FIXME: Themeable
        if( createRemoteFolder( _remoteFolder ) ) {
            // the creation started successfully, does not mean it will work
            qDebug() << "Creation of remote folder started successfully.";
        } else {
            // the start of the http request failed.
            _ocWizard->appendToResultWidget(tr("Creation of remote folder %1 could not be started.").arg(_remoteFolder));
            qDebug() << "Creation of remote folder failed.";
        }

    }
}

bool OwncloudSetupWizard::createRemoteFolder( const QString& folder )
{
    if( folder.isEmpty() ) return false;

    qDebug() << "creating folder on ownCloud: " << folder;

    ownCloudInfo::instance()->mkdirRequest( folder );

    return true;
}

void OwncloudSetupWizard::slotCreateRemoteFolderFinished( QNetworkReply::NetworkError error )
{
    qDebug() << "** webdav mkdir request finished " << error;
    bool success = true;

    if( error == QNetworkReply::NoError ) {
        _ocWizard->appendToResultWidget( tr("Remote folder %1 created successfully.").arg(_remoteFolder));
    } else if( error == 202 ) {
        _ocWizard->appendToResultWidget( tr("The remote folder %1 already exists. Connecting it for syncing.").arg(_remoteFolder));
    } else if( error > 202 && error < 300 ) {
        _ocWizard->appendToResultWidget( tr("The folder creation resulted in HTTP error code %d").arg((int)error) );
    } else if( error == QNetworkReply::OperationCanceledError ) {
        _ocWizard->appendToResultWidget( tr("<p><font color=\"red\">Remote folder creation failed probably because the provided credentials are wrong.</font>"
                                            "<br/>Please go back and check your credentials.</p>"));
        _localFolder.clear();
        _remoteFolder.clear();
        success = false;
    } else {
        _ocWizard->appendToResultWidget( tr("Remote folder %1 creation failed with error <tt>%2</tt>.").arg(_remoteFolder).arg(error));
        _localFolder.clear();
        _remoteFolder.clear();
        success = false;
    }

    finalizeSetup( success );
}

void OwncloudSetupWizard::finalizeSetup( bool success )
{
    if( success ) {
        if( !(_localFolder.isEmpty() || _remoteFolder.isEmpty() )) {
            _ocWizard->appendToResultWidget( tr("A sync connection from %1 to remote directory %2 was set up.")
                                             .arg(_localFolder).arg(_remoteFolder));
        }
        _ocWizard->appendToResultWidget( QLatin1String(" "));
        _ocWizard->appendToResultWidget( QLatin1String("<p><font color=\"green\"><b>")
                                                       + tr("Congratulations, your ownCloud can be connected!")
                                                       + QLatin1String("</b></font></p>"));
        _ocWizard->appendToResultWidget( tr("Press Finish to permanently accept this connection."));
    } else {
        _ocWizard->appendToResultWidget(QLatin1String("<p><font color=\"red\">")
                                        + tr("An ownCloud connection could not be established. Please check again.")
                                        + QLatin1String("</font></p>"));
    }
}

}
