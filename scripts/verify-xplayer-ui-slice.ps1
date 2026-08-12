$ErrorActionPreference = 'Stop'

$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
Set-Location -LiteralPath $projectRoot

function Read-Text {
    param([Parameter(Mandatory = $true)][string]$Path)
    return Get-Content -LiteralPath $Path -Raw
}

function Assert-Contains {
    param(
        [Parameter(Mandatory = $true)][string]$Content,
        [Parameter(Mandatory = $true)][string]$Pattern,
        [Parameter(Mandatory = $true)][string]$Message
    )

    if ($Content -notmatch $Pattern) {
        throw $Message
    }
}

function Assert-NotContains {
    param(
        [Parameter(Mandatory = $true)][string]$Content,
        [Parameter(Mandatory = $true)][string]$Pattern,
        [Parameter(Mandatory = $true)][string]$Message
    )

    if ($Content -match $Pattern) {
        throw $Message
    }
}

$settingsCard = Read-Text 'src/XplayerApp/components/settingscard.cpp'
$settingsSubPanel = Read-Text 'src/XplayerApp/components/settingssubpanel.cpp'
$settingsPageBase = Read-Text 'src/XplayerApp/views/settings/settingspagebase.cpp'
$settingsView = Read-Text 'src/XplayerApp/views/settings/settingsview.cpp'
$mediaDelegate = Read-Text 'src/XplayerApp/views/media/mediacarddelegate.cpp'
$mediaSection = Read-Text 'src/XplayerApp/components/mediasectionwidget.cpp'
$gallery = Read-Text 'src/XplayerApp/components/horizontallistviewgallery.cpp'
$qss = Read-Text 'src/XplayerApp/resources/qss/light-style.qss'
$darkQss = Read-Text 'src/XplayerApp/resources/qss/dark-style.qss'
$dashboard = Read-Text 'src/XplayerApp/views/user/dashboardview.cpp'
$mainWindow = Read-Text 'src/XplayerApp/mainwindow.cpp'
$posterStageHeader = Read-Text 'src/XplayerApp/components/posterstagewidget.h'
$posterStageSource = Read-Text 'src/XplayerApp/components/posterstagewidget.cpp'
$posterWallUtils = Read-Text 'src/XplayerApp/utils/posterwallutils.cpp'

Assert-Contains $settingsCard 'setObjectName\s*\(\s*"SettingsCardIconBox"\s*\)' `
    'SettingsCard must wrap icons in a styled SettingsCardIconBox.'
Assert-Contains $settingsCard 'setFixedSize\s*\(\s*40\s*,\s*40\s*\)' `
    'SettingsCard icon tile must be a 40px rounded square.'
Assert-Contains $settingsCard 'toneForSettingsIcon' `
    'SettingsCard must assign visual tone properties for colorful icon tiles.'
Assert-Contains $settingsSubPanel 'setObjectName\s*\(\s*"SettingsCardIconBox"\s*\)' `
    'SettingsSubPanel must share the same rounded icon tile treatment.'
Assert-Contains $settingsPageBase 'setContentsMargins\s*\(\s*48\s*,\s*42\s*,\s*64\s*,\s*48\s*\)' `
    'Settings pages must use larger premium desktop margins.'
Assert-Contains $settingsView 'setFixedWidth\s*\(\s*292\s*\)' `
    'Settings left navigation must use the wider premium sidebar.'
Assert-Contains $settingsView 'setSizeHint\s*\(\s*QSize\s*\(\s*232\s*,\s*52\s*\)\s*\)' `
    'Settings nav rows must use the taller 52px row rhythm.'

Assert-Contains $qss '#SettingsRootView\s*\{[\s\S]*background-color:\s*#F4F3F7' `
    'Settings root must use the approved warm light app background.'
Assert-Contains $qss '#SettingsCard\s*\{[\s\S]*border-radius:\s*22px' `
    'Settings cards must use large 22px rounded panels.'
Assert-Contains $qss '#SettingsCardIconBox\s*\{[\s\S]*border-radius:\s*14px' `
    'Settings icon tiles must use 14px rounded corners.'
Assert-Contains $qss '#SettingsCardIconBox\[tone="rose"\]' `
    'Settings icon tiles must support colorful tone variants.'
Assert-Contains $qss 'QLabel#SettingsPageTitle\s*\{[\s\S]*font-size:\s*34px' `
    'Settings page titles must use the large reference-style title scale.'

Assert-Contains $mediaDelegate 'kXplayerPosterRadius\s*=\s*16' `
    'Media card delegate must use a 16px poster radius.'
Assert-Contains $mediaDelegate 'kXplayerTileRadius\s*=\s*18' `
    'Media card delegate must use a larger 18px landscape tile radius.'
Assert-Contains $mediaDelegate 'drawImageHairline' `
    'Media cards must draw a subtle image hairline for a polished edge.'
Assert-Contains $mediaDelegate 'Qt::AlignLeft\s*\|\s*Qt::AlignVCenter' `
    'Media card text must align left like the reference media layout.'
Assert-Contains $mediaSection 'setContentsMargins\s*\(\s*36\s*,\s*24\s*,\s*36\s*,\s*0\s*\)' `
    'Media sections must use the tighter premium desktop margins.'
Assert-Contains $gallery 'rgba\(255,255,255,210\)' `
    'Horizontal gallery arrows must use translucent light glass controls.'
Assert-Contains $qss 'MediaCardThemeHelper#media-card-theme\s*\{[\s\S]*qproperty-titleColor:\s*#050505' `
    'Media card theme must use strong black title text.'
Assert-Contains $qss 'MediaCardThemeHelper#media-card-theme\s*\{[\s\S]*qproperty-subTitleColor:\s*#8A8A92' `
    'Media card theme must use the approved muted secondary text.'
Assert-Contains $dashboard 'PosterStageWidget' `
    'Dashboard must use PosterStageWidget for the top featured surface.'
Assert-Contains $dashboard 'm_posterStage->setModel' `
    'Dashboard must feed its poster model to the featured stage.'
Assert-Contains $dashboard 'int insertIndex = m_posterStage \? 1 : 0' `
    'Dashboard section ordering must retain the poster stage at the top.'
Assert-Contains $dashboard 'm_containerLayout->addWidget\(m_librarySection\)' `
    'Home must render the media libraries as one horizontal section.'
Assert-Contains $dashboard 'm_libraryGallery->setCardStyle\(MediaCardDelegate::LibraryTile\)' `
    'Media libraries must use horizontal landscape library cards.'
Assert-Contains $dashboard 'm_libraryGallery->setItems\(userViews\)' `
    'Fetched media libraries must populate the horizontal library gallery.'
Assert-NotContains $dashboard 'm_containerLayout->addWidget\(m_libraryGridSection\)' `
    'Home must not restore the old media-library grid.'
Assert-Contains $dashboard 'm_containerLayout->addWidget\(m_librarySectionsContainer\)' `
    'Home must restore each-library content rows beneath the library gallery.'
Assert-Contains $dashboard 'const bool showEachLibrary = store->get<bool>\(' `
    'Home must respect the Show Each Library setting instead of disabling it.'
Assert-Contains $dashboard 'ConfigKeys::forServer\(sid, ConfigKeys::ShowEachLibrary\)' `
    'Home must read the per-server Show Each Library setting.'
Assert-Contains $dashboard 'DashboardSectionOrderUtils::\s*EachLibrarySectionsSectionId' `
    'Home section ordering must recognize the each-library content rows.'
Assert-Contains $posterStageHeader 'bool hasContent\(\) const' `
    'Poster stage must expose an empty-state seam.'
Assert-Contains $posterStageSource 'kRotationIntervalMs = 8000' `
    'Poster stage rotation must use the approved 8-second cadence.'
Assert-Contains $posterStageSource 'tr\("精选推荐"\)' `
    'Poster stage eyebrow must use a time-neutral Chinese label.'
Assert-Contains $posterStageSource 'tr\("立即播放"\)' `
    'Poster stage primary action must be localized to Chinese.'
Assert-NotContains $posterStageSource 'tr\("今夜精选"\)' `
    'Poster stage must not use a night-only feature eyebrow.'
Assert-NotContains $posterStageSource 'tr\("FEATURED TONIGHT"\)' `
    'Poster stage must not retain the English feature eyebrow.'
Assert-NotContains $posterStageSource 'tr\("Play"\)' `
    'Poster stage must not retain the English play action.'
Assert-Contains $posterStageSource 'posterStageIndices' `
    'Poster stage must ask posterwallutils for a fixed three-slot selection.'
Assert-Contains $posterStageSource 'buildBackdropPixmap' `
    'Poster stage must generate a blurred backdrop from the current poster.'
Assert-Contains $posterStageSource 'updateBackdrop' `
    'Poster stage must refresh its backdrop when the current item or size changes.'
Assert-Contains $posterWallUtils 'kPosterSimilarAverageDistance' `
    'Poster wall utilities must use tolerant poster-image similarity instead of exact pixmap hashes.'
Assert-Contains $posterWallUtils 'areImagesVisuallySimilar' `
    'Poster wall utilities must deduplicate visually similar final poster images.'
Assert-Contains $posterStageSource 'kStagePosterSlots' `
    'Poster stage must reserve three fixed poster slots.'
Assert-Contains $posterStageSource 'PosterWallUtils::posterStageIndices' `
    'Poster stage must render the three-slot poster sequence through shared selection logic.'
Assert-Contains $posterWallUtils 'posterStageIndices' `
    'Poster wall utilities must fill the poster stage with fallback items before leaving gaps.'
Assert-Contains $posterWallUtils 'titleKeyForItem' `
    'Poster wall item merging must deduplicate same-title visual works.'
Assert-Contains $posterWallUtils 'providerKeyForItem' `
    'Poster wall item merging must deduplicate provider-matched visual works.'
Assert-Contains $posterStageSource 'stagePath\.addRoundedRect' `
    'Poster stage must clip the full painted surface to rounded corners.'
Assert-Contains $posterStageSource 'painter\.setClipPath\(stagePath\)' `
    'Poster stage must apply real painter clipping for its rounded outer shell.'
Assert-Contains $posterStageSource 'bottomFade\.setColorAt' `
    'Poster stage must paint a soft bottom transition into the next section.'
Assert-Contains $posterStageSource 'setObjectName\(QStringLiteral\("poster-stage-info"\)\)' `
    'Poster stage must use the dedicated compact information panel.'
Assert-Contains $posterStageSource 'infoPanel->setFixedWidth\(350\)' `
    'Poster stage information panel must retain a compact fixed width.'
Assert-Contains $qss 'QWidget#poster-stage-info\s*\{' `
    'Light poster stage must style the compact information panel.'
Assert-Contains $darkQss 'QWidget#poster-stage-info\s*\{' `
    'Dark poster stage must style the compact information panel.'
Assert-Contains $dashboard 'tr\("继续观看"\)' `
    'Dashboard continuation section must be localized to Chinese.'
Assert-Contains $dashboard 'tr\("最近新增"\)' `
    'Dashboard latest section must be localized to Chinese.'
Assert-Contains $dashboard 'tr\("为你推荐"\)' `
    'Dashboard recommendation section must be localized to Chinese.'
Assert-Contains $dashboard 'tr\("媒体库"\)' `
    'Dashboard media library section must be localized to Chinese.'
Assert-Contains $dashboard 'tr\("查看全部 >"\)' `
    'Dashboard library action must be localized to Chinese.'
Assert-Contains $dashboard 'launchDashboardTask\(loadResumeSection\(showResume, generation\)\)' `
    'Home must continue loading the resume section.'
Assert-Contains $dashboard 'launchDashboardTask\(loadLatestSection\(showLatest, generation\)\)' `
    'Home must continue loading the latest section.'
Assert-Contains $dashboard 'launchDashboardTask\(loadRecommendedSection\(showRecommended, generation\)\)' `
    'Home must continue loading the recommended section.'
Assert-Contains $dashboard 'launchDashboardTask\(loadCompletedSection\(showCompleted, generation\)\)' `
    'Home must continue loading the completed section.'
Assert-NotContains $mainWindow 'WA_TranslucentBackground' `
    'Main window must not make the full widget backing surface translucent.'
Assert-Contains $mainWindow 'windowBar->setFixedHeight\(52\)' `
    'Windows title bar must use the premium 52px vertical rhythm.'
Assert-Contains $mainWindow 'winTitlebarNavLayout->setSpacing\(8\)' `
    'Windows title bar navigation must use intentional icon spacing.'
Assert-Contains $mainWindow 'winTitlebarNav->hide\(\)' `
    'Windows title bar navigation must start hidden outside the home view.'
Assert-Contains $mainWindow 'winNav->setVisible\(currentView == m_homeView\)' `
    'Windows title bar navigation must only be visible for the home view.'
Assert-Contains $mainWindow 'm_globalSearchBox->setFixedSize\(440, 36\)' `
    'Windows title bar search must use the refined 440 by 36px control size.'
Assert-Contains $mainWindow 'titleLabel->setAlignment\(Qt::AlignLeft \| Qt::AlignVCenter\)' `
    'Windows title text must align vertically with the navigation icons.'
Assert-Contains $mainWindow 'titleLabel->setFixedHeight\(52\)' `
    'Windows title text must share the title bar height.'

Assert-Contains $qss 'QWidget#floating-sidebar,\s*\nQWidget#floating-sidebar\[pinned="true"\]\[sidebarSide="left"\],\s*\nQWidget#floating-sidebar\[pinned="true"\]\[sidebarSide="right"\]\s*\{\s*\n\s*background-color:\s*#F5F7FA' `
    'Light sidebar must use an opaque homepage surface.'
Assert-Contains $darkQss 'QWidget#floating-sidebar,\s*\nQWidget#floating-sidebar\[pinned="true"\]\[sidebarSide="left"\],\s*\nQWidget#floating-sidebar\[pinned="true"\]\[sidebarSide="right"\]\s*\{\s*\n\s*background-color:\s*#151922' `
    'Dark sidebar must use an opaque homepage surface.'
Assert-Contains $qss 'QWK--WindowBar\[bar-active=true\],\s*\nQWK--WindowBar\[bar-active=false\]\s*\{\s*\n\s*background-color:\s*#F5F7FA' `
    'Light title bar must use a solid, quiet navigation surface.'
Assert-Contains $darkQss 'QWK--WindowBar\[bar-active=true\],\s*\nQWK--WindowBar\[bar-active=false\]\s*\{\s*\n\s*background-color:\s*#151922' `
    'Dark title bar must use a solid, quiet navigation surface.'
Assert-Contains $qss 'QWidget#win-titlebar-nav\s*\{\s*\n\s*background-color:\s*#EEF1F5' `
    'Light title bar navigation must form a quiet grouped control.'
Assert-Contains $darkQss 'QWidget#win-titlebar-nav\s*\{\s*\n\s*background-color:\s*#212834' `
    'Dark title bar navigation must form a quiet grouped control.'

$visibleBrandFiles = @(
    'src/XplayerApp/mainwindow.cpp',
    'src/XplayerApp/views/public/loginview.cpp',
    'src/XplayerApp/views/settings/pagegeneral.cpp',
    'src/XplayerApp/views/settings/pageabout.cpp'
)

foreach ($path in $visibleBrandFiles) {
    $content = Read-Text $path
    Assert-NotContains $content 'tr\s*\(\s*"[^"]*Xplayer[^"]*"\s*\)' `
        "User-visible tr() strings in $path must use Xplayer instead of Xplayer."
}

Write-Host 'Xplayer UI slice verification passed.'
