# 🎯 gpu-mvfusion-recon

> **多视觉三维重建系统（Incremental SfM）** — 从稀疏重建到稠密重建，到平台部署

基于渐进式 SfM（Structure from Motion）框架的多视觉三维重建原型系统，实现从无序图像集到稠密点云、再到三维交互展示的全流程。

---

## 📌 项目定位

本项目为综合设计项目，围绕 **Incremental SfM 总体流程** 展开，分为三个阶段（三学期）递进式推进：

### 核心流程图

---

## 🗓️ 三学期路线图

| 阶段 | 学期 | 核心任务 | 关键产出 |
|------|------|---------|---------|
| **阶段I** | 2026 Fall | 稀疏重建基础 | OpenCV基础 → SIFT/ORB匹配 → 相机位姿 → 三角化点云 |
| **阶段II** | 2027 Spring | 稠密重建与优化 | PatchMatch/MVS深度图 → CUDA/GPU加速 → 稠密点云 |
| **阶段III** | 2027 Fall | 系统集成与部署 | 平台部署 → 三维交互展示 → 大创结题验收 |

**课程主线**：先恢复几何，再提升质量，最后走向平台化应用。

---

## 🛠️ 技术栈

| 模块 | 工具/库 |
|------|---------|
| 图像基础 | OpenCV (CUDA加速) |
| 位姿优化 | Ceres Solver (BA优化) |
| 稠密重建 | PatchMatch MVS / Colmap |
| 点云可视化 | Open3D / PCL |
| GPU加速 | NVIDIA CUDA |
| 开发语言 | Python / C++ |

---

## 📁 目录结构

gpu-mvfusion-recon/
├── 2026Fall/ # 阶段I：稀疏重建基础
│ ├── 00_Docs/ # 课件与实验指导书
│ ├── Lab1_OpenCV_Basics/ # 图像基本运算
│ ├── Lab2_Histogram/ # 直方图与图像增强
│ ├── Lab3_SpatialFilter/ # 空间滤波与去噪
│ ├── Lab4_EdgeSegmentation/# 边缘检测与分割
│ ├── Lab5_FeatureMatching/ # 特征提取与匹配(SIFT/ORB)
│ ├── Lab6_CameraPose/ # 相机位姿估计与基础矩阵
│ └── Lab7_Triangulation/ # 三角化与稀疏点云生成
├── 2027Spring/ # 阶段II：稠密重建与优化
├── 2027Fall/ # 阶段III：系统集成与部署
├── docs/ # 技术文档
└── README.md

---

## 🚀 快速开始

### 环境要求

- **操作系统**：Windows 10/11 或 Ubuntu 20.04+
- **编译器**：GCC 9+ / MSVC 2019+
- **依赖库**：
  - OpenCV ≥ 4.5（含 CUDA 模块）
  - Ceres Solver ≥ 2.0
  - Open3D ≥ 0.15 或 PCL ≥ 1.12
  - NVIDIA CUDA Toolkit ≥ 11.0（GPU加速可选）

### 克隆与编译

```bash
# 克隆仓库
git clone https://github.com/ScholarWuKong/gpu-mvfusion-recon.git
cd gpu-mvfusion-recon

# 以 Lab1 为例：编译并运行
cd 2026Fall/Lab1_OpenCV_Basics
mkdir build && cd build
cmake ..
make -j4
./Lab1_OpenCV_Basics

## 团队分工

| 成员 | 负责模块 | 联系方式 |
|------|---------|---------|
| @ScholarWuKong | 整体架构 + Lab5/6/7 + 系统集成 | GitHub Issue |
| 成员A | Lab1 + Lab2 | GitHub Issue |
| 成员B | Lab3 + Lab4 | GitHub Issue |
| 成员C | 文档 + 测试 + 三维展示 | GitHub Issue |

