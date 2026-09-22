# OpenCV 直方图与运算 API 调研笔记

## cv2.calcHist

**函数签名**：`cv2.calcHist(images, channels, mask, histSize, ranges[, hist[, accumulate]]) -> hist`

**关键参数说明**：

- `images`：输入图像列表，需加方括号如 `[img]`。
- `channels`：需要统计的通道索引列表，灰度图为 `[0]`，彩图可为 `[0], [1], [2]`。
- `mask`：掩膜图像，统计局部直方图时使用。全图统计传 `None`。
- `histSize`：直方图的 BINS 数量，一般为 `[256]`（将灰度分为256个等级）。
- `ranges`：像素值范围，一般为 `[0, 256]`（不包含上限256）。

> 数据来源：[OpenCV官方文档](https://docs.opencv.org/4.13.0/d6/dc7/group__imgproc__hist.html#ga4b2b5fd75503ff9e6844cc4dcdaed35d)

## cv2.equalizeHist

**函数签名**：`cv2.equalizeHist(src[, dst]) -> dst`

**关键参数说明**：

- `src`：输入图像，**必须是 8-bit 单通道图像** (uint8 的灰度图)。如果传入多通道彩色图会报错。
- **原理**：通过累积分布函数映射使得图像的灰度直方图趋向于均匀分布，从而提升全局对比度。

> 数据来源：[OpenCV官方文档](https://docs.opencv.org/4.13.0/d6/dc7/group__imgproc__hist.html#ga7e54091f0c937d49bf84152a16f76d6e)

## cv2.addWeighted

**函数签名**：`cv2.addWeighted(src1, alpha, src2, beta, gamma[, dst[, dtype]]) -> dst`

**关键参数说明**：

- `src1` / `src2`：两张需要混合的图像。要求**大小和通道数必须完全相同**。
- `alpha`：第一张图片的权重系数。
- `beta`：第二张图片的权重系数。
- `gamma`：加到最终结果上的标量常数。公式为 `dst = src1*alpha + src2*beta + gamma`。运算会自动做饱和截断防止溢出。

> 数据来源：[OpenCV官方文档](https://docs.opencv.org/4.13.0/d2/de8/group__core__array.html#gafafb2513349db3bcff51f54ee5592a19)