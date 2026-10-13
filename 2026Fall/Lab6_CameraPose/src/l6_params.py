from pathlib import Path
import csv
import cv2

ROOT = Path(__file__).resolve().parent.parent
IMAGE_PATH = ROOT / "data" / "photo01.jpg"
RESULTS_DIR = ROOT / "results"
CSV_PATH = RESULTS_DIR / "l6_parameter_comparison.csv"


def main():
    img = cv2.imread(str(IMAGE_PATH))

    if img is None:
        raise FileNotFoundError(f"无法读取图片：{IMAGE_PATH}")

    RESULTS_DIR.mkdir(parents=True, exist_ok=True)

    # 每组只改变一个参数，其余参数使用 OpenCV 默认值。
    experiments = [
        # nfeatures 实验
        ("nfeatures", "0（不限制）", {"nfeatures": 0}, None),
        ("nfeatures", "500", {"nfeatures": 500},
         "l6_param_nfeatures_500.jpg"),
        ("nfeatures", "1000", {"nfeatures": 1000},
         "l6_param_nfeatures_1000.jpg"),
        ("nfeatures", "2000", {"nfeatures": 2000},
         "l6_param_nfeatures_2000.jpg"),

        # contrastThreshold 实验
        ("contrastThreshold", "0.01",
         {"contrastThreshold": 0.01},
         "l6_param_contrast_001.jpg"),
        ("contrastThreshold", "0.04（默认）",
         {"contrastThreshold": 0.04}, None),
        ("contrastThreshold", "0.08",
         {"contrastThreshold": 0.08},
         "l6_param_contrast_008.jpg"),

        # edgeThreshold 实验
        ("edgeThreshold", "5",
         {"edgeThreshold": 5},
         "l6_param_edge_5.jpg"),
        ("edgeThreshold", "10（默认）",
         {"edgeThreshold": 10}, None),
        ("edgeThreshold", "20",
         {"edgeThreshold": 20},
         "l6_param_edge_20.jpg"),
    ]

    rows = []

    for category, setting, config, output_name in experiments:
        sift = cv2.SIFT_create(**config)
        keypoints, descriptors = sift.detectAndCompute(img, None)

        count = len(keypoints)
        dim = descriptors.shape[1] if descriptors is not None else 0

        row = {
            "实验类别": category,
            "参数设置": setting,
            "关键点数量": count,
            "描述子维度": dim,
        }
        rows.append(row)

        print(
            f"{category:20s} | "
            f"{setting:15s} | "
            f"关键点: {count:5d} | "
            f"描述子维度: {dim}"
        )

        # 对非默认参数保存关键点可视化图。
        if output_name is not None:
            vis = cv2.drawKeypoints(
                img,
                keypoints,
                None,
                flags=cv2.DRAW_MATCHES_FLAGS_DRAW_RICH_KEYPOINTS
            )

            output_path = RESULTS_DIR / output_name
            if not cv2.imwrite(str(output_path), vis):
                raise RuntimeError(f"保存图片失败：{output_path}")

    # 保存为 CSV，Excel 也可以直接打开。
    with open(CSV_PATH, "w", newline="", encoding="utf-8-sig") as f:
        writer = csv.DictWriter(
            f,
            fieldnames=[
                "实验类别",
                "参数设置",
                "关键点数量",
                "描述子维度",
            ],
        )
        writer.writeheader()
        writer.writerows(rows)

    print("\n参数对比表已保存:", CSV_PATH)
    print("可视化结果已保存到:", RESULTS_DIR)


if __name__ == "__main__":
    main()