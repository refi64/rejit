ReJit
=====

.. image:: https://travis-ci.org/kirbyfan64/rejit.svg?branch=master
    :target: https://travis-ci.org/kirbyfan64/rejit

ReJit is an open-source, JIT-powered regex engine that currently supports most
regular expression features (and even some that RE2 doesn't support, like
backreferences). The only supported architectures at the moment are X86 and X64.

Note that ReJit is NOT complete! See the `issue tracker
<https://github.com/kirbyfan64/rejit/issues>`_ for a list of open issues.

Also, ReJit does not work on Windows at the moment (or at least X64; X86 might
work). Ideally, it would be a simple fix to use the shadow stack instead of the red
zone and changing the call registers...but I'm not sure.

Requirements
************

- `Fbuild <https://github.com/felix-lang/fbuild>`_ (use the Git version, not the
  release).
- Python 3.
- Lua.
- `Libcut <https://github.com/kirbyfan64/libcut>`_ if you want to build the tests.

Building
********

Just run::
   
   $ fbuild

Usage
*****

Right now, there aren't any docs, but you can check out the `demo script
<https://github.com/kirbyfan64/rejit/blob/master/ex.c>`_ and the `test suite
<https://github.com/kirbyfan64/rejit/blob/master/tst.c>`_ for a bunch of examples.
