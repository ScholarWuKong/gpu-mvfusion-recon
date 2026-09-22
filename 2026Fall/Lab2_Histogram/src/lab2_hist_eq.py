import cv2
import numpy as np
import matplotlib.pyplot as plt

img = cv2.imread("../data/photo01.jpg", 0)

# --- 任务3：统计原图直方图 ---
hist = cv2.calcHist([img], [0], None, [256], [0, 256])

# --- 任务4：手写直方图均衡化 ---
# 计算累积分布函数 (CDF)
cdf = hist.ravel().cumsum()
# 归一化映射到 0-255 范围
cdf = (cdf - cdf.min()) * 255 / (cdf.max() - cdf.min())
# 查表映射生成新图
manual = cdf[img].astype(np.uint8)

# --- 任务4：调用 API 对比 ---
apiv = cv2.equalizeHist(img)

# 验证差异
diff = np.abs(manual.astype(int) - apiv.astype(int))
print("最大差异:", diff.max(), " 差异像素占比:", (diff > 1).mean())
# print(f"最大差异: {diff.max()}")
# print(f"差异像素占比: {(diff > 1).mean():.4f}")

# 保存对比图
combined = np.hstack((img, manual, apiv))
cv2.imwrite("../results/l2_eq_compare.jpg", combined)

# 绘制直方图对比
plt.figure(figsize=(10, 4))
plt.subplot(121)
plt.plot(hist)
# plt.plot(hist, color='blue')
plt.title("Histogram")

hist_eq = cv2.calcHist([manual], [0], None, [256], [0, 256]).ravel()
plt.subplot(122)
plt.plot(hist_eq, color='red')
plt.title("Equalized Histogram")
plt.savefig("../results/l2_hist.png")
plt.show()