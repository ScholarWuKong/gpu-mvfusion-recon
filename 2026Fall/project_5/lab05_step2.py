"""比较 Harris k 值；统计的是阈值像素，不是去重后的角点。"""
from pathlib import Path
import csv
import cv2
import numpy as np


def main():
    root = Path(__file__).resolve().parent
    gray = cv2.imdecode(np.fromfile(root / 'data/chess.png', dtype=np.uint8), 0)
    if gray is None:
        raise ValueError('棋盘格读取失败')
    out = root / 'results/step2_k_comparison'
    out.mkdir(parents=True, exist_ok=True)
    src = gray.astype(np.float32)
    original = cv2.cvtColor(gray, cv2.COLOR_GRAY2BGR)
    baseline = cv2.cornerHarris(src, 2, 3, 0.04)
    fixed_threshold = 0.01 * float(baseline.max())
    rows, panels = [], []
    previous = None

    def save(name, img):
        ok, encoded = cv2.imencode('.png', img)
        if not ok:
            raise RuntimeError('PNG 编码失败')
        encoded.tofile(out / name)

    for k in (0.04, 0.06, 0.1):
        response = cv2.cornerHarris(src, 2, 3, k)
        assert response.shape == gray.shape and np.isfinite(response).all()
        if previous is not None:
            # 相同输入下 k 增大，响应不应上升；允许浮点舍入误差。
            tolerance = 1e-6 * max(float(np.abs(previous).max()), 1.0)
            assert np.all(response <= previous + tolerance)
        previous = response
        threshold = 0.01 * float(response.max())
        mask = response > threshold
        fixed_mask = response > fixed_threshold
        marked = original.copy()
        marked[mask] = (0, 0, 255)
        save(f'harris_k_{k:.2f}.png', marked)
        row = dict(k=k, response_min=float(response.min()), response_max=float(response.max()),
                   relative_threshold=threshold, relative_selected_pixels=int(mask.sum()),
                   fixed_threshold=fixed_threshold, fixed_selected_pixels=int(fixed_mask.sum()))
        rows.append(row)
        # 缩图后重绘，以免红色标记消失。这不是新的检测结果。
        scale = 480 / gray.shape[1]
        preview = cv2.resize(original, (480, round(gray.shape[0]*scale)), interpolation=cv2.INTER_AREA)
        ys, xs = np.nonzero(mask)
        pts = np.unique(np.rint(np.column_stack((xs, ys))*scale).astype(int), axis=0)
        for x, y in pts:
            cv2.circle(preview, (int(x), int(y)), 1, (0, 0, 255), -1)
        preview = cv2.copyMakeBorder(preview, 65, 0, 0, 0, cv2.BORDER_CONSTANT, value=(245,245,245))
        cv2.putText(preview, f'k={k:.2f} | pixels={mask.sum()}', (12,25), cv2.FONT_HERSHEY_SIMPLEX,.65,(0,0,0),1)
        cv2.putText(preview, f'max R={response.max():.3e} | threshold=1% max', (12,50), cv2.FONT_HERSHEY_SIMPLEX,.45,(0,0,0),1)
        panels.append(preview)
    assert all(a['fixed_selected_pixels'] >= b['fixed_selected_pixels'] for a,b in zip(rows, rows[1:]))
    save('comparison.png', np.hstack(panels))
    with (out/'statistics.csv').open('w', encoding='utf-8-sig', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)
    report = [
        '# Harris 参数对比实测记录', '',
        f'输入为 data/chess.png，尺寸 {gray.shape[1]}×{gray.shape[0]}；OpenCV {cv2.__version__}。',
        '使用 float32 的 0～255 灰度，blockSize=2、ksize=3，只改变 k。未执行 NMS，以下数量均为像素数。', '',
        '| k | 最低响应 | 最高响应 | 各自最高分 1% 的阈值 | 相对阈值像素数 | 固定阈值像素数 |',
        '|---|---:|---:|---:|---:|---:|',
    ]
    for r in rows:
        report.append(f"| {r['k']} | {r['response_min']:.2f} | {r['response_max']:.2f} | {r['relative_threshold']:.2f} | {r['relative_selected_pixels']} | {r['fixed_selected_pixels']} |")
    report += ['', f'固定阈值取 k=0.04 的最高响应的 1%，即 {fixed_threshold:.2f}。', '',
               '## 如何解释', '',
               '对于相同位置的结构张量，k 增大，R=det(M)−k·trace(M)² 减小或保持不变。',
               '固定阈值下保留像素数不会增加；相对阈值随最高分改变，像素数没有一般的单调保证，应以上表实测为准。',
               '强棋盘角点可能在三组参数下均被保留，视觉差异主要出现在较弱响应和角点附近像素。',
               '不能仅根据像素数多少判断检测质量，也不能将文字上的有效拐角响应全部视为误检。', '',
               'comparison.png 为缩小预览，红色标记作了可视化增强；各 harris_k 图片保留原始分辨率与实际阈值掩膜。',
               '脚本检查了响应尺寸、有限值、k 增大时响应不升，以及固定阈值计数不增；这些检查不替代后续 pytest 测试任务。', '',
               '复现：在 project_5 下运行 `python lab05_step2.py`。']
    (out/'实验观察.md').write_text('\n'.join(report)+'\n', encoding='utf-8')
    for row in rows:
        print(row)


if __name__ == '__main__':
    main()
