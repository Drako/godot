def can_build(env, platform):
    return True

def configure(env):
    env.add_module_version_string("gothic")

def get_doc_classes():
    return [
        "VdfFile",
    ]

def get_doc_path():
    return "doc_classes"

def is_enabled():
    # The module is disabled by default. Use module_gothic_enabled=yes to enable it.
    return False

