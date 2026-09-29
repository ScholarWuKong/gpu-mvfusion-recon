# 实验一环境记录

## 核验信息

- 核验日期：2026-09-29。
- 安装日期：2026-09-29；在全新克隆的仓库上新建虚拟环境并安装全部依赖。
- 操作系统：Windows-11-10.0.26200-SP0。
- Python：3.14.1（venv 内）；基础解释器 C:\Users\lenovo\AppData\Local\Programs\Python\Python314\python.exe。
- 解释器：D:\Project\gpu-mvfusion-recon\.venv\Scripts\python.exe。
- 虚拟环境：位于仓库根目录 `.venv`，供 2026Fall 实验一至五共用。

## 核心依赖

| 用途 | 安装包名 | 导入名 | 已安装包版本 |
|---|---|---|---|
| 图像处理 | opencv-python | cv2 | 5.0.0.93 |
| 数组计算 | numpy | numpy | 2.5.3 |
| 绘图展示 | matplotlib | matplotlib | 3.11.2 |
| 单元测试（实验五） | pytest | pytest | 9.1.1 |
| 图像质量评价（实验三 SSIM） | scikit-image | skimage | 0.26.0（scipy 1.18.1） |

cv2.__version__ 输出为 5.0.0；pip 中的 opencv-python 包版本为 5.0.0.93，两者采用的版本标记不同。requirements.txt 使用 pip 包版本。

## 本次检查结果

- 运行 src/version_check.py：Python 路径指向仓库根 `.venv`，虚拟环境标记为 True，cv2/numpy/matplotlib 三个库均成功导入，退出码为 0。
- 运行 python -m pip check：输出 No broken requirements found.，退出码为 0。
- API 冒烟测试 21 项全部 PASS，覆盖实验一至五指导书用到的核心接口：imread/cvtColor/imwrite、线性与伽马变换、np.add 与 cv2.add、addWeighted、calcHist、equalizeHist、blur/GaussianBlur/medianBlur、Laplacian/USM/filter2D、PSNR/SSIM、Sobel、Canny、threshold(固定+OTSU)、adaptiveThreshold、cornerHarris、goodFeaturesToTrack、pyrDown/pyrUp、matplotlib 绘图、pytest 导入。
- baseline_pipeline.py 在 data/input/color.png 上运行 SUCCESS：输入 (1254,1254,3) uint8，输出 (1254,1254) uint8 单通道，耗时 0.052686 s，退出码 0；输出回读验证 shape/dtype/像素范围正常。

## PowerShell 核验命令

从 `2026Fall/Lab1_OpenCV_Basics` 目录运行（虚拟环境在仓库根目录，向上两级）：

```powershell
$pythonExe = '..\..\.venv\Scripts\python.exe'
& $pythonExe -X utf8 '.\src\version_check.py'
& $pythonExe -m pip check
& $pythonExe -m pip freeze
```

根目录 `requirements.txt` 与 `2026Fall/Lab1_OpenCV_Basics/requirements.txt` 为同一份 pip freeze 快照（共 23 项），包含核心依赖及间接依赖；是当前环境快照，不表示每个包都被实验代码直接导入。

## 在新环境安装的方法

以下为复现指引。准备 Python ≥ 3.13（opencv-python 5.0.0.93 为 cp37-abi3 通用 wheel，Python 3.14 可直接使用），在仓库根目录运行：

```powershell
python -m venv .venv
& '.\.venv\Scripts\python.exe' -m pip install -r requirements.txt
& '.\.venv\Scripts\python.exe' -X utf8 '.\2026Fall\Lab1_OpenCV_Basics\src\version_check.py'
& '.\.venv\Scripts\python.exe' -m pip check
```

安装时 pip 使用本机配置的镜像源（华为云）；无特殊配置时使用默认 PyPI。
