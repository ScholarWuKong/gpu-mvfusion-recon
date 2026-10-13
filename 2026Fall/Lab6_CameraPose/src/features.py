"""Reusable image feature extraction functions."""

from collections.abc import Mapping
from typing import Any

import cv2


def extract_sift(img, cfg: Mapping[str, Any] | None = None):
    """Extract SIFT keypoints and descriptors from an image.

    Args:
        img: Input image as a NumPy array, usually loaded by cv2.imread.
        cfg: Optional dictionary of keyword arguments for cv2.SIFT_create.

    Returns:
        (keypoints, descriptors):
            keypoints is a list of OpenCV KeyPoint objects.
            descriptors is an N x 128 float32 matrix, or None when
            no descriptors are produced.
    """
    if img is None:
        raise ValueError("img 不能为空，请先检查图像是否读取成功")

    params = {} if cfg is None else dict(cfg)
    sift = cv2.SIFT_create(**params)
    keypoints, descriptors = sift.detectAndCompute(img, None)

    return keypoints, descriptors