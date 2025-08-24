"""IR2Vec Python Bindings"""
from .py_ir2vec import *

try:
    from importlib.metadata import version
    __version__ = version("py-ir2vec")
except ImportError:
    __version__ = "Unknown"