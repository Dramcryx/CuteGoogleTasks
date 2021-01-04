#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <mutex>

#include <QNetworkReply>

#include <QWebEngineView>
#include <QWebEngineProfile>

#include <QMessageBox>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QTreeView>

#include <QStackedLayout>

#include <QPropertyAnimation>

#include "tasklist.h"
#include "oauthform.h"

constexpr const char * tree_style = "QTreeView { "
                                    " show-decoration-selected: 0;"
                                    " padding: 3px;"
                                    "}\n"
                                    "QTreeView::item {"
                                    " padding-left: 10px;"
                                    " border-bottom: 1px solid gray;"
                                    " alternate-background-color: transparent;"
                                    " selection-background-color: transparent;"
                                    "}\n"
                                    "QTreeView::item:hover {"
                                    " background: lightgray;"
                                    " selection-background-color: transparent;"
                                    " selection-color: black;"
                                    " alternate-background-color: transparent;"
                                    "}\n"
                                    "QTreeView::item:selected:active {"
                                    " background: transparent;"
                                    " selection-background-color: transparent;"
                                    " selection-color: black;"
                                    " alternate-background-color: transparent;"
                                    "}\n";

MainWindow::MainWindow(std::shared_ptr<AuthManager> auth, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , mCentralWidgetLayout(std::make_unique<QStackedLayout>())
{
    ui->setupUi(this);
    mCentralWidgetLayout->setStackingMode(QStackedLayout::StackAll);
    ui->centralwidget->setLayout(mCentralWidgetLayout.get());

    mAuthManager = auth;
    mAuthPointer = auth->flow();
    connect(mAuthPointer.get(), &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser,
            this, &MainWindow::startAuthorizingRoutine);
    connect(mAuthPointer.get(), &QOAuth2AuthorizationCodeFlow::granted,
            this, &MainWindow::onGranted);
    if (auth->initStatus() == AuthManager::InitFromCacheStatus::Success)
    {
        onGranted();
    }
    else
    {
        auto authForm = new OAuthForm(auth);
        connect(authForm, &OAuthForm::authReady, mAuthPointer.get(), &QAbstractOAuth::grant);
        mCentralWidgetLayout->addWidget(authForm);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showEvent(QShowEvent */*event*/)
{
    static std::once_flag authform_animation;

    if (auto authForm = qobject_cast<OAuthForm*>(mCentralWidgetLayout->currentWidget()))
    {
        std::call_once(authform_animation, &OAuthForm::animate, authForm);
    }
}

void MainWindow::createTaskListsView(QByteArray responseText)
{
    auto model = new TreeModel(QJsonDocument::fromJson(responseText).object().find("items")->toArray(),
                               mAuthPointer,
                               this);
    auto treeview = new QTreeView(this);
    treeview->setModel(model);
    treeview->setIndentation(0);
    treeview->setAnimated(true);
    treeview->setStyleSheet(tree_style);
    treeview->setHeaderHidden(true);

    mCentralWidgetLayout->addWidget(treeview);
    if (mCentralWidgetLayout->count() > 1)
    {
        slideToLeft(mCentralWidgetLayout->currentWidget(), treeview);
    }
}

void MainWindow::onGranted()
{
    // TODO move GET lists to TreeModel
    auto rest = mAuthPointer->get(QUrl("https://tasks.googleapis.com/tasks/v1/users/@me/lists"));
    connect(rest, &QNetworkReply::finished, [=]() {
        if (rest->error() == QNetworkReply::AuthenticationRequiredError) {
            mAuthPointer->refreshAccessToken();
            return;
        }
        if (rest->error() != QNetworkReply::NoError) {
            QMessageBox::critical(nullptr, "Failed to fetch task lists", rest->errorString() + QString::number(rest->error()));
            return;
        }

        createTaskListsView(rest->readAll());
    });
    connect(rest, &QNetworkReply::finished, rest, &QObject::deleteLater);
}

void MainWindow::startAuthorizingRoutine(const QUrl &url)
{
    auto view = new QWebEngineView();
    auto profile = view->page()->profile();
    profile->setHttpCacheType(QWebEngineProfile::NoCache);
    profile->setPersistentCookiesPolicy(QWebEngineProfile::NoPersistentCookies);
    // Workaround to fix "Unsafe browser" error from google authorization found in qutebrowser github
    profile->setHttpUserAgent("Mozilla/5.0 (X11; Linux x86_64; rv:57.0) Gecko/20100101 Firefox/57.0");

    view->load(url);

    mCentralWidgetLayout->addWidget(view);

    if (mCentralWidgetLayout->count() > 1)
    {
        slideToLeft(mCentralWidgetLayout->currentWidget(), view);
    }
}

void MainWindow::slideToLeft(QWidget *left, QWidget *right)
{
    auto leftEndPoint = QPoint(-left->width(), left->y());
    auto animationForLeft = new QPropertyAnimation(left, "pos");
    animationForLeft->setStartValue(left->pos());
    animationForLeft->setEndValue(leftEndPoint);
    animationForLeft->setEasingCurve(QEasingCurve::OutExpo);

    auto rightStartPoint = QPoint(left->x() + left->width(), left->y());
    auto animationForRight = new QPropertyAnimation(right, "pos");
    animationForRight->setStartValue(rightStartPoint);
    animationForRight->setEndValue(left->pos());
    animationForRight->setEasingCurve(QEasingCurve::OutExpo);

    connect(animationForRight, &QAbstractAnimation::finished,
            [this, animationForLeft, animationForRight, left, right]()
    {
        mCentralWidgetLayout->setCurrentWidget(right);
        mCentralWidgetLayout->removeWidget(left);
        left->deleteLater();
        animationForLeft->deleteLater();
        animationForRight->deleteLater();
    });

    animationForLeft->start();
    animationForRight->start();
}

