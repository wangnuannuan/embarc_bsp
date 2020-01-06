#!/usr/bin/env python
import os
import argparse
from argparse import Namespace
import json
from embarc_tools.utils import getcwd, uniqify
from embarc_tools.osp import osp
from embarc_tools.exporter import Exporter
import yaml


class ConfigurationError(Exception):
    def __init__(self, cfile, message):
        self.cfile = cfile
        self.message = message

    def __str__(self):
        return repr(self.cfile + ": " + self.message)


testcase_valid_keys = {"tags": {"type": "set", "required": False},
                       "type": {"type": "str", "default": "integration"},
                       "extra_args": {"type": "list"},
                       "extra_configs": {"type": "list"},
                       "build_only": {"type": "bool", "default": False},
                       "build_on_all": {"type": "bool", "default": False},
                       "skip": {"type": "bool", "default": False},
                       "slow": {"type": "bool", "default": False},
                       "timeout": {"type": "int", "default": 60},
                       "min_ram": {"type": "int", "default": 8},
                       "depends_on": {"type": "set"},
                       "min_flash": {"type": "int", "default": 32},
                       "extra_sections": {"type": "list", "default": []},
                       "platform_exclude": {"type": "set"},
                       "platform_whitelist": {"type": "set"},
                       "toolchain_exclude": {"type": "set"},
                       "toolchain_whitelist": {"type": "set"},
                       "filter": {"type": "str"},
                       "harness": {"type": "str"},
                       "harness_config": {"type": "map", "default": {}}
                       }


def yaml_load(filename):
    """
    Safely load a YAML document

    Follows recomendations from
    https://security.openstack.org/guidelines/dg_avoid-dangerous-input-parsing-libraries.html.

    :param str filename: filename to load
    :raises yaml.scanner: On YAML scan issues
    :raises: any other exception on file access erors
    :return: dictionary representing the YAML document
    """
    try:
        with open(filename, 'r') as f:
            return yaml.safe_load(f)
    except yaml.scanner.ScannerError as e:
        mark = e.problem_mark
        cmark = e.context_mark
        print("%s:%d:%d: error: %s (note %s context @%s:%d:%d %s)",
              mark.name, mark.line, mark.column, e.problem,
              e.note, cmark.name, cmark.line, cmark.column, e.context)
        raise


# If pykwalify is installed, then the validate functionw ill work --
# otherwise, it is a stub and we'd warn about it.
try:
    import pykwalify.core
    # Don't print error messages yourself, let us do it
    # logging.getLogger("pykwalify.core").setLevel(50)

    def _yaml_validate(data, schema):
        if not schema:
            return
        c = pykwalify.core.Core(source_data=data, schema_data=schema)
        c.validate(raise_exception=True)

except ImportError as e:
    print("can't import pykwalify; won't validate YAML (%s)", e)

    def _yaml_validate(data, schema):
        pass


def yaml_load_verify(filename, schema):
    """
    Safely load a testcase/sample yaml document and validate it
    against the YAML schema, returing in case of success the YAML data.

    :param str filename: name of the file to load and process
    :param dict schema: loaded YAML schema (can load with :func:`yaml_load`)

    # 'document.yaml' contains a single YAML document.
    :raises yaml.scanner.ScannerError: on YAML parsing error
    :raises pykwalify.errors.SchemaError: on Schema violation error
    """
    # 'document.yaml' contains a single YAML document.
    y = yaml_load(filename)
    _yaml_validate(y, schema)
    return y


def parse_arguments():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument(
        "--config", required=True, help="specify application configuration, default is to look for embarc_app.json") 
    parser.add_argument(
        "--embarc-root",
        required=True, help="Specify embARC_osp directory")
    parser.add_argument(
        "--test-suit", default=str(), help="Specify test suit", metavar='')
    parser.add_argument(
        "--test-case",
        action='append',
        default=[],
        help="Directories to search for test cases. (e.g., tests/driver)", metavar='')
    parser.add_argument(
        '--appl-defines', action='store', default=[
            '-DEMBARC_TEST_SUITE=\\"$(TEST_SUITE)\\"', '-DEMBARC_UNIT_TEST'],
        help='application defines', metavar='')

    return parser.parse_args()


def upper_dict_key(d):
    new_dict = dict((k.upper(), v) for k, v in d.items())
    return new_dict


def line_prepender(filename, line):
    with open(filename, 'r+') as f:
        content = f.read()
        f.seek(0, 0)
        f.write(line.rstrip('\r\n') + '\n' + content)


def nt_posix_path(path):
    path = os.path.abspath(path)
    if os.path.exists(path):
        return path.replace("\\", "/")
    else:
        print("{} doesn't exists".format(path))


def get_content(filename, start, end):
    with open(filename, 'r+') as f:
        content = f.readlines()
        content_slice = content[start:end]
        return "".join(content_slice)


class ConfigParser:

    def __init__(self, filename, schema):
        self.data = yaml_load_verify(filename, schema)
        self.filename = filename
        self.tests = dict()
        if "tests" in self.data:
            self.tests = self.data["tests"]

    def _cast_value(self, value, typestr):
        if isinstance(value, str):
            v = value.strip()
        if typestr == "str":
            return v

        elif typestr == "float":
            return float(value)

        elif typestr == "int":
            return int(value)

        elif typestr == "bool":
            return value

        elif typestr.startswith("list") and isinstance(value, list):
            return value
        elif typestr.startswith("list") and isinstance(value, str):
            vs = v.split()
            if len(typestr) > 4 and typestr[4] == ":":
                return [self._cast_value(vsi, typestr[5:]) for vsi in vs]
            else:
                return vs

        elif typestr.startswith("set"):
            vs = v.split()
            if len(typestr) > 3 and typestr[3] == ":":
                return set([self._cast_value(vsi, typestr[4:]) for vsi in vs])
            else:
                return set(vs)

        elif typestr.startswith("map"):
            return value
        else:
            raise ConfigurationError(
                self.filename, "unknown type '%s'" % value)

    def get_test(self, name, valid_keys):
        d = {}
        # for k, v in self.common.items():
        #     d[k] = v

        for k, v in self.tests[name].items():
            if k not in valid_keys:
                raise ConfigurationError(
                    self.filename,
                    "Unknown config key '%s' in definition for '%s'" %
                    (k, name))

            if k in d:
                if isinstance(d[k], str):
                    d[k] += " " + v
            else:
                d[k] = v

        for k, kinfo in valid_keys.items():
            if k not in d:
                if "required" in kinfo:
                    required = kinfo["required"]
                else:
                    required = False

                if required:
                    raise ConfigurationError(
                        self.filename,
                        "missing required value for '%s' in test '%s'" %
                        (k, name))
                else:
                    if "default" in kinfo:
                        default = kinfo["default"]
                    else:
                        default = self._cast_value("", kinfo["type"])
                    d[k] = default
            else:
                try:
                    d[k] = self._cast_value(d[k], kinfo["type"])
                except ValueError:
                    raise ConfigurationError(
                        self.filename,
                        "bad %s value '%s' for key '%s' in name '%s'" %
                        (kinfo["type"], d[k], k, name))

        return d


class MakeGenerator:

    C_SOURCE = """
#include "embARC_test.h"

#ifndef EMBARC_TEST_SUITE
#define EMBARC_TEST_SUITE NULL
#endif

int main(void) {
    /* call unit tests */
    unit_test_run(EMBARC_TEST_SUITE);

    EMBARC_PRINTF("----embARC unit test end----");
    return E_SYS;
}
"""

    def __init__(self, test_root):
        self.root = test_root
        if not os.path.exists(test_root):
            os.mkdir(test_root)
        self.makefile = os.path.join(test_root, "Makefile")
        self.main = os.path.join(test_root, "main.c")

    def generate(self, options, testcase_roots):
        options.APPL = "unit_test"
        options_u = upper_dict_key(vars(options))
        options_u["EXTRA_CSRCDIR"] = " ".join(testcase_roots)
        exporter = Exporter("application")
        exporter.gen_file_jinja(
            "makefile.tmpl",
            options_u,
            "Makefile",
            self.root)
        exporter.gen_file_jinja(
            "main.c.tmpl", options_u, "main.c", self.root
            )

        line_prepender(
            self.makefile,
            "TEST_SUITE ?={suit}".format(
                suit=options.test_suit))

        pre_content = get_content(self.main, 0, -9)

        with open(self.main, 'r+') as f:
            f.seek(0, 0)
            f.write(pre_content + self.C_SOURCE)


class TestSuite:
    def __init__(self, board, testcase_roots):
        self.testcases = list()
        self.platforms = None
        self.discards = []
        self.yaml_tc_schema = yaml_load(
            os.path.join(options.embarc_root,
                         "tests", "tc-schema.yaml"))
        for testcase_root in testcase_roots:
            print("Reading test case configuration files under %s..." %
                  testcase_root)
            for dirpath, _, filenames in os.walk(testcase_root,
                                                 topdown=True):
                for file in filenames:
                    if file.startswith("test_") \
                        and os.path.splitext(file)[-1] == ".c":
                        self.testcases.append(
                                nt_posix_path(dirpath))
                        break
        osp_class = osp.OSP()
        # if board in osp_class.supported_boards(options.embarc_root):
        #     self.platforms = board

    def apply_filters(self):
        platform_filter = options.board
        print("platform filter: " + str(platform_filter))
        discards = []
        for dirpath in self.testcases:
            if "testcase.yaml" in os.listdir(dirpath):
                filename = 'testcase.yaml'
                yaml_path = os.path.join(dirpath, filename)
                parsed_data = ConfigParser(
                                yaml_path, self.yaml_tc_schema)
                for name in parsed_data.tests.keys():
                    tc_dict = parsed_data.get_test(name, testcase_valid_keys)
                    if "platform_exclude" in tc_dict.keys():
                        if platform_filter in tc_dict["platform_exclude"]:
                            discards.append(nt_posix_path(dirpath))
        self.discards = discards
        return discards

    def execute(self):
        mg = MakeGenerator(options.outdir)
        testcase_roots = [i for i in self.testcases if i not in self.discards]
        mg.generate(options, testcase_roots)


def main():
    global options
    options = parse_arguments()
    options_dict = vars(options)
    app_config = None
    config_path = options.config
    if os.path.exists(config_path):
        with open(config_path, "r") as f:
            app_config = json.load(f)
    options_dict.update(app_config)
    options = Namespace(**options_dict)
    
    if not options.test_case:
        options.test_case = ["tests"]
    options.test_case = [
        os.path.abspath(options.embarc_tools)
        + "/" + i for i in options.test_case]

    if options.appl_defines:
        options.appl_defines = " ".join(
            options.appl_defines
        )
    ts = TestSuite(options.board, options.test_case)
    ts.apply_filters()
    ts.execute()


if __name__ == "__main__":
    main()
