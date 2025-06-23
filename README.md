# STM32/GD32编写BootLoader实现IAP升级

- 基于 STM32/GD32 微控制器的 BootLoader 项目，支持 IAP（In-Application Programming）在线升级。

## 项目简介

- 本项目基于 [stm32_oop_driver](https://github.com/903257958/stm32_oop_driver) 框架构建，采用面向对象风格封装外设驱动，模块清晰、复用性强；

- 参考教程：[B站【手把手教程 4G通信物联网 OTA远程升级 BootLoader程序设计】](https://www.bilibili.com/video/BV1SatHeBEVG/?spm_id_from=333.337.search-card.all.click)；

- 当前已适配 STM32F103、STM32F405、STM32F407、GD32F103 等主流 Cortex-M3/M4 系列芯片，由于采用统一的驱动抽象结构，您可基于已有模块快速适配更多芯片。
