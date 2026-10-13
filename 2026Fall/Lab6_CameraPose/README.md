# 动漫图形系统综合训练 I — 实验六

本项目用于完成 SIFT 特征提取、描述子生成、参数实验
和旋转/尺度变化下的特征匹配验证。

## 环境

- Python 3.12
- OpenCV（当前实验环境为 5.0.0）
- NumPy
- Git（用于课程后续版本管理流程）

## 目录结构

```text
project_3/
├── data/
│   └── photo01.jpg
├── docs/
│   └── l6_experiment.md
├── results/
│   ├── l6_sift.jpg
│   ├── l6_parameter_comparison.csv
│   └── l6_invariance_results.csv
└── src/
    ├── features.py
    ├── l6_basic.py
    ├── l6_params.py
    └── l6_invariance.py
```

## 运行实验

在项目根目录执行：

```
python src/l6_basic.py
python src/l6_params.py
python src/l6_invariance.py
```

## 模块接口

模块位置：`src/features.py`

接口：

```
extract_sift(img, cfg=None) -> (keypoints, descriptors)
```

参数：

- `img`：OpenCV 图像数组。
- `cfg`：可选配置字典，对应 `cv2.SIFT_create` 的参数。

返回值：

- `keypoints`：SIFT 关键点列表。
- `descriptors`：形状为 `(N, 128)` 的描述子矩阵；没有有效描述子时为 `None`。

使用示例：

```
import cv2from src.features import extract_siftimg = cv2.imread("data/photo01.jpg")keypoints, descriptors = extract_sift(img)# 使用自定义参数keypoints, descriptors = extract_sift(    img, {"contrastThreshold": 0.01})
```

## 实验结果

详细实验数据及分析见 `docs/l6_experiment.md`。