"""IR2Vec Python Bindings"""
from .py_mir2vec import *

try:
    from importlib.metadata import version
    __version__ = version("py-mir2vec")
except ImportError:
    __version__ = "Unknown"