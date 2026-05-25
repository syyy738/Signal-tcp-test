# Signal-TCP-Receiver - 水声信号TCP接收端

基于 Qt 6 的水声信号 TCP 数据接收与分析工具，是 [Signal](https://github.com/syyy738/Signal) 水声通信系统的配套接收端。

## 功能特性

- **TCP 服务端** — 监听指定 IP 和端口，接收多客户端连接
- **信号数据接收** — 接收水声信号元数据（信号类型、频率、采样率等）及采样数据
- **多线程处理** — DataProcessor 在工作线程中异步处理数据，不阻塞 UI
- **ACK 回复** — 收到数据后自动回复确认包
- **信号分析** — 实时计算信号统计信息（最小值、最大值、均值、标准差）
- **校验和验证** — 验证数据完整性
- **日志显示** — 实时显示连接状态、数据详情和分析结果

## 技术栈

| 组件 | 技术 |
|------|------|
| 框架 | Qt 6 (Widgets, Network) |
| 网络 | QTcpServer / QTcpSocket |
| 构建工具 | QMake |

## 构建

```bash
# 使用 Qt Creator 打开 TCPtest.pro 直接构建
# 或使用命令行：
qmake TCPtest.pro
make
```

## 项目结构

```
├── main.cpp                 # 程序入口
├── tpreceiverwindow.h/cpp   # 主窗口 + TCP服务器 + 数据处理器
└── tpreceiverwindow.ui      # UI布局
```

## 数据协议

接收的数据包格式（大端字节序）：

| 字段 | 类型 | 说明 |
|------|------|------|
| 信号类型长度 | quint32 | 信号类型字符串的字节数 |
| 信号类型 | char[] | 信号类型名称（UTF-8） |
| 信号频率 | double | Hz |
| 采样频率 | double | Hz |
| 低频截止 | double | Hz |
| 高频截止 | double | Hz |
| 数据大小 | quint32 | 采样点数量 |
| 信号数据 | double[] | 采样值 |
| 校验和 | quint32 | 数据完整性校验 |

## 应用场景

- 水声通信系统的数据接收与监控
- 信号采集数据的网络传输验证
- 声学信号实时分析
