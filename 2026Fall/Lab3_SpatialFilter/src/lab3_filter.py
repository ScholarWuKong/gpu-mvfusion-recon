# -*- coding: utf-8 -*-
"""实验三：空间滤波与图像增强。

覆盖指导书任务 1-4 与加分项：
  任务1  均值 / 高斯 / 中值滤波对高斯噪声的去噪对比（同一组参数）
  任务2  椒盐噪声场景下三种滤波对比（重点观察中值滤波）
  任务3  拉普拉斯与 USM 锐化
  任务4  PSNR / SSIM 定量质量评价（输出评分表）
  加分项 双边滤波（保边去噪）

模块入口符合 docs/interface_agreement.md 约定的统一接口：
    preprocess(img_bgr: np.ndarray, cfg: dict) -> np.ndarray

运行方式（在 Lab3_SpatialFilter 目录）：
    ..\..\.venv\Scripts\python.exe -X utf8 .\src\lab3_filter.py
输出写入 results/，评分表同时打印到终端并保存为 CSV。
"""

from __future__ import annotations

import sys
from pathlib import Path

import cv2
import numpy as np

BASE_DIR = Path(__file__).resolve().parent.parent
DATA_DIR = BASE_DIR / "data"
RESULT_DIR = BASE_DIR / "results"

# ---------- 噪声生成 ----------


def add_gaussian_noise(img: np.ndarray, sigma: float = 25.0, seed: int = 0) -> np.ndarray:
    """添加高斯噪声（随机灰度波动）。"""
    rng = np.random.default_rng(seed)
    return np.clip(img.astype(np.float32) + rng.normal(0, sigma, img.shape), 0, 255).astype(np.uint8)


def add_salt_pepper_noise(img: np.ndarray, prob: float = 0.05, seed: int = 0) -> np.ndarray:
    """添加椒盐噪声：随机像素置 0 或 255。"""
    rng = np.random.default_rng(seed)
    out = img.copy()
    mask = rng.random(img.shape)
    out[mask < prob / 2] = 0
    out[mask > 1 - prob / 2] = 255
    return out


# ---------- 滤波与锐化 ----------


def mean_filter(img: np.ndarray, ksize: int = 5) -> np.ndarray:
    """均值滤波：邻域无权重平均，平滑但损失边缘。"""
    return cv2.blur(img, (ksize, ksize))


def gaussian_filter(img: np.ndarray, sigma: float = 1.5) -> np.ndarray:
    """高斯滤波：按二维高斯分布加权，更柔和、相对保边。"""
    return cv2.GaussianBlur(img, (0, 0), sigma)


def median_filter(img: np.ndarray, ksize: int = 5) -> np.ndarray:
    """中值滤波：邻域中值替换中心，对椒盐噪声有效；ksize 必须为奇数。"""
    if ksize % 2 == 0:
        raise ValueError(f"中值滤波核尺寸必须为奇数，实际为 {ksize}")
    return cv2.medianBlur(img, ksize)


def bilateral_filter(img: np.ndarray, d: int = 9, sigma_color: float = 75.0, sigma_space: float = 75.0) -> np.ndarray:
    """双边滤波（加分项）：同时考虑灰度相似度与空间距离，保边去噪。"""
    return cv2.bilateralFilter(img, d, sigma_color, sigma_space)


def laplacian_sharpen(img: np.ndarray, alpha: float = 0.8) -> np.ndarray:
    """拉普拉斯锐化：g = f - alpha * Laplacian(f)。"""
    lap = cv2.Laplacian(img, cv2.CV_64F)
    return np.clip(img.astype(np.float64) - alpha * lap, 0, 255).astype(np.uint8)


def usm_sharpen(img: np.ndarray, sigma: float = 2.0, amount: float = 0.6) -> np.ndarray:
    """USM 锐化：g = (1+amount)*f - amount*blur(f)，比直接拉普拉斯更自然。"""
    blur = cv2.GaussianBlur(img, (0, 0), sigma)
    return cv2.addWeighted(img, 1.0 + amount, blur, -amount, 0)


# ---------- 质量评价 ----------


def psnr(a: np.ndarray, b: np.ndarray) -> float:
    """峰值信噪比：10*log10(255^2 / MSE)，越大越接近原图。"""
    a = a.astype(np.float64)
    b = b.astype(np.float64)
    if a.shape != b.shape:
        raise ValueError(f"PSNR 要求两图同尺寸，实际 {a.shape} 与 {b.shape}")
    mse = np.mean((a - b) ** 2)
    if mse == 0:
        return float("inf")
    return float(10 * np.log10(255.0**2 / mse))


def ssim(a: np.ndarray, b: np.ndarray) -> float:
    """结构相似度：更符合人眼感知，越接近 1 越好。"""
    from skimage.metrics import structural_similarity

    if a.shape != b.shape:
        raise ValueError(f"SSIM 要求两图同尺寸同 dtype，实际 {a.shape} 与 {b.shape}")
    return float(structural_similarity(a, b, data_range=255))


# ---------- 统一接口（接口约定） ----------


def preprocess(img_bgr: np.ndarray, cfg: dict) -> np.ndarray:
    """预处理模块统一入口（docs/interface_agreement.md）。

    cfg 支持的字段：
        method: blur | gaussian | median | laplacian | usm | bilateral
        ksize / sigma / alpha / amount / d / sigma_color / sigma_space
    输入为 BGR uint8 图像；返回处理后的 BGR uint8 图像，不修改输入。
    """
    if not isinstance(img_bgr, np.ndarray) or img_bgr.ndim != 3 or img_bgr.shape[2] != 3:
        raise TypeError(f"preprocess 要求 BGR 三通道图像，实际 {type(img_bgr)} shape={getattr(img_bgr, 'shape', None)}")
    if img_bgr.dtype != np.uint8:
        raise TypeError(f"preprocess 要求 uint8 输入，实际 {img_bgr.dtype}")
    if not isinstance(cfg, dict):
        raise TypeError("cfg 必须是 dict")

    method = cfg.get("method", "gaussian")
    # 灰度处理按单通道进行（与指导书一致），再转回 BGR 返回
    gray = cv2.cvtColor(img_bgr, cv2.COLOR_BGR2GRAY)

    if method == "blur":
        out_gray = mean_filter(gray, ksize=cfg.get("ksize", 5))
    elif method == "gaussian":
        out_gray = gaussian_filter(gray, sigma=cfg.get("sigma", 1.5))
    elif method == "median":
        out_gray = median_filter(gray, ksize=cfg.get("ksize", 5))
    elif method == "laplacian":
        out_gray = laplacian_sharpen(gray, alpha=cfg.get("alpha", 0.8))
    elif method == "usm":
        out_gray = usm_sharpen(gray, sigma=cfg.get("sigma", 2.0), amount=cfg.get("amount", 0.6))
    elif method == "bilateral":
        out_gray = bilateral_filter(gray, d=cfg.get("d", 9), sigma_color=cfg.get("sigma_color", 75.0),
                                    sigma_space=cfg.get("sigma_space", 75.0))
    else:
        raise ValueError(f"不支持的 method: {method}")

    return cv2.cvtColor(out_gray, cv2.COLOR_GRAY2BGR)


# ---------- 结果组织 ----------


def save_compare_grid(images: list, titles: list, out_name: str, ncols: int | None = None) -> None:
    """把多张灰度图并排保存为一张对比图（含标题）；ncols 指定列数（多行）。"""
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    plt.rcParams["font.sans-serif"] = ["Microsoft YaHei", "SimHei"]
    plt.rcParams["axes.unicode_minus"] = False

    n = len(images)
    ncols = ncols or n
    nrows = int(np.ceil(n / ncols))
    fig, axes = plt.subplots(nrows, ncols, figsize=(2.6 * ncols, 2.6 * nrows))
    axes = np.atleast_1d(axes).ravel()
    for ax, im, t in zip(axes, images, titles):
        ax.imshow(im, cmap="gray")
        ax.set_title(t, fontsize=9)
        ax.axis("off")
    for ax in axes[n:]:
        ax.axis("off")
    fig.tight_layout()
    fig.savefig(RESULT_DIR / out_name, dpi=110, bbox_inches="tight")
    plt.close(fig)


def main() -> int:
    RESULT_DIR.mkdir(parents=True, exist_ok=True)
    if not (DATA_DIR / "photo01.jpg").is_file():
        print("ERROR: 缺少 data/photo01.jpg，请先准备实验图片", file=sys.stderr)
        return 1

    print(f"OpenCV={cv2.__version__} NumPy={np.__version__}")
    img = cv2.imread(str(DATA_DIR / "photo01.jpg"), cv2.IMREAD_GRAYSCALE)
    if img is None:
        print("ERROR: 图片读取失败", file=sys.stderr)
        return 1
    print(f"输入: photo01.jpg shape={img.shape} dtype={img.dtype}")

    # ---- 噪声生成 ----
    gau = add_gaussian_noise(img)
    salt = add_salt_pepper_noise(img)
    cv2.imwrite(str(RESULT_DIR / "l3_gaussian_noise.jpg"), gau)
    cv2.imwrite(str(RESULT_DIR / "l3_salt_pepper.jpg"), salt)

    # ---- 任务1/2：三种滤波对比（同一组参数）----
    b1 = mean_filter(gau)
    b2 = gaussian_filter(gau)
    b3 = median_filter(gau)
    for name, out in (("l3_blur", b1), ("l3_gauss", b2), ("l3_median", b3)):
        cv2.imwrite(str(RESULT_DIR / f"{name}.jpg"), out)
    save_compare_grid([img, gau, b1, b2, b3],
                      ["原图", "高斯噪声", "均值滤波", "高斯滤波", "中值滤波"],
                      "l3_gau_compare.png")

    b1s = mean_filter(salt)
    b2s = gaussian_filter(salt)
    b3s = median_filter(salt)
    for name, out in (("l3_sp_blur", b1s), ("l3_sp_gauss", b2s), ("l3_sp_median", b3s)):
        cv2.imwrite(str(RESULT_DIR / f"{name}.jpg"), out)
    save_compare_grid([img, salt, b1s, b2s, b3s],
                      ["原图", "椒盐噪声", "均值滤波", "高斯滤波", "中值滤波"],
                      "l3_salt_compare.png")

    # ---- 任务3：锐化 ----
    sharp_lap = laplacian_sharpen(img)
    sharp_usm = usm_sharpen(img)
    cv2.imwrite(str(RESULT_DIR / "l3_sharp_lap.jpg"), sharp_lap)
    cv2.imwrite(str(RESULT_DIR / "l3_sharp_usm.jpg"), sharp_usm)
    save_compare_grid([img, sharp_lap, sharp_usm],
                      ["原图", "拉普拉斯锐化", "USM 锐化"],
                      "l3_sharpen_compare.png")

    # ---- 加分项：双边滤波 ----
    bil = bilateral_filter(img)
    cv2.imwrite(str(RESULT_DIR / "l3_bilateral.jpg"), bil)

    # ---- 任务4：PSNR / SSIM 评分表 ----
    rows = [
        ("原图", img),
        ("高斯噪声", gau),
        ("均值滤波(高斯噪声)", b1),
        ("高斯滤波(高斯噪声)", b2),
        ("中值滤波(高斯噪声)", b3),
        ("椒盐噪声", salt),
        ("均值滤波(椒盐噪声)", b1s),
        ("高斯滤波(椒盐噪声)", b2s),
        ("中值滤波(椒盐噪声)", b3s),
        ("双边滤波", bil),
    ]
    header = ["图像/方法", "PSNR(dB)", "SSIM"]
    table = [header]
    for name, arr in rows:
        table.append([name, f"{psnr(img, arr):.2f}", f"{ssim(img, arr):.4f}"])
    widths = [max(len(r[i]) for r in table) for i in range(3)]
    print("\nPSNR / SSIM 评分表（基准=原图）：")
    for r in table:
        print("  " + "  ".join(v.ljust(w) for v, w in zip(r, widths)))

    with open(RESULT_DIR / "l3_score_table.csv", "w", encoding="utf-8-sig", newline="") as f:
        import csv

        csv.writer(f).writerows(table)

    # 评分表存为图片，供实验报告引用（2 行 × 5 列）
    save_compare_grid([img, gau, b1, b2, b3, salt, b1s, b2s, b3s, bil],
                      [f"{r[0]}\nPSNR {r[1]}  SSIM {r[2]}" for r in table[1:]],
                      "l3_score_table.png", ncols=5)

    # ---- 接口约定演示：彩色 BGR 图走统一 preprocess ----
    img_bgr = cv2.imread(str(DATA_DIR / "photo01.jpg"), cv2.IMREAD_COLOR)
    out_bgr = preprocess(img_bgr, {"method": "gaussian", "sigma": 1.5})
    assert out_bgr.shape == img_bgr.shape and out_bgr.dtype == np.uint8
    cv2.imwrite(str(RESULT_DIR / "l3_preprocess_demo.jpg"), out_bgr)
    print("\npreprocess 接口演示通过：输入 BGR 彩色图，输出同尺寸 uint8 BGR 图")

    print("\nSUCCESS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
