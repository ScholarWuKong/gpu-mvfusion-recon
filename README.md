# 🎯 gpu-mvfusion-recon

> **多视觉三维重建系统（Incremental SfM）** — 从稀疏重建到稠密重建，到平台部署

基于渐进式 SfM（Structure from Motion）框架的多视觉三维重建原型系统，实现从无序图像集到稠密点云、再到三维交互展示的全流程。

---

## 📌 项目定位

本项目为大创项目，围绕 **Incremental SfM 总体流程** 展开，分为三个阶段（三学期）递进式推进：

```text
阶段I（稀疏重建基础） → 阶段II（稠密重建与优化） → 阶段III（系统集成与部署）

```

---

## 🗓️ 三学期路线图

| 阶段 | 学期 | 核心任务 | 关键产出 |
| :--- | :--- | :--- | :--- |
| **阶段I** | 2026 Fall | 稀疏重建基础 | OpenCV基础 → SIFT/ORB匹配 → 相机位姿 → 三角化点云 |
| **阶段II** | 2027 Spring | 稠密重建与优化 | PatchMatch/MVS深度图 → CUDA/GPU加速 → 稠密点云 |
| **阶段III** | 2027 Fall | 系统集成与部署 | 平台部署 → 三维交互展示 → 大创结题验收 |

---

## 🛠️ 技术栈

| 模块 | 工具/库 |
| :--- | :--- |
| 图像基础 | OpenCV (CUDA加速) |
| 位姿优化 | Ceres Solver (BA优化) |
| 稠密重建 | PatchMatch MVS / Colmap |
| 点云可视化 | Open3D / PCL |
| GPU加速 | NVIDIA CUDA |
| 开发语言 | Python / C++ |

---

## 📁 目录结构

```text
gpu-mvfusion-recon/
├── 2026Fall/
│   ├── 00_Docs/
│   ├── Lab1_OpenCV_Basics/
│   ├── Lab2_Histogram/
│   ├── Lab3_SpatialFilter/
│   ├── Lab4_EdgeSegmentation/
│   ├── Lab5_FeatureMatching/
│   ├── Lab6_CameraPose/
│   └── Lab7_Triangulation/
├── 2027Spring/
├── 2027Fall/
└── README.md

```

---

## 🚀 快速开始

### 环境要求

- Windows 10/11 或 Ubuntu 20.04+
- OpenCV ≥ 4.5（含 CUDA 模块）
- Ceres Solver ≥ 2.0
- NVIDIA CUDA Toolkit ≥ 11.0

---

## 👥 团队分工

| 成员 | 负责模块 |
| :--- | :--- |
| @ScholarWuKong | 整体架构 + Lab5/6/7 |
| 成员A | Lab1 + Lab2 |
| 成员B | Lab3 + Lab4 |
| 成员C | 文档 + 测试 |

---

> 🎓 电子科技大学 | 大创项目 | 以图像重建真实世界
