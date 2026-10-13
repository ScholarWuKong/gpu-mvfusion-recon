from pathlib import Path
import cv2

# 项目根目录：当前脚本所在目录的上一级
ROOT = Path(__file__).resolve().parent.parent
IMAGE_PATH = ROOT / "data" / "photo01.jpg"
OUTPUT_PATH = ROOT / "results" / "l6_sift.jpg"


def main():
    # 1. 读取图片
    img = cv2.imread(str(IMAGE_PATH))

    if img is None:
        raise FileNotFoundError(f"无法读取图片：{IMAGE_PATH}")

    print("图像路径:", IMAGE_PATH)
    print("图像尺寸 (高, 宽, 通道):", img.shape)

    # 2. 创建 SIFT 检测器
    sift = cv2.SIFT_create()

    # 3. 检测关键点并计算描述子
    keypoints, descriptors = sift.detectAndCompute(img, None)

    print("关键点数量:", len(keypoints))

    if descriptors is None:
        print("描述子为空，当前图像没有生成有效描述子。")
    else:
        print("描述子形状:", descriptors.shape)
        print("描述子数据类型:", descriptors.dtype)

    # 4. 绘制关键点：包含尺度圆圈和主方向
    output = cv2.drawKeypoints(
        img,
        keypoints,
        None,
        flags=cv2.DRAW_MATCHES_FLAGS_DRAW_RICH_KEYPOINTS
    )

    # 5. 保存可视化结果
    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    success = cv2.imwrite(str(OUTPUT_PATH), output)

    if not success:
        raise RuntimeError(f"图片保存失败：{OUTPUT_PATH}")

    print("可视化结果已保存:", OUTPUT_PATH)


if __name__ == "__main__":
    main()