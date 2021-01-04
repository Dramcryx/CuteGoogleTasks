#include "authmanager.h"

#include <QOAuthHttpServerReplyHandler>
#include <QStandardPaths>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include <QDebug>

AuthManager::AuthManager(): mFlow(std::make_shared<QOAuth2AuthorizationCodeFlow>())
{
    mFlow = std::make_shared<QOAuth2AuthorizationCodeFlow>();
    mFlow->setAuthorizationUrl(QUrl("https://accounts.google.com/o/oauth2/auth"));
    mFlow->setScope("https://www.googleapis.com/auth/tasks.readonly");
    mFlow->setAccessTokenUrl(QUrl("https://oauth2.googleapis.com/token"));
    mFlow->setReplyHandler(new QOAuthHttpServerReplyHandler(8080, mFlow.get()));
    mFlow->setModifyParametersFunction([ptr = mFlow](QAbstractOAuth::Stage stage,
                                             QVariantMap* parameters)
    {
        if(stage == QAbstractOAuth::Stage::RequestingAuthorization)
        {
            // Change redirect uri
            parameters->insert("redirect_uri","http://127.0.0.1:8080/");
            // This allows for token to be refreshed
            parameters->insert("access_type", "offline");
        }
        if(stage == QAbstractOAuth::Stage::RequestingAccessToken)
        {
            QByteArray code = parameters->value("code").toByteArray();
            (*parameters)["code"] = QUrl::fromPercentEncoding(code);
        }
        if (stage == QAbstractOAuth::Stage::RefreshingAccessToken)
        {
            // Google requires CID and CS to be in query params. Bad request reply otherwise
            parameters->insert("client_id", ptr->clientIdentifier());
            parameters->insert("client_secret", ptr->clientIdentifierSharedKey());
        }
    });
    tryInitFromCache();
}

AuthManager::~AuthManager()
{
    if (mFlow->clientIdentifier().length())
    {
    auto filename = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).append("/udata");
    QJsonObject object;
    object["cid"] = mFlow->clientIdentifier();
    object["csk"] = mFlow->clientIdentifierSharedKey();
    object["token"] = mFlow->token();
    object["rtoken"] = mFlow->refreshToken();

    if (QFile cacheFile(filename); cacheFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        cacheFile.write(QJsonDocument{object}.toJson());
    }
    }
}

std::shared_ptr<QOAuth2AuthorizationCodeFlow> AuthManager::flow() const
{
    return mFlow;
}

AuthManager::InitFromCacheStatus AuthManager::initStatus() const
{
    return mInitStatus;
}

bool AuthManager::readFromDroppedFile(QString &filename)
{
    QFile json(filename);
    if (json.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        auto jsonBuf = json.readAll();
        json.close();
        if (auto doc = QJsonDocument::fromJson(jsonBuf)["web"]; doc.isObject())
        {
            QJsonObject object = doc.toObject();
            auto client_id = object.find("client_id");
            auto client_secret = object.find("client_secret");
            if (client_id != object.end() && client_secret != object.end())
            {
                mFlow->setClientIdentifier(object["client_id"].toString());
                mFlow->setClientIdentifierSharedKey(object["client_secret"].toString());
                return true;
            }
        }
    }
    return false;
}

void AuthManager::tryInitFromCache()
{
    auto filename = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).append("/udata");
    if (QFile jsonCache(filename); jsonCache.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        if (auto doc = QJsonDocument::fromJson(jsonCache.readAll()); doc.isObject())
        {
            QJsonObject object = doc.object();
            mFlow->setClientIdentifier(object["cid"].toString());
            mFlow->setClientIdentifierSharedKey(object["csk"].toString());
            mFlow->setToken(object["token"].toString());
            mFlow->setRefreshToken(object["rtoken"].toString());
            mInitStatus = InitFromCacheStatus::Success;
        }
        else
        {
            mInitStatus = InitFromCacheStatus::NoCreds;
        }
    }
    else
    {
        mInitStatus = InitFromCacheStatus::NoCreds;
    }
}
