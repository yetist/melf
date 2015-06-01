/* vi: set sw=4 ts=4 wrap ai: */
/*
 * scn-dump.h: This file is part of ____
 *
 * Copyright (C) 2015 yetist <yetist@yetibook>
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

#ifndef __SCN_DUMP_H__ 
#define __SCN_DUMP_H__  1

#include <gelf.h>

Elf_Scn* find_section(Elf *elf, const char* scname);
int  print_section (Elf *elf, Elf_Scn *scn);
void print_header  (Elf *elf);
Elf_Scn* add_section(Elf *elf, char* section);
void create_section(Elf *elf);

#endif /* __SCN_DUMP_H__ */
