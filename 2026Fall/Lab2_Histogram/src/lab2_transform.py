import cv2
import numpy as np
import matplotlib.pyplot as plt

# 读入偏暗图片，以灰度模式读取 (参数 0)
img = cv2.imread("../data/photo01.jpg", 0)
if img is None:
    raise ValueError("请检查 data/photo01.jpg 是否存在！")

# 线性变换: out = 1.5 * img + 30 (提升对比度和亮度)
# 必须先转 float32 运算，用 np.clip 限制在 0-255，再转回 uint8
linear = np.clip(1.5 * img.astype(np.float32) + 30, 0, 255).astype(np.uint8)

# 伽马校正: 提亮暗部 (gamma = 2.2)。压暗就把 2.2 改成 0.5
gamma = (255 * ((img / 255.0) ** (1 / 2.2))).astype(np.uint8)

# 保存结果
cv2.imwrite("../results/l2_linear.jpg", linear)
cv2.imwrite("../results/l2_gamma.jpg", gamma)

# matplotlib 可视化对比 (1x3 子图)
plt.figure(figsize=(12, 4))
plt.subplot(131), plt.imshow(img, cmap='gray'), plt.title('Original (Dark)')
plt.subplot(132), plt.imshow(linear, cmap='gray'), plt.title('Linear (a=1.5, b=30)')
plt.subplot(133), plt.imshow(gamma, cmap='gray'), plt.title('Gamma (2.2)')
plt.tight_layout()
plt.savefig("../results/l2_transform_compare.png")
plt.show()