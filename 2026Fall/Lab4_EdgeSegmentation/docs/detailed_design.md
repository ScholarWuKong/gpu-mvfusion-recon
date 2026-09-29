# 实验四：详细设计

## 1. 文档目的

本文档对实验四的预处理与图像分析模块进行函数级详细设计，说明各函数的输入、输出、主要算法及异常处理。

## 2. 模块划分

```text
main
├── ensure_results_dir
├── load_gray_image
├── save_image
├── run_sobel
├── experiment_sobel
├── run_canny
├── experiment_canny
├── experiment_global_threshold
└── experiment_adaptive_threshold
```

## 3. 函数详细设计

### 3.1 `ensure_results_dir`

**输入：** 无

**输出：** 无

**功能：** 创建 `results/` 目录。

**算法：**
调用 `Path.mkdir(..., exist_ok=True)`，目录不存在时创建，存在时不重复创建。

**异常处理：**
目录无写权限时由 Python 抛出异常。

---

### 3.2 `load_gray_image`

**输入：**
- `image_path: Path`，图片路径

**输出：**
- `np.ndarray`，灰度图像

**功能：**
读取实验图片，并保证后续算法输入为灰度图。

**算法：**
调用 `cv2.imread(..., cv2.IMREAD_GRAYSCALE)`。

**异常处理：**
当图片不存在、路径错误或 OpenCV 无法读取时，抛出 `FileNotFoundError`。

---

### 3.3 `save_image`

**输入：**
- `filename: str`，结果文件名
- `image: np.ndarray`，待保存图像

**输出：** 无

**功能：**
将结果图统一保存到 `results/`。

**算法：**
调用 `cv2.imwrite()`。

**异常处理：**
保存失败时抛出 `IOError`。

---

### 3.4 `run_sobel`

**输入：**
- 灰度图像
- `ksize`：Sobel 核尺寸，实验中使用 3 和 5

**输出：**
- 0~255 的 `uint8` 梯度幅值图

**算法：**
1. 分别计算 x、y 方向 Sobel 梯度；
2. 计算 `sqrt(Gx² + Gy²)`；
3. 归一化到 0~255；
4. 转换为 `uint8`。

**异常处理：**
当整幅图的梯度幅值最大值为 0 时，避免除零。

---

### 3.5 `experiment_sobel`

**输入：** 灰度图像

**输出：** 无

**功能：** 依次运行 `ksize=3` 与 `ksize=5`，保存两个结果。

---

### 3.6 `run_canny`

**输入：**
- 灰度图像
- `threshold1`
- `threshold2`

**输出：**
- 二值边缘图

**算法：**
调用 `cv2.Canny()` 完成 Canny 边缘检测。

---

### 3.7 `experiment_canny`

**输入：** 灰度图像

**输出：** 无

**功能：**
测试：
- `(50, 150)`
- `(100, 200)`
- `(150, 250)`

观察阈值升高后边缘密度变化。

---

### 3.8 `experiment_global_threshold`

**输入：** 灰度图像

**输出：**
- Otsu 自动选择的阈值

**功能：**
同时完成固定阈值 128 和 Otsu 阈值分割。

**算法：**
调用 `cv2.threshold()`：
- `THRESH_BINARY`
- `THRESH_BINARY + THRESH_OTSU`

**异常处理：**
OpenCV 输入异常由 OpenCV 抛出异常信息。

---

### 3.9 `experiment_adaptive_threshold`

**输入：** 光照不均的灰度图像

**输出：** 无

**功能：**
使用局部邻域统计进行分割。

**算法：**
调用 `cv2.adaptiveThreshold()`：
- `ADAPTIVE_THRESH_GAUSSIAN_C`
- `blockSize=31`
- `C=5`

**异常处理：**
保证输入为灰度图，并保证 `blockSize` 为正奇数。
