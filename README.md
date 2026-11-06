
# vcos_components_rt_framework代码仓库说明

## 1. 代码仓库简介

vcos_components_rt_framework 为通信服务、基础服务和车控服务等上层功能和应用提供统一、稳定的操作系统抽象接口，同时兼容 AUTOSAR OS 规范。其底层封装了轻量级实时内核 NuttX，提供任务调度、中断管理和资源分配等基本功能。在此基础上，进一步集成了硬实时系统所需的关键特性，确保各模块高效协同与精准调度。框架具有高度模块化和良好的可扩展性，不仅有效保证实时任务的快速响应，也提升了系统整体的可靠性和稳定性，从而满足现代嵌入式应用对严格实时性的多重要求

## 2. 代码目录

vcos_components_rt_framework 源代码在 haloosspace/vcos/components/rt_framework 目录下，目录结构如下图所示：

```text
rt_framework
├── README.md        # rt_framework 仓库介绍
├── CMakeLists.txt   # rt_framework 构建脚本，负责定义项目的构建环境、编译选项、依赖库以及目标文件生成
├── DISCLAIMER.md    # rt_framework 免责声明
├── LICENSE          # rt_framework 开源协议
├── Kconfig          # 用于 rt_framework 的配置选项、参数及功能开关
├── inc              # 存放 rt_framework 的头文件，包括接口声明、数据结构定义以及公共宏等
├── src              # 存放 rt_framework 的核心功能实现，包括与底层实时操作系统的集成、任务调度及关键功能处理等内容
└── config_plug_in   # 包括 rt_framework UI 配置、代码生成和配置校验功能：
    ├── auto_config  # 参数自动配置相关代码
    ├── base_config  # 定义了 UI 上的配置选项
    ├── generator    # 代码生成脚本
    └── validation   # 配置校验规则
```

## 3. 仓库使用入门

使用手册详见[VCOS内核开发者手册](https://gitee.com/haloos/vcos/blob/master/user_manual/kernel/kernel_user_manual.md)
