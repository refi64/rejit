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
- `Headerdoc <https://web.archive.org/web/20160217120658/https://developer.apple.com/library/mac/documentation/DeveloperTools/Conceptual/HeaderDoc/intro/intro.html>`_
  if you want to build the documentation. (Note that Headerdoc is no longer actually
  maintained, so this is a link to the page on archive.org). The download is
  available `here <https://opensource.apple.com/tarballs/headerdoc/headerdoc-8.9.26.tar.gz>`_.

Building
********

Just run::

   $ fbuild

To build the documentation, run::

   $ fbuild docs

Usage
*****

Check out the `demo example
<https://github.com/kirbyfan64/rejit/blob/master/ex.c>`_ and the `API docs
<http://kirbyfan64.github.io/rejit/>`_.
