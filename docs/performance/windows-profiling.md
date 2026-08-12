# Xplayer Windows 性能采样手册

这份手册用于验证滚动、图片加载、页面切换和播放器交互的真实瓶颈。它不预设任何 FPS、CPU 或内存收益；所有量化结论都必须来自同一机器、同一数据、同一操作脚本下的前后采样。

## 采样前准备

- 使用 `Release` 构建，关闭调试器和不必要的后台程序。
- 固定窗口尺寸、Windows 缩放比例、显示器刷新率和测试媒体库。
- 记录机器信息：CPU、GPU、内存、Windows 版本、DPI 缩放、Qt 版本。
- 每个场景至少采样 30 秒，并重复两次；第一次用于预热，第二次用于对比。
- 采样前关闭旧的 `Xplayer.exe`，避免把上一次进程的资源算进本次结果。

## WPR/WPA

以管理员 PowerShell 打开 Windows Performance Recorder。先确认本机可用 profile：

```powershell
wpr.exe -profiles
```

推荐先用 CPU profile 做基线：

```powershell
New-Item -ItemType Directory -Force D:\Xplayer\artifacts\profiling | Out-Null
wpr.exe -start CPU -filemode
& D:\Xplayer\build-xplayer\bin\Xplayer.exe
```

完成一个场景后停止并保存：

```powershell
wpr.exe -stop D:\Xplayer\artifacts\profiling\xplayer-scene.wpr
wpa.exe D:\Xplayer\artifacts\profiling\xplayer-scene.wpr
```

如果本机没有名为 `CPU` 的 profile，以 `wpr.exe -profiles` 输出的实际名称替换；不要自行猜 profile 名称。GPU 合成、DWM 和输入延迟需要在 WPR 中选择包含 GPU/DWM 的 profile 后单独采样，避免把所有 provider 一次性打开造成采样本身干扰。

WPA 中重点查看：

- UI 线程 CPU 时间、Ready time 和上下文切换。
- `QWidget::paintEvent`、delegate paint、FlowLayout/Layout 激活附近的调用栈。
- 图片解码、缩放、`QPixmap` 创建和网络线程等待。
- Private Bytes、Working Set、Commit 随滚动时间的变化。
- DWM/GPU 队列是否出现长时间等待或提交堆积。

## Qt Creator

用 Qt Creator 打开同一个 `Release` 构建目录，在 Analyzer 中执行 CPU Usage 或相关 Windows 外部采样。Qt Creator 适合快速定位函数级调用栈；WPR/WPA 更适合判断线程调度、内存长期增长和系统合成问题。两者不要混合解释同一个时间百分比。

## 固定场景

1. Media 列表：数百条媒体快速滚轮上下滚动 30 秒，停留等待图片稳定。
2. 首页 Hero：连续切换、快速反向切换、窗口最大化/还原。
3. 播放器：连续 seek、音量调整、键盘长按、鼠标边缘长按。
4. 搜索历史：打开、关闭、展开、收起、Recent/Count 快速切换。
5. 管理页：任务页自动刷新期间滚动；媒体库拖拽排序；日志文件选择和大文本查看。
6. 长时间运行：重复进入/离开上述页面 10 分钟，观察进程内存和后台请求是否持续增长。

## 记录模板

每次采样保存一份 Markdown 记录，至少包含：

```text
场景：
机器/DPI/刷新率：
构建版本：
数据规模：
操作步骤：
UI 线程 CPU：
最重 paint/layout 调用栈：
图片解码/网络等待：
峰值 Private Bytes：
30 秒后 Private Bytes：
结论：
需要继续验证的假设：
```

只有当同一场景的前后 trace 都已保存，并且调用栈/内存曲线能解释变化时，才记录“已验证”。否则写成“假设”或“待采样”，不写估算收益。
