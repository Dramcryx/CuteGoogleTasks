#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>

#include <QMainWindow>

#include "authmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QStackedLayout;

class OAuthForm;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(std::shared_ptr<AuthManager> auth, QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void showEvent(QShowEvent * event) override;

private slots:
    void onGranted();

private:
    Ui::MainWindow *ui;

    std::unique_ptr<QStackedLayout> mCentralWidgetLayout;

    std::shared_ptr<AuthManager> mAuthManager;
    std::shared_ptr<QOAuth2AuthorizationCodeFlow> mAuthPointer;
    void startAuthorizingRoutine(const QUrl & url);
    void slideToLeft(QWidget * left, QWidget * right);
    void createTaskListsView(QByteArray rest);
};
#endif // MAINWINDOW_H
