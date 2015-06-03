/* vi: set sw=4 ts=4 wrap ai: */
/*
 * o.c: This file is part of ____
 *
 * Copyright (C) 2015 yetist <xiaotian.wu@i-soft.com.cn>
 *
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * */

#include <stdlib.h>
#include <error.h>
#include <argp.h>
#include <string.h>

#include "elf-print.h"

const char *argp_program_version = "argp-ex4 1.0";
const char *argp_program_bug_address = "<xiaotian.wu@i-soft.com.cn>";

static char doc[] =
"\nShow or change ELF file section data.\n\n"
"COMMAND:\n"
"  print  Print the section content of ELF file\n"
"  set    Set the section content of ELF file\n"
"\nExample:\n"
"  print --section .strtab FILE\n"
"  set --section .strtab --file file FILE\n"
"  set --section .strtab --string file FILE\n"
"\nOPTIONS:";

static char args_doc[] = "<COMMAND> [FILE...]";

static struct argp_option options[] = {
	{"verbose",  'v', 0,           0, "Produce verbose output" },
	{"file",     'f', "FILENAME",  0, "Change the section content of executable given to file." },
	{"string",   't', "STRING",    0, "Change the section content of executable given to string." },
	{"section",  's', ".shstrtab", 0, "Which section can be show or set. Default is \".shstrtab\"." },
	{ 0 }
};

struct arguments
{
	char *command;
	char *section;
	char *file;
	char *string;
	char **elf_files;
	int verbose;
};

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
	struct arguments *arguments = state->input;

	switch (key)
	{
		case 'v':
			arguments->verbose = 1;
			break;
		case 'f':
			arguments->file = arg;
			break;
		case 't':
			arguments->string = arg;
			break;
		case 's':
			arguments->section = arg;
			break;
		case ARGP_KEY_NO_ARGS:
			argp_usage (state);

		case ARGP_KEY_ARG:
			if (!(strcmp(arg, "print") == 0 || strcmp(arg, "set") == 0)) {
				argp_usage (state);
			}
			arguments->command = arg;
			if (strcmp(arguments->command, "set") == 0) {
				if ((arguments->file != NULL && arguments->string != NULL) ||
						(arguments->file == NULL && arguments->string == NULL)) {
					argp_usage (state);
				}
			} else {
				if (arguments->file != NULL || arguments->string != NULL) {
					argp_usage (state);
				}
			}
			arguments->elf_files = &state->argv[state->next];
			state->next = state->argc;
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

int main (int argc, char **argv)
{
	int i, j;
	struct arguments arguments;
	struct argp argp = { options, parse_opt, args_doc, doc };

	arguments.verbose = 0;
	arguments.file = NULL;
	arguments.string = NULL;
	arguments.section = ".shstrtab";

	argp_parse (&argp, argc, argv, 0, 0, &arguments);

	if (strcmp(arguments.command, "print") == 0) {
		for (i = 0; arguments.elf_files[i]; i++) {
			show_section(arguments.elf_files[i], arguments.section);
		}
	} else {
		if (arguments.file != NULL) {
			for (i = 0; arguments.elf_files[i]; i++) {
				set_section_file(arguments.elf_files[i], arguments.section, arguments.file);
			}
		}
		if (arguments.string != NULL) {
			for (i = 0; arguments.elf_files[i]; i++) {
				set_section_string(arguments.elf_files[i], arguments.section, arguments.string);
			}
		}
	}
	return 0;
}
