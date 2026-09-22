import cv2
import matplotlib.pyplot as plt

img1 = cv2.imread("../data/photo01.jpg", 0)
img2 = cv2.imread("../data/photo02.jpg", 0)
img3 = cv2.imread("../results/linear.jpg", 0)

# 绘制直方图
plt.figure(figsize=(15, 4))
plt.subplot(131)
plt.plot(cv2.calcHist([img1], [0], None, [256], [0, 256]), color='blue')
plt.title("Dark Histogram")

plt.subplot(132)
plt.plot(cv2.calcHist([img2], [0], None, [256], [0, 256]).ravel(), color='red')
plt.title("Light Histogram")

plt.subplot(133)
plt.plot(cv2.calcHist([img3], [0], None, [256], [0, 256]).ravel(), color='green')
plt.title("Blur Histogram")

plt.savefig("../results/l2_histogram_all.png")
plt.show()