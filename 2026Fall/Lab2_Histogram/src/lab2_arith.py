import cv2
import numpy as np
import matplotlib.pyplot as plt

# 读入两张图片（彩色或灰度均可，这里用彩色）
a = cv2.imread("../data/photo01.jpg")
b = cv2.imread("../data/photo02.jpg")

# 注意：代数运算要求两张图尺寸完全一致，如果不一致需先 Resize
# b = cv2.resize(b, (a.shape[1], a.shape[0]))

# 加法对比
# np加法：uint8类型下 250+10=4 (回绕现象，产生奇怪颜色条纹)
add_np = (a.astype(np.int16) + b.astype(np.int16)) % 256

# cv2加法：250+10=255 (饱和运算，平滑高光)
add_cv = cv2.add(a, b)

# 加权混合 (适用于转场特效或水印)
blend = cv2.addWeighted(a, 0.6, b, 0.4, 0)

# 保存
cv2.imwrite("../results/l2_add_np.jpg", add_np)
cv2.imwrite("../results/l2_add_cv.jpg", add_cv)
cv2.imwrite("../results/l2_blend.jpg", blend)

# 可视化对比
plt.figure(figsize=(15, 5))
# OpenCV读入是BGR，matplotlib显示需要RGB
plt.subplot(131), plt.imshow(cv2.cvtColor(add_np.astype(np.uint8), cv2.COLOR_BGR2RGB)), plt.title('Numpy Add (Overflow)')
plt.subplot(132), plt.imshow(cv2.cvtColor(add_cv, cv2.COLOR_BGR2RGB)), plt.title('OpenCV Add (Saturated)')
plt.subplot(133), plt.imshow(cv2.cvtColor(blend, cv2.COLOR_BGR2RGB)), plt.title('AddWeighted (0.6/0.4)')
plt.savefig("../results/l2_add_compare.png")
plt.show()