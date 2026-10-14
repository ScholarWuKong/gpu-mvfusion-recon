import cv2
import numpy as np


def harris_corners(img):
    """输入 uint8 灰度图，返回形状为 (N, 2) 的整数坐标。"""

    # 1. 检查输入
    if not isinstance(img, np.ndarray):
        raise TypeError("输入必须是 numpy 数组")

    if img.ndim != 2:
        raise ValueError("输入必须是二维灰度图")

    if img.size == 0:
        raise ValueError("输入图像不能为空")

    if img.dtype != np.uint8:
        raise TypeError("图像类型必须是 uint8")

    # 2. 计算 Harris 响应
    response = cv2.cornerHarris(
        img,
        blockSize=2,
        ksize=3,
        k=0.04
    )

    max_response = float(response.max())

    # 没有正响应时，返回形状固定的空数组
    if max_response <= 0:
        return np.empty((0, 2), dtype=np.int32)

    # 3. 阈值筛选与局部最大值筛选
    local_max = cv2.dilate(
        response,
        np.ones((3, 3), dtype=np.uint8)
    )

    mask = (
        (response > 0.01 * max_response)
        & (response == local_max)
    )

    # 4. 将数组下标 (行, 列) 转成坐标 (x, y)
    ys, xs = np.where(mask)
    points = np.column_stack((xs, ys)).astype(np.int32)

    return points