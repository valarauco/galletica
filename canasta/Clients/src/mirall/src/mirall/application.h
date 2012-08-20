/*
 * Copyright (C) by Duncan Mac-Vicar P. <duncan@kde.org>
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

#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>
#include <QSystemTrayIcon>
#include <QNetworkReply>

#include "qtsingleapplication.h"

#include "mirall/syncresult.h"
#include "mirall/folder.h"
#include "mirall/logbrowser.h"
#include "mirall/folderman.h"

class QAction;
class QMenu;
class QSystemTrayIcon;
class QNetworkConfigurationManager;
class QSignalMapper;
class QNetworkReply;

namespace Mirall {
class Theme;
class FolderWatcher;
class FolderWizard;
class StatusDialog;
class OwncloudSetupWizard;
class ownCloudInfo;
class UpdateDetector;

class Application : public SharedTools::QtSingleApplication
{
    Q_OBJECT
public:
    explicit Application(int &argc, char **argv);
    ~Application();

    bool giveHelp();

signals:

protected slots:
    void slotAddFolder();
    void slotOpenStatus();
    void slotRemoveFolder( const QString& );
    void slotEnableFolder( const QString&, const bool );
    void slotInfoFolder( const QString& );
    void slotConfigure();
    void slotConfigureProxy();

    void slotSyncStateChange( const QString& );
protected:

    void setupActions();
    void setupSystemTray();
    void setupContextMenu();
    void setupLogBrowser();
    void setupProxy();

    //folders have to be disabled while making config changes
    void computeOverallSyncStatus();

protected slots:
    void slotTrayClicked( QSystemTrayIcon::ActivationReason );
    void slotFolderOpenAction(const QString & );
    void slotOpenOwnCloud();
    void slotStartFolderSetup(int result = 1); // defaulting to Accepted
    void slotOwnCloudFound( const QString&, const QString&, const QString&, const QString& );
    void slotNoOwnCloudFound( QNetworkReply* );
    void slotCheckAuthentication();
    void slotAuthCheck( const QString& ,QNetworkReply* );
    void slotOpenLogBrowser();

    void slotStartUpdateDetector();

private:
    void showHelp();

    // configuration file -> folder
    QSystemTrayIcon *_tray;
    QAction *_actionQuit;
    QAction *_actionAddFolder;
    QAction *_actionOpenStatus;
    QAction *_actionConfigure;
    QAction *_actionOpenoC;
    QAction *_actionConfigureProxy;

#if QT_VERSION >= 0x040700
    QNetworkConfigurationManager *_networkMgr;
#endif

    FolderWizard  *_folderWizard;
    OwncloudSetupWizard *_owncloudSetupWizard;

    // tray's menu
    QMenu *_contextMenu;
    StatusDialog *_statusDialog;

    FolderMan *_folderMan;
    Theme *_theme;
    QSignalMapper *_folderOpenActionMapper;
    UpdateDetector *_updateDetector;
    QMap<QString, QString> _overallStatusStrings;
    LogBrowser *_logBrowser;
    bool        _helpOnly;
};

} // namespace Mirall

#endif // APPLICATION_H
