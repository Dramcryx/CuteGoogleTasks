#include "oauthform.h"
#include "ui_oauthform.h"

#include <QDropEvent>
#include <QMimeData>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QOAuth2AuthorizationCodeFlow>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QtNetwork>
#include <QMessageBox>

#include <QPropertyAnimation>
#include <QGraphicsEffect>

OAuthForm::OAuthForm(std::shared_ptr<AuthManager> auth, QWidget *parent):
    QWidget(parent),
    ui(new Ui::OAuthForm),
    mAuth(auth),
    mAuthPtr(auth->flow())
{
    ui->setupUi(this);
    auto effect = new QGraphicsDropShadowEffect();
    effect->setOffset(1, 1);
    effect->setBlurRadius(10);
    ui->loginButton->setGraphicsEffect(effect);

    effect = new QGraphicsDropShadowEffect();
    effect->setOffset(1, 1);
    effect->setBlurRadius(10);
    ui->frame->setGraphicsEffect(effect);
    setAcceptDrops(true);
    if (auth->initStatus() == AuthManager::InitFromCacheStatus::NoToken)
    {
        ui->clientID->setText(mAuthPtr->clientIdentifier());
        ui->clientSecret->setText(mAuthPtr->clientIdentifierSharedKey());
    }
}

OAuthForm::~OAuthForm()
{
    delete ui;
}

void OAuthForm::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void OAuthForm::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}

void OAuthForm::dropEvent(QDropEvent *event)
{
    auto mimes = event->mimeData();
    if (mimes->hasUrls()) {
        auto file = mimes->urls().front().toLocalFile();

        if (mAuth->readFromDroppedFile(file))
        {
            ui->clientID->setText(mAuthPtr->clientIdentifier());
            ui->clientSecret->setText(mAuthPtr->clientIdentifierSharedKey());
        }
        event->acceptProposedAction();
    }
}

void OAuthForm::animate()
{
    //auto effect = new QGraphicsOpacityEffect(this);
    //effect->setOpacity(1.0);
    //this->setGraphicsEffect(effect);
//    auto animation = new QPropertyAnimation(effect, "opacity");
//    animation->setStartValue(0.0);
//    animation->setEndValue(1.0);
//    animation->setDuration(750);
//    animation->setEasingCurve(QEasingCurve::InExpo);
//    animation->start(QAbstractAnimation::DeleteWhenStopped);
    auto animation = new QPropertyAnimation(this, "pos");
    animation->setStartValue(QPoint(0, 500));
    animation->setEndValue(QPoint(0, 0));
    animation->setDuration(1000);
    animation->setEasingCurve(QEasingCurve::OutExpo);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void OAuthForm::on_loginButton_clicked()
{
    mAuthPtr->setClientIdentifier(ui->clientID->text());
    mAuthPtr->setClientIdentifierSharedKey(ui->clientSecret->text());
    emit authReady();
}
