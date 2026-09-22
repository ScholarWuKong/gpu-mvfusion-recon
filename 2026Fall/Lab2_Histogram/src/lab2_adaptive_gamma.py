import cv2
import numpy as np

def adaptive_gamma_correction(img):
    # 计算归一化后的平均亮度
    mean_brightness = np.mean(img) / 255.0
    # 设定中间亮度标准 (通常是 0.5)
    # 巧妙的公式：gamma = log(0.5) / log(mean_brightness)
    # 如果图像暗(mean<0.5)，计算出 gamma>1，进行提亮。
    gamma = np.log(0.5) / np.log(mean_brightness)
    
    # 应用伽马校正
    out = (255 * ((img / 255.0) ** (1 / gamma))).astype(np.uint8)
    return out, gamma

img = cv2.imread("../data/photo02.jpg", 0)
adaptive_img, g_val = adaptive_gamma_correction(img)
print(f"自动计算的 Gamma 值为: {g_val:.2f}")
cv2.imwrite("../results/l2_adaptive_gamma.jpg", adaptive_img)