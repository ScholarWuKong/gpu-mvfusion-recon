# Lab3_SpatialFilter: 空间滤波与图像增强

## 实验目标

- 均值 / 高斯 / 中值滤波对比（高斯噪声、椒盐噪声两种场景）
- 拉普拉斯与 USM 锐化
- PSNR / SSIM 定量质量评价（输出评分表）
- 加分项：双边滤波（保边去噪）
- 软件工程：SFM 系统架构数据流图 + 预处理模块接口约定

## 运行（Python 主流程）

```powershell
..\..\.venv\Scripts\python.exe -X utf8 .\src\lab3_filter.py
```

输出写入 `results/`；PSNR/SSIM 评分表同时打印到终端并保存为 `results/l3_score_table.csv`。

## 目录

| 路径 | 说明 |
|---|---|
| `src/lab3_filter.py` | 全部算法实现 + 主流程（含 `preprocess(img_bgr, cfg)` 统一接口） |
| `docs/architecture.md` | SFM 系统数据流图与模块边界 |
| `docs/interface_agreement.md` | 预处理模块接口约定 |
| `docs/report.md` | 实验报告（滤波/锐化对比图 + PSNR/SSIM 评分表） |
| `results/` | 噪声图、滤波图、锐化图、对比图与评分表 |
| `data/photo01.jpg, photo02.jpg` | 实验输入（纹理丰富场景） |

## C++ 骨架（可选）

仓库保留的 CMake 骨架需另行配置 OpenCV C++ 环境：

```powershell
cmake -B build -S .
cmake --build build --config Release
```

Python 交付不依赖 C++ 编译。
