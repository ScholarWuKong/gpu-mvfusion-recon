# 实验四：边缘检测与图像分割

本项目对应《实验四 边缘检测与图像分割》，重点完成老师要求的 6 个任务：
1. Sobel 梯度：ksize=3/5，对比梯度幅值
2. Canny：三组双阈值对比
3. 固定阈值 128 与 Otsu 对比
4. 自适应阈值与 Otsu 对比
5. 详细设计 `docs/detailed_design.md`
6. 编码规范与设计走查 `docs/coding_standards.md`、`docs/review_record.md`

## 目录

```text
experiment4_edge_segmentation/
├─ data/                  # 放实验图片
│  └─ README.md
├─ results/               # 程序运行后生成实验结果
├─ src/
│  └─ main.py             # 主程序
├─ docs/
│  ├─ detailed_design.md  # 详细设计
│  ├─ coding_standards.md # 编码规范
│  └─ review_record.md    # 设计走查记录
├─ report/
│  └─ experiment_report.md # 实验报告草稿
├─ requirements.txt
└─ README.md
```

## 环境安装（Windows PowerShell）

建议使用 Python 虚拟环境：

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
pip install -r requirements.txt
```

运行：

```powershell
python src\main.py
```

## 图片准备

在 `data/` 中放两张图片：

- `photo01.jpg`：清晰纹理图，用于 Sobel / Canny / 阈值分割
- `photo02.jpg`：光照不均图，用于自适应阈值与 Otsu 对比

如果图片文件名不同，修改 `src/main.py` 开头的配置即可。
