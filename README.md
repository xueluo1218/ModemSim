# ModemSim
该软件用于某冲击波井下仪模拟器，用于协助上位机（前端机/PC）软件开发人员调试冲击波仪器的网络通讯功能。
该软件由MFC/VC2010编写，其TCP通讯部分参照了http://www.flounder.com/kb192570.htm

1. 项目概述 (Project Overview)
配接冲击波井下仪器时，因公司内部不具备硬件联调的条件，特开发此Modem模拟器，摆脱对外部硬件资源的依赖，缩短配接时间。
该软件可运行在PC/Windows系统上，模拟真实硬件的网络行为，实现命令解析逻辑以及测井数据上传逻辑。

2. 系统架构与网络模型 (System Architecture)
本系统采用双通道非对称 TCP 通讯模型。为了完全仿真硬件行为，模拟器需同时扮演 TCP 客户端和 TCP 服务器的角色。
2.1 通讯通道定义
![本地图片](./images/通讯通道.png) 

2.2 数据流向图
 ![本地图片](./images/数据流向.png)

3. 软件界面 (UI)
3.1. CModemSimDlg (主界面)
![本地图片](./images/ModemSim.png) 

3.2 PC端测试程序
![本地图片](./images/ModemTest.png) 

