import sys
import platform
from importlib import import_module


def main():
    print("操作系统：", platform.platform())
    print("Python 版本：", platform.python_version())
    print("Python 路径：", sys.executable)
    print("是否使用虚拟环境：", sys.prefix != sys.base_prefix)

    failed = False
    for module_name in ("cv2", "numpy", "matplotlib"):
        try:
            module = import_module(module_name)
            print(f"{module_name} 版本：{module.__version__}")
        except Exception as error:
            print(f"{module_name} 导入失败：{type(error).__name__}: {error}")
            failed = True

    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
