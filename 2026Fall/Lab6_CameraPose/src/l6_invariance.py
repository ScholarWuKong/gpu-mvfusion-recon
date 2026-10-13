from pathlib import Path
import csv

import cv2
import numpy as np

ROOT = Path(__file__).resolve().parent.parent
IMAGE_PATH = ROOT / "data" / "photo01.jpg"
RESULTS_DIR = ROOT / "results"


def rotate_expand(img, angle, scale=1.0):
    """旋转并扩大画布，返回变换后的图片和有效区域掩码。"""
    h, w = img.shape[:2]
    center = (w / 2.0, h / 2.0)

    matrix = cv2.getRotationMatrix2D(center, angle, scale)

    cos_value = abs(matrix[0, 0])
    sin_value = abs(matrix[0, 1])

    new_w = int(round(h * sin_value + w * cos_value))
    new_h = int(round(h * cos_value + w * sin_value))

    # 将原图中心移动到新画布中心，避免旋转时裁掉内容
    matrix[0, 2] += new_w / 2.0 - center[0]
    matrix[1, 2] += new_h / 2.0 - center[1]

    transformed = cv2.warpAffine(
        img, matrix, (new_w, new_h),
        flags=cv2.INTER_LINEAR,
        borderMode=cv2.BORDER_CONSTANT,
        borderValue=(0, 0, 0)
    )

    # 只允许在原图实际覆盖的区域内检测关键点
    original_mask = np.full((h, w), 255, dtype=np.uint8)
    transformed_mask = cv2.warpAffine(
        original_mask, matrix, (new_w, new_h),
        flags=cv2.INTER_NEAREST,
        borderMode=cv2.BORDER_CONSTANT,
        borderValue=0
    )

    return transformed, transformed_mask


def resize_image(img, scale):
    """按指定比例缩放，并返回有效区域掩码。"""
    transformed = cv2.resize(
        img, None,
        fx=scale,
        fy=scale,
        interpolation=cv2.INTER_AREA
    )

    mask = np.full(
        transformed.shape[:2], 255, dtype=np.uint8
    )
    return transformed, mask


def main():
    img = cv2.imread(str(IMAGE_PATH))

    if img is None:
        raise FileNotFoundError(f"无法读取图片：{IMAGE_PATH}")

    RESULTS_DIR.mkdir(parents=True, exist_ok=True)

    # 三组变换：分别测试旋转、缩放、旋转加缩放
    experiments = [
        (
            "rotation_30",
            "旋转 30°",
            *rotate_expand(img, 30, 1.0)
        ),
        (
            "scale_06",
            "缩放 0.6 倍",
            *resize_image(img, 0.6)
        ),
        (
            "rotation_30_scale_06",
            "旋转 30° + 缩放 0.6 倍",
            *rotate_expand(img, 30, 0.6)
        ),
    ]

    sift = cv2.SIFT_create()
    original_mask = np.full(img.shape[:2], 255, dtype=np.uint8)

    kp1, des1 = sift.detectAndCompute(img, original_mask)

    if des1 is None or len(des1) < 2:
        raise RuntimeError("原图特征不足，无法执行 KNN 匹配。")

    matcher = cv2.BFMatcher(cv2.NORM_L2)
    rows = []

    print("原图关键点数量:", len(kp1))

    for name, description, transformed, mask in experiments:
        image_path = RESULTS_DIR / f"l6_{name}.jpg"
        match_path = RESULTS_DIR / f"l6_matches_{name}.jpg"

        if not cv2.imwrite(str(image_path), transformed):
            raise RuntimeError(f"图片保存失败：{image_path}")

        kp2, des2 = sift.detectAndCompute(transformed, mask)

        if des2 is None or len(des2) < 2:
            good_matches = []
        else:
            # 使用全部描述子，不截断为前 300 个
            knn_matches = matcher.knnMatch(des1, des2, k=2)

            good_matches = []
            for pair in knn_matches:
                if len(pair) == 2:
                    m, n = pair
                    if m.distance < 0.75 * n.distance:
                        good_matches.append(m)

        # 用通过 ratio test 的匹配数除以原图关键点数
        ratio = len(good_matches) / max(len(kp1), 1)

        print(f"\n实验：{description}")
        print("变换后关键点数量:", len(kp2))
        print("通过 ratio test 的匹配数:", len(good_matches))
        print(f"匹配比例（good / 原图关键点）: {ratio:.3f} ({ratio:.1%})")

        # 按描述子距离排序，展示其中最好的 80 对匹配
        shown_matches = sorted(
            good_matches, key=lambda m: m.distance
        )[:80]

        match_image = cv2.drawMatches(
            img, kp1,
            transformed, kp2,
            shown_matches,
            None,
            flags=cv2.DRAW_MATCHES_FLAGS_NOT_DRAW_SINGLE_POINTS
        )

        if not cv2.imwrite(str(match_path), match_image):
            raise RuntimeError(f"匹配可视化保存失败：{match_path}")

        rows.append({
            "实验": description,
            "原图关键点数量": len(kp1),
            "变换后关键点数量": len(kp2),
            "通过ratio_test的匹配数": len(good_matches),
            "匹配比例": round(ratio, 4),
        })

    csv_path = RESULTS_DIR / "l6_invariance_results.csv"

    with open(csv_path, "w", newline="", encoding="utf-8-sig") as f:
        writer = csv.DictWriter(f, fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)

    print("\n验证结果已保存:", csv_path)
    print("变换图片和匹配可视化已保存到:", RESULTS_DIR)


if __name__ == "__main__":
    main()