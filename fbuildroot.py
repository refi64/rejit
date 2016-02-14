# Any copyright is dedicated to the Public Domain.
# http://creativecommons.org/publicdomain/zero/1.0/

from fbuild.builders.c import guess_static
from fbuild.builders import find_program
from fbuild.config import c as c_test
from fbuild.record import Record
from fbuild.path import Path
import fbuild.db

from optparse import make_option
import re

def pre_options(parser):
    group = parser.add_option_group('config options')
    group.add_options((
        make_option('--buildtype', help='The build type',
                    choices=['debug', 'release'], default='debug'),
        make_option('--lua', help='Use the given Lua executable'),
        make_option('--cc', help='Use the given C compiler'),
        make_option('--cflag', help='Pass the given flag to the C++ compiler',
                    action='append', default=[]),
        make_option('--use-color', help='Use colored output',
                    action='store_true'),
        make_option('--release', help='Build in release mode',
                    action='store_true', default=False),
    ))

class Libcut_h(c_test.Test):
    libcut_h = c_test.header_test('libcut.h')

class DynAsmBuilder(fbuild.db.PersistentObject):
    def __init__(self, ctx, *, exe=None, defs=[]):
        self.ctx = ctx
        self.exe = find_program(ctx, exe or ['lua', 'luajit'])
        self.defs = defs

    @fbuild.db.cachemethod
    def translate(self, src: fbuild.db.SRC, dst) -> fbuild.db.DST:
        dst = Path.addroot(dst, self.ctx.buildroot)
        dst.parent.makedirs()
        cmd = [self.exe, 'dynasm/dynasm.lua']
        for d in self.defs:
            cmd.extend(('-D', d))
        cmd.extend(('-o', dst))
        cmd.append(src)
        self.ctx.execute(cmd, 'dynasm', '%s -> %s' % (src, dst), color='yellow')
        return dst

@fbuild.db.caches
def get_target_arch(ctx, c):
    ctx.logger.check('determining target architecture')
    cmd = [c.compiler.cc.exe, '-dumpmachine']
    cmd.extend(c.compiler.flags)
    triple = ctx.execute(cmd, quieter=1)[0].decode('ascii')
    arch = triple.split('-')[0]
    ctx.logger.passed('ok ' + arch)
    return arch

@fbuild.db.caches
def configure(ctx):
    flags = ['-Wall', '-Werror']+ctx.options.cflag
    testflags = []
    defs = []
    kw = {}
    if ctx.options.use_color:
        flags.append('-fdiagnostics-color')

    if ctx.options.release:
        kw['optimize'] = True
        kw['macros'] = ['NDEBUG']
    else:
        kw['debug'] = True
        kw['macros'] = ['DEBUG']

    c = guess_static(ctx, exe=ctx.options.cc, flags=flags, includes=['utf'],
        platform_options=[
            ({'posix'}, {'external_libs+': ['rt']}),
            ({'gcc'}, {'flags+': ['-Wno-maybe-uninitialized']}),
            ({'clang'}, {'flags+': ['-Wno-unknown-warning-option']}),
        ],
        **kw)
    arch = get_target_arch(ctx, c)
    if arch == 'x86_64':
        defs.append('X64')
    elif re.match('i\d86', arch):
        # X86 is implemented in the x86_64.dasc file.
        arch = 'x86_64'
    else:
        raise fbuild.ConfigFailed('unsupported target architecture ' + arch)

    dasm = DynAsmBuilder(ctx, exe=ctx.options.lua, defs=defs)

    if Libcut_h(c).libcut_h:
        ctx.logger.passed('found libcut.h; will build tests')
        tests = True
        testflags.append('-std=gnu11')
    else:
        ctx.logger.failed('cannot find libcut.h; will not build tests')
        tests = False
    return Record(dasm=dasm, c=c, arch=arch, tests=tests, testflags=testflags)

def build(ctx):
    rec = configure(ctx)
    c = rec.c
    dasm = rec.dasm
    src = dasm.translate('src/x86_64.dasc', 'codegen.c')
    rejit = c.build_lib('rejit', Path.glob('src/*.c') + [Path('utf/utf.c')],
        includes=['.', ctx.buildroot])
    c.build_exe('bench', ['bench.c'], includes=['src'], libs=[rejit])
    c.build_exe('ex', ['ex.c'], includes=['src'], libs=[rejit])
    if rec.tests:
        c.build_exe('tst', ['tst.c'], includes=['src'], cflags=rec.testflags,
            libs=[rejit])
