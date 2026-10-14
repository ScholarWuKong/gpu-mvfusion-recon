import numpy as np
import pytest

from src.feature import harris_corners


def make_chessboard():
    """生成 8×8 棋盘格，每个方格为 20×20 像素。"""
    ys, xs = np.indices((160, 160))
    return (((xs // 20 + ys // 20) % 2) * 255).astype(np.uint8)


# 1. 非数组输入应报错
def test_invalid_input_type():
    with pytest.raises(TypeError):
        harris_corners("这不是图像")


# 2. 浮点图像不符合本函数的 uint8 输入约定
def test_invalid_dtype():
    img = np.zeros((50, 50), dtype=np.float32)

    with pytest.raises(TypeError):
        harris_corners(img)


# 3. 彩色图不符合二维灰度图约定
def test_color_image_rejected():
    img = np.zeros((50, 50, 3), dtype=np.uint8)

    with pytest.raises(ValueError):
        harris_corners(img)


# 4. 空图像应报错
def test_empty_image():
    img = np.empty((0, 0), dtype=np.uint8)

    with pytest.raises(ValueError):
        harris_corners(img)


# 5. 纯色图应返回空坐标数组
def test_plain_image_empty():
    img = np.full((50, 50), 128, dtype=np.uint8)
    points = harris_corners(img)

    assert isinstance(points, np.ndarray)
    assert points.shape == (0, 2)


# 6. 棋盘格应能检测到角点
def test_chessboard_has_corners():
    points = harris_corners(make_chessboard())

    assert len(points) > 0


# 7. 输出应为整数坐标，且全部位于图像范围内
def test_output_coordinates():
    # 使用非正方形图像，更容易发现 x、y 搞反的问题
    img = make_chessboard()[:100, :]
    points = harris_corners(img)
    height, width = img.shape

    assert isinstance(points, np.ndarray)
    assert points.ndim == 2
    assert points.shape[1] == 2
    assert np.issubdtype(points.dtype, np.integer)
    assert len(points) > 0

    xs = points[:, 0]
    ys = points[:, 1]

    assert np.all((xs >= 0) & (xs < width))
    assert np.all((ys >= 0) & (ys < height))