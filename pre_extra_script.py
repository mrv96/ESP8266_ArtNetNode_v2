from pathlib import Path

Import("env")


DATA_DIR = Path(env.subst('$PROJECT_DATA_DIR'))
BUILD_DATA_DIR = Path(env.subst('$BUILD_DIR')) / DATA_DIR.name


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


env.Replace(PROJECT_DATA_DIR=BUILD_DATA_DIR)
env.AddPreAction(
    str(Path("$BUILD_DIR", "${ESP8266_FS_IMAGE_NAME}.bin")),
    [
        Delete(BUILD_DATA_DIR),
        minify_web_sources,
    ]
)
