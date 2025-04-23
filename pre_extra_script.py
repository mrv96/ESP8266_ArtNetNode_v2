import re
import shutil
from pathlib import Path

Import("env")


DATA_DIR = Path(env.subst('$PROJECT_DATA_DIR'))
BUILD_DATA_DIR = Path(env.subst('$BUILD_DIR')) / DATA_DIR.name


def rm_lib_src(libdir:str):
    lib = Path(env.get('PROJECT_LIBDEPS_DIR')) / env.get('PIOENV') / libdir
    piopm = lib / '.piopm'
    library_json = lib / 'library.json'

    if len(tuple(lib.iterdir())) <= 2:
        return

    with piopm.open() as f1, library_json.open() as f2:
        piopm_content = f1.read()
        library_json_content = f2.read()

    shutil.rmtree(str(lib), ignore_errors=True)
    lib.mkdir()

    with piopm.open('w') as f1, library_json.open('w') as f2:
        f1.write(piopm_content)
        f2.write(library_json_content)


def surround_with_ifndef(file:Path, define_name:str):
    indexes = []

    with file.open() as f:
        lines = f.readlines()
        for i, line in enumerate(lines):
            if re.match(r'\s*#define\s+' + define_name, line) and \
               re.match(r'\s*#ifndef\s+' + define_name, lines[i-1]) == None:
                indexes.append(i)

    if not indexes:
        return

    for i in indexes[::-1]:
        lines.insert(i+1, '#endif\n')
        lines.insert(i, f'#ifndef {define_name}\n')

    with file.open('w') as f:
        f.write(''.join(lines))


def minify_web_sources(source, target, env):
    try:
        import minify_html, rcssmin, rjsmin
    except ModuleNotFoundError:
        env.Execute("$PYTHONEXE -m pip install minify-html~=0.16.4 rcssmin~=1.2.1 rjsmin~=1.2.4")
        import minify_html, rcssmin, rjsmin

    BUILD_DATA_DIR.mkdir()

    for p in DATA_DIR.rglob('*.html'):
        new_p = BUILD_DATA_DIR / p.relative_to(DATA_DIR)
        new_p.write_text(
            minify_html.minify(
                p.read_text(),
                allow_noncompliant_unquoted_attribute_values=False,
                allow_optimal_entities=False,
                allow_removing_spaces_between_attributes=False,
                minify_css=True,
                minify_doctype=False,
                minify_js=True,
                remove_bangs=False,
                remove_processing_instructions=False,
            )
        )

    for p in DATA_DIR.rglob('*.css'):
        new_p = BUILD_DATA_DIR / p.relative_to(DATA_DIR)
        new_p.write_text(rcssmin.cssmin(p.read_text()))

    for p in DATA_DIR.rglob('*.js'):
        new_p = BUILD_DATA_DIR / p.relative_to(DATA_DIR)
        new_p.write_text(rjsmin.jsmin(p.read_text()))


# Remove EthernetWebServer unneeded deps to avoid build conflicts
rm_lib_src('Ethernet_Generic')


# Allow MAX_SOCK_NUM GCC define surrounding C MAX_SOCK_NUM defines with #ifndef/endif
platform = env.PioPlatform()
for package in platform.dump_used_packages():
    if 'framework' in package['name']:
        package_dir = Path(platform.get_package_dir(package['name']))
        surround_with_ifndef(package_dir/'libraries'/'Ethernet'/'src'/'Ethernet.h', 'MAX_SOCK_NUM')
        surround_with_ifndef(package_dir/'cores'/'esp8266'/'wl_definitions.h', 'MAX_SOCK_NUM')
        break


env.Replace(PROJECT_DATA_DIR=BUILD_DATA_DIR)
env.AddPreAction(
    str(Path("$BUILD_DIR", "${ESP8266_FS_IMAGE_NAME}.bin")),
    [
        Delete(BUILD_DATA_DIR),
        minify_web_sources,
    ]
)
