# 实验一：需求分析与 OpenCV 环境基线

本项目为“数字动漫综设·三维重建”课程实验一，实现静态图像的 **读取 → 检查 → 灰度转换 → 保存**。用户通过命令行指定输入、输出路径，程序输出图像属性、处理方式、耗时和执行状态，为后续图像处理实验提供基础入口。

本实验不包含图形界面、视频处理、特征匹配或三维重建。

## 1. 项目目录

```text
project_i/
├── README.md                  安装、运行与复现说明
├── requirements.txt           固定版本依赖清单
├── src/
│   ├── version_check.py        检查解释器、虚拟环境及库版本
│   └── baseline_pipeline.py    图像处理命令行程序
├── data/
│   ├── input/                 原始输入与测试样本，保留不修改
│   └── output/                灰度结果
├── docs/
│   ├── requirements.md        需求、用例和验收标准
│   ├── environment.md         原环境及依赖记录
│   └── test_report.md         测试结果与分析
└── evidence/                  终端截图、耗时日志与输出信息汇总
```

`docs/requirements.md` 描述程序应做什么；`requirements.txt` 用于安装 Python 依赖。输出图片、运行证据和测试报告分别保存在 `data/output/`、`evidence/` 和 `docs/`。

## 2. 环境与安装

原环境记录核验于 2026-09-15：

| 项目 | 记录值 |
|---|---|
| 操作系统 | Windows 11，Windows-11-10.0.26200-SP0 |
| Python | 3.13.7 |
| opencv-python | 5.0.0.93（`cv2.__version__` 为 5.0.0） |
| NumPy | 2.5.2 |
| Matplotlib | 3.11.1 |

完整依赖见 [requirements.txt](requirements.txt)。基线程序使用 OpenCV 和 NumPy；环境检查还检查 Matplotlib，供后续绘图实验使用。

以下命令使用 **Windows PowerShell**，均在 `project_i` 目录执行。先进入项目；如果项目已复制到其他位置，请替换路径：

```powershell
Set-Location -LiteralPath 'D:\自学\数字动漫综设·三维重建\数漫实验\project_i'
```

### 方式 A：使用本机已有环境

现有虚拟环境位于科目目录 `D:\自学\数字动漫综设·三维重建\.venv`，从项目目录需向上两级访问：

```powershell
$pythonExe = '..\..\.venv\Scripts\python.exe'
& $pythonExe -X utf8 '.\src\version_check.py'
& $pythonExe -m pip check
```

确认解释器路径正确、虚拟环境标记为 `True`、三个库均可导入，且 `pip check` 无依赖冲突。早期文档中的 `..\.venv` 是项目移动前的路径，不适用于当前目录结构。

### 方式 B：在另一台电脑新建环境

准备 Python 3.13.7，将完整 `project_i` 文件夹复制到本机，在项目目录执行：

```powershell
python --version
python -m venv .venv
$pythonExe = '.\.venv\Scripts\python.exe'
& $pythonExe -m pip install -r '.\requirements.txt'
& $pythonExe -X utf8 '.\src\version_check.py'
& $pythonExe -m pip check
```

创建前确认 `python --version` 为所需版本；如不是，请使用 Python 3.13.7 解释器的实际路径创建环境。此方式创建的 `.venv` 在项目内部，与方式 A 的位置不同。安装依赖需要能够访问包源；新电脑或全新环境的安装复现尚未验证，不能将上述步骤视为已完成的复现记录。

两种方式任选一种。后续命令均沿用该方式设置的 `$pythonExe`，无需激活虚拟环境；重新打开 PowerShell 后需重新进入项目目录并设置该变量。

## 3. 快速运行

先按上一节设置 `$pythonExe`，查看参数说明：

```powershell
& $pythonExe -X utf8 '.\src\baseline_pipeline.py' --help
```

处理项目自带的彩色 PNG。每次使用新的结果目录，避免与历史结果冲突：

```powershell
$runDir = '.\data\output\readme_' + [guid]::NewGuid().ToString('N')
& $pythonExe -X utf8 '.\src\baseline_pipeline.py' --input '.\data\input\color.png' --output "$runDir\color_gray.png"
$LASTEXITCODE
```

成功时终端包含版本、输入输出完整路径、`shape`、`dtype`、通道数、处理方式、`elapsed_seconds` 和 `SUCCESS`，退出码为 `0`。结果保存在 `$runDir` 下的 `color_gray.png`，缺失目录由程序自动创建。

自带 `color.png` 的已记录输入为 `(1254, 1254, 3)`，输出应为 `(1254, 1254)`、`uint8`、单通道。耗时随机器和运行条件变化。

也可处理灰度图或中文、空格路径：

```powershell
& $pythonExe -X utf8 '.\src\baseline_pipeline.py' --input '.\data\input\gray.png' --output "$runDir\gray_copy.png"
$LASTEXITCODE
& $pythonExe -X utf8 '.\src\baseline_pipeline.py' --input '.\data\input\中文 测试.png' --output "$runDir\中文 灰度.png"
$LASTEXITCODE
```

重复运行同一条转换命令会因输出已存在而被拒绝。重新生成 `$runDir` 或更换输出文件名即可；不要为重跑实验删除历史证据。

## 4. 参数与输入输出约定

| 参数 | 是否必填 | 含义 |
|---|---|---|
| `--input` | 是 | 输入图片路径，支持 `.png`、`.jpg`、`.jpeg`，扩展名不区分大小写 |
| `--output` | 是 | 输出图片路径，扩展名必须为 `.png`，不区分大小写 |
| `--help` | 否 | 显示帮助并退出 |

相对路径以**终端当前目录**为基准；含空格的路径应加引号。读取采用 Python 文件字节读取配合 OpenCV 解码，保存采用 OpenCV 编码配合 Python 文件写入，以支持 Windows 中文路径。

| 输入类型 | 处理方式 |
|---|---|
| 灰度图，`shape=(H,W)` | 保持像素值，直接保存 |
| 三通道图，`shape=(H,W,3)` | 按 BGR 顺序转换为灰度 |
| 四通道图，`shape=(H,W,4)` | 丢弃 Alpha，再将 BGR 转为灰度 |

- 仅接收可解码的 `uint8` 图像，像素范围为 0～255；16 位等其他类型明确拒绝，不自动缩放。
- `H` 为高度（行数），`W` 为宽度（列数），像素访问为 `img[y, x]`；`ndim` 是数组维数，灰度为 2，BGR/BGRA 为 3。
- OpenCV 彩色数据使用 BGR/BGRA 顺序。以后使用 Matplotlib 显示彩色图时需转为 RGB；显示灰度图可指定 `cmap='gray'`。
- Alpha 被直接舍弃，不进行背景合成。完全透明像素仍按其存储的 BGR 值计算灰度，因此结果可能与图片查看器的透明背景显示不同。
- 输出统一为同宽高的单通道 `uint8` PNG，不修改原始输入。
- 输入输出解析后的路径相同或输出路径已存在时拒绝处理；输出目录不存在时自动创建。

## 5. 状态与常见问题

| 现象 | 含义与处理方式 |
|---|---|
| `SUCCESS`，退出码 `0` | 处理完成，检查指定输出文件 |
| `ERROR`，退出码 `1` | 输入校验、转换或保存失败，按错误原因处理 |
| 参数错误，退出码 `2` | 检查是否提供 `--input`、`--output`，可运行 `--help` |
| 输入不存在或不是普通文件 | 核对当前目录、文件名和路径 |
| 图片无法解码或文件为空 | 检查文件内容；修改扩展名不能修复损坏图片 |
| 仅支持 `uint8` 输入 | 改用 8 位样本；本程序不自动转换位深 |
| 输出路径已存在，拒绝覆盖 | 换一个输出名称或新建结果目录 |
| 输出扩展名必须是 `.png` | 将输出后缀改为 `.png` |
| 无法创建目录或写入文件 | 检查目录权限，以及父路径是否被普通文件占用 |
| 无法导入 `cv2`、`numpy` 等 | 核对 `$pythonExe`，使用同一解释器安装依赖并执行环境检查 |

在程序命令之后立即查看 `$LASTEXITCODE`，避免被后续命令更新。对不存在路径、损坏文件等测试，预期的 `ERROR` 和非零退出码表示正确拒绝异常输入。

## 6. 测试记录与复现

依据 [测试报告](docs/test_report.md) 中 2026-09-16 的记录，T01～T10 均有证据支持通过；本 README 的整理不代表重新执行了这些测试。

| 用例 | 测试内容 | 输入样本 |
|---|---|---|
| T01 | 彩色 PNG 转灰度 | `color.png` |
| T02 | JPEG 读取与保存 | `color.jpg` |
| T03 | 不存在路径报错 | `does_not_exist.png`（应不存在） |
| T04 | 灰度输入保持像素 | `gray.png` |
| T05 | 舍弃 Alpha 后转换 | `rgba.png` |
| T06 | 中文与空格路径 | `中文 测试.png` |
| T07 | 损坏文件报错 | `broken.png` |
| T08 | 自动创建输出目录 | `color.png`，输出到此前不存在的目录 |
| T09 | 全黑、全白边界输入 | `black.png`、`white.png` |
| T10 | 1920×1080 图片性能 | `large_1080p.png`，运行三次 |

表中输入相对于 `data/input/`。复现时按快速运行的命令形式替换输入文件，每次选择尚不存在的输出路径。T08 应先用 `Test-Path` 确认目标父目录不存在，再运行并核对输出文件已生成。

如需保存一次新的运行记录，可使用以下命令；日志需要显式保存，程序不会自动更新 `evidence/output_info.txt`：

```powershell
$runId = [guid]::NewGuid().ToString('N')
$logPath = ".\evidence\readme_$runId.txt"
$outputPath = ".\data\output\readme_$runId.png"
& $pythonExe -X utf8 '.\src\baseline_pipeline.py' --input '.\data\input\color.png' --output $outputPath 2>&1 | Tee-Object -FilePath $logPath
$exitCode = $LASTEXITCODE
"exit_code=$exitCode" | Tee-Object -FilePath $logPath -Append
```

复现时应记录命令、输入输出路径、属性、状态、退出码和耗时，并重新读取结果核对尺寸、类型及像素。T04 应与输入逐像素一致；T09 输出应分别全 0、全 255；T03、T07 应报错且不产生输出。完整历史证据索引见 [输出信息汇总](evidence/output_info.txt)。

T10 历史三次耗时为 0.034961、0.029327、0.030701 秒，平均 0.031663 秒，均低于本机验收阈值 2 秒。程序计时从路径处理前开始，覆盖路径检查、读取、转换和保存，不含解释器启动与库导入；该结果不保证其他机器或图片具有相同耗时。

## 7. 已知限制与后续约定

- T11、T12 为需求草案追加测试，本次未执行，AC-07 未验收；不能宣称 16 位输入、覆盖冲突和写入失败等场景均已实测通过。
- 尚未完成另一台电脑或全新虚拟环境的安装复现。
- 历史记录未保存独立程序版本号、源码哈希及输入运行前后的哈希，不能据此完成历史版本追溯或原图未改动的哈希证明。
- 每次命令处理一张静态图像；不提供批处理、透明背景合成或位深自动转换。

后续实验沿用 `--input` / `--output` 参数、`uint8` 数据约定、BGR 通道顺序、Alpha 处理规则、同尺寸单通道灰度输出及退出码规则。新增处理步骤时继续保留原图、禁止覆盖已有输出，并补充对应测试。

## 8. 相关文档

- [需求规格与验收标准](docs/requirements.md)
- [环境记录](docs/environment.md)
- [测试报告](docs/test_report.md)
- [运行证据汇总](evidence/output_info.txt)

README 更新日期：2026-09-20。早期文档中“README 尚待完成”为当时状态；当前使用与复现方法以本文件为准，历史测试结论仍以测试报告和原始证据为准。
