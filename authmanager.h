#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include <memory>

#include <QOAuth2AuthorizationCodeFlow>

class AuthManager
{
public:
    enum class InitFromCacheStatus
    {
        Success,
        NoCreds,
        NoToken
    };

    AuthManager();

    ~AuthManager();

    std::shared_ptr<QOAuth2AuthorizationCodeFlow> flow() const;

    InitFromCacheStatus initStatus() const;

    bool readFromDroppedFile(QString & filename);

private:
    std::shared_ptr<QOAuth2AuthorizationCodeFlow> mFlow;

    InitFromCacheStatus mInitStatus;

    void tryInitFromCache();
};

#endif // AUTHMANAGER_H
