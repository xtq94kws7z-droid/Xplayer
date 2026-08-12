#include "xplayercore.h"
#include "api/networkmanager.h"
#include "api/proxymanager.h"
#include "services/manager/servermanager.h"
#include "services/auth/authservice.h"
#include "services/media/mediaservice.h" 
#include "services/admin/adminservice.h" 
#include "services/danmaku/danmakuservice.h"
#include "services/introdb/introdbservice.h"

XplayerCore::XplayerCore(QObject *parent)
    : QObject(parent)
{
    
    ProxyManager::installApplicationFactory();

    
    m_networkManager = new NetworkManager(this);

    
    m_serverManager = new ServerManager(m_networkManager, this);

    
    
    
    ProxyManager::instance()->attachServerManager(m_serverManager);

    
    m_authService = new AuthService(m_networkManager, m_serverManager, this);

    
    m_mediaService = new MediaService(m_serverManager, this);

    
    m_adminService = new AdminService(m_serverManager, this);

    
    m_danmakuService = new DanmakuService(m_networkManager, m_serverManager, this);

    
    m_introDBService = new IntroDBService(m_networkManager, this);
}

XplayerCore::~XplayerCore()
{
    
    
    
}

ServerManager* XplayerCore::serverManager() const
{
    return m_serverManager;
}

AuthService* XplayerCore::authService() const
{
    return m_authService;
}

MediaService* XplayerCore::mediaService() const
{
    return m_mediaService;
}

AdminService* XplayerCore::adminService() const
{
    return m_adminService;
}

DanmakuService* XplayerCore::danmakuService() const
{
    return m_danmakuService;
}

IntroDBService* XplayerCore::introDBService() const
{
    return m_introDBService;
}
