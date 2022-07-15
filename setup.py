
from distutils.core import setup
from distutils.extension import Extension
from Cython.Distutils import build_ext
try:
    import numpy as np
    include_dirs = [np.get_include()]
except ImportError:
    include_dirs = []

setup(
    name='picual',
    description='Library for efficient pickling.',
    version='1.0.0',
    url='https://github.com/shaldokin/cotta',
    author='Shaldokin',
    author_email='shaldokin@protonmail.com',
    python_requires='>=3.0',
    install_requires=['Cython'],
    classifiers=[
        'Programming Language :: Cython',
        'Programming Language :: Python :: 3 :: Only',
        'License :: OSI Approved :: MIT License',
    ],
    ext_modules=[
        Extension('picual', sources=['picual/picual.pyx'], language='c++')
    ],
    cmdclass={'build_ext': build_ext},
    include_dirs=include_dirs,
)

