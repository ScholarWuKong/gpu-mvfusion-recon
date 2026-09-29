from pathlib import Path

import cv2
import numpy as np


# =========================
# 基本配置
# =========================

PROJECT_ROOT = Path(__file__).resolve().parent.parent
DATA_DIR = PROJECT_ROOT / "data"
RESULTS_DIR = PROJECT_ROOT / "results"

CLEAR_IMAGE = DATA_DIR / "photo01.jpg"
UNEVEN_IMAGE = DATA_DIR / "photo02.jpg"


# =========================
# 工具函数
# =========================

def ensure_results_dir() -> None:
    """创建结果目录。"""
    RESULTS_DIR.mkdir(parents=True, exist_ok=True)


def load_gray_image(image_path: Path) -> np.ndarray:
    """
    读取灰度图像。

    输入：
        image_path: 图片路径

    输出：
        灰度图像，numpy.ndarray

    异常：
        图片不存在或读取失败时抛出 FileNotFoundError。
    """
    image = cv2.imread(str(image_path), cv2.IMREAD_GRAYSCALE)

    if image is None:
        raise FileNotFoundError(f"无法读取图片：{image_path}")

    return image


def save_image(filename: str, image: np.ndarray) -> None:
    """将结果图保存到 results 目录。"""
    output_path = RESULTS_DIR / filename
    ok = cv2.imwrite(str(output_path), image)

    if not ok:
        raise IOError(f"图片保存失败：{output_path}")


# =========================
# 任务 1：Sobel
# =========================

def run_sobel(image: np.ndarray, ksize: int) -> np.ndarray:
    """
    计算 Sobel x/y 梯度，并得到梯度幅值。

    G = sqrt(Gx^2 + Gy^2)

    最后归一化到 0~255 并转换为 uint8。
    """
    gx = cv2.Sobel(image, cv2.CV_64F, 1, 0, ksize=ksize)
    gy = cv2.Sobel(image, cv2.CV_64F, 0, 1, ksize=ksize)

    magnitude = np.sqrt(gx ** 2 + gy ** 2)

    max_value = magnitude.max()
    if max_value > 0:
        magnitude = magnitude / max_value * 255.0

    return np.clip(magnitude, 0, 255).astype(np.uint8)


def experiment_sobel(image: np.ndarray) -> None:
    """执行 Sobel 3×3 与 5×5 对比实验。"""
    for ksize in (3, 5):
        magnitude = run_sobel(image, ksize)
        save_image(f"l4_sobel_mag_k{ksize}.jpg", magnitude)


# =========================
# 任务 2：Canny
# =========================

def run_canny(image: np.ndarray, threshold1: int, threshold2: int) -> np.ndarray:
    """执行 Canny 双阈值边缘检测。"""
    return cv2.Canny(image, threshold1, threshold2)


def experiment_canny(image: np.ndarray) -> None:
    """执行三组 Canny 参数对比。"""
    settings = {
        "c50": (50, 150),
        "c100": (100, 200),
        "c150": (150, 250),
    }

    for name, (t1, t2) in settings.items():
        edges = run_canny(image, t1, t2)
        save_image(f"l4_{name}.jpg", edges)


# =========================
# 任务 3：固定阈值与 Otsu
# =========================

def experiment_global_threshold(image: np.ndarray) -> float:
    """执行固定阈值 128 与 Otsu 阈值分割，并返回 Otsu 阈值。"""
    _, fixed = cv2.threshold(
        image, 128, 255, cv2.THRESH_BINARY
    )

    otsu_threshold, otsu = cv2.threshold(
        image, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU
    )

    save_image("l4_threshold_fixed128.jpg", fixed)
    save_image("l4_threshold_otsu.jpg", otsu)

    return float(otsu_threshold)


# =========================
# 任务 4：自适应阈值
# =========================

def experiment_adaptive_threshold(image: np.ndarray) -> None:
    """执行自适应阈值分割。"""
    adaptive = cv2.adaptiveThreshold(
        image,
        255,
        cv2.ADAPTIVE_THRESH_GAUSSIAN_C,
        cv2.THRESH_BINARY,
        31,
        5,
    )

    save_image("l4_adaptive.jpg", adaptive)


# =========================
# 主程序
# =========================

def main() -> None:
    ensure_results_dir()

    print("=== 实验四：边缘检测与图像分割 ===")

    # 任务 1~3 使用清晰纹理图
    image = load_gray_image(CLEAR_IMAGE)

    print(f"[INFO] 使用图片：{CLEAR_IMAGE}")

    experiment_sobel(image)
    print("[OK] Sobel 完成")

    experiment_canny(image)
    print("[OK] Canny 完成")

    otsu_threshold = experiment_global_threshold(image)
    print(f"[OK] 固定阈值 + Otsu 完成，Otsu 阈值 = {otsu_threshold:.2f}")

    # 任务 4 使用光照不均图
    uneven_image = load_gray_image(UNEVEN_IMAGE)

    print(f"[INFO] 光照不均图片：{UNEVEN_IMAGE}")

    experiment_adaptive_threshold(uneven_image)
    print("[OK] 自适应阈值完成")

    print(f"\n结果已保存到：{RESULTS_DIR}")


if __name__ == "__main__":
    main()
