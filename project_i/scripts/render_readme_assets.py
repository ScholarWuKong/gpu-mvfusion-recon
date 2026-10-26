"""Render README figures from an existing colored reconstruction, without rerunning SfM.

Requires numpy and Pillow. Example (run from project_i):
    python scripts/render_readme_assets.py
"""
from pathlib import Path
import argparse
import json
import math
import numpy as np
from PIL import Image, ImageDraw, ImageFont


def read_cloud(path):
    with path.open(encoding="ascii") as source:
        header = []
        for line in source:
            header.append(line.strip())
            if line.strip() == "end_header":
                break
        if "format ascii 1.0" not in header:
            raise ValueError("Expected an ASCII colored PLY")
        count = int(next(line for line in header if line.startswith("element vertex ")).split()[-1])
        values = np.loadtxt(source)
    if values.shape != (count, 6) or not np.isfinite(values).all():
        raise ValueError("Invalid XYZ/RGB point data")
    return values[:, :3], values[:, 3:].astype(np.uint8)


def font(size):
    for path in (Path("C:/Windows/Fonts/segoeui.ttf"), Path("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")):
        if path.is_file():
            return ImageFont.truetype(str(path), size)
    return ImageFont.load_default()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cloud", type=Path, default=Path("data/output/gpu_calibrated_20261020/coverage_20k/points_colored.ply"))
    parser.add_argument("--output", type=Path, default=Path("docs/assets"))
    parser.add_argument("--frames", type=int, default=72)
    options = parser.parse_args()
    if options.frames < 12:
        parser.error("Use at least 12 frames")
    options.output.mkdir(parents=True, exist_ok=True)
    names = ("point-cloud-rotation.gif", "point-cloud-front.png", "reconstruction-comparison.png", "image-coverage-comparison.jpg", "readme-assets.json")
    for name in names:
        if (options.output / name).exists():
            raise FileExistsError(f"Preserving existing asset: {options.output / name}")

    points, colors = read_cloud(options.cloud)
    lower, upper = np.percentile(points, (.2, 99.8), axis=0)
    center = (lower + upper) / 2
    centered = points - center
    # Subject framing changes only the camera viewport, never the source cloud.
    radius = float(np.linalg.norm(upper - lower) / 2)
    width, height = 760, 570
    scale = 430 * .48 / radius
    title_font, text_font, foot_font = font(22), font(16), font(13)
    pitch = .08

    def render(angle):
        cosine, sine = math.cos(angle), math.sin(angle)
        x = cosine * centered[:, 0] + sine * centered[:, 2]
        z = -sine * centered[:, 0] + cosine * centered[:, 2]
        y = math.cos(pitch) * centered[:, 1] - math.sin(pitch) * z
        depth = math.sin(pitch) * centered[:, 1] + math.cos(pitch) * z
        px, py = np.rint(width / 2 + x * scale).astype(int), np.rint(310 + y * scale).astype(int)
        frame = Image.new("RGB", (width, height), (13, 17, 23))
        draw = ImageDraw.Draw(frame)
        for index in np.argsort(-depth):
            if 8 <= px[index] < width - 8 and 94 <= py[index] < height - 40:
                color = tuple(int(value) for value in colors[index])
                draw.rectangle((int(px[index]), int(py[index]), int(px[index]) + 1, int(py[index]) + 1), fill=color)
        draw.text((width / 2, 30), "GPU MVFusion Recon", font=title_font, anchor="mm", fill=(230, 237, 243))
        draw.text((width / 2, 63), f"36 views  |  {len(points):,} sparse points  |  calibrated intrinsics", font=text_font, anchor="mm", fill=(139, 148, 158))
        draw.text((width / 2, height - 20), "Actual SfM output - sampled RGB - subject framing - arbitrary scale", font=foot_font, anchor="mm", fill=(139, 148, 158))
        return frame

    frames = [render(2 * math.pi * index / options.frames) for index in range(options.frames)]
    frames[0].save(options.output / "point-cloud-front.png", optimize=True)
    palette_source = Image.new("RGB", (width * 2, height * 2))
    for index in range(4):
        palette_source.paste(frames[index * options.frames // 4], ((index % 2) * width, (index // 2) * height))
    palette = palette_source.quantize(colors=192)
    indexed = [frame.quantize(palette=palette, dither=Image.Dither.NONE) for frame in frames]
    indexed[0].save(options.output / "point-cloud-rotation.gif", save_all=True, append_images=indexed[1:], duration=80,
                    loop=0, optimize=False, disposal=2)

    results = options.cloud.parent.parent
    with Image.open(results / "point_cloud_comparison.png") as image:
        image.thumbnail((1600, 1200))
        image.save(options.output / "reconstruction-comparison.png", optimize=True)
    with Image.open(results / "image_coverage_comparison.png") as image:
        image.convert("RGB").save(options.output / "image-coverage-comparison.jpg", quality=88, optimize=True)
    metadata = {
        "source": options.cloud.as_posix(), "point_count": len(points), "input_views": 36,
        "framing": "0.2 / 99.8 percentile subject viewport; original points unchanged",
        "animation": {"frames": options.frames, "duration_ms": 80, "loop": 0, "size": [width, height]},
        "color_source": "RGB from enabled observations in original images",
        "note": "Result figures use actual reconstruction data. No generated imagery is used for reconstruction results."
    }
    (options.output / "readme-assets.json").write_text(json.dumps(metadata, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps({name: (options.output / name).stat().st_size for name in names}, indent=2))


if __name__ == "__main__":
    main()
