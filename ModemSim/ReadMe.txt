

工作流程：
1，MODEM板作为设备，发起对服务器（前端机1234端口）的连接，连接成功后，此socket作为服务器的数据端口，向服务器发送数据
2，服务器收到modem板的连接后，主动连接modem板的端口为服务器反向连接MODEM的1104端口，用于服务器向modem板发送命令

2026/1/20
1，接收命令使用vector类
2，接收命令增加校验和验证

12/23
1，使用M_PI的方法：在stdafx.h中，添加一行：#define _USE_MATH_DEFINES
   在.cpp中添加引用：#include <cmath>


12/3
1，参照iWasModem羡慕，改成异步发消息例程


2025/11.21
1，创建项目

为了清晰地管理连接，需要三个 Socket 对象：
m_sktUpload (客户端角色): 连接 PC 的 1234 端口，用于发送测井数据。
m_sktListen (监听角色): 本地监听 1104 端口，等待 PC 的命令连接请求。
m_sktCmd (服务端通信角色): 当 m_sktListen 收到 PC 连接后，由这个 Socket 接管连接，用于接收 PC 的命令。