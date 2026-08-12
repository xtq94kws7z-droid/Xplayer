#ifndef XPLAYERCORE_H
#define XPLAYERCORE_H

#include "XplayerCore_global.h"
#include <QObject>


class NetworkManager;
class ServerManager;
class AuthService;
class MediaService; 
class AdminService; 
class DanmakuService;
class IntroDBService;

class XPLAYERCORE_EXPORT XplayerCore : public QObject
{
    Q_OBJECT
public:
    explicit XplayerCore(QObject *parent = nullptr);
    ~XplayerCore() override;

    
    ServerManager* serverManager() const;
    AuthService* authService() const;
    MediaService* mediaService() const; 
    AdminService* adminService() const; 
    DanmakuService* danmakuService() const;
    IntroDBService* introDBService() const;

private:
    
    NetworkManager* m_networkManager;
    ServerManager* m_serverManager;
    AuthService* m_authService;
    MediaService* m_mediaService; 
    AdminService* m_adminService; 
    DanmakuService* m_danmakuService;
    IntroDBService* m_introDBService;
};

#endif 
