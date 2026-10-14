from pathlib import Path
import cv2
import numpy as np
root=Path(__file__).resolve().parent
image_path=root/"data"/"chess.png"
gray=cv2.imdecode(
    np.fromfile(image_path,dtype=np.uint8),
    cv2.IMREAD_GRAYSCALE
)
if gray is None:
    raise ValueError("图片读取失败")
print("图片尺寸：",gray.shape)
corners=cv2.goodFeaturesToTrack(
    gray,
    maxCorners=100,
    qualityLevel=0.01,
    minDistance=30
)
if corners is None:
    points=np.empty((0,2),dtype=np.float32)
else:
    points=corners.reshape(-1,2)
print("检测到的角点数：", len(points))
print("前 5 个角点坐标：")
print(points[:5])
marked=cv2.cvtColor(gray,cv2.COLOR_GRAY2BGR)
for point in points:
    x,y=np.rint(point).astype(int)
    cv2.circle(marked,(x,y),4,(0,255,0),-1)
results_dir=root/"results"
results_dir.mkdir(exist_ok=True)
output_path = results_dir / "step3_shi_tomasid30.png"

success, encoded = cv2.imencode(".png", marked)
if not success:
    raise RuntimeError("图片编码失败")

encoded.tofile(output_path)
print("结果已保存到：", output_path)
levels=[gray]
for level in range(3):
    smaller=cv2.pyrDown(levels[-1])
    levels.append(smaller)
for i,level in enumerate(levels):
    print(f"第{i}层:{level.shape}")
for i,level in enumerate(levels):
    output_path=results_dir/f"step4_pyramid_{i}.png"
    sucess,encoded=cv2.imencode(".png",level)
    if not sucess:
        raise RuntimeError(f"第{i}层图片编码失败")
    output_path.write_bytes(encoded.tobytes())
    print("已保存:",output_path)
base=levels[0].astype(np.float32)
aligned=cv2.resize(
    levels[1].astype(np.float32),
    (base.shape[1],base.shape[0]),
    interpolation=cv2.INTER_LINEAR
)
dog=base-aligned
print("差分图尺寸：", dog.shape)
print("最小响应：", dog.min())
print("最大响应：", dog.max())
dog_abs=np.abs(dog)
max_value=float(dog_abs.max())
dog_show=(
    dog_abs/max(max_value,1e-6)*255
).astype(np.uint8)
output_path = results_dir / "step5_dog.png"

success, encoded = cv2.imencode(".png", dog_show)
if not success:
    raise RuntimeError("差分图编码失败")

output_path.write_bytes(encoded.tobytes())
print("差分图已保存：", output_path)
dogs=[]
for i in range(len(levels)-1):
    base=levels[i].astype(np.float32)
    aligned=cv2.resize(
        levels[i+1].astype(np.float32),
        (base.shape[1],base.shape[0]),
        interpolation=cv2.INTER_LINEAR
    )
    current_dog=base-aligned
    dogs.append(current_dog)
    print(
        f"第 {i} 组差分：",
        current_dog.shape,
        "最小值：", current_dog.min(),
        "最大值：", current_dog.max()
    )
global_max = max(
    float(np.abs(d).max())
    for d in dogs
)
for i, current_dog in enumerate(dogs):
    dog_show = (
        np.abs(current_dog)
        / max(global_max, 1e-6)
        * 255
    ).astype(np.uint8)

    output_path = results_dir / f"step5_dog_pair_{i}_{i + 1}.png"

    success, encoded = cv2.imencode(".png", dog_show)
    if not success:
        raise RuntimeError(f"第 {i} 组差分图编码失败")

    output_path.write_bytes(encoded.tobytes())
    print("已保存：", output_path)
# 在每组差分图中标出响应幅度的局部峰值区域
for i, current_dog in enumerate(dogs):
    magnitude = np.abs(current_dog)

    # 每个像素周围 3×3 邻域的最大响应幅度
    local_max = cv2.dilate(
        magnitude,
        np.ones((3, 3), dtype=np.uint8)
    )

    # 使用前面计算的 global_max，统一各组筛选阈值
    mask = (
        (magnitude == local_max)
        & (magnitude > 0.2 * global_max)
    )

    # 排除图像最外侧一圈
    mask[0, :] = False
    mask[-1, :] = False
    mask[:, 0] = False
    mask[:, -1] = False

    marked = cv2.cvtColor(levels[i], cv2.COLOR_GRAY2BGR)
    marked[mask] = (0, 0, 255)

    output_path = results_dir / f"step5_peak_regions_{i}.png"
    success, encoded = cv2.imencode(".png", marked)
    if not success:
        raise RuntimeError("图片编码失败")
    encoded.tofile(output_path)

    print(f"第 {i} 组：峰值区域包含 {np.count_nonzero(mask)} 个像素")
    print("已保存：", output_path)