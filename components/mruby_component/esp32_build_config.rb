MRuby::Build.new do |conf|
  toolchain :gcc

  [conf.cc, conf.objc, conf.asm].each do |cc|
    cc.command = 'gcc'
    cc.flags = [%w(-g -std=gnu99 -O3 -Wall -Werror-implicit-function-declaration -Wdeclaration-after-statement -Wwrite-strings)]
  end

  [conf.cxx].each do |cxx|
    cxx.command = 'g++'
    cxx.flags = [%w(-g -O3 -Wall -Werror-implicit-function-declaration)]
  end

  conf.linker do |linker|
    linker.command = 'gcc'
    linker.flags = [%w()]
    linker.libraries = %w(m)
    linker.library_paths = []
  end

  conf.archiver do |archiver|
    archiver.command = "ar"
  end

  conf.gembox 'default'
end

MRuby::CrossBuild.new('esp32') do |conf|
  toolchain :gcc

  conf.cc do |cc|
    cc.include_paths << ENV["COMPONENT_INCLUDES"].split(' ')

    cc.flags << '-Wno-maybe-uninitialized'
    cc.flags.collect! { |x| x.gsub('-MP', '') }

    cc.defines << %w(MRB_HEAP_PAGE_SIZE=64)
    cc.defines << %w(MRB_USE_IV_SEGLIST)
    cc.defines << %w(KHASH_DEFAULT_SIZE=8)
    cc.defines << %w(MRB_STR_BUF_MIN_SIZE=20)
    cc.defines << %w(MRB_GC_STRESS)

    cc.defines << %w(ESP_PLATFORM)
  end

  conf.cxx do |cxx|
    cxx.include_paths = conf.cc.include_paths.dup

    cxx.flags.collect! { |x| x.gsub('-MP', '') }

    cxx.defines = conf.cc.defines.dup
  end

  conf.bins = []
  conf.build_mrbtest_lib_only
  conf.disable_cxx_exception

  conf.gem :core => "mruby-print"
  conf.gem :core => "mruby-compiler"
  conf.gem :github => "mruby-esp32/mruby-esp32-system"
  conf.gem :github => "mruby-esp32/mruby-esp32-wifi"
  conf.gem :core => "mruby-numeric-ext"
#  conf.gem :core => "mruby-kernel-ext"
#  conf.gem :core => "mruby-math"
#  conf.gem :core => "mruby-random"
#  conf.gem :core => "mruby-proc-ext"
#  conf.gem :core => "mruby-string-ext"
#  conf.gem :core => "mruby-array-ext"
#  conf.gem :core => "mruby-hash-ext"

  #conf.gem :core => "mruby-time"
  #conf.gem :core => "mruby-bin-mirb"

  #mruby-struct - standard Struct class
  #mruby-compar-ext - Enumerable module extension
  #mruby-enum-ext - Enumerable module extension
  #mruby-numeric-ext - Numeric class extension
  #mruby-range-ext - Range class extension
  #mruby-symbol-ext - Symbol class extension
  #mruby-object-ext - Object class extension
  #mruby-objectspace - ObjectSpace class
  #mruby-fiber - Fiber class
  #mruby-enumerator - Enumerator class
  #mruby-enum-lazy - Enumerator::Lazy class
  #mruby-toplevel-ext - toplevel object (main) methods extension
  #mruby-compiler - mruby compiler library
  #  - Binaries: mirb
  #mruby-error - extensional error handling
  #mruby-bin-mruby - mruby command
  #  - Binaries: mruby
  #mruby-bin-strip - irep dump debug section remover command
  #  - Binaries: mruby-strip
  #mruby-class-ext - class/module extension
  #mruby-bin-mrbc - mruby compiler executable
end
