project('cdcc', 'c')

compiler = meson.get_compiler('c')

glib   = dependency('glib-2.0')
gio    = dependency('gio-2.0')
sqlite = dependency('sqlite3')
json   = dependency('json-glib-1.0')

alldep = [glib, gio, sqlite, json]

c_flags   = ['-Wall', '-Wunreachable-code']
ld_flags  = []

executable('cdcc-cc',
	   sources: [ 'cc.c', 'db.c' ],
	   cpp_args: c_flags,
	   link_args: ld_flags,
	   dependencies: [glib, gio, sqlite],
	   install: true)


executable('cdcc-gen',
	   sources: [ 'gen.c', 'db.c' ],
	   cpp_args: c_flags,
	   link_args: ld_flags,
	   dependencies: alldep,
	   install: true)


executable('cdcc-query',
	   sources: [ 'query.c', 'db.c' ],
	   cpp_args: c_flags,
	   link_args: ld_flags,
	   dependencies: alldep,
	   install: true)

