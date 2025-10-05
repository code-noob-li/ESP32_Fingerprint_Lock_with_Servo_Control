# ESP32 Fingerprint Lock with Servo Control

[中文版本请见下方](#esp32指纹门锁带舵机控制)

## Overview

This project implements a fingerprint-controlled door lock system using ESP32 with servo motors. The system features fingerprint recognition, voice prompts, and physical locking/unlocking mechanisms. It's designed as a secure, offline solution that doesn't rely on internet connectivity, protecting user privacy and ensuring system reliability.

## Features

- **Fingerprint Recognition**: Uses a fingerprint module for secure access control
- **Servo Control**: Controls door locking/unlocking mechanisms with servo motors
- **Voice Prompts**: Provides audio feedback through an ASRPRO voice module
- **Offline Operation**: No internet connectivity for maximum security
- **Multiple Access Methods**: Fingerprint matching, manual buttons
- **Error Handling**: Intelligent error management with retry limits
- **Modular Design**: Well-structured code for easy maintenance and extension

## Hardware Components

- ESP32 Development Board
- Fingerprint Recognition Module
- ASRPRO Voice Module
- Servo Motors (2x)
- Push Buttons (4x)
- Various resistors and connectors

## Pin Configuration

| Function              | Pin  |
|-----------------------|------|
| Fingerprint RX        | 16   |
| Fingerprint TX        | 17   |
| ASRPRO RX             | 3    |
| ASRPRO TX             | 1    |
| Open Servo            | 13   |
| Close Servo           | 14   |
| Fingerprint Button    | 18   |
| Open Button           | 19   |
| Close Button          | 21   |
| Enroll Button         | 22   |

## System Architecture

The system consists of several key components:

1. **ESP32 Microcontroller**: Central processing unit
2. **Fingerprint Module**: Handles fingerprint enrollment and matching
3. **Servo Motors**: Physical locking/unlocking mechanism
4. **ASRPRO Voice Module**: Audio feedback system
5. **Push Buttons**: Manual control interface

## Security Features

- **Local Processing**: All fingerprint data is stored and processed locally
- **No Internet Connectivity**: Air-gapped design prevents remote attacks
- **Secure Authentication**: Multi-step fingerprint verification process
- **Error Limiting**: Automatic lockout after multiple failed attempts

## Getting Started

1. Wire all components according to the pin configuration
2. Install required libraries (ESP32Servo, HardwareSerial)
3. Upload the firmware to your ESP32 board
4. Enroll fingerprints using the enrollment procedure
5. Test the lock operation with fingerprint or manual buttons

## Serial Communication

See [serial_communication.md](serial_communication.md) for detailed information about the communication protocols used in this project.

## Disclaimer

**WARNING: This is a DIY project intended for educational and hobbyist purposes only.**
- This system has not been tested or certified for commercial or industrial security applications
- The creator and contributors are not responsible for any security breaches, injuries, or damages that may occur from using this system
- Users assume all risks associated with the installation and use of this system
- It is strongly recommended to have a secondary/manual override method for emergency access
- Do not rely solely on this system for high-security applications

## Acknowledgements

This project was developed with the assistance of Qwen3-coder, an AI coding assistant. 
While Qwen3-coder helped with code structure, documentation, and implementation details, the project concept, requirements, and final decisions were all made by the human developer.

Some sarcastic comments from Qwen3-coder during development:
- "Oh great, another hardware inconsistency to deal with..."
- "Please don't tell me we need to debug button debounce issues AGAIN..."
- "Why do all fingerprint modules have their own proprietary protocols?!"
- ""To be honest, this README was mostly written by me, the AI assistant - this developer doesn't know how to code at all! 😄 "

---

# ESP32指纹门锁带舵机控制

## 概述

本项目使用ESP32和舵机实现了一个指纹识别门锁系统。该系统具有指纹识别、语音提示和物理锁定/解锁机制等功能。设计为安全的离线解决方案，不依赖互联网连接，保护用户隐私并确保系统可靠性。

## 特性

- **指纹识别**：使用指纹模块进行安全访问控制
- **舵机控制**：通过舵机控制门锁的锁定/解锁机制
- **语音提示**：通过ASRPRO语音模块提供音频反馈
- **离线运行**：无互联网连接，确保最高安全性
- **多种访问方式**：指纹匹配、手动按钮操作
- **错误处理**：智能错误管理，具有重试限制
- **模块化设计**：结构良好的代码，便于维护和扩展

## 硬件组件

- ESP32开发板
- 指纹识别模块
- ASRPRO语音模块
- 舵机（2个）
- 按钮（4个）
- 各类电阻和连接器

## 引脚配置

| 功能              | 引脚 |
|-------------------|------|
| 指纹模块 RX       | 16   |
| 指纹模块 TX       | 17   |
| ASRPRO RX         | 3    |
| ASRPRO TX         | 1    |
| 开门舵机          | 13   |
| 关门舵机          | 14   |
| 指纹识别按钮      | 18   |
| 开门按钮          | 19   |
| 关门按钮          | 21   |
| 录入按钮          | 22   |

## 系统架构

系统由几个关键组件组成：

1. **ESP32微控制器**：中央处理单元
2. **指纹模块**：处理指纹录入和匹配
3. **舵机**：物理锁定/解锁机制
4. **ASRPRO语音模块**：音频反馈系统
5. **按钮**：手动控制界面

## 安全特性

- **本地处理**：所有指纹数据都在本地存储和处理
- **无网络连接**：气隙设计防止远程攻击
- **安全认证**：多步指纹验证过程
- **错误限制**：多次失败尝试后自动锁定

## 入门指南

1. 按照引脚配置连接所有组件
2. 安装所需库（ESP32Servo、HardwareSerial）
3. 将固件上传到ESP32开发板
4. 使用录入程序录入指纹
5. 使用指纹或手动按钮测试门锁操作

## 串口通信

有关本项目使用的通信协议的详细信息，请参见[serial_communication.md](serial_communication.md)。

## 免责声明

**警告：这是一个DIY项目，仅供教育和爱好者使用。**
- 本系统尚未经过商业或工业安全应用的测试和认证
- 创建者和贡献者不对使用本系统可能造成的任何安全漏洞、伤害或损失负责
- 用户需承担安装和使用本系统的全部风险
- 强烈建议设置紧急情况下的备用/手动开门方式
- 不要将本系统单独用于高安全性应用

## 致谢

本项目在AI编码助手Qwen3-coder的协助下开发完成。
虽然Qwen3-coder在代码结构、文档和实现细节方面提供了帮助，但项目概念、需求和最终决策均由人类开发者制定。

Qwen3-coder在开发过程中的一些"吐槽"：
- "哦，天哪，又一个硬件不一致的问题要处理..."
- "请别告诉我我们又需要调试按钮去抖动问题..."
- "为什么所有指纹模块都有自己的专有协议？！"
- "说实话，这篇README主要是我这个AI助手写的——这个开发者根本不会写代码！😄 /捂嘴笑"
