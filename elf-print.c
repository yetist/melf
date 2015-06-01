/* vi: set sw=4 ts=4 wrap ai: */
/*
 * scn-dump.c: This file is part of ____
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

#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <gelf.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <inttypes.h>
#include <unistd.h>
#include <libebl.h>
#include "elf-print.h"

static void print_bytes (Elf_Data *data)
{
	size_t size = data->d_size;
	off_t offset = data->d_off;
	unsigned char *buf = (unsigned char *) data->d_buf;
	size_t cnt;

	if (buf[size] == '\0') {
		printf("data[str] = %s\n", buf);
	}
	for (cnt = 0; cnt < size; cnt += 16)
	{
		size_t inner;

		printf ("%*Zx: ", sizeof (size_t) == 4 ? 8 : 16, (size_t) offset + cnt);

		for (inner = 0; inner < 16 && cnt + inner < size; ++inner)
			printf (" %02hhx", buf[cnt + inner]);

		puts ("");
	}
}


static void print_symtab (Elf *elf, Elf_Data *data)
{
	int class = gelf_getclass (elf);
	size_t nsym = data->d_size / (class == ELFCLASS32
			? sizeof (Elf32_Sym) : sizeof (Elf64_Sym));
	size_t cnt;

	for (cnt = 0; cnt < nsym; ++cnt)
	{
		GElf_Sym sym_mem;
		GElf_Sym *sym = gelf_getsym (data, cnt, &sym_mem);

		printf ("%5Zu: %*" PRIx64 " %6" PRIx64 " %4d\n",
				cnt,
				class == ELFCLASS32 ? 8 : 16,
				sym->st_value,
				sym->st_size,
				GELF_ST_TYPE (sym->st_info));
	}
}

Elf_Scn* find_section(Elf *elf, const char* scname)
{
	size_t strndx;
	GElf_Ehdr ehdr;
	Elf_Scn *scn = NULL;

	if (gelf_getehdr (elf, &ehdr) == NULL)
	{
		printf ("cannot get the ELF header: %s\n", elf_errmsg (-1));
		return NULL;
	}
	strndx = ehdr.e_shstrndx;

	while ((scn = elf_nextscn (elf, scn)) != NULL)
	{
		char *name = NULL;
		GElf_Shdr shdr;

		if (gelf_getshdr (scn, &shdr) != NULL) {
			name = elf_strptr (elf, strndx, (size_t) shdr.sh_name);
			/* XXX: should free name? */
			if (strcmp(name, scname) == 0) {
				goto found;
			}
		}
	}
	/* XXX: should free scn? */
	return NULL;
found:
	return scn;
}

int print_section (Elf *elf, Elf_Scn *scn)
{
	GElf_Ehdr *ehdr;
	GElf_Ehdr ehdr_mem;
	GElf_Shdr *shdr;
	GElf_Shdr shdr_mem;
	Elf_Data *data;

	/* First get the ELF and section header.  */
	ehdr = gelf_getehdr (elf, &ehdr_mem);
	shdr = gelf_getshdr (scn, &shdr_mem);
	if (ehdr == NULL || shdr == NULL)
		return 1;

	/* Print the information from the ELF section header.   */
	printf ("name      = %s\n"
			"type      = %" PRId32 "\n"
			"flags     = %" PRIx64 "\n"
			"addr      = %" PRIx64 "\n"
			"offset    = %" PRIx64 "\n"
			"size      = %" PRId64 "\n"
			"link      = %" PRId32 "\n"
			"info      = %" PRIx32 "\n"
			"addralign = %" PRIx64 "\n"
			"entsize   = %" PRId64 "\n",
			elf_strptr (elf, ehdr->e_shstrndx, shdr->sh_name),
			shdr->sh_type,
			shdr->sh_flags,
			shdr->sh_addr,
			shdr->sh_offset,
			shdr->sh_size,
			shdr->sh_link,
			shdr->sh_info,
			shdr->sh_addralign,
			shdr->sh_entsize);

	/* Get the section data now.  */
	data = elf_getdata (scn, NULL);
	if (data == NULL)
		return 1;

	/* Now proces the different section types accordingly.  */
	switch (shdr->sh_type)
	{
		case SHT_SYMTAB:
			print_symtab (elf, data);
			break;

		case SHT_PROGBITS:
		default:
			print_bytes (data);
			break;
	}

	/* Separate form the next section.  */
	puts ("");

	/* All done correctly.  */
	return 0;
}

void print_header (Elf *elf)
{
	int i;
	GElf_Ehdr ehdr;

	if (gelf_getehdr (elf, &ehdr) == NULL) {
		printf ("cannot get the ELF header: %s\n", elf_errmsg (-1));
		return;
	}

	/* Print the ELF header values.  */
	printf("ELF Header\nMagic: ");
	for (i = 0; i < EI_NIDENT; ++i)
		printf (" %02x", ehdr.e_ident[i]);
	printf ("\n");
	printf("type = %hu\nmachine = %hu\nversion = %u\nentry = %u\nphoff = %u\n"
			"shoff = %u\nflags = %u\nehsize = %hu\nphentsize = %hu\n"
			"phnum = %hu\nshentsize = %hu\nshnum = %hu\nshstrndx = %hu\n\n",
			ehdr.e_type,
			ehdr.e_machine,
			ehdr.e_version,
			ehdr.e_entry,
			ehdr.e_phoff,
			ehdr.e_shoff,
			ehdr.e_flags,
			ehdr.e_ehsize,
			ehdr.e_phentsize,
			ehdr.e_phnum,
			ehdr.e_shentsize,
			ehdr.e_shnum,
			ehdr.e_shstrndx);
}

void create_section(Elf *elf)
{
	struct Ebl_Strtab *shst;
	struct Ebl_Strent *shstrtabse;
	Elf_Scn *scn;
	Elf_Data *data;

	GElf_Ehdr *ehdr;
	GElf_Ehdr ehdr_mem;

	GElf_Shdr *shdr;
	GElf_Shdr shdr_mem;

	shst = ebl_strtabinit (true);

	scn = elf_newscn (elf);
	if (scn == NULL)
	{
		printf ("cannot create SHSTRTAB section: %s\n", elf_errmsg (-1));
		return;
	}
	shdr = gelf_getshdr (scn, &shdr_mem);
	if (shdr == NULL)
	{
		printf ("cannot get header for SHSTRTAB section: %s\n", elf_errmsg (-1));
		return;
	}

	shstrtabse = ebl_strtabadd (shst, ".first", 0);

	shdr->sh_type = SHT_PROGBITS;
	shdr->sh_flags = SHF_ALLOC | SHF_EXECINSTR;
	shdr->sh_addr = 0;
	shdr->sh_link = 0;
	shdr->sh_info = 0;
	shdr->sh_entsize = 1;

	data = elf_newdata (scn);
	if (data == NULL)
	{
		printf ("cannot create data first section: %s\n", elf_errmsg (-1));
		return;
	}

	data->d_buf = "hello";
	data->d_type = ELF_T_BYTE;
	data->d_version = EV_CURRENT;
	data->d_size = 5;
	data->d_align = 16;

	ehdr = gelf_getehdr (elf, &ehdr_mem);
	/* We have to store the section index in the ELF header.  */
	ehdr->e_shstrndx = elf_ndxscn (scn);

	/* No more sections, finalize the section header string table.  */
	ebl_strtabfinalize (shst, data);

	//elf32_getshdr (elf_getscn (elf, 4))->sh_name = ebl_strtaboffset (fourthse);
	shdr->sh_name = ebl_strtaboffset (shstrtabse);

	/* Let the library compute the internal structure information.  */
	if (elf_update (elf, ELF_C_NULL) < 0)
	{
		printf ("failure in elf_update(NULL): %s\n", elf_errmsg (-1));
		return;
	}

	ehdr = gelf_getehdr (elf, &ehdr_mem);
	/* Write out the file.  */
	if (elf_update (elf, ELF_C_WRITE) < 0)
	{
		printf ("failure in elf_update(WRITE): %s\n", elf_errmsg (-1));
		exit (1);
	}

	/* We don't need the string table anymore.  */
	ebl_strtabfree (shst);

	/* And the data allocated in the .shstrtab section.  */
	free (data->d_buf);
}

Elf_Scn* add_section(Elf *elf, char* section)
{
	Elf_Scn *scn = NULL, *strscn = NULL;
#ifdef MANUAL_LAYOUT
	size_t tblsize = 0;
#endif /* MANUAL_LAYOUT */
	Elf_Data *data;
	GElf_Ehdr ehdr;
	GElf_Shdr shdr;

	if(gelf_getehdr(elf, &ehdr) == NULL) {
		printf("Failed to obtain ELF header: %s", elf_errmsg(-1));
		return NULL;
	}

	/* TODO: Support creating a string table for objects that don't have one */
	if(!ehdr.e_shstrndx) {
		printf("No ELF string table");
		return NULL;
	}

	strscn = elf_getscn(elf, ehdr.e_shstrndx);

	if (strscn == NULL) {
		printf("Failed to open string table: %s.", elf_errmsg(-1));
		return NULL;
	}
	data = elf_newdata(strscn);
	if(data == NULL) {
		printf ("cannot create data SHSTRTAB section: %s\n", elf_errmsg (-1));
		return NULL;
	}
	data->d_align = 1;

#ifdef MANUAL_LAYOUT
	{
		GElf_Shdr strshdr;

		if(gelf_getshdr(strscn, &strshdr) != &strshdr) {
			printf("Failed to obtain ELF section header: %s", elf_errmsg(-1));
			return 0;
		}
		data->d_off = strshdr.sh_size;
#endif /* MANUAL_LAYOUT */

		data->d_size = (size_t) strlen(section)+1;
		data->d_type = ELF_T_BYTE;
		data->d_buf = section;
		data->d_version = EV_CURRENT;

#ifdef MANUAL_LAYOUT
		if(expand_section(e, strscn, data->d_size, false) != LIBR_OK)
			return false;
	}
#else
	/* Update the internal offset information */
	if(elf_update(elf, ELF_C_NULL) < 0) {
		printf("Failed to perform dynamic update: %s.", elf_errmsg(-1));
		return NULL;
	}
#endif /* MANUAL_LAYOUT */

	/* seek to the end of the section data */
	if((scn = elf_newscn(elf)) == NULL) {
		printf("Failed to create new section");
		return NULL;
	}
	if(gelf_getshdr(scn, &shdr) != &shdr) {
		printf("Failed to obtain ELF section header: %s", elf_errmsg(-1));
		return NULL;
	}
	shdr.sh_addralign = 1;
#ifdef MANUAL_LAYOUT
	shdr.sh_offset = file_handle->file_size;
#endif /* MANUAL_LAYOUT */
	shdr.sh_size = 0;
	shdr.sh_name = data->d_off;
	shdr.sh_type = SHT_NOTE; /* TODO: Does "NOTE" type fit best? */
	shdr.sh_flags = SHF_WRITE;
	shdr.sh_entsize = 0;
	if(gelf_update_shdr(scn, &shdr) < 0) {
		printf("Failed to perform dynamic update: %s.", elf_errmsg(-1));
		return NULL;
	}
	return scn;
}
