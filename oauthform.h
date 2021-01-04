#ifndef OAUTHFORM_H
#define OAUTHFORM_H

#include <memory>

#include <QWidget>

#include "authmanager.h"

namespace Ui {
class OAuthForm;
}

class QOAuth2AuthorizationCodeFlow;

class OAuthForm : public QWidget
{
    Q_OBJECT

public:
    OAuthForm(std::shared_ptr<AuthManager> auth, QWidget *parent = nullptr);
    ~OAuthForm();

    void animate();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent * event) override;

signals:
    void authReady();

private slots:
    void on_loginButton_clicked();

private:
    Ui::OAuthForm *ui;

    std::shared_ptr<AuthManager> mAuth;
    std::shared_ptr<QOAuth2AuthorizationCodeFlow> mAuthPtr;
};

#endif // OAUTHFORM_H
