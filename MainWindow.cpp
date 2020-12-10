/*
   Copyright (C) 2005 korewaisai
   korewaisai@yahoo.co.jp

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
*/

#include <QApplication>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QSplitter>
#include <QToolBar>

#include "MainWindow.h"
#include "Crontab.h"
#include "CronModel.h"
#include "CronView.h"
#include "TCommandEdit.h"
#include "VariableEdit.h"
#include "ExecuteList.h"
#include "ExecuteView.h"
#include "SaveDialog.h"
#include "Version.h"
#include "Clib.h"

MainWindow::MainWindow()
{
    readSettings();

    createActions();

    //	statusBar();

    CronModel *cronModel = new CronModel(&crontabs);
    cronView = new CronView(cronModel);
    TCommandEdit *tCommandEdit = new TCommandEdit();
    VariableEdit *variableEdit = new VariableEdit();
    executeList = new ExecuteList(exeMaxNum, exeMaxDate, &crontabs);

    QTabWidget *tab = new QTabWidget;
    {
        tab->addTab(tCommandEdit, QIcon::fromTheme("xfce-edit", QIcon(":/images/edit_small.png")),
                    tr("Edit &Command"));
        tab->addTab(variableEdit, QIcon::fromTheme("xfce-edit", QIcon(":/images/edit_small.png")),
                    tr("Edit &Variable"));
        tab->addTab(executeList, QIcon::fromTheme("view-list-symbolic", QIcon(":/images/view_text.png")),
                    tr("E&xecute List"));
    }

    QSplitter *spl = new QSplitter;
    {
        spl->addWidget(cronView);
        spl->addWidget(tab);
    }

    connect(cronView, &CronView::viewSelected, this, &MainWindow::changeCurrent);
    connect(cronView, &CronView::viewSelected, tCommandEdit, &TCommandEdit::changeCurrent);
    connect(cronView, &CronView::viewSelected, variableEdit, &VariableEdit::changeCurrent);
    connect(cronView, &CronView::viewSelected, executeList, &ExecuteList::changeCurrent);
    connect(executeList->executeView, &ExecuteView::viewSelected, cronView, &CronView::changeCurrent);

    connect(cronView, &CronView::dataChanged, this, &MainWindow::dataChanged);
    connect(cronView, &CronView::dataChanged, executeList, &ExecuteList::dataChanged);
    connect(tCommandEdit, &TCommandEdit::dataChanged, this, &MainWindow::dataChanged);
    connect(tCommandEdit, &TCommandEdit::dataChanged, executeList, &ExecuteList::dataChanged);
    connect(tCommandEdit, &TCommandEdit::dataChanged, cronView, &CronView::tCommandChanged);
    connect(variableEdit, &VariableEdit::dataChanged, this, &MainWindow::dataChanged);

    connect(deleteAction, &QAction::triggered, cronView, &CronView::removeTCommand);
    connect(copyAction, &QAction::triggered, cronView, &CronView::copyTCommand);
    connect(cutAction, &QAction::triggered, cronView, &CronView::cutTCommand);
    connect(pasteAction, &QAction::triggered, cronView, &CronView::pasteTCommand);
    connect(newAction, &QAction::triggered, cronView, &CronView::newTCommand);

    connect(quitAction, &QAction::triggered, this, &MainWindow::close);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveCron);
    connect(reloadAction, &QAction::triggered, this, &MainWindow::reloadCron);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::aboutQroneko);
    connect(aboutQtAction, &QAction::triggered, qApp, QApplication::aboutQt);

    connect(cronView, &CronView::pasted, pasteAction, &QAction::setEnabled);

    initCron();

    resize(winSize);
    cronView->resize(viewSize);

    setWindowTitle(Clib::uName() + " - qroneko");
    setWindowIcon(QIcon(":/images/qroneko.png"));

    setCentralWidget(spl);
}

void MainWindow::createActions()
{

    QMenu *fileMenu = new QMenu(tr("&File"), this);
    QToolBar *fileToolBar = addToolBar(tr("File"));

    newAction = fileMenu->addAction(
                QIcon::fromTheme("filenew", QIcon(":/images/filenew.png")), tr("&New Item"));
    newAction->setShortcut(QKeySequence(tr("Ctrl+N")));
    fileMenu->addSeparator();
    reloadAction = fileMenu->addAction(
                QIcon::fromTheme("reload", QIcon(":/images/reload.png")), tr("&Reload"));
    reloadAction->setShortcut(QKeySequence(tr("Ctrl+R")));
    saveAction = fileMenu->addAction(
                QIcon::fromTheme("filesave", QIcon(":/images/filesave.png")), tr("&Save"));
    saveAction->setShortcut(QKeySequence(tr("Ctrl+S")));
    fileMenu->addSeparator();

    quitAction = fileMenu->addAction(
                 QIcon::fromTheme("exit", QIcon(":images/exit.png")), tr("E&xit"));
    quitAction->setShortcut(QKeySequence(tr("Ctrl+Q")));

    fileToolBar->addAction(saveAction);
    fileToolBar->addAction(reloadAction);
    fileToolBar->addAction(newAction);
    menuBar()->addMenu(fileMenu);

    QMenu *editMenu = new QMenu(tr("&Edit"), this);
    QToolBar *editToolBar = addToolBar(tr("Edit"));
    cutAction = editMenu->addAction(
                QIcon::fromTheme("edit-cut", QIcon(":/images/editcut.png")), tr("Cut"));
    cutAction->setShortcut(QKeySequence(tr("Ctrl+X")));
    copyAction = editMenu->addAction(
                QIcon::fromTheme("edit-copy", QIcon(":/images/editcopy.png")), tr("Copy"));
    copyAction->setShortcut(QKeySequence(tr("Ctrl+C")));
    pasteAction = editMenu->addAction(
                QIcon::fromTheme("edit-paste", QIcon(":/images/editpaste.png")), tr("Paste"));
    pasteAction->setShortcut(QKeySequence(tr("Ctrl+V")));
    editMenu->addSeparator();
    deleteAction = editMenu->addAction(
                QIcon::fromTheme("edit-delete", QIcon(":/images/editdelete.png")), tr("Delete"));
    deleteAction->setShortcut(QKeySequence(tr("Del")));
    editToolBar->addAction(cutAction);
    editToolBar->addAction(copyAction);
    editToolBar->addAction(pasteAction);
    editToolBar->addSeparator();
    editToolBar->addAction(deleteAction);
    menuBar()->addMenu(editMenu);

    QMenu *helpMenu = new QMenu(tr("&Help"), this);
    aboutAction = helpMenu->addAction(
                QIcon(":/images/qroneko.png"), tr("About"));
    aboutQtAction = helpMenu->addAction(tr("About Qt"));
    menuBar()->addMenu(helpMenu);

    saveAction->setEnabled(false);
    pasteAction->setEnabled(false);

}

void MainWindow::initCron()
{
    for (Crontab *d : crontabs) delete d;
    crontabs.clear();

    QString user = Clib::uName();
    Crontab *cron = new Crontab(user);
    if (cron->tCommands.count() == 0 && cron->comment == "" &&
            cron->variables.count() == 0) {
        cron->comment = "";
        cron->variables << new Variable("HOME", Clib::uHome(), "Home");
        cron->variables << new Variable( "PATH", Clib::getEnv("PATH"), "Path");
        cron->variables << new Variable( "SHELL", Clib::uShell(), "Shell");
    }
    crontabs << cron;
    if (Clib::uId() == 0) {
        cron = new Crontab( "/etc/crontab" );
        crontabs << cron;
        for (const QString &s : Clib::allUsers()) {
            if (s == user)
                continue;

            cron = new Crontab(s);
            if (cron->tCommands.count() == 0)
                delete cron;
            else
                crontabs << cron;
        }
    } else {
        cronView->hideUser();
    }
    executeList->dataChanged();
    cronView->resetView();

    saveAction->setEnabled(false);
}

void MainWindow::reloadCron()
{

    if (saveAction->isEnabled()) {
        if (!QMessageBox::question(this,
                                   tr("qroneko"),
                                   tr("Not saved since last change.\nAre you OK to reload?"),
                                   tr("&Ok"), tr("&Cancel"), QString(), 0, 1)) {
            initCron();
        }
    }

}

void MainWindow::saveCron()
{

    bool saved = false;
    bool notSaved =false;

    for (Crontab *cron : crontabs) {
        if (cron->changed) {
            SaveDialog dialog(cron->cronOwner, cron->cronText());
            if (dialog.exec()==QDialog::Accepted) {
                bool ret = cron->putCrontab(dialog.getText());
                if (!ret) {
                    QMessageBox::critical(this, "qroneko", cron->estr);
                    notSaved = true;
                } else {
                    Crontab *newCron = new Crontab(cron->cronOwner);
                    int p = crontabs.indexOf(cron);
                    crontabs.replace(p, newCron);
                    delete cron;
                    saved = true;
                }
            } else {
                notSaved = true;
            }
        }
    }

    if (saved) {
        executeList->dataChanged();
        cronView->resetView();
    }
    saveAction->setEnabled(notSaved);

}

void MainWindow::dataChanged()
{
    Crontab *cron = cronView->getCurrentCrontab();
    cron->changed = true;
    saveAction->setEnabled(true);
    cronView->resizeColumnToContents(0);
}

void MainWindow::changeCurrent(Crontab *, TCommand *cmnd)
{
    bool flg;
    if (cmnd == nullptr)
        flg = false;
    else
        flg = true;

    cutAction->setEnabled(flg);
    copyAction->setEnabled(flg);
    deleteAction->setEnabled(flg);
}

void MainWindow::readSettings()
{
    QSettings settings("MX-Linux", "qroneko");

    settings.beginGroup("Main");
    exeMaxNum = settings.value("MaxListNum", 100 ).toInt();
    exeMaxDate = settings.value("MaxListDate", 1 ).toInt();
    winSize = settings.value("WindowSize", QSize(670,480)).toSize();
    viewSize = settings.value("ViewSize", QSize(200,460)).toSize();
    settings.endGroup();
}

void MainWindow::writeSettings()
{
    QSettings settings("MX-Linux", "qroneko");

    settings.beginGroup("Main");
    settings.setValue("MaxListNum", executeList->maxNum);
    settings.setValue("MaxListDate", executeList->maxDate);
    settings.setValue("WindowSize", size());
    settings.setValue("ViewSize", cronView->size());
    settings.endGroup();
}

void MainWindow::closeEvent(QCloseEvent *event)
{

    int changed = false;
    for (Crontab *cron : crontabs) {
        if (cron->changed) {
            changed = true;
            break;
        }
    }
    if (changed) {
        if (QMessageBox::question(this,
                                  "qroneko",
                                  tr("Not saved since last change.\nAre you OK to exit?"),
                                  tr("&Ok"), tr("&Cancel"), QString(), 0, 1)) {
            event->ignore();
            return;
        }
    }
    writeSettings();
    event->accept();
}

void MainWindow::aboutQroneko()
{
    QMessageBox::about(this, tr("About qroneko"),
                       "<b>qroneko</b> - " + tr("Qt cron utility") + "<br>" + tr("Version %1").arg(VERSION));
}