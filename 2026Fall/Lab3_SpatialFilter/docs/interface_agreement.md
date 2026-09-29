# 接口约定：预处理模块（实验三交付物）

> 软件工程环节：接口约定（任务 6）。小组约定统一函数签名与数据结构，供 L3-L4（预处理）及下游 L5-L7 模块共同遵守。

## 1. 统一函数签名

```python
def preprocess(img_bgr: np.ndarray, cfg: dict) -> np.ndarray:
    """预处理模块统一入口。

    参数
    ----
    img_bgr : np.ndarray
        输入 BGR 彩色图像，要求 uint8、ndim=3、shape[2]==3。
    cfg : dict
        处理配置，method 指定算法，其余字段为对应算法参数。

    返回
    ----
    np.ndarray
        处理后的 BGR uint8 图像，尺寸与输入一致；不修改输入数组。
    """
```

## 2. cfg 配置字典约定

| 字段 | 取值 | 默认 | 说明 |
|---|---|---|---|
| `method` | `blur` / `gaussian` / `median` / `laplacian` / `usm` / `bilateral` | `gaussian` | 处理算法 |
| `ksize` | 奇数 int | 5 | 均值 / 中值滤波核尺寸 |
| `sigma` | float | 1.5（滤波）/ 2.0（USM） | 高斯滤波 / USM 模糊尺度 |
| `alpha` | float | 0.8 | 拉普拉斯锐化增强系数 |
| `amount` | float | 0.6 | USM 增强强度 |
| `d` | int | 9 | 双边滤波邻域直径 |
| `sigma_color` | float | 75.0 | 双边滤波灰度相似度 σ |
| `sigma_space` | float | 75.0 | 双边滤波空间距离 σ |

示例：

```python
out1 = preprocess(img_bgr, {"method": "gaussian", "sigma": 1.5})     # 去噪
out2 = preprocess(img_bgr, {"method": "usm", "sigma": 2.0, "amount": 0.6})  # 锐化
```

## 3. 输入输出与异常约定

- 输入：BGR、uint8、`(H, W, 3)`；灰度中间处理在模块内部完成，**对外始终返回 BGR**。
- 输出：与输入同尺寸的 uint8 BGR 图像；**不修改输入数组**（基于副本处理）。
- 异常：非法类型（非 ndarray、非 uint8、非 3 通道）抛 `TypeError`；非法参数（如 `method` 未知、中值核为偶数）抛 `ValueError`；调用方按异常类型统一处理，不静默失败。
- 约定来源：小组协商（2026 Fall，实验三指导书任务 6）。

## 4. 与下游模块的衔接约定

| 下游实验 | 依赖的预处理能力 | 传递约定 |
|---|---|---|
| L4 边缘检测与分割 | 灰度化、去噪、锐化 | 灰度图由模块内部转换 |
| L5 角点与尺度空间 | 去噪后稳定灰度输入 | 输出仍为 BGR，由 L5 自行转灰度 |
| L6-L7 位姿与三角化 | 干净图像保证特征可重复检测 | 不修改原图，保证可追溯 |

## 5. 落地实现

实现见 `src/lab3_filter.py`：模块内全部函数（`mean_filter`、`gaussian_filter`、`median_filter`、`laplacian_sharpen`、`usm_sharpen`、`bilateral_filter`、`psnr`、`ssim`）均以本契约为准，`preprocess` 为唯一对外入口。
