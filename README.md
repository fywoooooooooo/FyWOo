# 目录结构调整

```
src/
├── core/               # 核心功能模块
│   ├── monitor/        # 基础监控功能
│   ├── storage/        # 基础存储功能
│   └── analysis/       # 基础分析功能
├── ui/                 # 用户界面
│   ├── base/           # 基础UI组件
│   ├── charts/         # 图表展示
│   └── pages/          # 功能页面
├── extensions/         # 扩展功能 (可选模块)
│   ├── visualization3d/ # 3D可视化
│   ├── distributed/    # 分布式监控
│   └── blockchain/     # 区块链存储
├── common/             # 通用工具类
├── main.cpp           
└── mainwindow.cpp      # 主窗口实现
```
