"""第一步：计算 Harris 响应，观察超过阈值的区域。"""
from pathlib import Path
import json

import cv2
import numpy as np


def main():
    # 按脚本位置查找图片，从任何工作目录运行都可以。
    root = Path(__file__).resolve().parent
    source = root / "data" / "chess.png"
    results = root / "results"

    # imdecode + fromfile 支持 Windows 中文路径。
    gray = cv2.imdecode(np.fromfile(source, dtype=np.uint8), cv2.IMREAD_GRAYSCALE)
    if gray is None:
        raise ValueError(f"无法读取图片：{source}")

    # 每个像素得到一个分数；此处没有提取独立角点坐标。
    response = cv2.cornerHarris(gray.astype(np.float32), 2, 3, 0.04)
    maximum = float(response.max())
    threshold = 0.01 * maximum
    mask = response > threshold if maximum > 0 else np.zeros(gray.shape, bool)

    original = cv2.cvtColor(gray, cv2.COLOR_GRAY2BGR)
    marked = original.copy()
    marked[mask] = (0, 0, 255)  # OpenCV 使用 BGR，红色是 (0,0,255)。

    results.mkdir(exist_ok=True)

    def save(name, img):
        ok, encoded = cv2.imencode(".png", img)
        if not ok:
            raise RuntimeError(f"图片编码失败：{name}")
        encoded.tofile(results / name)

    save("step1_harris.png", marked)
    np.save(results / "step1_harris_response.npy", response)

    # 预览先缩小原图再重画标记，避免细小红点在缩图时消失。
    scale = min(1.0, 700 / gray.shape[1])
    size = (round(gray.shape[1] * scale), round(gray.shape[0] * scale))
    left = cv2.resize(original, size, interpolation=cv2.INTER_AREA)
    right = left.copy()
    ys, xs = np.nonzero(mask)
    preview_points = np.unique(np.rint(np.column_stack((xs, ys)) * scale).astype(int), axis=0)
    for x, y in preview_points:
        cv2.circle(right, (int(x), int(y)), 2, (0, 0, 255), -1)
    left = cv2.copyMakeBorder(left, 40, 0, 0, 0, cv2.BORDER_CONSTANT, value=(245, 245, 245))
    right = cv2.copyMakeBorder(right, 40, 0, 0, 0, cv2.BORDER_CONSTANT, value=(245, 245, 245))
    cv2.putText(left, "Original", (15, 27), cv2.FONT_HERSHEY_SIMPLEX, .7, (0, 0, 0), 2)
    cv2.putText(right, "Harris: k=0.04, threshold=1% max", (15, 27), cv2.FONT_HERSHEY_SIMPLEX, .6, (0, 0, 0), 1)
    save("step1_comparison.png", np.hstack((left, right)))

    summary = {
        "image_shape_height_width": list(gray.shape),
        "opencv_version": cv2.__version__,
        "input_dtype": "float32",
        "input_range": "0..255",
        "blockSize": 2, "ksize": 3, "k": 0.04,
        "response_min": float(response.min()),
        "response_max": maximum,
        "threshold_ratio": 0.01, "threshold": threshold,
        "selected_pixel_count_not_corner_count": int(mask.sum()),
        "note": "No NMS. Preview marks enlarged for visibility; full-size result marks exact selected pixels.",
    }
    (results / "step1_summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
