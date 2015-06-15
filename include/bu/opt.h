/*                         O P T . H
 * BRL-CAD
 *
 * Copyright (c) 2015 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */

#ifndef BU_OPT_H
#define BU_OPT_H

#include "common.h"
#include "bu/defines.h"
#include "bu/ptbl.h"
#include "bu/vls.h"

__BEGIN_DECLS

/** @addtogroup bu_opt
 * @brief
 * Generalized option handling.
 *
 * This module implements a callback and assignment based mechanism for generalized
 * handling of option handling.  Functionally it is intended to provide capabilities
 * similar to getopt_long, Qt's QCommandLineParser, and the tclap library.  Results
 * are returned by way of variable assignment, and function callbacks are used to
 * convert and validate argument strings.
 *
 * The bu_opt option parsing system does not make any use of global values, unless
 * a user defines an option definiton array that passes in pointers to global variables
 * for setting.
 *
 * To set up a bu_opt parsing system, an array of bu_opt_desc (option description)
 * structures is defined and terminated with a BU_OPT_DESC_NULL entry.  This array
 * is then used by \link bu_opt_parse \endlink to process an argv array.
 *
 * When defining a bu_opt_desc entry, the type of the set_var assignment variable
 * needed is determined by the arg_process callback.  If no callback is present and
 * the max arg count is zero, set_var is expected to be an integer that will be set
 * to 1 if the option is present in the argv string.  Because it is necessary for
 * arbitrary variables to be assigned to the set_var slot (the type needed is determined
 * by the arg_process callback function and may be anything) all set_var variables
 * are identified by their pointers (which are in turn cast to void.)
 *
 * There are two styles in which a bu_opt_desc array may be initialized.  The first
 * is very compact but in C89 based code requires static variables as set_var entries,
 * as seen in the following example:
 *
 * @code
 * #define help_str "Print help and exit"
 * static int ph = 0;
 * static int i = 0;
 * static fastf_t f = 0.0;
 * struct bu_opt_desc opt_defs[] = {
 *     {"h", "help",    "",  NULL,            (void *)&ph, help_str},
 *     {"n", "num",     "#", &bu_opt_int,     (void *)&i,  "Read int"},
 *     {"f", "fastf_t", "#", &bu_opt_fastf_t, (void *)&f,  "Read float"},
 *     BU_OPT_DESC_NULL
 * };
 * @endcode
 *
 * This style of initialization is suitable for application programs, but in libraries
 * such static variables will preclude thread and reentrant safety.  For libraries,
 * the BU_OPT and BU_OPT_NULL macros are used to construct a bu_opt_desc array that
 * does not require static variables:
 *
 * @code
 * #define help_str "Print help and exit"
 * int ph = 0;
 * int i = 0;
 * fastf_t f = 0.0;
 * struct bu_opt_desc opt_defs[4];
 * BU_OPT(opt_defs[0], "h", "help",    "",  NULL,            (void *)&ph, help_str);
 * BU_OPT(opt_defs[1], "n", "num",     "#", &bu_opt_int,     (void *)&i,  "Read int");
 * BU_OPT(opt_defs[2], "f", "fastf_t", "#", &bu_opt_fastf_t, (void *)&f,  "Read float");
 * BU_OPT_NULL(opt_defs[3]);
 * @endcode
 *
 * Given the option description array and argc/argv data, \link bu_opt_parse \endlink will do the
 * rest.  The design of \link bu_opt_parse \endlink is to fail early when an invalid option
 * situation is encountered, so code using this system needs to be ready to handle
 * such cases.
 *
 * For generating descriptive help strings from a bu_opt_desc array use the \link bu_opt_describe \endlink
 * function, which supports multiple output styles and formats.
 */
/** @{ */
/** @file bu/opt.h */

/**
 * Callback function signature for bu_opt_desc argument processing functions. Any
 * user defined argument processing function should match this signature and return
 * values as documented below.
 *
 * \returns
 * Val | Interpretation
 * --- | --------------
 * -1  | Invalid argument encountered, or argument expected but not found.
 *  0  | No argument processed (not an error.)
 * >0  | Number of argv elements used in valid argument processing.
 *
 */
typedef int (*bu_opt_arg_process_t)(struct bu_vls *, int argc, const char **argv, void *);


#define BU_OPT_CHECK_ARGV0(_log_vls, _argc, _argv, _opt_name) \
if (_argc < 1 || !_argv || !_argv[0] || _argv[0][0] == '\0') { \
    if (_log_vls) { \
	bu_vls_printf(_log_vls, "Error: missing required argument: " _opt_name "\n"); \
    } \
    return -1; \
}


/**
 * @brief
 * "Option description" structure.
 *
 * Arrays of this structure are used to define command line options.
 */
struct bu_opt_desc {
    const char *shortopt;             /**< @brief "Short" option (i.e. -h for help option) */
    const char *longopt;              /**< @brief "Long" option (i.e. --help for help option) */
    const char *arg_helpstr;          /**< @brief Documentation describing option argument, if any (i.e. "file" in --input file)*/
    bu_opt_arg_process_t arg_process; /**< @brief Argument processing function pointer */
    void *set_var;                    /**< @brief Pointer to the variable or structure that collects this option's results */
    const char *help_string;          /**< @brief Option description */
};

/** Convenience initializer for NULL bu_opt_desc array terminator */
#define BU_OPT_DESC_NULL {NULL, NULL, NULL, NULL, NULL, NULL}

/** Macro for assigning values to bu_opt_desc array entries. */
#define BU_OPT(_desc, _so, _lo, _ahelp, _aprocess, _var, _help) { \
    _desc.shortopt = _so; \
    _desc.longopt = _lo; \
    _desc.arg_helpstr = _ahelp; \
    _desc.arg_process = _aprocess; \
    _desc.set_var = _var; \
    _desc.help_string = _help; \
}

/** Convenience macro for setting a bu_opt_desc struct to BU_OPT_DESC_NULL */
#define BU_OPT_NULL(_desc) { \
    _desc.shortopt = NULL; \
    _desc.longopt = NULL; \
    _desc.arg_helpstr = NULL; \
    _desc.arg_process = NULL; \
    _desc.set_var = NULL; \
    _desc.help_string = NULL; \
}

/**
 * Parse argv array using option descs.
 *
 * The bu_opt_desc array ds must be null terminated with BU_OPT_DESC_NULL.
 *
 * \returns
 * Val | Interpretation
 * --- | --------------
 * -1  | fatal error in parsing.  Program must decide to recover or exit.
 *  0  | all argv options handled.
 *  >0 | number of unused argv entries returned at the begging of the argv array.
 *
 * msgs will collect any informational messages generated by the parser (typically
 * used for error reporting.)
 *
 * Note: An option definition without an arg_process callback that indicates
 * the option requires >0 arguments in its definition will be treated as a
 * fatal error, should that option be encountered while processing an argv
 * input.
 */
BU_EXPORT extern int bu_opt_parse(struct bu_vls *msgs, int ac, const char **argv, struct bu_opt_desc *ds);


/** Output format options for bu_opt documentation generation */
typedef enum {
    BU_OPT_ASCII,
    BU_OPT_DOCBOOK /* TODO */
} bu_opt_format_t;

/**
 * Construct a textual description of the options defined by
 * the array.
 *
 * The structure is as follows:
 *
 * Offset    Options      Descriptions
 * ******--------------*********************
 *       --test-option This is a test option
 *
 * Opt_col specifies how wide the options column is, and desc_cols
 * specifies how wide the description column is.
 */

/* TODO - support actually using the struct... */
struct bu_opt_desc_opts {
    bu_opt_format_t format;
    int offset;
    int option_columns;
    int description_columns;
};

/**
 *
 * Using the example definition:
 *
 * @code
 * struct bu_opt_desc opt_defs[] = {
 *     {"h", "help",    "",  NULL,            (void *)&ph, "Print help string and exit."},
 *     {"n", "num",     "#", &bu_opt_int,     (void *)&i,  "Read int"},
 *     {"f", "fastf_t", "#", &bu_opt_fastf_t, (void *)&f,  "Read float"},
 *     BU_OPT_DESC_NULL
 * };
 * @endcode
 *
 * bu_opt_describe would generate the following help string by default:
 *
\verbatim
  -h, --help                    Print help string and exit.
  -n #, --num #                 Read int
  -f #, --fastf_t #             Read float
\endverbatim
 *
 * When multiple options use the same set_var to capture
 * their effect, they are considered aliases for documentation
 * purposes.  For example, if we add multiple aliases to the
 * help option and make it more elaborate:
 *
 * @code
 * #define help_str "Print help and exit. If a type is specified to --help, print help specific to that type"
 * struct help_struct hs;
 * struct bu_opt_desc opt_defs[] = {
 *     {"h", "help",    "[type]",  &hfun, (void *)&hs, help_str},
 *     {"H", "HELP",    "[type]",  &hfun, (void *)&hs, help_str},
 *     {"?", "",        "[type]",  &hfun, (void *)&hs, help_str},
 *     {"n", "num",     "#", &bu_opt_int,     (void *)&i,  "Read int"},
 *     {"f", "fastf_t", "#", &bu_opt_fastf_t, (void *)&f,  "Read float"},
 *     BU_OPT_DESC_NULL
 * };
 * @endcode
 *
 * the generated help string reflects this:
 *
\verbatim
  -h [type], -H [type], -? [type], --help [type], --HELP [type]
                                Print help and exit. If a type is specified to
                                --help, print help specific to that type
  -n #, --num #                 Read int
  -f #, --fastf_t #             Read float
\endverbatim
 *
 * \returns
 * The generated help string.  Note that the string uses allocated memory - it is the responsibility of the
 * caller to free it with \link bu_free \endlink.
 */
BU_EXPORT extern const char *bu_opt_describe(struct bu_opt_desc *ds, struct bu_opt_desc_opts *settings);


/** @} */

/** @addtogroup bu_opt_arg_process
 *
 * Standard option validators - if a custom option argument
 * validation isn't needed, the functions below can be
 * used for most valid data types. When data conversion is successful,
 * the user_data pointer in bu_opt_data will point to the results
 * of the string->[type] translation in order to allow a calling
 * program to use the int/long/etc. without having to repeat the
 * conversion.
 */
/** @{ */
/** Process 1 argument to set a boolean type */
BU_EXPORT extern int bu_opt_bool(struct bu_vls *msg, int argc, const char **argv, void *set_var);
/** Process 1 argument to set an integer */
BU_EXPORT extern int bu_opt_int(struct bu_vls *msg, int argc, const char **argv, void *set_var);
/** Process 1 argument to set a long */
BU_EXPORT extern int bu_opt_long(struct bu_vls *msg, int argc, const char **argv, void *set_var);
/** Process 1 argument to set a \link fastf_t \endlink (either a float or a double, depending on how BRL-CAD was compiled) */
BU_EXPORT extern int bu_opt_fastf_t(struct bu_vls *msg, int argc, const char **argv, void *set_var);
/** Process 1 argument to set a char pointer (uses the original argv string, does not make a copy) */
BU_EXPORT extern int bu_opt_str(struct bu_vls *msg, int argc, const char **argv, void *set_var);
/** Process 1 argument to append to a vls (places a space before the new entry if the target vls is not empty) */
BU_EXPORT extern int bu_opt_vls(struct bu_vls *msg, int argc, const char **argv, void *set_var);
/** @} */


__END_DECLS

#endif  /* BU_OPT_H */

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
