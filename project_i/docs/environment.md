# 实验一环境记录

## 核验信息

- 核验日期：2026-09-15。
- 安装日期：未记录；本次沿用已有虚拟环境，没有重新安装依赖。
- 操作系统：Windows-11-10.0.26200-SP0。
- Python：3.13.7。
- 解释器：D:\自学\数字动漫综设·三维重建\.venv\Scripts\python.exe。
- 虚拟环境：位于科目目录下的 .venv，当前实验使用该环境；不是 project_i 内新建的环境。

## 核心依赖

| 用途 | 安装包名 | 导入名 | 已安装包版本 |
|---|---|---|---|
| 图像处理 | opencv-python | cv2 | 5.0.0.93 |
| 数组计算 | numpy | numpy | 2.5.2 |
| 绘图展示 | matplotlib | matplotlib | 3.11.1 |

cv2.__version__ 输出为 5.0.0；pip 中的 opencv-python 包版本为 5.0.0.93，两者采用的版本标记不同。requirements.txt 使用 pip 包版本。

## 本次检查结果

运行 src/version_check.py：Python 路径指向上述虚拟环境，虚拟环境标记为 True，三个核心库均成功导入，退出码为0。

运行 python -m pip check：输出 No broken requirements found.，退出码为0。此项表示当前已安装依赖的声明关系无冲突；不代表实验功能测试已完成。

本次未遇到导入或依赖检查异常；历史安装过程及异常没有记录。尚未在另一台电脑或全新环境中验证复现。

## PowerShell 核验命令

在 project_i 目录中运行：

```powershell
& '..\.venv\Scripts\python.exe' -X utf8 '.\src\version_check.py'
& '..\.venv\Scripts\python.exe' -m pip check
& '..\.venv\Scripts\python.exe' -m pip freeze
```

requirements.txt 来源于本次虚拟环境的 pip freeze，包含核心依赖及间接依赖，共12项。它是当前环境快照，不表示每个包都被实验代码直接导入。

## 在新环境安装的方法

以下为复现指引，本次未重新执行安装。准备 Python 3.13.7 后，在 project_i 目录运行：

```powershell
python -m venv .venv
& '.\.venv\Scripts\python.exe' -m pip install -r requirements.txt
& '.\.venv\Scripts\python.exe' -X utf8 '.\src\version_check.py'
& '.\.venv\Scripts\python.exe' -m pip check
```

这里的新环境位于 project_i 内；与当前使用的上一级 .venv 区别在路径层级。
