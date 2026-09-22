"""实验一：读取、检查、灰度转换、保存。"""

import argparse
import platform
import sys
from pathlib import Path
from time import perf_counter

import cv2
import numpy as np


class InputParser(argparse.ArgumentParser):
    def error(self, message):
        # 缺少参数等情况也使用统一的 ERROR 标记。
        self.print_usage(sys.stderr)
        self.exit(2, f"ERROR: 参数错误：{message}\n")


def parse_args():
    parser = InputParser(description="将8位PNG/JPEG转换为灰度PNG")
    parser.add_argument("--input", required=True, help="原始图片路径")
    parser.add_argument("--output", required=True, help="输出PNG路径，不覆盖已有文件")
    return parser.parse_args()


def read_image(path):
    if not path.is_file():
        raise ValueError(f"输入文件不存在或不是普通文件：{path}")
    if path.suffix.lower() not in (".png", ".jpg", ".jpeg"):
        raise ValueError("输入仅支持PNG/JPEG扩展名")

    # 先用Python读取文件字节，再让OpenCV解码，支持Windows中文路径。
    content = path.read_bytes()
    if not content:
        raise ValueError("输入文件为空，读取失败")
    image = cv2.imdecode(np.frombuffer(content, dtype=np.uint8), cv2.IMREAD_UNCHANGED)
    if image is None:
        raise ValueError("图片无法解码，读取失败")
    if image.dtype != np.uint8:
        raise ValueError(f"仅支持uint8输入，实际为{image.dtype}")
    return image


def to_gray(image):
    if image.ndim == 2:
        return image, 1, "灰度输入，保持像素值"
    if image.ndim == 3 and image.shape[2] in (3, 4):
        channels = image.shape[2]
        gray = cv2.cvtColor(image[:, :, :3], cv2.COLOR_BGR2GRAY)
        note = "丢弃Alpha，不与背景合成" if channels == 4 else "BGR转灰度"
        return gray, channels, note
    raise ValueError(f"不支持的图像形状：{image.shape}")


def save_gray(path, gray):
    # imencode检查编码结果；Python文件接口负责中文路径及禁止覆盖。
    success, encoded = cv2.imencode(".png", gray)
    if not success:
        raise OSError("PNG编码失败")
    path.parent.mkdir(parents=True, exist_ok=True)
    created = False
    try:
        with path.open("xb") as output_file:
            created = True
            payload = encoded.tobytes()
            if output_file.write(payload) != len(payload):
                raise OSError("输出写入不完整")
    except OSError:
        # 只清理本次新建且写入失败的文件，绝不删除原有文件。
        if created:
            path.unlink(missing_ok=True)
        raise


def main():
    args = parse_args()
    print(f"Python={platform.python_version()} OpenCV={cv2.__version__} NumPy={np.__version__}")
    start = perf_counter()
    try:
        input_path = Path(args.input).resolve()
        output_path = Path(args.output).resolve()
        print(f"input: {input_path}")
        print(f"output: {output_path}")
        if input_path == output_path:
            raise ValueError("输入与输出不能是同一个路径")
        if output_path.suffix.lower() != ".png":
            raise ValueError("输出扩展名必须是.png")
        if output_path.exists():
            raise ValueError("输出路径已存在，拒绝覆盖；请更换输出名称")

        image = read_image(input_path)
        gray, channels, note = to_gray(image)
        print(f"input shape={image.shape} dtype={image.dtype} channels={channels}")
        print(f"处理方式：{note}")
        save_gray(output_path, gray)
        elapsed = perf_counter() - start
        print(f"output shape={gray.shape} dtype={gray.dtype} channels=1")
        print(f"elapsed_seconds={elapsed:.6f}")
        print("SUCCESS")
        return 0
    except (ValueError, OSError, cv2.error) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        print(f"elapsed_seconds={perf_counter() - start:.6f}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
